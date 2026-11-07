#pragma once
#include <map>
#include <string>
#include <vector>
#include "ages/data.h"
#include "ages/state.h"
#include "bank.h"
#include "predicate.h"

// item 10: what a scenario reads of the world: the vocab fields per role kind and of dragon, lair and world
// (vocab.json "fields"), tags, and the act log by waking

namespace scn {

// computed once per selection; acts' waking index is the number of sleep acts before them
struct Cache {
    int waking = 0;   // sleeps so far: the waking the dragon is in
    std::vector<int> act_waking;
    std::map<std::string, std::vector<int>> acts_by_type;   // act indices, oldest first
    std::map<std::string, int> renown, wealth_rank;
    std::string strongest;
    int strongest_era = 0;
    int ignored_streak = 0;
};
Cache make_cache(const ages::World& w, const ages::Params& p);

struct Ctx {
    const ages::World& w;
    const ages::Params& p;
    const Def& d;
    const Cache& c;
    std::map<std::string, std::string> bind;   // role -> id (an act role: its index)
    bool bound(const std::string& role) const { return bind.count(role) > 0; }
    std::string kind(const std::string& role) const;
    std::string id(const std::string& role) const;
    const ages::Act* act(const std::string& role) const;
};

int act_age_wakings(const Cache& c, int act);
std::string owner_of(const ages::World& w, const std::string& id, const std::string& kind);
std::string act_court(const ages::World& w, const ages::Act& a);   // the court an act fell on

Value field(const Ctx& x, const std::string& role, const std::string& f);
Value faction_value(const Ctx& x, const ages::Faction& f, const std::string& k);
bool has_tag_of(const Ctx& x, const std::string& role, const std::string& tag);
PredEnv env_of(const Ctx& x);
bool tag_match(const ages::Tags& t, const std::string& tag);   // exact, or `tag:<anything>`

// the person helpers the binders share
std::string person_court(const ages::World& w, const ages::Person& x);   // "lair" for a hostage
bool person_role(const ages::World& w, const ages::Person& x, const std::string& role);
bool person_tag(const ages::World& w, const ages::Person& x, const std::string& tag);
bool person_is_lie(const ages::World& w, const ages::Person& x);
std::string memory_tier_word(const ages::World& w, const ages::Params& p, const ages::Faction& f);
int memory_rank(const std::string& tier);
std::string size_word(const Numbers& n, double magnitude);

// vocab gate rows on a court (arrival.needs)
bool gate_open(const ages::World& w, const ages::Params& p, const Bank& b, const std::string& gate, const ages::Faction& f);

}
