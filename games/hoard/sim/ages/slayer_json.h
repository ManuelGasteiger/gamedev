#pragma once
#include "json.h"
#include "state.h"

// item 08: the roster in saves (empty parts left out, so worlds without orders hash as before) and the view scrub

namespace ages {

void slayer_person_json(const Person& x, nlohmann::json& j);
void slayer_person_from(const nlohmann::json& j, Person& x);
void orders_json(const World& w, nlohmann::json& j);
void orders_from(const nlohmann::json& j, World& w);
void scrub_unknown(const Person& x, nlohmann::json& j);   // views: a trait the dragon has not learned reads "?"

}
