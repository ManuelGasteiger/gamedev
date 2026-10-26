#pragma once
#include <string>
#include "ages/json.h"

namespace text {

// a generated name in a case; data/text/<lang>/names.json "gen": {suffix, apostrophe, apostrophe_after[], overrides{}}.
// Names are never upper-cased (Weiß would become WEISS).
std::string name_in_case(const nlohmann::json& names, const std::string& name, const std::string& cas);

}
