#pragma once
#include "ages.h"

// the API verbs without recording; recorded.cpp wraps each one so a replay sees every outermost call
namespace ages {

std::vector<ReactionLine> apply_acts_unrecorded(World& w, const Params& p, const std::vector<Act>& acts, std::vector<ActRefusal>* refused = nullptr);
// item 09: acts a verb has already checked (its own act, a replay copy): logged and resolved without rule, gate or routing
std::vector<ReactionLine> apply_logged_acts(World& w, const Params& p, const std::vector<Act>& acts);
SleepResult sleep_unrecorded(World& w, const Params& p, int years);
bool pick_talent_unrecorded(World& w, const Params& p, const std::string& id, std::string& reason);
bool accept_offer_unrecorded(World& w, const Params& p, size_t index);
bool decline_offer_unrecorded(World& w, const Params& p, size_t index);
DemandResult demand_oath_unrecorded(World& w, const Params& p, const std::string& target, const std::string& pred,
                                    const std::map<std::string, std::string>& args = {});
bool answer_hook_unrecorded(World& w, const Params& p, const std::string& id, const std::string& choice, std::string& reason);
bool kill_person_unrecorded(World& w, const Params& p, const std::string& id, std::string& reason, std::vector<ReactionLine>* lines);
CastResult cast_spell_unrecorded(World& w, const Params& p, const std::string& spell, const std::string& target,
                                 const std::string& region, const std::string& against);
CastResult cast_spell_effect(World& w, const Params& p, const std::string& spell, const std::string& target,
                             const std::string& region, const std::string& against);   // no act logged
Act cast_act(const World& w, const CastResult& r);
TerraformResult terraform_unrecorded(World& w, const Params& p, const std::string& region, const std::string& kind);

}
