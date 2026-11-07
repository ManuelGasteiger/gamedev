#pragma once
#include <string>
#include <vector>
#include "data.h"
#include "state.h"

// item 09: what the scripted policies' newer acts aim at (a person of a court, a region kind, a second court) and the
// waking's leftovers they answer by act (hooks ignored, offers sworn, a ready ending taken)

namespace ages {

std::vector<std::string> extra_targets(const World& w, const Params& p, const std::string& key);   // faithful | awed | orc | hostages | pretender
std::string person_of(const World& w, const Faction& f, const std::string& key);                   // ruler | heir | hero | sage | priest | hostage | willing | visitor | slayer | judged; "" when none
std::string region_of(const World& w, const Faction* f, const std::string& key);                   // lair | wild | wild_burned | temple
std::string second_court(const World& w, const Params& p, const Faction& f, const std::string& key);   // neighbour | enemy | non_orc_neighbour
void policy_leftovers(World& w, const Params& p, const Policy& policy);

}
