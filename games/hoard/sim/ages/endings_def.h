#pragma once
#include <map>
#include <string>
#include <vector>
#include "cards.h"

// item 04: data/endings.json (the registry, the prophecy templates, the legend weaving)

namespace ages {

struct RumorRoute {
    std::string via;                 // death_report | prophecy | legend | event
    std::vector<std::string> kinds;  // event: the kinds a briefing must tell
};

struct EndingCond {
    std::string id;
    std::vector<CardPred> when;
    int hold_years = 0, lapse_years = 0;
    bool with_previous = false;
    std::vector<RumorRoute> rumors;
};

struct EndingDef {
    std::string id, exit_kind, offer_text_key, epilogue_text_key, sketch;
    bool enabled = false;
    std::vector<EndingCond> conds;
};

struct WeaveDef {
    std::string kind, verdict, on, tag;   // verdict: awe | dread | slain | any; on: court | region
};

struct LegendWeave {
    double chance = 0.5;
    int max_active = 3;
    int year_lo = -900, year_hi = -200;   // relative to the hatching
    int events_lo = 1, events_hi = 3;
    std::map<std::string, int> tier_decades;   // how long a legend tag lasts per recorded tier (0: always)
    std::vector<WeaveDef> weaves;
};

struct ProphecyLineDef {
    std::string key, cond;
    int variants = 1;
    std::map<std::string, std::string> slots;
};

struct ProphecyDef {
    std::string ending, known_tag = "prophecy_known", book_tag = "prophecy_book";
    int years_lo = 100, years_hi = 300;   // before the hatching
    std::vector<ProphecyLineDef> lines;
};

struct EndingsDef {
    bool present = false;
    int epilogue_decades = 50;
    int myth_deeds = 5;
    LegendWeave legends;
    ProphecyDef prophecy;
    std::vector<EndingDef> endings;
};

bool endings_from_json(const std::string& text, EndingsDef& out, std::string& err);   // endings_data.cpp
const EndingDef* ending_def(const EndingsDef& d, const std::string& id);

}
