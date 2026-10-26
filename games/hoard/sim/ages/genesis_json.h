#pragma once
#include "json.h"
#include "state.h"

namespace ages {

// the S2 fields of the world JSON: region names and empty years, the genesis block, the storyteller's quiet clock
void genesis_fields_to_json(const World& w, nlohmann::json& j);
void genesis_fields_from_json(const nlohmann::json& j, World& w);

}
