#pragma once
#include <string>
#include <vector>
#include "data.h"
#include "state.h"

// v0.4 (S1): a settled region's defences, derived from its court whenever they are read (data/defences.json).
// Damage is after fear (x (1 - fear_damp x fear)) and the hero at the capital, before the dragon's armor.

namespace ages {

struct DefenceView {
    const DefenceDef* def = nullptr;
    double per_act = 0, passive = 0, burn_mult = 1;
    bool wrecked = false;
};

std::vector<DefenceView> region_defences(const World& w, const Params& p, const Region& r);

// the next anti-dragon level at which a defence the court's era and race allow would stand (min_anti_dragon, or
// slayers_at for a defence opened by the slayers tag); at < 0 when none is left
struct Unlock {
    double at = -1;
    std::string defence;
};
Unlock next_unlock(const World& w, const Params& p, const Faction& f);
std::string capital_region(const World& w, const Faction& f);
const Region* act_region(const World& w, const Act& a);   // the act's region, else the target court's capital
double armor_against(const World& w, const Params& p, const DefenceDef& d);
double burn_mult_at(const World& w, const Params& p, const Region& r);   // wards halve burns unless ward_breaker

}
