#pragma once
#include <map>
#include <string>
#include <vector>
#include "move_data.h"

// item 08: data/slayers.json (the roster of a slayer order)

namespace ages {

struct FamilyDef {
    std::string fear;
    std::vector<std::string> hates;
};

struct SlayerBook {
    bool present = false;
    double court_magnitude = 0.6, court_weight = 0.3, person_s = 0.3;
    int person_sev = 3, first_slayers = 2, masters = 2, slayers = 6;
    int power_start_min = 1, power_start_max = 4, power_cap = 12;
    double hunter_power = 0.25, ready_ratio = 0.8, grandmaster_odds = 0.1, counter_mult = 1.25, hate_bonus = 1;
    std::map<std::string, std::string> counters;   // counter trait -> growth axis
    std::vector<std::string> hates, fears, zealots, opportunists;
    std::map<std::string, FamilyDef> families;
    std::map<std::string, std::string> act_family, scars;
    int scar_max = 3;
    double w_train = 4, w_hunt = 2, w_seek = 1, w_march = 3;
    int train_hunters = 1, hunters_max = 12;
    double hunt_power = 0.3, seek_artifact = 0.1, seek_magic_min = 1, relative_div = 1;
    int hunters_per_slayer = 3, hunters_start = 2, peace_years = 50, peace_loss = 1;
    double recruit_chance = 0.5, kin_chance = 0.1;
    std::map<std::string, int> rel;
    double fight_base_share = 0.02, fight_death_ratio = 1.5, fight_artifact = 1;
    double march_death = 0.5, march_hunters_lost = 0.5, gate_death = 0.5;
    int march_promote = 1, rival_gain = 1, ally_promote = 1, reveal_hostage = 1, reveal_source = 2;
    double inherit_negative = 0.5;
    MemoryRow grief, mentor_grief, humiliated, scarred;
    int estimate_min = 10, estimate_max = 300, estimate_round = 10;
};

}
