#include "rules.h"
#include "../world_util.h"

namespace ages {

static int reign_length(const Faction& f, const Constants& c, Rng& rng) {
    double base = f.ruler.cls == "warlord" ? c.warlord_lifespan : f.culture.lifespan;
    double jitter = 1.0 + (rng.uniform() * 2 - 1) * c.succession_jitter;
    int years = static_cast<int>(base * jitter + 0.5);
    return years < c.step_years ? c.step_years : years;
}

void init_faction_ruler(Faction& f, const Constants& c, Rng& rng) {
    f.ruler.cls = f.culture.ruler_class;
    f.ruler.reigned = 0;
    f.ruler.reign_length = reign_length(f, c, rng);
}

void rule_cohesion(World& w, const Params& p) {
    const Constants& c = p.c;
    for (auto& f : w.factions) {
        double pw = power(f, c);
        f.cohesion -= f.eff.cohesion_decay * std::max(pw / c.power_cap, c.cohesion_decay_floor) * dts(w, c);

        if (f.ruler.cls == "immortal") { f.cohesion = clamp01(f.cohesion); continue; }
        f.ruler.reigned += w.dt;
        if (f.ruler.reigned < f.ruler.reign_length) { f.cohesion = clamp01(f.cohesion); continue; }

        if (f.ruler.cls == "mortal") f.cohesion += c.succession_bonus_mortal * f.eff.succession_bonus;
        else if (f.ruler.cls == "longlived") f.cohesion += c.succession_bonus_longlived * f.eff.succession_bonus;
        else if (f.ruler.cls == "warlord") {
            f.cohesion = std::min(f.cohesion, c.warlord_death_cohesion);
            emit(w, "warlord_died", f.id, "", 0, {}, because("warlord_death", {}, {{"reigned", static_cast<double>(f.ruler.reigned)}}));
        }
        f.ruler.reigned = 0;
        f.ruler.reign_length = reign_length(f, c, w.rng);
        f.cohesion = clamp01(f.cohesion);
    }
}

}
