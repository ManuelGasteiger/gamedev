#pragma once
#include "ages/ages.h"
#include "ages/json.h"

// item 15: begin_sortie {sortie} -> the snapshot as the shell reads it; end_sortie {report} -> the acts the diff made and
// what apply_acts returns (changed, events, lines, refused, hooks_resolved, wounds, slain) plus warnings
nlohmann::json raid_begin_sortie(ages::World& w, const ages::Params& p, const nlohmann::json& in);
nlohmann::json raid_end_sortie(ages::World& w, const ages::Params& p, const nlohmann::json& in);
