#include "rules.h"
#include "../memory_tiers.h"
#include "../world_util.h"

namespace ages {

static int max_era(const Constants& c) { return static_cast<int>(c.era_threshold.size()); }

// the arms race in one line (act-api 6.4): research_base x era x dread_factor x wealth share, and the court pays
// research_spend wealth per unit of progress, wealth that would otherwise compound
static void research(const Constants& c, Faction& f) {
    const MemoryConstants& m = c.mem;
    double dread_factor = 1.0 + m.research_dread * dread_eff(f) * (1.0 - m.pressure_awe * awe_eff(f));
    double wealth = std::max(0.0, f.wealth);
    double wealth_share = wealth / (wealth + m.research_wealth_half);
    double progress = m.research_base * f.eff.anti_dragon_rate * era_mult(c, f.era) * dread_factor * wealth_share;
    f.anti_dragon += progress;
    f.wealth = std::max(0.0, f.wealth - m.research_spend * progress);
}

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

        if (f.era >= c.anti_dragon_min_era) research(c, f);
        bool grievance = c.mem.grievance_written > 0 && f.dread.written >= c.mem.grievance_written;
        if (f.anti_dragon >= c.slayers_at && !has_tag(f.tags, "slayers") && (dread_eff(f) >= c.mem.slayer_dread || grievance)) {
            f.tags.insert("slayers");
            int burn = last_event_id(w, "act_burn", f.id);
            if (!burn) burn = std::max(last_event_id(w, "act_kill", f.id), last_event_id(w, "act_take", f.id));
            emit(w, "slayers_founded", f.id, "", 0, {}, because(burn ? "dragon_fire" : "fear", {burn}, {{"dread", dread_eff(f)}, {"era", static_cast<double>(f.era)}}));
        }
    }
}

}
