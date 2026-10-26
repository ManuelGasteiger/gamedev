#pragma once
#include <map>
#include <string>
#include "data.h"
#include "state.h"

namespace ages {

using Pop = std::map<std::string, double>;

void blend_culture(Faction& f, const Params& p);
void shift_pop(Faction& f, const Params& p, const Pop& toward, double amount);
void blend_traits(Traits& t, const Traits& toward, double amount);
void refugees(World& w, const Params& p, const Faction& from, double share);

}
