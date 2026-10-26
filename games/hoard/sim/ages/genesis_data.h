#pragma once
#include <map>
#include <string>
#include <vector>

namespace ages {

// data/genesis.json (core-loop.md sections 5 and 6): the terrain deal, region names, the start deck, the pre-history,
// the irreversible events' numbers and the storyteller. A missing file leaves `present` false.

struct Span {
    double lo = 0, hi = 0;   // drawn uniformly from [lo, hi]; integers from lo..hi inclusive
};

struct TerrainDef {
    double land = 0.5, jitter = 0;
    std::vector<std::string> name_a, name_b;   // a region's name is a + b
};

// the deck: `base` cards, a template's own minimum, then `wild` draws by weight until every region has one
struct DealDef {
    std::map<std::string, int> base, max;
    std::map<std::string, double> wild;
    std::vector<std::string> cluster;                 // dealt as one connected run when there are two or more
    std::map<std::string, int> lair_max, lair_min;    // among the lair's neighbours
    int tries = 20;
};

struct CourtSpec {
    std::string label;
    std::map<std::string, double> race;   // weights; "fallen" stands for the fallen kingdom's race
    std::map<std::string, double> mix;    // a minority people blended into the court ...
    Span mix_share;                       // ... at this share
    Span count{1, 1}, regions{1, 1}, wealth{20, 20}, cohesion{0.9, 0.9}, era{0, 0};
    double chance = 1;
    std::vector<std::string> terrain;     // capital terrains; empty: the race's home terrain
    bool any_terrain = false;             // grows onto free land of any terrain
    std::string near;                     // "ruins" or a court label: the capital borders it when it can
    std::map<std::string, double> traits; // added to the jittered race traits
    std::vector<std::string> tags;        // "$fallen" is the fallen kingdom's name
    bool undying = false;                 // the ruler does not die (undying.cpp)
};

// war | vassal (b swears to a) | alliance | old_enemies | mark (tag on a, "$b" is b's id); b "neighbour": any neighbour of a
struct RelationSpec {
    std::string kind, a, b, tag;
    double chance = 1;
    int decades = 0;
};

struct RemnantSpec {
    std::map<std::string, double> race;
    Span strength;
    double chance = 1;
};

struct StartTemplate {
    std::string id;
    double weight = 1;
    std::map<std::string, int> deal;                // at least this many regions of a terrain
    std::map<std::string, double> fallen;           // race weights of a kingdom that fell just before the start
    Span ruins, ruin_burn, ruin_age, fallen_reign{300, 300};
    int fallen_era = 0;
    double ruin_wasteland = 0;
    std::map<std::string, double> fallback_race;
    std::vector<CourtSpec> courts;
    std::vector<RelationSpec> relations;
    std::vector<RemnantSpec> remnants;
};

struct LegendsDef {
    int max = 10, per_kind = 2, per_century = 2;
    std::vector<std::string> skip;   // kinds never told as legends
};

struct ExtinctionDef {
    std::vector<std::string> races;  // the peoples that can die out
    double strength = 0.25;          // their remnant is weaker than this, or gone
    double pop = 0.15;               // no court is this much of them
    int gone_years = 200;            // they have had no court for this long ...
    double assimilate = 0.05;        // ... and from then on their share of every court shrinks by this per decade
    double chance = 0.02;            // per decade once all of it holds
};

struct UndyingDef {
    double cohesion_floor = 0.35;    // while the undying ruler lives the realm does not fall apart
    double fall_chance = 0.004;      // per decade the undying ruler is overthrown
    double fall_cohesion = -0.3;
    int empire_years = 1000;         // the reign the interest tool counts as a long empire
};

struct MagicDef {
    double start = 0.3, rise = 0.04, cap = 0.8, ebb = 0.01;   // world.magic per decade during the age of magic, and after it
};

struct EmptyLandDef {
    double razed_burn = 0.3;         // land this burned when its owner left is `razed` (the restless dead rise from it)
};

struct DeckEntry {
    std::string card;
    double weight = 1;
};

// the storyteller counts in wakings: a waking with a major event is followed by a breather; otherwise a waking brings a
// storyteller major with chance base + per_window x quiet years / the phase's moult window (capped), and is forced to
// after forced_after wakings without one. Every sleep of at least the short window ends with an event of at least
// briefing importance_min near its end (from wake_deck when nothing else did it). The storyteller's events land at the
// end of the waking.
struct StorytellerDef {
    double major_importance = 0.5;   // an event this important is major: it resets the quiet clock
    double chance_base = 0.15, chance_per_window = 0.25, chance_max = 0.8;
    int forced_after = 3;
    int forced_years = 0;            // ... or once the valley has been this quiet (0: never)
    double second_chance = 0.5;      // a storyteller major brings a second one
    double near_end_share = 0.2;     // the end of a sleep: this share of it, at least one step
    int prehistory_waking_years = 100;   // the pre-history runs the cycle in wakings of this length
    int grace_years = 60;            // quiet years before major cards grow likelier ...
    double major_per_year = 0.01, major_max = 3;   // ... by this per quiet year, up to this multiplier
    double breather_mult = 0.5;      // major cards during a breather
    std::vector<DeckEntry> deck, wake_deck;
};

struct GenesisDef {
    bool present = false;
    std::map<std::string, TerrainDef> terrain;
    DealDef deal;
    std::vector<std::string> rim;    // regions at the valley's edge, where newcomers come in
    int min_courts = 3, max_courts = 6;
    CourtSpec fallback;
    Span prehistory{300, 900};       // years before the egg hatches, whole decades
    LegendsDef legends;
    std::vector<StartTemplate> templates;
    ExtinctionDef extinction;
    UndyingDef undying;
    MagicDef magic;
    EmptyLandDef empty_land;
    StorytellerDef story;
};

bool genesis_from_json(const std::string& text, GenesisDef& out, std::string& err);

}
