#include "rules.h"
#include "core/detmath.h"
#include <cmath>
#include "../memory_tiers.h"
#include "../oaths.h"
#include "../talents.h"
#include "../world_util.h"

namespace ages {

// v0.4: Cult of the Mountain keeps the most faithful court at cult_faith, Brood of Mortals the dragon-raised courts at
// brood_faith; a court in awe of the dragon (temple_awe) keeps faith_floor (act-api 6.4)
static void faith_floors(World& w, const Params& p) {
    const MemoryConstants& m = p.c.mem;
    for (auto& f : w.factions) if (awe_eff(f) >= m.temple_awe) f.faith = std::max(f.faith, m.faith_floor);
    if (effect(w, p, "brood") > 0)
        for (auto& f : w.factions) if (has_tag(f.tags, "dragon_raised")) f.faith = std::max(f.faith, p.c.brood_faith);
    if (effect(w, p, "cult") <= 0) return;
    Faction* best = nullptr;
    for (auto& f : w.factions) if (!best || f.faith > best->faith) best = &f;
    if (best) best->faith = std::max(best->faith, p.c.cult_faith);
}

// faith memories fade at the court's memory half-life (betrayals at half speed), dread and awe per tier
static void faith_decay(World& w, const Constants& c, Faction& f) {
    double hl = f.eff.memory_halflife > 0 ? f.eff.memory_halflife : f.culture.memory_halflife;
    if (hl > 0) {
        double decay = det::half_life(w.dt, hl);
        double betrayal_decay = det::half_life(w.dt * c.betrayal_decay_mult, hl);
        for (auto& m : f.memory) if (m.axis == "faith") m.weight *= has_tag(m.tags, "betrayal") ? betrayal_decay : decay;
        f.memory.erase(std::remove_if(f.memory.begin(), f.memory.end(),
                                      [&](const MemoryEntry& m) { return m.axis == "faith" && m.weight < c.memory_drop; }),
                       f.memory.end());
    }
    recompute_faith(f, c);
}

// a kept oath is remembered as a kindness every decade it holds
static void kept_oaths(World& w, const Params& p) {
    double k = dts(w, p.c) * p.c.mem.kept_oath_awe;
    if (k <= 0) return;
    for (auto& o : w.oaths) {
        if (o.status != "active") continue;
        Faction* f = find_faction(w, o.parties[0] == "dragon" ? o.parties[1] : o.parties[0]);
        if (!f) continue;
        Deposit d;
        d.type = "oath_kept";
        d.awe = k;
        deposit(w, p, *f, d);
    }
}

void rule_memory(World& w, const Params& p) {
    for (auto& f : w.factions) {
        decay_tiers(w, p, f);
        faith_decay(w, p.c, f);
    }
    kept_oaths(w, p);
    for (auto& f : w.factions) {
        bool high = awe_eff(f) >= p.c.mem.temple_awe;
        if (!high) f.awe_since = -1;
        else if (f.awe_since < 0) f.awe_since = w.year;
    }
    faith_floors(w, p);
}

}
