#pragma once
#include <string>
#include "data.h"
#include "state.h"

// v0.4 (S1): health and wounds. Wounds stay until slept off; reaching health_max kills the dragon (world tag
// dragon_slain, event dragon_slain with cause `defences`, the court whose defence struck last as subject).

namespace ages {

double health_max(const World& w, const Params& p);
bool dragon_dead(const World& w);
void wound_dragon(World& w, const Params& p, const std::string& court, const std::string& region, double damage, const Tags& defences);
void heal(World& w, const Params& p, int years);   // health_max x min(1, years / short window) x heal_mult

}
