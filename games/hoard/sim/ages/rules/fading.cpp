#include "rules.h"
#include "../world_util.h"

namespace ages {

// share of the valley's wealth held by human-dominated factions
static double human_share(const World& w) {
    double total = 0, human = 0;
    for (auto& f : w.factions) {
        total += f.wealth;
        if (dominant_race(f) == "human") human += f.wealth;
    }
    return total > 0 ? human / total : 0;
}

static void depart(World& w, const Params& p, Faction& f) {
    const Constants& c = p.c;
    std::vector<std::string> away;
    for (auto& rid : f.regions) {
        const Region* r = find_region(w, rid);
        if (r && !home_terrain(f.culture, *r)) away.push_back(rid);
    }
    if (away.empty() || f.regions.size() < 2 || !w.rng.chance(c.fading_depart_chance)) return;
    std::string rid = away[static_cast<size_t>(w.rng.range(0, static_cast<int>(away.size())))];
    erase_value(f.regions, rid);
    f.last_region_lost = w.year;
    if (Region* r = find_region(w, rid)) r->owner.clear();
    bool found = false;
    for (auto& rem : w.remnants) if (rem.race == "elf") { rem.strength += c.fading_remnant_gain; found = true; }
    if (!found) {
        Remnant rem;
        rem.race = "elf";
        rem.strength = c.fading_remnant_gain;
        rem.origin = rid;
        rem.max_era = f.era;
        w.remnants.push_back(rem);
        std::sort(w.remnants.begin(), w.remnants.end(), [](const Remnant& a, const Remnant& b) { return a.race < b.race; });
    }
    emit(w, "elves_departed", f.id, rid, 0, {}, because("fading", {last_event_id(w, "fading_begins", f.id)}));
}

void rule_fading(World& w, const Params& p) {
    const Constants& c = p.c;
    double share = human_share(w);
    if (share > c.fading_human_share) w.human_share_decades += 1; else w.human_share_decades = 0;
    bool start = w.human_share_decades >= c.fading_decades;
    bool end = share < c.fading_end_share;
    for (auto& f : w.factions) {
        if (dominant_race(f) != "elf") { f.tags.erase("fading"); continue; }
        bool fading = has_tag(f.tags, "fading");
        if (!fading && start) {
            f.tags.insert("fading");
            int push = std::max(last_event_id(w, "hegemony"), last_event_id(w, "era_advance"));
            emit(w, "fading_begins", f.id, "", share, {}, because("human_wealth", {push}, {{"share", share}}));
            fading = true;
        } else if (fading && end) {
            f.tags.erase("fading");
            int fall = std::max(last_event_id(w, "hegemony_ended"), std::max(last_event_id(w, "collapse"), last_event_id(w, "plague")));
            emit(w, "fading_ends", f.id, "", share, {}, because("human_wealth_fell", {fall}, {{"share", share}}));
            fading = false;
        }
        if (fading) depart(w, p, f);
    }
}

}
