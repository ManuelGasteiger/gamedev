#pragma once
#include <map>
#include <string>
#include <vector>
#include "ages/ages.h"
#include "ages/profile.h"
#include "bank.h"

// item 10: the scenario runtime (scenario-spec.md sections 3 to 6). Namespace scn; ages::run_scenario is the policy
// runner and unrelated. The bank lives in Params.scenarios; the runtime runs only while World.scn.on.

namespace scn {

bool bank_from_json(const ages::DataJson& d, Bank& out, std::string& err);   // bank.cpp

struct Pick {
    const Def* def = nullptr;
    std::map<std::string, std::string> roles;
    std::vector<int> cites;
    std::string slot;
    double score = 0;
};

// what one selection saw, for the dry run (not saved, not hashed)
struct Diag {
    std::vector<std::string> eligible;
    std::map<std::string, double> scores;
    int consequence_depth = 0;   // eligible consequences
    bool acted_last = false;     // a witnessed act in the last waking
    bool pity = false;
    int era = 0;   // of the strongest court
};

// select.cpp: the pure selection for the dragon's current waking; wake() records it as shown
std::vector<Pick> select(const ages::World& w, const ages::Params& p, Diag* diag = nullptr);
void wake(ages::World& w, const ages::Params& p);   // also fills World.scn.diag_*
int current_waking(const ages::World& w);   // sleep acts in the log
void enable(ages::World& w, const ages::Profile* pr = nullptr);   // turns the runtime on; seen counts frozen from the profile
void note_seen(ages::Profile& pr, const ages::World& w);   // adds this run's shown scenarios to profile.seen

// present.cpp: the briefing's scenarios[] block for the current waking
nlohmann::json briefing_block(const ages::World& w, const ages::Params& p, const std::string& lang = "en");
// the option's act with the bound ids, in act-api.schema.json shape; false when a role it needs is unbound
bool resolve_option(const ages::World& w, const Def& d, const Option& o, const std::map<std::string, std::string>& roles, double magnitude,
                    nlohmann::json& act);

// answer.cpp: answer_scenario; reason is a code (ages.h reason_code)
struct Answer {
    bool ok = false;
    std::string reason;
    int act = -1;
    std::vector<ages::ReactionLine> lines;
    std::vector<std::string> texts;   // result and reveal lines, rendered in English
};
Answer answer_unrecorded(ages::World& w, const ages::Params& p, const std::string& id, const std::string& option, double magnitude = -1);
Answer answer_scenario(ages::World& w, const ages::Params& p, const std::string& id, const std::string& option, double magnitude = -1);
nlohmann::json answer_json(const Answer& a);

// lab.cpp: a minimal world built from a spec {courts, persons, acts, ...} for the forced-state test
bool lab_world(ages::World& w, const ages::Params& p, const nlohmann::json& spec, std::string& err);

// scn_json.cpp
nlohmann::json state_json(const ages::ScnState& s);
void state_from_json(const nlohmann::json& j, ages::ScnState& s);

}
