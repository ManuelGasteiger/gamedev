#include "rules.h"
#include "../world_util.h"

namespace ages {

void rule_wealth(World& w, const Params& p) {
    const Constants& c = p.c;
    for (auto& f : w.factions) {
        double land = 0;
        for (auto& rid : f.regions) {
            const Region* r = find_region(w, rid);
            if (r) land += r->eff_land * (1.0 - r->burn);
        }
        double penalty = f.at_war_with.empty() ? 0.0 : c.war_penalty;
        double k = dts(w, c);
        double gain = f.eff.growth * land * c.land_wealth_scale * (1.0 - penalty) * k;
        f.wealth += gain - f.wealth * c.wealth_upkeep * k;
        if (f.wealth < 0) f.wealth = 0;
        f.last_growth = gain;
        f.slow_growth += gain;
    }
}

}
