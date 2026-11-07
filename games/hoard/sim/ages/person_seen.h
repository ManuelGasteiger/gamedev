#pragma once
#include "state.h"

// item 04b: the player sees a person only after contact: met (the known ledger), in the lair, at the ledge (an envoy
// or a visitor), or named by what the hoard holds: a captive or a source (books) of their court, or the kept
// historian, who names the rulers and heirs of every court. The sim simulates everyone; only the views filter.

namespace ages {

bool person_seen(const World& w, const Person& x);

}
