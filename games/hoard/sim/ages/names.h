#pragma once
#include <string>
#include "data.h"
#include "state.h"

namespace ages {

std::string new_faction_name(World& w, const Params& p, const std::string& race);

// a new person with id <race><n>, a syllable name from data/names.json, random traits and an epithet for a strong trait;
// not yet added to w.people (the caller sets role, faction and lifespan, then calls add_person)
Person new_person(World& w, const Params& p, const std::string& race, const std::string& faction, const std::string& role);
void add_person(World& w, const Person& person);
std::string epithet_for(const Params& p, const Person& person);
std::string display_name(const World& w, const std::string& id);   // person name, faction or region label, "you" for the dragon

}
