#pragma once
#include <string>
#include "data.h"
#include "json.h"

// constants.json "memory" and data/reactions.json (gains and rows)

namespace ages {

void memory_constants_from_json(const nlohmann::json& j, MemoryConstants& m);
bool reactions_from_json(const nlohmann::json& j, Params& p, std::string& err);

}
