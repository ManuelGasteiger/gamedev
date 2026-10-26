#include "rules.h"
#include "../world_util.h"

namespace ages {

// 17.7: elves withdraw from a border region when a human neighbour is overwhelming (power > elf_withdraw_ratio x theirs)
// AND cold toward them (affinity < elf_withdraw_affinity), or when their own fear of the dragon passes elf_withdraw_fear;
// at war with that neighbour they yield at once, otherwise with chance elf_withdraw_chance per decade
static double withdraw_pressure(const Params& p, const Faction& elves, const Faction& human, double ratio) {
    const Constants& c = p.c;
    if (at_war(elves, human.id) && ratio > c.elf_withdraw_ratio) return 1;
    bool pressed = ratio > c.elf_withdraw_ratio && affinity(p, elves, human) < c.elf_withdraw_affinity;
    bool afraid = elves.fear > c.elf_withdraw_fear;
    return pressed || afraid ? c.elf_withdraw_chance : 0;
}

static void elves_withdraw(World& w, const Params& p) {
    const Constants& c = p.c;
    for (auto& f : w.factions) {
        if (dominant_race(f) != "elf" || f.regions.size() < 2) continue;
        double mine = power(f, c);
        std::string border, pressing;
        double pressure = 0;
        for (auto& rid : f.regions) {
            const Region* r = find_region(w, rid);
            if (!r) continue;
            for (auto& n : r->neighbours) {
                const Region* nr = find_region(w, n);
                if (!nr || nr->owner.empty() || nr->owner == f.id) continue;
                const Faction* o = find_faction(w, nr->owner);
                if (!o || dominant_race(*o) != "human") continue;
                double ratio = mine > 0 ? power(*o, c) / mine : c.elf_withdraw_ratio * 2;
                double pr = withdraw_pressure(p, f, *o, ratio);
                if (pr > pressure) { pressure = pr; border = rid; pressing = o->id; }
            }
        }
        if (border.empty() || !w.rng.chance(pressure)) continue;
        Region* r = find_region(w, border);
        erase_value(f.regions, border);
        f.last_region_lost = w.year;
        r->owner.clear();
        r->tags.insert("elf_withdrawn");
        bool afraid = f.fear > c.elf_withdraw_fear;
        int ref = afraid ? last_event_id(w, "act_burn", f.id) : std::max(last_event_id(w, "war_started", pressing, f.id), last_event_id(w, "region_taken", pressing));
        emit(w, "elves_withdrew", f.id, border, pressure, {pressing}, because(afraid ? "fear_of_dragon" : "pressed", {ref}, {{"fear", f.fear}}));
    }
}

static void settle(World& w, const Constants& c) {
    for (auto& f : w.factions) {
        if (f.race == "orc" || f.cohesion < c.settle_min_cohesion || !f.at_war_with.empty()) continue;
        std::vector<std::string> open;
        for (auto& rid : f.regions) {
            const Region* r = find_region(w, rid);
            if (!r) continue;
            for (auto& n : r->neighbours) {
                const Region* nr = find_region(w, n);
                if (!nr || nr->lair || !nr->owner.empty() || nr->burn > c.settle_max_burn || contains(open, n)) continue;
                if (!f.culture.temperament.settles_off_home && !home_terrain(f.culture, *nr)) continue;
                open.push_back(n);
            }
        }
        if (open.empty() || !w.rng.chance(c.settle_chance * f.eff.settle_chance)) continue;
        std::sort(open.begin(), open.end());
        const std::string& pick = open[static_cast<size_t>(w.rng.range(0, static_cast<int>(open.size())))];
        transfer_region(w, pick, "", f.id);
        int freed = std::max(last_event_id(w, "elves_withdrew", "", pick), std::max(last_event_id(w, "region_abandoned", "", pick), last_event_id(w, "faction_destroyed")));
        emit(w, "region_settled", f.id, pick, 0, {}, because("open_land", {freed}, {{"cohesion", f.cohesion}}));
    }
}

static void abandon_razed(World& w, const Constants& c) {
    for (auto& r : w.regions) {
        if (r.owner.empty() || r.burn < c.abandon_burn) continue;
        Faction* f = find_faction(w, r.owner);
        if (!f || f->regions.size() < 2) continue;
        erase_value(f->regions, r.id);
        f->last_region_lost = w.year;
        r.owner.clear();
        emit(w, "region_abandoned", f->id, r.id, r.burn, {}, because("burned_out", {last_event_id(w, "act_burn", "", r.id)}, {{"burn", r.burn}}));
    }
}

static void dwarves_dig_in(World& w, const Constants& c) {
    for (auto& f : w.factions) {
        if (dominant_race(f) != "dwarf" || !losing(w, c, f)) continue;
        for (auto& rid : f.regions) {
            Region* r = find_region(w, rid);
            if (r && !has_tag(r->tags, "dug_in")) {
                r->tags.insert("dug_in");
                int war = 0;
                for (auto& e : f.at_war_with) war = std::max(war, std::max(last_event_id(w, "war_started", e, f.id), last_event_id(w, "war_started", f.id, e)));
                emit(w, "dug_in", f.id, rid, 0, {}, because("losing", {war}));
            }
        }
    }
}

static bool weak_neighbour(const World& w, const Region& r, const Constants& c) {
    for (auto& n : r.neighbours) {
        const Region* nr = find_region(w, n);
        if (!nr || nr->owner.empty()) continue;
        const Faction* o = find_faction(w, nr->owner);
        if (!o || o->race == "orc") continue;
        if (o->cohesion < c.orc_spawn_weak_cohesion || power(*o, c) < c.orc_spawn_weak_power) return true;
    }
    return false;
}

static void orcs_spawn(World& w, const Params& p) {
    const Constants& c = p.c;
    auto race = p.races.find("orc");
    if (race == p.races.end()) return;
    for (auto& r : w.regions) {
        if (!r.owner.empty() || r.lair) continue;
        if (r.burn <= c.orc_spawn_burn && !has_tag(r.tags, "wasteland")) continue;
        if (!weak_neighbour(w, r, c)) continue;
        if (!w.rng.chance(c.orc_spawn_chance)) continue;
        Faction o = found_faction(w, p, "orc", r.id, c.orc_spawn_wealth, c.orc_spawn_cohesion, 0);
        r.owner = o.id;
        int emptied = std::max(last_event_id(w, "act_burn", "", r.id), std::max(last_event_id(w, "region_abandoned", "", r.id), last_event_id(w, "wasteland_formed", r.id)));
        emit(w, "orcs_spawned", o.id, r.id, 0, {}, because("empty_ruin", {emptied}, {{"burn", r.burn}}));
        w.factions.push_back(o);
        sort_by_id(w);
        return;
    }
}

void rule_migration(World& w, const Params& p) {
    elves_withdraw(w, p);
    abandon_razed(w, p.c);
    settle(w, p.c);
    dwarves_dig_in(w, p.c);
    orcs_spawn(w, p);
}

}
