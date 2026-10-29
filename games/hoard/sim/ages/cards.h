#pragma once
#include <map>
#include <string>
#include <vector>

namespace ages {

// one condition of a card trigger (data/events/*.json). The key that names it sets `type`:
//   tag       {"tag": "grudge:*", "on": "self", "bind": "other", "absent": false, "min_age_years": 0, "ref": "kind@var"}
//   field     {"field": "cohesion", "on": "self", "lt": 0.4, "of": "other.power"}   (or "in": [...]; world fields include
//             "regions:<race>" and "remnant:<race>"; see card_match.cpp for the field names)
//   event     {"event": "region_taken", "self_as": "tag", "other_from": "subject", "within_years": 20, "min_count": 1,
//              "event_tag": "loot:art", "min_magnitude": 0.3}
//   person    {"person": "ruler", "on": "self", "tag": "tyrant", "bind": "person"}   ("" for any role; a person's
//             fields include "alive", so a bound person who died since can be read)
//   adjacent  {"adjacent": "other"} (self borders it; "lair" for the dragon's region)
//   at_war    {"at_war": "other"}
//   find      {"find": "faction", "tag": "successor_of:$other", "adjacent_to": "self", "race": "human", "bind": "rival"}
//   lineage   {"lineage": "other"} (the dragon holds a lineage of it)
//   var       {"var": "other", "eq": "dragon"}   (an unbound variable reads as "")
//   let       {"let": "arg", "field": "race", "on": "self"}
//   any       {"any": [ ... ]}
//   count     {"count": "faction", "bind": "court", "where": [ ... ], "gte": 4, "races_gte": 3, "mean": "dread", "mean_lte": 0.5}
//             (item 04: the courts where `where` holds with $court bound; their number, races and the mean of a field)
// item 04: an event condition with "within_wakings": 2 reads the events since the second-newest waking.
// "absent": true negates tag, event, person, adjacent, at_war, find and lineage conditions. Strings substitute
// variables: $self, $other, $region, $owner, $person, $year and whatever a condition or effect binds.
struct CardPred {
    std::string type;
    std::string on = "self";
    std::string name;                 // tag, field, person role, var name, adjacency or war target, find kind
    std::vector<std::string> kinds;   // event kinds ("act_*" matches every act)
    std::string bind;
    bool absent = false;
    bool optional = false;
    int min_age = 0;
    int within = 100;
    int min_count = 1;
    double min_magnitude = -1e9;       // event: only events at least this large
    std::string op;
    double value = 0;
    std::string of;
    std::vector<std::string> in;
    std::string str;
    std::string self_as, other_from, region_from, person_from, event_tag;
    std::string ref;
    std::string tag, adjacent_to, race;
    std::vector<CardPred> any;
    bool this_sleep = false;          // v0.4 hooks (S3): event conditions read only the events of the current sleep
    // item 04
    int within_wakings = 0;
    std::vector<CardPred> where;
    int races_min = 0;
    std::string mean;
    double mean_max = 1e9;
};

// one effect; `type` from "do", string options in `s`, numbers in `n` (see card_effects.cpp for the list)
struct CardEffect {
    std::string type;
    std::map<std::string, std::string> s;
    std::map<std::string, double> n;
    std::vector<std::string> tags;
    std::vector<CardPred> when;       // "if": the effect applies only when these hold
    std::map<std::string, double> table;   // v0.4: an object-valued option ("races": {"human": 3, "orc": 2})
};

// item 07: "boost" on a branch: while `if` holds its weight becomes (weight + add) x mult
struct WeightBoost {
    std::vector<CardPred> when;
    double add = 0, mult = 1;
};

struct CardBranch {
    double weight = 1;
    std::vector<CardPred> when;       // "if": the first branch whose conditions hold is taken; else a weighted roll
    std::vector<WeightBoost> boost;
    std::vector<CardEffect> effects;
    std::string text_key;
    std::string tag;
};

struct ChanceMult {
    std::vector<CardPred> when;
    double mult = 1;
};

struct CardDef {
    std::string id;
    std::string scope = "faction";    // faction | region | world
    int era_min = 0, era_max = 99;    // the self faction's era (the owner's for a region card)
    int phase_min = 1, phase_max = 5; // the dragon's phase
    std::vector<CardPred> trigger;
    double chance = 1;                // per decade
    std::vector<ChanceMult> chance_mult;
    bool once = false;                // at most once per self
    int cooldown = 0;                 // decades between firings on the same self
    std::vector<CardEffect> effects;
    std::vector<CardBranch> branches;
    std::string text_key, cause, object = "$other", thread;
    std::vector<std::string> tags;
    // v0.4 world (S2)
    std::string kind;                 // the chronicle event's kind (default: the id), so variants share one kind
    std::string subject = "$self";    // read after the effects ran, so it may name what they bound ("$new")
    bool major = false;               // the storyteller raises its chance the longer the valley has been quiet
    bool needs_dragon = false;        // skipped while the egg has not hatched (world tag egg)
    bool keyed = false;               // item 04: the chance rolls on rng_for(seed, id, self@year), not the world stream
};

// data/events/*.json joined into one JSON array (data_io.cpp, or the shell's "events" entry)
bool cards_from_json(const std::string& text, std::vector<CardDef>& out, std::string& err);

}
