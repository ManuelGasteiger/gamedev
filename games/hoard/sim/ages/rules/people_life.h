#pragma once
#include <string>
#include "rules.h"

// shared by the people rule files: people.cpp (lives, deaths, crowning), people_rise.cpp, hostages.cpp
namespace ages {
namespace lives {

bool immortal(const Faction& f);
Person* ruler_of(World& w, const std::string& faction);
Person* heir_of(World& w, const std::string& faction);
void grown(World& w, const Params& p, Person& x);   // born accession_age (with jitter) ago, death of age set
int retire(World& w, const Params& p, Person person, const std::string& reason, const Cause& cause);   // person_died; returns its id
void crown(World& w, const Params& p, Faction& f, Person& person, const std::string& how, const Cause& cause);
void crown_new(World& w, const Params& p, Faction& f, const std::string& how, const Cause& cause);

void name_reigns(World& w, const Params& p);         // people_rise.cpp
void heroes(World& w, const Params& p);
void sages_and_priests(World& w, const Params& p);
void raise_hostages(World& w, const Params& p);      // hostages.cpp

}
}
