#pragma once
#include <string>
#include "data.h"
#include "state.h"

// Dread and awe as stocks per memory tier (act-api section 6): deposits, decay, the library and the monuments.

namespace ages {

double tiers_eff(const Tiers& t);   // 1 - prod(1 - x) over the tiers
inline double dread_eff(const Faction& f) { return tiers_eff(f.dread); }
inline double awe_eff(const Faction& f) { return tiers_eff(f.awe); }

// the highest tier holding at least tier_shown of dread or awe ("" when none)
std::string memory_tier(const Faction& f, const MemoryConstants& m);

// a court writes its memories down when it has a living sage or historian, or a region tagged library
bool keeps_records(const World& w, const Faction& f);

// what one act leaves on one court: base gains already times magnitude, witness weight and relationship; the
// deposit applies the (1 - eff) saturation, writes each tier the court has, and records the memory entries.
// tier_up (Dread Flame on a burn): written at full strength, monumental at the written share
struct Deposit {
    std::string type;
    double magnitude = 0;
    double dread = 0, awe = 0;
    int act = -1;
    std::string region;
    Tags tags;
    bool tier_up = false;
};
void deposit(World& w, const Params& p, Faction& f, const Deposit& d);

// moves the oral stock so the tier's effective value becomes `want` (cards and hooks that set or floor dread or awe)
void set_eff(Tiers& t, double want);

// the same-type acts this court remembers from the last rep_years (the awe repetition factor counts them)
int recent_acts(const Faction& f, const std::string& type, int year, double years);

void decay_tiers(World& w, const Params& p, Faction& f);
void refresh_monumental(Faction& f);
void burn_library(Faction& f, const MemoryConstants& m);            // written stocks and entries x library_burn
void raze_monuments(Faction& f, const std::string& region, const std::string& bias_any);   // "" any bias

double oath_stability(const Faction& f, const MemoryConstants& m);    // 1 - 0.8 D (1 - A)
double slayer_pressure(const Faction& f, const MemoryConstants& m);   // D (1 - 0.7 A)

}
