#pragma once
#include <string>
#include "data.h"
#include "json.h"
#include "state.h"

// item 08: what the briefing tells of an order: the grandmaster's scars and traits (`?` for the unlearned), the band's
// power against the dragon's threat, and the year of the attempt as the sim would estimate it now

namespace ages {

std::string slayer_details(const World& w, const Params& p, const Person& x);   // "marked by you: burned; counter ?, hates smoke, fears ?"
int attempt_estimate(const World& w, const Params& p, const Faction& f);        // the year, rounded up to estimate.round
nlohmann::json slayers_block(const World& w, const Params& p);                  // one entry per order at stage forming or ready, or with a premonition

}
