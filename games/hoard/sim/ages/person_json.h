#pragma once
#include "json.h"
#include "state.h"

namespace ages {

void person_record_json(const Person& x, nlohmann::json& j);
void person_record_from(const nlohmann::json& j, Person& x);
void secrets_json(const World& w, nlohmann::json& j);
void secrets_from(const nlohmann::json& j, World& w);

}
