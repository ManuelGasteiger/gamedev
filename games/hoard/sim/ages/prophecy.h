#pragma once
#include "data.h"
#include "state.h"

// item 04: the hatching prophecy (data/endings.json "prophecy"): written into the most lettered court's library during
// the pre-history, known by the courts holding the known tag, spread by a take of books from or a teach on a knowing court

namespace ages {

int prophecy_year(const World& w, const Params& p);   // the pre-history year it is written (0: none)
void write_prophecy(World& w, const Params& p);
void refresh_prophecy(World& w, const Params& p);    // known_by from the known tags (successors inherit them)
void prophecy_after_act(World& w, const Params& p, const Act& act);
void prophecy_after_teach(World& w, const Params& p, const std::string& court);

}
