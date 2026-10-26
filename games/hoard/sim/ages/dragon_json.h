#pragma once
#include "ages.h"
#include "json.h"

// v0.4 (S1): the dragon's growth, talents, wounds and the valley's defences in JSON

namespace ages {

void dragon_state_to_json(const Dragon& d, nlohmann::json& dragon);      // saved state: growth, talents, wounds, embers
void dragon_state_from_json(const nlohmann::json& dragon, Dragon& d);
void dragon_view_to_json(const World& w, const Params& p, nlohmann::json& view);   // derived: picks, effects, defences, ...
void sleep_growth_to_json(const SleepResult& r, nlohmann::json& j);       // growth_gained, healed
nlohmann::json talents_catalogue_json(const Params& p);

}
