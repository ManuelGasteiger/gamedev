#pragma once
#include "data.h"
#include "state.h"

// v0.4 (S1): what the dragon's acts meet and leave behind: the defences strike a harmful act (burn, take, kill,
// take_hostage) before it lands (with tower_breaker a burn then wrecks them), the `wound` act carries the page's
// passive damage, wards damp burns, burns leave embers and (with talents) spread and linger, a take from burning
// land melts more gold

namespace ages {

bool harmful_act(const std::string& type);
double strike_damage(const World& w, const Params& p, const Act& a, Tags* defences = nullptr);   // after armor
void strike(World& w, const Params& p, const Act& a);
void passive_wound(World& w, const Params& p, const Act& a);
double warded_burn(const World& w, const Params& p, const Act& a, double m);
void after_burn(World& w, const Params& p, const Faction& f, const Act& a, double m);
double molten_gold_mult(const World& w, const Params& p, const Faction& f, const Act& a);

}
