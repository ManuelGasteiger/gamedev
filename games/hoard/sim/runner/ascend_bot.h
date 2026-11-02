#pragma once
#include <memory>
#include <string>
#include "ages/ages.h"
#include "ages/json.h"

// item 04b: the Ascension test bot (data/bots/ascension.json) and its probe. The probe keeps, per Ascension
// condition, its hold state and the best value of each of its counts, plus the parts the counts are built from.
// free: no stamina budget (an upper bound on what the rules allow).
struct AscendBot {
    std::shared_ptr<ages::Policy> base;
    std::shared_ptr<nlohmann::json> probe;
};

bool attach_ascend_bot(ages::Scenario& s, const ages::Params& p, bool free, AscendBot& out, std::string& err);
void finish_ascend_probe(const ages::World& w, const ages::Params& p, nlohmann::json& probe);
