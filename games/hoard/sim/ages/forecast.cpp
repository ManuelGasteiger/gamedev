#include "ages.h"
#include "memory_tiers.h"
#include "step.h"
#include "talents.h"
#include "world_util.h"
#include "rules/slayers.h"

namespace ages {

static const uint64_t kForecastSalt = 0x5EED0F0CA57ull;

static double blur(double v, double amplitude, Rng& rng) {
    return v * (1.0 + amplitude * (rng.uniform() * 2 - 1));
}

// dry run on a copy: the copy's rng is derived from the real rng state plus a constant, so the real stream is untouched
Forecast forecast(const World& world, const Params& p, int years) {
    World w = world;
    Rng noise;
    noise.seed(world.rng.state ^ kForecastSalt);
    w.rng = noise;
    SleepResult r = sleep(w, p, years);

    Forecast f;
    f.years = r.years_slept;
    f.wake_reason = r.wake_reason;
    f.hoard_after = w.dragons[kPlayer].hoard;
    f.growth_gained = r.growth_gained;
    f.healed = r.healed;
    double amplitude = p.c.forecast_noise / (1.0 + world.dragons[kPlayer].knowledge) * effect(world, p, "forecast_mult");
    f.size = blur(w.dragons[kPlayer].size, amplitude, noise);
    f.knowledge = blur(w.dragons[kPlayer].knowledge, amplitude, noise);
    f.magic = blur(w.dragons[kPlayer].magic, amplitude, noise);
    f.hunger = clamp01(blur(w.dragons[kPlayer].hunger, amplitude, noise));

    const Faction* strongest = nullptr;
    for (auto& x : w.factions)
        if (!strongest || power(x, p.c) > power(*strongest, p.c)) strongest = &x;
    if (strongest) {
        f.strongest = strongest->id;
        f.strongest_era = static_cast<int>(blur(strongest->era, amplitude, noise) + 0.5);
    }
    for (auto& x : w.factions) {
        CourtForecast c;
        c.id = x.id;
        c.dread = clamp01(blur(dread_eff(x), amplitude, noise));
        c.awe = clamp01(blur(awe_eff(x), amplitude, noise));
        c.pressure = clamp01(blur(slayer_pressure(x, p.c.mem), amplitude, noise));
        bool can = has_tag(x.tags, "slayers") && dread_eff(x) >= p.c.mem.slayer_dread;
        c.slay_odds = can ? clamp01(blur(slaying_odds(w, p, x), amplitude, noise)) : 0;
        f.courts.push_back(c);
    }
    // only a band that is ready when the dragon lies down can try in this sleep (the slayer clock), so this is exact
    for (auto& x : world.factions)
        if (slayer_stage(world, x.id) == "slayers_ready") f.slay_possible = true;
    // v0.4 hooks (S3): star-reading (a sharper forecast) shows the likely hook of the next waking
    if (effect(world, p, "forecast_mult") < 1 && !w.hooks.empty()) {
        f.hook_kind = w.hooks.front().kind;
        f.hook_title = w.hooks.front().title;
        f.hook_region = w.hooks.front().region;
    }
    return f;
}

}
