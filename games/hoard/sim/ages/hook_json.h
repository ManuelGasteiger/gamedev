#pragma once
#include "ages.h"
#include "json.h"

// v0.4 hooks (S3): hooks in JSON. The saved shape is the view shape (the page saves what it reads), so one reader
// serves both; fight_damage is derived and only read back as absent.

namespace ages {

nlohmann::json hook_json(const World& w, const Params& p, const Hook& h);
nlohmann::json hooks_json(const World& w, const Params& p, const std::vector<Hook>& hooks);
void hooks_state_to_json(const World& w, const Params& p, nlohmann::json& j);   // hooks, hook_fired
void hooks_state_from_json(const nlohmann::json& j, World& w);
void forecast_hook_to_json(const Forecast& f, nlohmann::json& j);              // hook_preview when the stars were read

}
