#pragma once
#include <map>
#include <string>
#include <vector>
#include "move_data.h"

// item 09: data/gates.json (what each act needs) and constants.json "acts" (what the new acts do)

namespace ages {

struct GateAlt {
    std::map<std::string, double> axes;   // growth points per axis
    std::vector<std::string> talents;     // any one of them
    int phase = 0;
};

struct ActRule {
    std::vector<std::string> target, secondary, numbers, flags;
    std::map<std::string, std::vector<std::string>> args;   // required; an empty list takes any value
    std::vector<GateAlt> gates;                             // one alternative must hold; none means no gate
    std::map<std::string, double> pool;                     // magic | knowledge | faith_pool, paid on acceptance
    bool magnitude = false, secret = false, once = false;
    std::string via;                                        // the API verb the act calls
};

struct ActNumbers {
    double library_era_rate = 0.9; int library_decades = 10;
    double temple_faith = -0.3;
    double foundry_era_progress = -20, foundry_anti_dragon = -0.1;
    double granary_wealth = 0.1;
    double ruin_burn = 1.0;
    double take_item_artifacts = 1.0;
    double craft_era_progress = 10, war_militarism = 0.1, war_anti_dragon = 0.1, magic_world = 0.05, intrigue_cunning = 0.1;
    double trade_growth = 1.1; int trade_decades = 10;
    double haunt_abdicate = 0.1, haunt_desert = 0.2, haunt_cult = 0.3;
    double guise_discovery = 0.1, guise_per_sage = 0.05, guise_forewarned = 0.1, guise_show = 0.5;
    double guise_preach_faith = 0.1, guise_learn_knowledge = 1.0;
    double curse_research_mult = 0.75; int curse_research_stacks = 3;
    double curse_blight_land = 0.5;
    int consort_line_years = 100;
    std::map<std::string, MemoryRow> haunt;   // per message: the dream the person remembers
};

struct ActBook {
    std::map<std::string, ActRule> acts;
    std::map<std::string, std::vector<GateAlt>> keep_roles, build_what;
    std::vector<GateAlt> reveal_lie, sponsor_person;
    ActNumbers n;
};

}
