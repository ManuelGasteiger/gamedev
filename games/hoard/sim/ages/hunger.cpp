#include "ages.h"
#include "step.h"
#include "world_util.h"

namespace ages {

// one decade of sleep: nourishment is what digested this decade; a shortfall raises hunger, a surplus does nothing
// (only feeding acts at wake lower it). Waking is the rising edge through hunger_wake_at. Returns the size lost to atrophy.
// 17.7: the need scales with the phase (hunger_phase_mult), 17.4: per-decade numbers scale with the step length
double hunger_need(const World& w, const Constants& c) {
    return (c.hunger_need + c.hunger_need_per_size * w.dragons[kPlayer].size) * c.hunger_phase_mult[static_cast<size_t>(phase_index(w.dragons[kPlayer]))];
}

double hunger_step(World& w, const Params& p, double nourishment) {
    const Constants& c = p.c;
    Dragon& d = w.dragons[kPlayer];
    double before = d.hunger;
    double k = dts(w, c);
    double shortfall = hunger_need(w, c) * k - nourishment;
    if (shortfall > 0) d.hunger = clamp01(d.hunger + shortfall * c.hunger_rate);
    double atrophy = 0;
    if (d.hunger > c.hunger_atrophy_at && d.size > 0) {
        atrophy = std::min(d.size, c.hunger_atrophy * k);
        d.size -= atrophy;
    }
    if (before < c.hunger_wake_at && d.hunger >= c.hunger_wake_at) {
        emit(w, "dragon_starving", "dragon", "", d.hunger, {}, because("hoard_empty", {}, {{"need", hunger_need(w, c)}, {"food", nourishment / k}}));
        if (w.wake_reason.empty()) w.wake_reason = "hunger";
    }
    return atrophy;
}

void feed(World& w, const Params& p, const Act& a, double m) {
    const Constants& c = p.c;
    if (a.type == "kill") w.dragons[kPlayer].hunger = clamp01(w.dragons[kPlayer].hunger - c.hunger_kill_feed * m);
    else if (a.type == "take" && a.payload.count("food")) w.dragons[kPlayer].hunger = clamp01(w.dragons[kPlayer].hunger - c.hunger_food_feed * m);
}

}
