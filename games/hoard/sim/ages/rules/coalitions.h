#pragma once
#include <string>
#include <vector>
#include "../data.h"
#include "../state.h"

namespace ages {

std::vector<std::string> side_members(const War& war, bool side_a);
double side_power(const World& w, const War& war, bool side_a, const Constants& c);
void join_side(World& w, War& war, bool side_a, Faction& member, const std::string& reason, const Cause& cause);
void form_coalitions(World& w, const Params& p);
void join_alliances(World& w, const Params& p);
void clear_side_wars(World& w, const War& war);

}
