#include "genesis.h"
#include "growth.h"
#include "importance.h"
#include "rules/world_rules.h"
#include "step.h"
#include "world_util.h"

// the generated world: a fresh world on the same region graph and seed with the world tag `generated` (the S2 world rules
// run only there), the start drawn at year 0, then the pre-history in decade steps while the egg lies in the mountain
// (world tag `egg`: no dragon cards), its events tagged `prehistory` once it is over, and the hatching at year
// prehistory_years (years stay absolute)

namespace ages {

bool make_genesis(World& w, const Params& p, std::string& err) {
    const GenesisDef& g = p.genesis;
    if (!g.present) { err = "genesis requested but data/genesis.json is missing"; return false; }
    World n;
    n.seed = w.seed;
    n.rng.seed(w.seed);
    n.regions = w.regions;
    for (auto& r : n.regions) {
        r.owner.clear();
        r.burn = 0;
        r.tags.clear();
        r.hot_decades = r.cold_decades = r.empty_years = 0;
        r.name.clear();
    }
    int step = p.c.step_years;
    int lo = static_cast<int>(g.prehistory.lo) / step, hi = static_cast<int>(g.prehistory.hi) / step;
    int years = (hi > lo ? n.rng.range(lo, hi + 1) : lo) * step;
    n.genesis.prehistory_years = years;
    n.genesis.hatched_year = years;
    n.tags.insert("generated");
    if (!genesis_start(n, p, err)) return false;
    n.tags.insert("egg");
    int waking = g.story.prehistory_waking_years;
    while (n.year < years) {
        decade_step(n, p);
        if (waking > 0 && n.year % waking == 0) storyteller_wake(n, p, waking, false);
    }
    n.tags.erase("egg");
    for (auto& e : n.chronicle) e.tags.insert("prehistory");
    n.sleep_started = n.year;
    std::string lair;
    for (auto& r : n.regions) if (r.lair) lair = r.id;
    emit(n, "egg_hatched", "dragon", lair, 0, {}, because("hatching", {}, {{"years", static_cast<double>(years)}}));
    hatch(n, p);
    score_events(n, p);
    w = n;
    return true;
}

}
