#pragma once
#include <string>
#include "ages/ages.h"
#include "ages/profile.h"

// item 04: the inter-run pacing bots. Every bot takes an exit the waking offers. amnesiac plays the base policy;
// carry steers toward the ending its profile knows most atoms of; oracle steers toward its target from the first run.
// Steering swaps the waking's acts for data/policies/steer_<ending>.json and, for Mortality, receives every visitor.

struct Bot {
    std::string kind;     // amnesiac | carry | oracle
    std::string target;   // oracle: the ending it aims at
};

bool parse_bot(const std::string& spec, Bot& out);   // "carry", "oracle:ascension"
std::string bot_target(const Bot& bot, const ages::Profile& pr, const ages::Params& p);
void attach_bot(ages::Scenario& s, const ages::Params& p, const std::string& target);
