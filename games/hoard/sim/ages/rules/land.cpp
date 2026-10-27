#include "rules.h"
#include "../memory_tiers.h"
#include "../world_util.h"

namespace ages {

void rule_land(World& w, const Params& p) {
    const Constants& c = p.c;
    for (auto& r : w.regions) {
        bool waste = has_tag(r.tags, "wasteland");
        double recovery = (waste ? c.burn_recovery_wasteland : c.burn_recovery) * dts(w, c);
        if (has_tag(r.tags, "clinging_fire")) recovery *= c.clinging_recovery_mult;   // v0.4: Clinging Fire
        r.burn = clamp01(r.burn - recovery);
        if (r.burn <= 0) r.tags.erase("clinging_fire");

        if (r.burn > c.wasteland_burn) r.hot_decades += w.dt; else r.hot_decades = 0;
        if (r.burn < c.wasteland_clear_burn) r.cold_decades += w.dt; else r.cold_decades = 0;

        if (!waste && r.hot_decades >= c.wasteland_decades * c.step_years) {
            r.tags.insert("wasteland");
            r.hot_decades = 0;
            for (auto& f : w.factions) raze_monuments(f, r.id, "");   // nothing stands on waste
            emit(w, "wasteland_formed", r.id, "", 0, {}, because("repeated_fire", {last_event_id(w, "act_burn", "", r.id)}));
        } else if (waste && r.cold_decades >= c.wasteland_clear_decades * c.step_years) {
            r.tags.erase("wasteland");
            r.cold_decades = 0;
            emit(w, "wasteland_healed", r.id, "", 0, {}, because("long_calm", {last_event_id(w, "wasteland_formed", r.id)}));
        }
    }
}

}
