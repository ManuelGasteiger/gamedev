#pragma once
#include <map>
#include <string>
#include "data.h"
#include "json.h"
#include "state.h"

// item 04: endings and the prophecy in the save, the world view and the briefing

namespace ages {

void endings_fields_to_json(const World& w, nlohmann::json& j);
void endings_fields_from_json(const nlohmann::json& j, World& w);

// a template of data/text/<lang>/endings.json with {court.name} and plain vars; "" when the texts are not loaded
std::string endings_text(const World& w, const Params& p, const std::string& key, const std::map<std::string, std::string>& vars,
                         const std::string& court = "", const std::string& lang = "en");

// the briefing's Endings block: per enabled ending, per condition met, years held, what is missing
nlohmann::json endings_view_json(const World& w, const Params& p);
nlohmann::json prophecy_view_json(const World& w, const Params& p);   // null without a prophecy
nlohmann::json ending_view_json(const World& w, const Params& p);     // null while the run goes on
nlohmann::json myth_view_json(const World& w, const Params& p);       // null while the dragon lives: does its death qualify

}
