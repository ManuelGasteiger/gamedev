#include "rules.h"
#include "coalitions.h"
#include "../world_util.h"

namespace ages {

void transfer_region(World& w, const std::string& region, const std::string& from, const std::string& to) {
    Region* r = find_region(w, region);
    if (!r) return;
    if (Faction* f = find_faction(w, from)) {
        erase_value(f->regions, region);
        f->last_region_lost = w.year;
        if (find_faction(w, to)) f->last_invaded = w.year;
    }
    r->owner = to;
    if (Faction* t = find_faction(w, to)) {
        t->regions.push_back(region);
        std::sort(t->regions.begin(), t->regions.end());
        t->max_regions = std::max(t->max_regions, static_cast<int>(t->regions.size()));
    }
}

static War* find_war(World& w, const std::string& a, const std::string& b) {
    for (auto& x : w.wars)
        if ((x.a == a && x.b == b) || (x.a == b && x.b == a)) return &x;
    return nullptr;
}

// the newest blow to the defender's cohesion: a lost region, a plague, a collapse it rose from, a succession crisis
int weakness_cause(const World& w, const Constants& c, const Faction& b) {
    int since = w.year - c.cause_window_years;
    int ev = last_event_id(w, "region_taken", "", "", b.id, since);
    for (const char* k : {"plague", "succession_crisis", "successor_founded", "collapse", "tyrant_died", "warlord_died"})
        ev = std::max(ev, last_event_id(w, k, b.id, "", "", since));
    return ev;
}

War& start_war(World& w, Faction& a, Faction& b, const std::string& reason, const Cause& cause) {
    if (!at_war(a, b.id)) a.at_war_with.push_back(b.id);
    if (!at_war(b, a.id)) b.at_war_with.push_back(a.id);
    War war;
    war.a = a.id;
    war.b = b.id;
    war.started = w.year;
    war.thread = "war:" + a.id + ":" + b.id + ":" + std::to_string(w.year);
    war.declared = emit(w, "war_started", a.id, b.id, 0, reason.empty() ? Tags{} : Tags{reason}, cause, war.thread);
    open_thread(w, war.thread);
    w.wars.push_back(war);
    return w.wars.back();
}

void end_war(World& w, const War& war, const std::string& reason, const Cause& cause) {
    clear_side_wars(w, war);
    Cause c = cause;
    add_ref(c, war.declared);
    c.params["years"] = static_cast<double>(w.year - war.started);
    emit(w, war.coalition ? "coalition_ended" : "war_ended", war.a, war.b, 0, {reason}, c, war_thread(war));
    close_thread(w, war_thread(war));
    w.wars.erase(std::remove_if(w.wars.begin(), w.wars.end(),
                                [&](const War& x) { return x.a == war.a && x.b == war.b; }),
                 w.wars.end());
}

static void declare_wars(World& w, const Params& p) {
    const Constants& c = p.c;
    for (size_t i = 0; i < w.factions.size(); ++i) {
        for (size_t j = 0; j < w.factions.size(); ++j) {
            if (i == j) continue;
            Faction& a = w.factions[i];
            Faction& b = w.factions[j];
            if (at_war(a, b.id) || !adjacent(w, a, b) || is_vassal(a) || is_vassal(b)) continue;
            if (a.race == "orc" && b.race == "orc") continue;
            double pa = power(a, c), pb = power(b, c);
            if (a.race == "orc") {
                if (b.cohesion < c.orc_war_cohesion)
                    start_war(w, a, b, "", because("orc_raid", {weakness_cause(w, c, b)}, {{"cohesion", b.cohesion}}));
                continue;
            }
            if (pb <= 0 || pa / pb <= c.war_power_ratio || b.cohesion >= c.war_target_cohesion) continue;
            if (allied(a, b)) continue;
            double hostility = std::max(1.0, 1.0 - affinity(p, a, b));
            if (w.rng.chance(c.war_chance * a.eff.war_chance * hostility * dts(w, c)))
                start_war(w, a, b, "", because("weak_neighbour", {weakness_cause(w, c, b)}, {{"ratio", pa / pb}, {"cohesion", b.cohesion}}));
        }
    }
}

// the losing side's member with a border to the winning side and the fewest regions, and one of its border regions
static bool pick_target(World& w, const War& war, bool winner_a, std::string& loser_id, std::string& region) {
    std::vector<std::string> winners = side_members(war, winner_a);
    std::vector<std::string> candidates;
    const Faction* chosen = nullptr;
    for (auto& lid : side_members(war, !winner_a)) {
        const Faction* l = find_faction(w, lid);
        if (!l) continue;
        std::vector<std::string> border;
        for (auto& rid : l->regions) {
            const Region* r = find_region(w, rid);
            if (!r) continue;
            for (auto& n : r->neighbours) {
                const Region* nr = find_region(w, n);
                if (nr && contains(winners, nr->owner)) { border.push_back(rid); break; }
            }
        }
        if (border.empty()) continue;
        if (!chosen || l->regions.size() < chosen->regions.size()) { chosen = l; candidates = border; }
    }
    if (!chosen) return false;
    loser_id = chosen->id;
    region = candidates[static_cast<size_t>(w.rng.range(0, static_cast<int>(candidates.size())))];
    return true;
}

static void capture(World& w, const Params& p, War& live, bool winner_a, Faction& winner, Faction& loser, const std::string& region) {
    const Constants& c = p.c;
    if (winner.race == "orc") {
        double plunder = loser.wealth * c.orc_plunder;
        loser.wealth -= plunder;
        winner.wealth += plunder;
    }
    // refugees leave before the region changes hands: afterwards a one-region loser borders nobody
    refugees(w, p, loser, c.refugee_share);
    transfer_region(w, region, loser.id, winner.id);
    shift_pop(winner, p, loser.pop, c.conquest_pop_shift);
    blend_traits(winner.culture.traits, loser.culture.traits, c.conquest_pop_shift);
    if (winner_a) live.taken_from_b += 1; else live.taken_from_a += 1;
    emit(w, "region_taken", winner.id, region, 0, {loser.id}, because("war", {live.declared}), war_thread(live));
}

static void fight(World& w, const Params& p) {
    const Constants& c = p.c;
    std::vector<War> wars = w.wars;
    for (auto& war : wars) {
        War* live = find_war(w, war.a, war.b);
        if (!live) continue;
        Faction* a = find_faction(w, war.a);
        Faction* b = find_faction(w, war.b);
        if (!a || !b) { end_war(w, *live, "collapse", because("party_collapsed", {last_event_id(w, "collapse", a ? war.b : war.a)})); continue; }
        live->years_fought += w.dt;
        live->decades = live->years_fought / c.step_years;
        double k = dts(w, c);

        double pa = side_power(w, *live, true, c), pb = side_power(w, *live, false, c);
        bool a_stronger = pa >= pb;
        double ratio = std::min(pa, pb) > 0 ? std::max(pa, pb) / std::min(pa, pb) : 10.0;
        Faction& lead = a_stronger ? *a : *b;
        for (auto& lid : side_members(*live, !a_stronger))
            if (Faction* l = find_faction(w, lid)) l->wealth *= (1.0 - c.war_loser_wealth_loss * k);

        std::string loser_id, region;
        if (pick_target(w, *live, a_stronger, loser_id, region)) {
            Faction* loser = find_faction(w, loser_id);
            const Region* r = find_region(w, region);
            double take = std::min(1.0, c.war_take_chance * ratio) * (r ? r->eff_invasion : 1.0) * k;
            if (loser && w.rng.chance(take)) {
                if (loser->regions.size() == 1 && lead.race != "orc") {
                    make_vassal(w, *loser, lead, because("last_region", {live->declared}));
                    live = find_war(w, war.a, war.b);
                    if (!live) continue;
                } else {
                    capture(w, p, *live, a_stronger, lead, *loser, region);
                }
            }
        }

        Faction* principal_loser = find_faction(w, a_stronger ? war.b : war.a);
        int lost = a_stronger ? live->taken_from_b : live->taken_from_a;
        int taken = last_event_id(w, "region_taken", lead.id, "", "", w.year);
        if (!principal_loser || principal_loser->regions.empty()) end_war(w, *live, "annihilation", because("no_land_left", {taken}));
        else if (lost >= c.war_end_regions) end_war(w, *live, "conquest", because("regions_lost", {taken}, {{"regions", static_cast<double>(lost)}}));
        else if (live->decades >= c.war_max_decades) end_war(w, *live, "exhaustion", because("long_war", {}, {{"years", static_cast<double>(live->years_fought)}}));
    }
}

void rule_war(World& w, const Params& p) {
    declare_wars(w, p);
    if (w.dt >= p.c.step_years) form_coalitions(w, p);
    join_alliances(w, p);
    fight(w, p);
}

}
