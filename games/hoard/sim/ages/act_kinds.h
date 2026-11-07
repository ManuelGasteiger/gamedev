#pragma once
#include <string>
#include "data.h"
#include "moves.h"
#include "state.h"

// item 09: the acts by family. *_refusal checks the world before anything is written ("" when the act may land);
// *_effects resolves the act after it is logged and returns false for a type the family does not own. Routed acts
// call the API verb that existed before (that verb logs them).

namespace ages {

bool act_routed(const Params& p, const Act& a);
std::string apply_routed(World& w, const Params& p, const Act& a);   // "" when the verb accepted it

std::string force_refusal(const World& w, const Params& p, const Act& a);
bool force_effects(World& w, const Params& p, Act& a, double m, int act_index);
std::string speech_refusal(const World& w, const Params& p, const Act& a);
bool speech_effects(World& w, const Params& p, Act& a, double m, int act_index);
std::string arcana_refusal(const World& w, const Params& p, const Act& a);
bool arcana_effects(World& w, const Params& p, Act& a, double m, int act_index);
std::string lair_refusal(const World& w, const Params& p, const Act& a);
bool lair_effects(World& w, const Params& p, Act& a, double m, int act_index);

// the Teach, Corrupt and Court moves' context: the dragon's growth and talents, in guise at the person's court this waking
MoveCtx move_ctx(const World& w, const Person& x, int act_index);

void sponsor_remnant(World& w, const Params& p, const Act& a);   // acts.cpp: the remnant named by a.race rises
void lair_roles_after_sleep(World& w, const Params& p);         // acts_lair.cpp: a kept consort's line

// a court learns a secret act after the fact (reveal, a discovery roll): the act's reaction row at weight 1
void learn_act(World& w, const Params& p, Faction& f, int act_index);

}
