#pragma once
#include <string>
#include "../data.h"
#include "../state.h"

namespace ages {

// v0.4 world (S2): the rules after the event cards, in this order (world.cpp)
void rule_world(World& w, const Params& p);
void rule_empty_land(World& w, const Params& p);    // world.cpp: years ownerless, `razed` land
void rule_magic(World& w, const Params& p);         // magic.cpp
void rule_undying(World& w, const Params& p);       // undying.cpp
void rule_extinction(World& w, const Params& p);    // extinction.cpp
void rule_storyteller(World& w, const Params& p);   // storyteller.cpp: the quiet clock

// storyteller.cpp: the chance multiplier of major cards (quiet years, breathers), and the end of a waking (sleep.cpp; the
// pre-history every prehistory_waking_years): the cycle's major event, and with `guarantee` the wake guarantee
double storyteller_major_mult(const World& w, const Params& p);
void storyteller_wake(World& w, const Params& p, int slept, bool guarantee);

// extinction.cpp: the race is gone for good: its remnant and its last minorities go, world tag extinct:<race>
void extinguish(World& w, const Params& p, const std::string& race);

// undying.cpp: the ruler of f stops dying; the realm cannot fall apart while that ruler lives. Announced by an
// undying_rose event unless `event` (a card's) already tells it.
void make_undying(World& w, const Params& p, Faction& f, const Cause& cause, int event = 0);

}
