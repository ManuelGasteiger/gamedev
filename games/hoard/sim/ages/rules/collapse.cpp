#include "rules.h"
#include "../names.h"
#include "../world_util.h"

namespace ages {

void remove_faction(World& w, const std::string& id) {
    Faction* f = find_faction(w, id);
    if (!f) return;
    w.graveyard.push_back(record_of(*f, w.year));
    for (auto& r : w.regions) if (r.owner == id) r.owner.clear();
    end_wars_of(w, id);
    for (auto& o : w.oaths)
        if (o.status == "active" && (o.parties[0] == id || o.parties[1] == id)) o.status = "expired";
    w.factions.erase(std::remove_if(w.factions.begin(), w.factions.end(),
                                    [&](const Faction& x) { return x.id == id; }),
                     w.factions.end());
}

Faction found_faction(World& w, const Params& p, const std::string& race, const std::string& region, double wealth,
                      double cohesion, int era) {
    Faction f;
    f.id = new_faction_name(w, p, race);
    f.race = race;
    f.culture = p.races.at(race).culture;
    f.pop[race] = 1.0;
    f.regions.push_back(region);
    f.wealth = wealth;
    f.cohesion = cohesion;
    f.era = era;
    f.born = w.year;
    f.max_regions = 1;
    f.era_years.assign(static_cast<size_t>(era) + 1, w.year);
    jitter_traits(f, p.c, w.rng);
    init_faction_ruler(f, p.c, w.rng);
    return f;
}

static Faction make_successor(World& w, const Params& p, const Faction& parent, const std::string& region, double wealth) {
    const Constants& c = p.c;
    Faction s;
    s.id = new_faction_name(w, p, parent.race);
    s.race = parent.race;
    s.culture = parent.culture;
    s.pop = parent.pop;
    s.regions.push_back(region);
    s.wealth = wealth;
    s.cohesion = c.successor_cohesion;
    s.era = parent.era;
    s.era_progress = parent.era_progress;
    s.fear = parent.fear;
    s.faith = parent.faith;
    s.memory = parent.memory;
    s.anti_dragon = parent.anti_dragon;
    s.tags.insert("successor_of:" + parent.id);
    for (auto& t : parent.tags) if (t.rfind("successor_of:", 0) == 0 || t.rfind("vassal_of:", 0) == 0) s.tags.insert(t);
    if (has_tag(parent.tags, "slayers")) s.tags.insert("slayers");
    if (has_tag(parent.tags, "fading")) s.tags.insert("fading");
    s.born = w.year;
    s.max_regions = 1;
    s.era_years.assign(static_cast<size_t>(parent.era) + 1, w.year);
    init_faction_ruler(s, c, w.rng);
    return s;
}

static void civil_war(World& w, const std::vector<std::string>& ids, int collapse_id, const std::string& thread) {
    for (size_t i = 0; i < ids.size(); ++i) {
        Faction* a = find_faction(w, ids[i]);
        if (!a) continue;
        a->tags.insert("civil_war");
        for (size_t j = i + 1; j < ids.size(); ++j) {
            Faction* b = find_faction(w, ids[j]);
            if (!b || !adjacent(w, *a, *b)) continue;
            War& war = start_war(w, *a, *b, "civil_war", because("successors_quarrel", {collapse_id}));
            war.thread = thread;
            w.chronicle.back().thread = thread;
        }
    }
}

// the reason the realm fell apart: the newest succession crisis, lost war or plague, else its own size
static Cause collapse_cause(const World& w, const Faction& f) {
    int ev = 0;
    for (const char* k : {"succession_crisis", "war_ended", "plague", "tyrant_died"}) {
        int e = k == std::string("war_ended") ? std::max(last_event_id(w, "war_ended", "", f.id), last_event_id(w, "war_ended", f.id)) : last_event_id(w, k, f.id);
        ev = std::max(ev, e);
    }
    const Event* e = find_event(w, ev);
    bool recent = e && w.year - e->year <= 100;
    return because(recent ? "weakened" : "own_size", {recent ? ev : 0}, {{"cohesion", f.cohesion}, {"regions", static_cast<double>(f.regions.size())}});
}

// one `collapse` event carrying the successors as tags, then one `successor_founded` per successor on the same thread
static void split(World& w, const Params& p, const Faction parent) {
    const Constants& c = p.c;
    std::vector<std::string> regions = parent.regions;
    std::sort(regions.begin(), regions.end());
    double share = parent.wealth / static_cast<double>(regions.size());
    std::string thread = "collapse:" + parent.id + ":" + std::to_string(w.year);
    int collapse_id = emit(w, "collapse", parent.id, "", static_cast<double>(regions.size()), {}, collapse_cause(w, parent), thread);
    Event& ev = w.chronicle.back();
    bool war = parent.culture.traits.authority > c.trait_authority_high && regions.size() > 1;
    if (war) ev.tags.insert("civil_war");
    remove_faction(w, parent.id);
    std::vector<std::string> ids;
    for (auto& rid : regions) {
        Faction s = make_successor(w, p, parent, rid, share);
        if (Region* r = find_region(w, rid)) r->owner = s.id;
        emit(w, "successor_founded", s.id, rid, 0, {parent.id}, because("collapse", {collapse_id}), thread);
        ids.push_back(s.id);
        w.factions.push_back(s);
    }
    for (auto& e : w.chronicle) if (e.id == collapse_id) for (auto& sid : ids) e.tags.insert("successor:" + sid);
    sort_by_id(w);
    if (war) civil_war(w, ids, collapse_id, thread);
}

void rule_collapse(World& w, const Params& p) {
    const Constants& c = p.c;
    std::vector<std::string> landless, collapsing;
    for (auto& f : w.factions) {
        if (f.regions.empty()) landless.push_back(f.id);
        else if (f.cohesion < c.collapse_cohesion) collapsing.push_back(f.id);
    }
    for (auto& id : landless) {
        const Faction* f = find_faction(w, id);
        if (f) refugees(w, p, *f, c.refugee_share);
        int lost = std::max(last_event_id(w, "region_taken", "", "", id), last_event_id(w, "region_abandoned", id));
        emit(w, "faction_destroyed", id, "", 0, {}, because("landless", {lost}));
        remove_faction(w, id);
    }
    for (auto& id : collapsing) {
        const Faction* f = find_faction(w, id);
        if (!f) continue;
        if (f->regions.size() == 1) {
            Faction single = *f;
            std::string thread = "collapse:" + id + ":" + std::to_string(w.year);
            int collapse_id = emit(w, "collapse", id, "", 1, {}, collapse_cause(w, single), thread);
            remove_faction(w, id);
            Faction s = make_successor(w, p, single, single.regions[0], single.wealth);
            if (Region* r = find_region(w, single.regions[0])) r->owner = s.id;
            for (auto& e : w.chronicle) if (e.id == collapse_id) e.tags.insert("successor:" + s.id);
            emit(w, "successor_founded", s.id, single.regions[0], 0, {single.id}, because("collapse", {collapse_id}), thread);
            w.factions.push_back(s);
            sort_by_id(w);
        } else {
            split(w, p, *f);
        }
    }
}

}
