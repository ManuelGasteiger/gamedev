#include "rules.h"
#include "core/detmath.h"
#include <cmath>
#include "../world_util.h"

namespace ages {

static Remnant* find_remnant(World& w, const std::string& race) {
    for (auto& r : w.remnants) if (r.race == race) return &r;
    return nullptr;
}

void note_race_eras(World& w) {
    std::map<std::string, size_t> largest;
    for (auto& f : w.factions) {
        std::string race = dominant_race(f);
        int& best = w.race_max_era[race];
        best = std::max(best, f.era);
        if (f.regions.empty()) continue;
        auto it = largest.find(race);
        if (it == largest.end() || f.regions.size() > it->second) {
            largest[race] = f.regions.size();
            w.race_last_region[race] = f.regions[0];
        }
    }
}

static void create_missing(World& w, const Params& p) {
    for (auto& race : p.races) {
        bool alive = false;
        for (auto& f : w.factions) if (dominant_race(f) == race.first) alive = true;
        if (alive || find_remnant(w, race.first) || has_tag(w.tags, "extinct:" + race.first)) continue;
        bool existed = w.race_max_era.count(race.first) > 0;
        if (!existed && race.second.remnant_start <= 0) continue;
        Remnant rem;
        rem.race = race.first;
        rem.strength = existed ? p.c.remnant_start_strength : race.second.remnant_start;
        rem.origin = w.race_last_region[race.first];
        if (rem.origin.empty())
            for (auto& r : w.regions) if (!r.lair && home_terrain(race.second.culture, r)) { rem.origin = r.id; break; }
        rem.max_era = w.race_max_era[race.first];
        w.remnants.push_back(rem);
        std::sort(w.remnants.begin(), w.remnants.end(), [](const Remnant& a, const Remnant& b) { return a.race < b.race; });
        int gone = 0;
        for (auto& g : w.graveyard) if (g.race == race.first && g.died == w.year) gone = std::max(gone, std::max(last_event_id(w, "faction_destroyed", g.id), last_event_id(w, "vassal_absorbed", g.id)));
        emit(w, "remnant_formed", race.first, rem.origin, rem.strength, {existed ? "last_faction_gone" : "off_map"}, because(existed ? "last_faction_gone" : "off_map", {gone}));
    }
}

void refound_remnant(World& w, const Params& p, Remnant& rem, const std::string& region, const std::string& reason) {
    const Constants& c = p.c;
    Faction f = found_faction(w, p, rem.race, region, c.remnant_refound_wealth, c.remnant_refound_cohesion,
                              std::max(0, rem.max_era - 1));
    f.tags.insert("returned");
    if (Region* r = find_region(w, region)) r->owner = f.id;
    int formed = last_event_id(w, "remnant_formed", rem.race);
    emit(w, "race_returned", f.id, region, rem.strength, {rem.race, reason}, because(reason == "sponsored" ? "sponsored" : "open_home", {formed}, {{"strength", rem.strength}}));
    w.factions.push_back(f);
    sort_by_id(w);
    w.remnants.erase(std::remove_if(w.remnants.begin(), w.remnants.end(),
                                    [&](const Remnant& x) { return x.race == rem.race; }),
                     w.remnants.end());
}

void rule_remnants(World& w, const Params& p) {
    const Constants& c = p.c;
    note_race_eras(w);
    create_missing(w, p);
    double decay = det::half_life(c.step_years, c.remnant_halflife);
    std::vector<std::string> races;
    for (auto& r : w.remnants) {
        auto rt = p.races.find(r.race);
        r.strength = std::max(rt == p.races.end() ? 0.0 : rt->second.remnant_floor, r.strength * decay);
        races.push_back(r.race);
    }
    for (auto& race : races) {
        Remnant* rem = find_remnant(w, race);
        if (!rem || rem->strength <= c.remnant_min_strength) continue;
        auto rt = p.races.find(race);
        if (rt == p.races.end()) continue;
        std::vector<std::string> open;
        for (auto& r : w.regions)
            if (r.owner.empty() && !r.lair && home_terrain(rt->second.culture, r)) open.push_back(r.id);
        if (open.empty() || !w.rng.chance(c.remnant_refound_chance)) continue;
        std::string pick = open[static_cast<size_t>(w.rng.range(0, static_cast<int>(open.size())))];
        refound_remnant(w, p, *rem, pick, "returned");
    }
}

}
