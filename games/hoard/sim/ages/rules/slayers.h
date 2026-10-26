#pragma once
#include <string>
#include "../data.h"
#include "../state.h"

// v0.4 (S1): the slayer clock. A court with the slayers tag and fear above slay_fear forms a band (slayers_forming),
// readies it at least one waking later (slayers_ready, slay_ready_rate per decade) and tries the slaying at the
// first step of the sleep after that; Premonition turns the attempt into a wake once. The band's strength follows the
// dragon, as RimWorld's raid points do: (base + hoard + growth points) x the court's era multiplier x the adaptation
// factor; the odds set it against the dragon's health.

namespace ages {

double slayer_strength(const World& w, const Params& p, const Faction& f);
double slaying_odds(const World& w, const Params& p, const Faction& f);   // at the dragon's current wounds
std::string slayer_stage(const World& w, const std::string& faction);    // "", "slayers_forming" or "slayers_ready"
double adaptation_factor(const World& w, const Params& p);
void slayer_clock(World& w, const Params& p);
void disperse_slayers(World& w, Faction& f);   // the band is gone (killed at the gate): the clock starts over
void adapt_after_sleep(World& w, const Params& p, double wounds_share_at_start, size_t first_event);

}
