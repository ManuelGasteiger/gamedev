#pragma once
#include <string>
#include <vector>
#include "data.h"
#include "state.h"

// item 08: the people of a slayer order (data/slayers.json). Rolls here draw from keyed streams, never world.rng.

namespace ages {

Order* find_order(World& w, const std::string& court);
const Order* find_order(const World& w, const std::string& court);
std::vector<Person*> roster(World& w, const std::string& court);   // by id
std::vector<const Person*> roster(const World& w, const std::string& court);
const Person* grandmaster(const World& w, const std::string& court);
Person* grandmaster(World& w, const std::string& court);

double power_total(const World& w, const Params& p, const Order& o);   // named power plus hunters at hunter_power
std::string dragon_axis(const World& w);                              // the growth axis the dragon holds most of
bool counters_dragon(const World& w, const Params& p, const Person& x);
bool is_zealot(const Params& p, const Person& x);
bool is_opportunist(const Params& p, const Person& x);
std::vector<std::string> weaknesses(const Params& p, const Person& x);   // the fear and every axis the counter does not cover

const char* const kTraitKinds[3] = {"counter", "fear", "hate"};   // the order a reveal learns them in
bool knows(const Person& x, const std::string& kind);
void reveal(Person& x, int n);   // learns n unknown kinds
void reveal_all(Person& x);

void raise_power(const Params& p, Person& x, int by);   // clamped to 1..cap; the fear stays at every power
bool add_scar(World& w, const Params& p, Person& x);    // the cause follows the dragon's strongest axis; writes scarred and the memory
void remember(World& w, const Params& p, Person& x, const MemoryRow& row, const std::string& src);

void enrol(World& w, const Params& p, Person& x, const std::string& rank);   // traits and power from keyed streams
Person* recruit(World& w, const Params& p, Order& o, const std::string& rank);   // a new named slayer

bool promote(World& w, const Params& p, const std::string& court, const std::string& rank, const std::string& exclude = "");   // best candidate from the rank below
void rival_gain(World& w, const Params& p, const Person& harmed);   // the harmed slayer's rivals gain rival_gain power
void slayer_died(World& w, const Params& p, const Person& dead);    // mentor grief, the successor and the grandmaster's grudge (called from bury)

}
