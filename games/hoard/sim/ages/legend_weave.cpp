#include <algorithm>
#include <cmath>
#include "profile.h"
#include "rules/card_ctx.h"
#include "world_util.h"

// item 04: legends of earlier runs woven into a new world's pre-history. Each legend rolls weave_chance on its own keyed
// stream, the most extreme verdicts first, at most max_active; a woven legend brings one to three events at years
// -900 to -200 before the hatching, tags on courts or regions that last as long as its recorded tier, and the world tag
// legend:<id>. Never a stat.

namespace ages {

namespace {

double extremity(const LegendEntry& l) { return std::fabs(l.dread - l.awe) + std::max(l.dread, l.awe); }

bool fits(const WeaveDef& d, const LegendEntry& l) {
    if (d.verdict == "any") return true;
    if (d.verdict == "slain") return l.slain_by_order;
    return d.verdict == (l.awe > l.dread ? "awe" : "dread");
}

}

std::vector<Weave> plan_weaves(const World& w, const Params& p, const Profile& pr) {
    const LegendWeave& lw = p.endings.legends;
    std::vector<Weave> out;
    if (!p.endings.present || pr.legends.empty()) return out;
    std::vector<const LegendEntry*> order;
    for (auto& l : pr.legends) order.push_back(&l);
    std::stable_sort(order.begin(), order.end(), [](const LegendEntry* a, const LegendEntry* b) {
        return extremity(*a) > extremity(*b) || (extremity(*a) == extremity(*b) && a->id < b->id);
    });
    int step = p.c.step_years, hatched = w.genesis.hatched_year, active = 0;
    int lo = std::max(step, hatched + lw.year_lo), hi = std::max(lo, hatched + lw.year_hi);
    for (const LegendEntry* l : order) {
        if (active >= lw.max_active) break;
        Rng r = rng_for(w.seed, "legend", l->id);
        if (!r.chance(lw.chance)) continue;
        active += 1;
        int n = r.range(lw.events_lo, lw.events_hi + 1);
        for (int i = 0; i < n; ++i) {
            Weave wv;
            wv.legend = *l;
            wv.index = i;
            wv.year = r.range(lo / step, hi / step + 1) * step;
            out.push_back(wv);
        }
    }
    std::stable_sort(out.begin(), out.end(), [](const Weave& a, const Weave& b) { return a.year < b.year; });
    return out;
}

void weave_legend(World& w, const Params& p, const Weave& wv) {
    const LegendWeave& lw = p.endings.legends;
    const LegendEntry& l = wv.legend;
    Rng r = rng_for(w.seed, "legend_weave", l.id + "#" + std::to_string(wv.index));
    std::vector<const WeaveDef*> kinds;
    for (auto& d : lw.weaves) if (fits(d, l)) kinds.push_back(&d);
    if (kinds.empty()) return;
    const WeaveDef& d = *kinds[static_cast<size_t>(r.range(0, static_cast<int>(kinds.size())))];
    std::vector<std::string> targets;
    if (d.on == "region") { for (auto& g : w.regions) if (!g.owner.empty()) targets.push_back(g.id); }
    else for (auto& f : w.factions) targets.push_back(f.id);
    std::string target = targets.empty() ? "world" : targets[static_cast<size_t>(r.range(0, static_cast<int>(targets.size())))];
    std::string legend_tag = "legend:" + l.id;
    auto tier = lw.tier_decades.find(l.tier);
    int decades = tier == lw.tier_decades.end() ? 0 : tier->second;
    int ev = emit(w, d.kind, target, l.id, 0, {"src:legend", legend_tag}, because("legend"));
    if (target != "world") write_tag(w, target, d.tag + ":" + l.id, decades, true, ev);
    w.tags.insert(legend_tag);
}

}
