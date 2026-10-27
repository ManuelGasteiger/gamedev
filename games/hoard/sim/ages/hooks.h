#pragma once
#include <string>
#include <vector>
#include "data.h"
#include "state.h"

// v0.4 hooks (S3): the situations waiting for the dragon when it wakes (docs/core-loop.md section 5), from
// data/hooks.json. Generated at the end of every sleep into w.hooks, answered by the acts of the raid (apply_acts) or
// by answer_hook, expired with their ignore effect at the start of the next sleep.

namespace ages {

// hooks.cpp
std::vector<Hook> generate_hooks(World& w, const Params& p);   // w.hooks after the call
void expire_hooks(World& w, const Params& p);                  // hook_ignored plus the ignore effect for every open hook
std::string lair_region(const World& w);
const HookDef* hook_def(const Params& p, const Hook& h);

// hook_resolve.cpp
bool hook_matches(const World& w, const Hook& h, const Act& a);
void resolve_hooks(World& w, const Params& p, const Act& a);   // answers every open hook the act matches
bool answer_hook(World& w, const Params& p, const std::string& id, const std::string& choice, std::string& reason);
std::map<std::string, double> apply_reward(World& w, const Params& p, const std::string& faction, const std::string& other, const std::map<std::string, double>& r);   // item 06: the reward keys of hooks.json
double hook_fight_damage(const World& w, const Params& p, const Hook& h);   // after armor
void apply_hook_ignore(World& w, const Params& p, const Hook& h);

// hook_policy.cpp
void answer_hooks(World& w, const Params& p, const Policy& policy);   // a policy with answer_hooks resolves what it can

}
