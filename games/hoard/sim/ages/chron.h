#pragma once
#include <map>
#include <string>
#include <vector>
#include "data.h"
#include "../sift/sift.h"

// item 05: chronicle levels, sources in the hoard, the kept historian, relics with a voice, the level-3 counterfactual

namespace ages {

// levels: 0 dreams, 1 a source, 2 a kept historian, 3 a historian and high Lore
bool historian_at(const World& w, int year);
const Person* historian_now(const World& w);
bool lore_counterfactual(const World& w, const Params& p);
std::vector<Source> sources_for(const World& w, const Params& p, int match);   // held, plus what the captives of a court know of it
int line_level(const World& w, const Params& p, int year, int match);

// sources
void take_sources(World& w, const Params& p, const Faction& f, double m);
void give_sources(World& w, double m);

// the historian
void close_tenures(World& w);
bool keep_person_unrecorded(World& w, const Params& p, const std::string& id, const std::string& role, std::string& reason);
void age_in_hoard(World& w, int years);   // a kept person tagged sleeps_in_hoard ages at a tenth

// relics
void find_relic(World& w, const Params& p, const Faction& f, double m);
int waking_index(const World& w);
const Relic* speaking_relic(const World& w);
std::map<std::string, std::string> voice_line(const World& w, const Params& p);   // empty when nothing speaks
bool commune_unrecorded(World& w, const Params& p, const std::string& relic, const std::string& answer, std::string& reason);

// the recorded verbs (recorded.cpp)
bool keep_person(World& w, const Params& p, const std::string& id, const std::string& role, std::string& reason);
bool commune(World& w, const Params& p, const std::string& relic, const std::string& answer, std::string& reason);

// level 3
void snapshot_at_wake(World& w, const Params& p);
void counterfactual_after_sleep(World& w, const Params& p, int years, size_t acts_end);

}
