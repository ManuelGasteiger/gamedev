#include "rules.h"
#include "../memory_tiers.h"
#include <cstdlib>
#include "../defences.h"
#include "../world_util.h"

// v0.4 (S1): a court's research against the dragon. Below anti_dragon_min_era a court that dreads the dragon and
// remembers its fire still researches, at dread_research x dread x the burn memory per decade (the era rule covers the
// rest). The chronicle tells each share of anti_dragon_marks passed on the way to the next defence the court would
// raise (anti_dragon_progress, object the defence id); the last mark told is kept as anti_dragon_mark:<defence>:<i>.

namespace ages {

namespace {

const char* const kMark = "anti_dragon_mark:";

// the highest unlock level the court has already passed (0 when none)
double passed_level(const Params& p, const Faction& f) {
    double best = 0;
    for (auto& d : p.defences.defences) {
        if (f.era < d.min_era || (!d.races_any.empty() && !contains(d.races_any, dominant_race(f)))) continue;
        if (d.min_anti_dragon > 0 && d.min_anti_dragon <= f.anti_dragon) best = std::max(best, d.min_anti_dragon);
        if (contains(d.tags_any, "slayers") && p.c.slayers_at <= f.anti_dragon) best = std::max(best, p.c.slayers_at);
    }
    return best;
}

double burn_memory(const Faction& f) {
    double top = 0;
    for (auto& m : f.memory) if (m.type == "burn" && m.axis == "dread") top = std::max(top, m.weight);
    return top;
}

void dread_research(World& w, const Params& p) {
    const Constants& c = p.c;
    if (c.dread_research <= 0) return;
    for (auto& f : w.factions)
        if (f.era < c.anti_dragon_min_era && dread_eff(f) >= c.mem.slayer_dread)
            f.anti_dragon += c.dread_research * dread_eff(f) * burn_memory(f) * dts(w, c);
}

}

void anti_dragon_research(World& w, const Params& p) { dread_research(w, p); }

// after the cards, so a card's research jump is told in the sleep it happened; silent while the egg lies unhatched
void anti_dragon_marks(World& w, const Params& p) {
    const std::vector<double>& marks = p.c.anti_dragon_marks;
    bool egg = w.tags.count("egg") > 0;
    if (marks.empty()) return;
    for (auto& f : w.factions) {
        std::string told;
        for (auto& t : f.tags) if (t.rfind(kMark, 0) == 0) told = t;
        Unlock next = next_unlock(w, p, f);
        if (next.at < 0) { if (!told.empty()) f.tags.erase(told); continue; }
        double from = passed_level(p, f);
        double share = next.at > from ? (f.anti_dragon - from) / (next.at - from) : 0;
        int mark = -1;
        for (size_t i = 0; i < marks.size(); ++i) if (share >= marks[i]) mark = static_cast<int>(i);
        int told_mark = -1;
        std::string prefix = std::string(kMark) + next.defence + ":";
        if (told.rfind(prefix, 0) == 0) told_mark = std::atoi(told.c_str() + prefix.size());
        if (mark == told_mark || (mark < 0 && told.empty())) continue;
        if (!told.empty()) f.tags.erase(told);
        if (mark >= 0) f.tags.insert(prefix + std::to_string(mark));
        if (mark > told_mark && !egg)
            emit(w, "anti_dragon_progress", f.id, next.defence, marks[static_cast<size_t>(mark)], {},
                 because("research", {}, {{"anti_dragon", f.anti_dragon}, {"at", next.at}}));
    }
}

}
