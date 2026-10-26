#include "rules.h"
#include <cmath>
#include "../talents.h"
#include "../world_util.h"

namespace ages {

// v0.4: Cult of the Mountain keeps the most faithful court at cult_faith, Brood of Mortals the dragon-raised courts at
// brood_faith
static void faith_floors(World& w, const Params& p) {
    if (effect(w, p, "brood") > 0)
        for (auto& f : w.factions) if (has_tag(f.tags, "dragon_raised")) f.faith = std::max(f.faith, p.c.brood_faith);
    if (effect(w, p, "cult") <= 0) return;
    Faction* best = nullptr;
    for (auto& f : w.factions) if (!best || f.faith > best->faith) best = &f;
    if (best) best->faith = std::max(best->faith, p.c.cult_faith);
}

void rule_memory(World& w, const Params& p) {
    const Constants& c = p.c;
    double longer = effect(w, p, "memory_mult");
    for (auto& f : w.factions) {
        double hl = f.culture.memory_halflife;
        if (hl > 0) {
            if (f.eff.memory_halflife > 0) hl = f.eff.memory_halflife;
            hl *= longer;
            double decay = std::pow(0.5, w.dt / hl);
            double betrayal_decay = std::pow(0.5, w.dt * c.betrayal_decay_mult / hl);
            for (auto& m : f.memory) m.weight *= has_tag(m.tags, "betrayal") ? betrayal_decay : decay;
            f.memory.erase(std::remove_if(f.memory.begin(), f.memory.end(),
                                          [&](const MemoryEntry& m) { return m.weight < c.memory_drop; }),
                           f.memory.end());
        }
        recompute_fear_faith(f, c);
    }
    faith_floors(w, p);
}

}
