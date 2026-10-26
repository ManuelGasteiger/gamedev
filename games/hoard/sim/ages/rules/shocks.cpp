#include "rules.h"
#include <cstdlib>
#include "../world_util.h"

namespace ages {

static void plague_and_weather(World& w, const Shocks& s) {
    for (auto& f : w.factions) {
        if (!w.rng.chance(s.plague_chance)) continue;
        f.wealth *= s.plague_wealth_mult;
        f.cohesion = clamp01(f.cohesion + s.plague_cohesion);
        emit(w, "plague", f.id, "", 0, {}, because("chance", {}, {{"wealth", f.wealth}}));
    }
    for (auto& r : w.regions) {
        if (!w.rng.chance(s.bad_century_chance)) continue;
        add_modifier(w, "shock.bad_century", r.id, "land", "mul", s.bad_century_land_mult, s.bad_century_decades);
        emit(w, "bad_century", r.id, "", 0, {}, because("chance"));
    }
}

static void volcanic(World& w, const Shocks& s) {
    bool live = false;
    for (auto& m : w.modifiers) if (m.source == "shock.volcanic_winter") live = true;
    if (has_tag(w.tags, "volcanic_winter") && !live) w.tags.erase("volcanic_winter");
    if (has_tag(w.tags, "mega_raze_pending")) {
        w.tags.erase("mega_raze_pending");
        if (!w.rng.chance(s.volcanic_chance)) return;
        w.tags.insert("volcanic_winter");
        add_modifier(w, "shock.volcanic_winter", "*", "land", "mul", s.volcanic_land_mult, s.volcanic_decades);
        emit(w, "volcanic_winter", "world", "", 0, {}, because("mega_raze", {last_event_id(w, "act_burn")}));
    }
}

// a great ruler reigns one lifespan: a golden age is a growth modifier plus cohesion, a tyrant hardens the regime
// and leaves a cohesion hole at death (tag tyrant_until:<year>)
static void great_rulers(World& w, const Constants& c) {
    for (auto& f : w.factions) {
        std::string ends;
        for (auto& t : f.tags) if (t.rfind("tyrant_until:", 0) == 0) ends = t;
        if (!ends.empty()) {
            if (w.year >= std::atoi(ends.c_str() + 13)) {
                f.tags.erase(ends);
                f.cohesion = clamp01(f.cohesion + c.tyrant_death_cohesion);
                emit(w, "tyrant_died", f.id, "", 0, {}, because("reign_ended", {last_event_id(w, "tyrant_rises", f.id)}));
            }
            continue;
        }
        if (f.ruler.cls == "immortal" || !w.rng.chance(c.great_ruler_chance)) continue;
        int decades = std::max(1, static_cast<int>(f.culture.lifespan / c.step_years + 0.5));
        if (w.rng.chance(0.5)) {
            add_modifier(w, "shock.golden_age", f.id, "growth", "mul", c.golden_age_growth_mult, decades);
            f.cohesion = clamp01(f.cohesion + c.golden_age_cohesion);
            emit(w, "golden_age", f.id, "", decades * c.step_years, {}, because("great_ruler"));
        } else {
            f.culture.traits.authority = clamp01(f.culture.traits.authority + c.tyrant_authority);
            f.culture.traits.militarism = clamp01(f.culture.traits.militarism + c.tyrant_militarism);
            f.tags.insert("tyrant_until:" + std::to_string(w.year + decades * c.step_years));
            emit(w, "tyrant_rises", f.id, "", decades * c.step_years, {}, because("great_ruler"));
        }
    }
}

// a mortal succession that happened this decade (reigned reset to 0 by rule_cohesion) may turn into a crisis
static void succession_crises(World& w, const Params& p) {
    const Constants& c = p.c;
    std::vector<std::string> splitting;
    for (auto& f : w.factions) {
        if (f.ruler.cls != "mortal" || f.ruler.reigned != 0 || f.born == w.year) continue;
        if (!w.rng.chance(c.succession_crisis_chance)) continue;
        f.cohesion = clamp01(f.cohesion + c.succession_crisis_cohesion);
        emit(w, "succession_crisis", f.id, "", 0, {}, because("disputed_succession", {}, {{"authority", f.culture.traits.authority}}));
        if (f.culture.traits.authority > c.trait_authority_high && f.regions.size() > 1) splitting.push_back(f.id);
    }
    for (auto& id : splitting) {
        Faction* f = find_faction(w, id);
        if (!f) continue;
        std::string largest;
        double best = -1;
        for (auto& rid : f->regions) {
            const Region* r = find_region(w, rid);
            if (r && r->land > best) { best = r->land; largest = rid; }
        }
        Faction parent = *f;
        double share = parent.wealth / static_cast<double>(parent.regions.size());
        f->wealth -= share;
        erase_value(f->regions, largest);
        f->last_region_lost = w.year;
        Faction s = found_faction(w, p, parent.race, largest, share, c.successor_cohesion, parent.era);
        s.culture = parent.culture;
        s.pop = parent.pop;
        s.dread = parent.dread;
    s.awe = parent.awe;
        s.faith = parent.faith;
        s.memory = parent.memory;
        s.tags.insert("successor_of:" + parent.id);
        s.tags.insert("civil_war");
        if (Region* r = find_region(w, largest)) r->owner = s.id;
        find_faction(w, id)->tags.insert("civil_war");
        int crisis = last_event_id(w, "succession_crisis", id, "", "", w.year);
        std::string thread = "civil_war:" + id + ":" + std::to_string(w.year);
        emit(w, "successor_founded", s.id, largest, 0, {parent.id}, because("succession_crisis", {crisis}), thread);
        w.factions.push_back(s);
        sort_by_id(w);
        Faction* fs = find_faction(w, s.id);
        Faction* fp = find_faction(w, id);
        War& war = start_war(w, *fs, *fp, "civil_war", because("succession_crisis", {crisis}));
        war.thread = thread;
        w.chronicle.back().thread = thread;
    }
}

void rule_shocks(World& w, const Params& p) {
    plague_and_weather(w, p.shocks);
    volcanic(w, p.shocks);
    great_rulers(w, p.c);
    succession_crises(w, p);
}

}
