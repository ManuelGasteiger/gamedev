#pragma once
#include <ostream>
#include <string>
#include <vector>
#include "ages/ages.h"

// item 10: the runner's scenario commands. --scenario-bot answers each waking's scenarios by the bot's act preference
// (data/bots/scenario_answers.json); --scenario-log writes one JSON line per waking; --lab runs the forced-state test.

namespace ages {

struct ScnRun {
    std::string bot;          // a key of data/bots/scenario_answers.json ("" answers nothing)
    std::ostream* log = nullptr;
    int logged = -1;          // the last waking written
    std::vector<std::string> refused;   // answers the sim refused this waking: "<scenario>.<option>: <reason>"
};

bool attach_scenarios(Scenario& s, const Params& p, const std::string& data, ScnRun& run, std::string& err);
void log_waking(const World& w, ScnRun& run);
int scn_lab_cmd(const std::string& data, const std::string& spec_path);

}
