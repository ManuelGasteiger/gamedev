#pragma once
#include <string>
#include "data.h"
#include "state.h"

// v0.4 (S1): permanent growth points on five axes; Might also raises size; the total opens phases (world_util.h
// phase_score) through a moult

namespace ages {

extern const char* const kGrowthAxes[5];   // might, flame, arcana, lore, dominion

void hatch(World& w, const Params& p);   // the egg's hoard (constants dragon.start_hoard)
void add_growth(World& w, const Params& p, const std::string& axis, double points);
double total_growth(const Dragon& d);
double next_phase_points(const Dragon& d, const Constants& c);   // -1 at the last phase

}
