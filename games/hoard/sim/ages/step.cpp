#include "step.h"
#include "arrivals.h"
#include "rules/rules.h"
#include "rules/world_rules.h"

namespace ages {

void step(World& w, const Params& p, int dt) {
    w.dt = dt;
    w.year += dt;
    w.slow_clock += dt;
    bool slow = w.slow_clock >= p.c.step_years;
    if (slow) w.slow_clock -= p.c.step_years;
    rule_modifiers(w, p);
    rule_wealth(w, p);
    rule_cohesion(w, p);
    if (slow) rule_era(w, p);
    rule_war(w, p);
    rule_vassals(w, p);
    if (slow) rule_collapse(w, p);
    rule_memory(w, p);
    rule_dragon_rolls(w, p);
    if (slow) rule_fading(w, p);
    if (slow) rule_migration(w, p);
    if (slow) rule_remnants(w, p);
    rule_regimes(w, p);
    if (slow) rule_hegemony(w, p);
    rule_land(w, p);
    rule_oaths(w, p);
    rule_arrivals(w, p);   // item 06
    if (slow) rule_shocks(w, p);
    rule_cards(w, p);
    rule_endings(w, p);   // item 04
    rule_world(w, p);   // v0.4 world (S2)
    anti_dragon_marks(w, p);
    rule_people(w, p);
    rule_threads(w, p);
    rule_chronicle(w, p);
}

}
