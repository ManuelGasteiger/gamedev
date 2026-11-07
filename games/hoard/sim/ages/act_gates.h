#pragma once
#include <string>
#include <vector>
#include "data.h"
#include "state.h"

// item 09: an act's fields against its type's rule (data/gates.json) and the dragon against its gates. Refusals are
// reason codes (ages.h reason_code): act.unknown, act.malformed|field=, act.bad_value|field=|value=, gate.axis|axis=|n=,
// gate.talent|talent=, gate.phase|n=, gate.pool|pool=|n=, gate.once

namespace ages {

const ActRule* act_rule(const Params& p, const std::string& type);
std::string act_fields_refusal(const World& w, const Params& p, const Act& a);
std::string gates_refusal(const World& w, const Params& p, const std::vector<GateAlt>& gates);
std::string act_gate_refusal(const World& w, const Params& p, const Act& a);
void pay_act(World& w, const Params& p, const Act& a);
bool act_secret(const Params& p, const Act& a);   // corrupt on a person, haunt, guise, commune, keep, build

}
