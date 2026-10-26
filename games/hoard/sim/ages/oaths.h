#pragma once
#include <string>
#include <vector>
#include "data.h"
#include "state.h"

namespace ages {

const Oath* active_oath_with_pred(const World& w, const std::string& faction, const std::string& pred);
bool clause_holds(const World& w, const Params& p, const Oath& o, const Clause& cl, int window_from, bool& by_dragon);
void apply_oath_effect(World& w, const Params& p, const std::string& effect, const std::string& faction);

}
