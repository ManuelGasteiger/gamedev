#pragma once
#include "../data.h"
#include "../pop.h"
#include "../state.h"

namespace ages {

void rule_modifiers(World&, const Params&);
void rule_wealth(World&, const Params&);
void rule_cohesion(World&, const Params&);
void rule_era(World&, const Params&);
void rule_war(World&, const Params&);
void rule_vassals(World&, const Params&);
void rule_collapse(World&, const Params&);
void rule_memory(World&, const Params&);
void rule_dragon_rolls(World&, const Params&);
void anti_dragon_research(World&, const Params&);   // v0.4 (S1), anti_dragon.cpp
void anti_dragon_marks(World&, const Params&);      // v0.4 (S1), anti_dragon.cpp
void rule_fading(World&, const Params&);
void rule_migration(World&, const Params&);
void rule_remnants(World&, const Params&);
void rule_regimes(World&, const Params&);
void rule_hegemony(World&, const Params&);
void rule_land(World&, const Params&);
void rule_oaths(World&, const Params&);
void rule_shocks(World&, const Params&);
void rule_cards(World&, const Params&);      // event cards of data/events (doc section 10, catalogue rule 14)
void rule_people(World&, const Params&);     // 17.2
void rule_threads(World&, const Params&);    // 17.8: closes threads whose arc ended this step
void rule_chronicle(World&, const Params&);

void init_faction_ruler(Faction& f, const Constants& k, Rng& rng);
void jitter_traits(Faction& f, const Constants& k, Rng& rng);
void make_vassal(World& w, Faction& vassal, const Faction& overlord, const Cause& cause);
Faction found_faction(World& w, const Params& p, const std::string& race, const std::string& region, double wealth,
                      double cohesion, int era);
void refound_remnant(World& w, const Params& p, Remnant& rem, const std::string& region, const std::string& reason);
void note_race_eras(World& w);
void transfer_region(World& w, const std::string& region, const std::string& from, const std::string& to);
void remove_faction(World& w, const std::string& id);
War& start_war(World& w, Faction& a, Faction& b, const std::string& reason, const Cause& cause);
void end_war(World& w, const War& war, const std::string& reason, const Cause& cause);
int weakness_cause(const World& w, const Constants& c, const Faction& b);

// people.cpp
void init_people(World& w, const Params& p);
void kill_person_now(World& w, const Params& p, const std::string& id, const std::string& reason, const Cause& cause);
Person new_adult(World& w, const Params& p, const std::string& race, const std::string& faction, const std::string& role);
void crown_person(World& w, const Params& p, Faction& f, const std::string& id, const std::string& how, const Cause& cause);
Person* pick_hostage(World& w, const std::string& faction);   // the heir, else the ruler
void take_hostage_person(World& w, const Params& p, Person& person, bool consent, const Cause& cause);
void release_hostage_person(World& w, const Params& p, const std::string& faction, const Cause& cause);
void fall_person(World& w, const Params& p, const std::string& id, const std::string& reason, const Cause& cause);   // item 07

// item 07: person memory, secrets, the inquisition (after rule_people)
void rule_person_memory(World&, const Params&);
void rule_secrets(World&, const Params&);
void rule_inquisition(World&, const Params&);
double secret_rate(const World& w, const Params& p, const Secret& s);       // discovery chance per decade
double purge_strength(const World& w, const Params& p, const Secret& s);    // share of the court in the purge faction

}
