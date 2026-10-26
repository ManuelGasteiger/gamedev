#include "world_rules.h"
#include "../pop.h"
#include "../world_util.h"

// a people dies out for good (core-loop.md section 6): after gone_years without a court of its own it assimilates (its
// share of every court shrinks), and once no court is much of it and its remnant is gone or weaker than
// extinction.strength, a chance per decade ends it. Its remnant goes and its last minorities are absorbed; the world tag
// extinct:<race> keeps it from returning.

namespace ages {

static bool has_court(const World& w, const std::string& race) {
    for (auto& f : w.factions) if (dominant_race(f) == race) return true;
    return false;
}

static bool lives_on(const World& w, const ExtinctionDef& x, const std::string& race) {
    for (auto& f : w.factions) {
        auto it = f.pop.find(race);
        if (it != f.pop.end() && it->second >= x.pop) return true;
    }
    for (auto& r : w.remnants) if (r.race == race && r.strength >= x.strength) return true;
    return false;
}

// each court keeps `keep` of its share of the race (0: all of it goes), the rest spread over its other peoples
static void shrink_share(World& w, const Params& p, const std::string& race, double keep) {
    for (auto& f : w.factions) {
        auto it = f.pop.find(race);
        if (it == f.pop.end()) continue;
        if (keep > 0 && it->second * keep > 1e-4) it->second *= keep; else f.pop.erase(it);
        double sum = 0;
        for (auto& kv : f.pop) sum += kv.second;
        if (sum <= 0) { f.pop[f.race] = 1.0; sum = 1.0; }
        for (auto& kv : f.pop) kv.second /= sum;
        blend_culture(f, p);
    }
}

void extinguish(World& w, const Params& p, const std::string& race) {
    w.remnants.erase(std::remove_if(w.remnants.begin(), w.remnants.end(), [&](const Remnant& r) { return r.race == race; }), w.remnants.end());
    shrink_share(w, p, race, 0);
    w.tags.insert("extinct:" + race);
}

void rule_extinction(World& w, const Params& p) {
    const ExtinctionDef& x = p.genesis.extinction;
    double k = dts(w, p.c);
    for (auto& race : x.races) {
        if (has_tag(w.tags, "extinct:" + race) || !w.race_max_era.count(race) || has_court(w, race)) continue;
        int formed = last_event_id(w, "remnant_formed", race);
        const Event* gone = find_event(w, formed);
        if (!gone || w.year - gone->year < x.gone_years) continue;
        shrink_share(w, p, race, 1.0 - x.assimilate * k);
        if (lives_on(w, x, race) || !w.rng.chance(x.chance * k)) continue;
        std::string origin = gone->object;
        extinguish(w, p, race);
        emit(w, "race_extinct", race, origin, static_cast<double>(w.year - gone->year), {}, because("died_out", {formed}, {{"years", static_cast<double>(w.year - gone->year)}}));
    }
}

}
