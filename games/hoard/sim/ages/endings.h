#pragma once
#include <string>
#include <vector>
#include "data.h"
#include "state.h"

// item 04: endings as condition sets on world state (data/endings.json), the exit act and its epilogue

namespace ages {

// one tick of a condition at `year`: a gap longer than lapse_years resets held_since
void advance_hold(CondHold& h, bool met, int year, const EndingCond& c);
bool cond_held(const CondHold& h, int year, const EndingCond& c);   // met now and held for hold_years
int years_held(const CondHold& h, int year);

// conditions of an ending evaluated now; `court` gets the court the conditions bound
std::vector<bool> conditions_met(const World& w, const Params& p, const EndingDef& e, std::string* court = nullptr);

const EndingState* ending_state(const World& w, const std::string& id);
bool ending_ready(const World& w, const std::string& id);
bool run_ended(const World& w);

// the exit act: refused unless ending_ready:<kind> holds; ends the run, emits `ending` and runs the epilogue
bool exit_unrecorded(World& w, const Params& p, const std::string& kind, std::string& reason);
bool exit_run(World& w, const Params& p, const std::string& kind, std::string& reason);   // recorded.cpp

// the exit offer rows of the ready endings (offers.cpp, at the waking)
void exit_offers(World& w, const Params& p);

}
