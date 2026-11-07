#pragma once
#include "json.h"
#include "state.h"

namespace ages {

// item 09: the world-aging fields of each region, written only when set
void aging_fields_to_json(const World& w, nlohmann::json& j);
void aging_fields_from_json(const nlohmann::json& j, World& w);

}
