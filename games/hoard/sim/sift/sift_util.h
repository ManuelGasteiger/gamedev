#pragma once
#include <string>
#include <vector>
#include "ages/data.h"

namespace ages {

const Event* sift_event(const World& w, int id);   // a chronicle event or an emitted episode or story
int event_level(const Event& e);                    // 0 sim events, 1 episode.*, 2 story.*
bool is_dragon_act(const Event& e);
bool reaches(const World& w, const Event& from, int target, int max_hops);   // target among from's causes
const SiftPattern* find_pattern(const Params& p, const std::string& id);
int template_tier(const World& w, const Params& p, const Event& e);
std::string court_of(const World& w, const std::string& id);   // a region's owner, a person's court, a court itself
std::string race_of(const World& w, const std::string& id);
int valley_era(const World& w);
std::string war_winner(const World& w, const Event& ended);
std::string protagonist(const SiftPattern& p, const SiftThread& t);

}
