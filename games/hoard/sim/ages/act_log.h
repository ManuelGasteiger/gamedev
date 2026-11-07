#pragma once
#include <string>
#include "json.h"
#include "state.h"

// item 09: the act record as act-api.schema.json writes it. Acts arrive in the schema's shape or the older flat one;
// normalise_act turns either into the internal form (region and person targets resolved to their court, the legacy
// fields filled from the typed payload and back), act_schema_json writes the schema's shape again.

namespace ages {

void act_payload_from(const nlohmann::json& j, Act& a);   // target and secondary refs, the typed payload
void normalise_act(const World& w, Act& a);
nlohmann::json act_schema_json(const World& w, const Act& a, int id);
std::string act_log_json(const World& w);   // every logged act in schema form, for tools/act_check.py

}
