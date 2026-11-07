#pragma once
#include <string>
#include <vector>
#include "data.h"
#include "state.h"

// item 08: the duel behind the `fight` act (item 09). Applies everything: the dragon's wounds (wound_dragon, which may
// kill it), the slayer's death (artifact to the hoard) or flight with a scar, +1 power, a humiliated memory and the
// rival's gain, and all three traits learned. Outcome is empty when `slayer` is not a rostered person.
// family is the dragon's act family (data/slayers.json "families": fire, heights, dark, dead; "" for none); a
// family that matches the slayer's fear leaves the dragon unwounded, one that triggers his hate adds power.

namespace ages {

struct FightResult {
    double dragon_wounds = 0;
    std::string slayer_outcome;   // "dead" | "fled_scarred"
    bool artifact = false;        // the slayer carried one and it fell to the hoard
    std::vector<std::string> traits_revealed;   // kinds newly learned: counter, fear, hate
};

FightResult resolve_fight(World& w, const Params& p, const std::string& slayer, const std::string& family = "");
std::string act_family(const Params& p, const std::string& act_type);   // "" when the act has no family
void reveal_order(World& w, const std::string& court);   // reveal(slayer_plan): every named slayer's traits become known

}
