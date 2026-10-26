#include "rules.h"
#include "../world_util.h"

namespace ages {

static int max_era(const Constants& c) { return static_cast<int>(c.era_threshold.size()); }

void rule_era(World& w, const Params& p) {
    const Constants& c = p.c;
    std::vector<int> eras_before;
    for (auto& f : w.factions) eras_before.push_back(f.era);

    for (size_t i = 0; i < w.factions.size(); ++i) {
        Faction& f = w.factions[i];
        f.era_progress += f.eff.era_rate * f.slow_growth;
        f.slow_growth = 0;

        for (size_t j = 0; j < w.factions.size(); ++j) {
            if (i == j) continue;
            const Faction& g = w.factions[j];
            if (eras_before[j] - eras_before[i] >= c.era_diffusion_gap && adjacent(w, f, g)) {
                double aff = std::max(0.0, affinity(p, f, g));
                f.era_progress += c.era_diffusion * f.eff.diffusion * g.eff.diffusion * (1.0 + aff);
                break;
            }
        }

        if (!f.at_war_with.empty()) f.anti_dragon += c.war_siege_anti_dragon * era_mult(c, f.era);

        while (f.era < max_era(c) && f.era_progress >= c.era_threshold[static_cast<size_t>(f.era)]) {
            f.era_progress -= c.era_threshold[static_cast<size_t>(f.era)];
            f.era += 1;
            f.era_years.push_back(w.year);
            emit(w, "era_advance", f.id, "", f.era, {}, because("progress", {last_event_id(w, "era_advance", f.id)}, {{"era", static_cast<double>(f.era)}}));
        }

        if (f.fear > c.anti_dragon_fear && f.era >= c.anti_dragon_min_era)
            f.anti_dragon += c.anti_dragon_rate * f.eff.anti_dragon_rate * f.fear * era_mult(c, f.era);
        if (f.anti_dragon >= c.slayers_at && !has_tag(f.tags, "slayers")) {
            f.tags.insert("slayers");
            int burn = last_event_id(w, "act_burn", f.id);
            if (!burn) burn = std::max(last_event_id(w, "act_kill", f.id), last_event_id(w, "act_take", f.id));
            emit(w, "slayers_founded", f.id, "", 0, {}, because(burn ? "dragon_fire" : "fear", {burn}, {{"fear", f.fear}, {"era", static_cast<double>(f.era)}}));
        }
    }
}

}
