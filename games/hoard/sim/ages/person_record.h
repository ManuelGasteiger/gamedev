#pragma once
#include <string>
#include "data.h"
#include "state.h"

// item 07: the person record (traits, acquired tags, memory of the dragon, relations); the rules that age it are in
// rules/person_memory.cpp, the moves in moves.cpp

namespace ages {

struct Axes {
    double dread = 0, awe = 0, debt = 0;
};

void assign_traits(const World& w, const Params& p, Person& x);   // keyed rng, no world draw
bool has_trait(const Person& x, const std::string& trait);
bool tracked(const World& w, const Params& p, const Person& x);   // within the court's cap of full records
Axes axes(const Params& p, const Person& x);                      // clamped sums of s x sev x ax over mem and permanent
double entry_value(const PersonMemory& m, const std::string& axis);   // s x sev x ax

int acquired_count(const Params& p, const Person& x);
bool acquire(const Params& p, Person& x, const std::string& tag);   // false when the slots are full

PersonMemory memory_entry(const World& w, const Params& p, const Person& x, const MemoryRow& row, const std::string& src, int act = -1);
void remember_dragon(const Params& p, Person& x, const PersonMemory& m);
void add_rel(Person& x, const std::string& to, const std::string& kind, int weight);

// a dragon act on a court: its tracked people saw it (witness), the tracked people of the witness courts heard of it (told)
void witness_act(World& w, const Params& p, const Faction& f, const Act& a, double m, int act_index);
// the dead's memory of the dragon passes to the children: 50 % of negative, 25 % of positive strength
void inherit_memory(World& w, const Params& p, const Person& dead);

}
