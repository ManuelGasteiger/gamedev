#pragma once
#include <string>
#include <vector>
#include "data.h"
#include "hooks.h"
#include "offers.h"
#include "state.h"

// item 06: nobody comes to the lair at random. data/arrivals.json names, per kind, the gates a court must pass; the
// rule (rules/arrivals.cpp) lets one visitor per court and tick through; this file's functions keep what knows the
// lair, what waits there, and what the dragon's answers do.

namespace ages {

// arrival_gates.cpp
int lair_distance(const World& w, const std::string& region);   // regions from the lair, 1000 when unreachable
void learn_lair(World& w, Faction& f, bool exact);
void note_lair_sight(World& w, const Params& p, const Act& a);   // after an act is witnessed: who now knows where the lair is
void refresh_lair_knowledge(World& w, const Params& p);          // the oral tier fades, written records keep it
bool route_open(const World& w, const Faction& f);

struct GateCtx {
    const World& w;
    const Params& p;
    const Faction& f;
    bool motive = false;   // an envoy motive is on offer (the rule fills it in)
};
bool gate_holds(const GateCtx& g, const ArrivalGateDef& d);
std::string gate_key(const ArrivalGateDef& d);

// arrivals.cpp
const ArrivalDef* arrival_def(const Params& p, const std::string& kind);
const Arrival* arrival_for(const World& w, const std::string& kind, const std::string& court);
Arrival* find_arrival(World& w, const std::string& id);
void arrival_consume(World& w, const std::string& id);
void arrivals_depart(World& w, const Params& p);    // the dragon sleeps: whoever still waits goes home
void arrivals_at_wake(World& w, const Params& p);   // the dragon wakes: short windows and expired offers have passed
void arrival_ignored(World& w, const Params& p, const Hook& h);   // an unanswered hook of an arrival
void envoy_returned(World& w, const Offer& of);     // the envoy leaves alive: the court has seen the lair
std::string ledge_text(const World& w, const Params& p, const Arrival& a);

// arrival_acts.cpp: the verbs (recorded wrappers are in recorded.cpp)
bool receive_unrecorded(World& w, const Params& p, const std::string& id, const std::string& outcome, std::string& reason);
bool receive_outcome(World& w, const Params& p, const Arrival& a, const std::string& outcome);
bool hunt_unrecorded(World& w, const Params& p, const std::string& region, std::string& reason);
bool receive(World& w, const Params& p, const std::string& id, const std::string& outcome, std::string& reason);
bool hunt(World& w, const Params& p, const std::string& region, std::string& reason);

// rules/arrivals.cpp
void rule_arrivals(World& w, const Params& p);

}
