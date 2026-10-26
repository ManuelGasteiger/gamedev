#include "rules.h"
#include "../world_util.h"

namespace ages {

static double jitter(double v, const Constants& c, Rng& rng) {
    return clamp01(v + (rng.uniform() * 2 - 1) * c.trait_jitter);
}

void jitter_traits(Faction& f, const Constants& c, Rng& rng) {
    Traits& t = f.culture.traits;
    t.authority = jitter(t.authority, c, rng);
    t.openness = jitter(t.openness, c, rng);
    t.militarism = jitter(t.militarism, c, rng);
    t.piety = jitter(t.piety, c, rng);
}

static double max_neighbour_openness(const World& w, const Faction& f) {
    double best = -1;
    for (auto& g : w.factions)
        if (g.id != f.id && adjacent(w, f, g)) best = std::max(best, g.culture.traits.openness);
    return best;
}

void rule_regimes(World& w, const Params& p) {
    const Constants& c = p.c;
    std::vector<double> openness;
    for (auto& f : w.factions) openness.push_back(max_neighbour_openness(w, f));
    for (size_t i = 0; i < w.factions.size(); ++i) {
        Faction& f = w.factions[i];
        Traits& t = f.culture.traits;
        double k = dts(w, c);
        if (!f.at_war_with.empty()) {
            f.last_war_year = w.year;
            t.militarism += c.drift_militarism_war * k;
        } else if (w.year - f.last_war_year >= c.drift_peace_decades * c.step_years) {
            t.militarism += c.drift_militarism_peace * k;
        }
        if (has_tag(f.tags, "burned_by_dragon")) {
            f.tags.erase("burned_by_dragon");
            t.authority += c.drift_burned_authority;
            t.piety += c.drift_burned_piety;
        }
        if (openness[i] > t.openness) t.openness += c.drift_openness_border * k;
        t.authority = clamp01(t.authority);
        t.openness = clamp01(t.openness);
        t.militarism = clamp01(t.militarism);
        t.piety = clamp01(t.piety);
    }
}

}
