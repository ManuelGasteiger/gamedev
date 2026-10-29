#pragma once
#include <string>
#include "data.h"
#include "state.h"

namespace ages {

// v0.4 genesis (core-loop.md section 6), opt-in: a world built from data/genesis.json instead of factions.json, a
// pure function of the seed and the data (w.rng only). Years stay absolute: the pre-history runs from year 0 and the
// egg hatches at genesis.hatched_year; the pre-history's events carry the tag `prehistory`.

// genesis_world.cpp: replaces w with a generated world (same seed and region graph), runs the pre-history while the egg
// lies in the mountain (world tag `egg`), then hatches it
bool make_genesis(World& w, const Params& p, std::string& err);
// item 04: with the legends of a profile woven into the pre-history; the prophecy is written in both
struct Profile;
bool make_genesis(World& w, const Params& p, const Profile& profile, std::string& err);

// genesis_map.cpp: deals the terrain of every region but the lair, sets land from terrain with jitter, names regions
void deal_map(World& w, const Params& p, const StartTemplate& t);
std::string region_name(World& w, const GenesisDef& g, const std::string& terrain);

// genesis_start.cpp: draws a start template, deals the map, lays the ruins and places courts, relations and remnants
bool genesis_start(World& w, const Params& p, std::string& err);

// legends.cpp: the pre-history's highlights, chronological: {ok, events: [{year, text, importance}]}
std::string legends_json(const World& w, const Params& p);

}
