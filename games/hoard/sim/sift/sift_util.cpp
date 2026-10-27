#include "sift_util.h"
#include <algorithm>
#include "ages/importance.h"
#include "ages/world_util.h"

namespace ages {

const Event* sift_event(const World& w, int id) {
    if (id < kCompoundBase) return find_event(w, id);
    auto& v = w.sift.compounds;
    auto it = std::lower_bound(v.begin(), v.end(), id, [](const Event& e, int k) { return e.id < k; });
    return it != v.end() && it->id == id ? &*it : nullptr;
}

int event_level(const Event& e) {
    if (e.kind.rfind("story.", 0) == 0) return 2;
    if (e.kind.rfind("episode.", 0) == 0) return 1;
    return 0;
}

bool is_dragon_act(const Event& e) { return e.kind.rfind("act_", 0) == 0; }

bool reaches(const World& w, const Event& from, int target, int max_hops) {
    std::vector<int> frontier = from.cause.refs, seen;
    for (int hop = 0; hop < max_hops && !frontier.empty(); ++hop) {
        std::vector<int> next;
        for (int id : frontier) {
            if (id == target) return true;
            if (std::find(seen.begin(), seen.end(), id) != seen.end()) continue;
            seen.push_back(id);
            if (id < target) continue;   // causes are older than their effects
            if (const Event* e = sift_event(w, id)) next.insert(next.end(), e->cause.refs.begin(), e->cause.refs.end());
        }
        frontier.swap(next);
    }
    return false;
}

const SiftPattern* find_pattern(const Params& p, const std::string& id) {
    for (auto& x : p.sift.patterns) if (x.id == id) return &x;
    return nullptr;
}

// the template's tier, raised by what the event means to the dragon now (a person it knows, a sworn court)
int template_tier(const World& w, const Params& p, const Event& e) {
    if (e.id >= kCompoundBase) {
        for (auto& x : p.sift.patterns) if (x.emit_kind == e.kind) return x.tier;
        return 3;
    }
    auto it = p.sift.tiers.find(template_key(e));
    if (it == p.sift.tiers.end()) it = p.sift.tiers.find(e.kind);
    int t = it == p.sift.tiers.end() ? 1 : it->second;
    if (t == 0 || t >= 3) return t;
    // lifted only by what importance.cpp adds on top of the template (a known person, a sworn court, a hostage)
    auto base = p.importance.find(template_key(e));
    if (base == p.importance.end()) base = p.importance.find(e.kind);
    double b = base == p.importance.end() ? p.c.importance_default : base->second;
    double imp = e.importance >= 0 ? e.importance : event_importance(w, p, e);
    if (imp <= b + 1e-9) return t;
    if (imp >= 0.5) return 3;
    if (imp >= 0.3) return std::max(t, 2);
    return t;
}

std::string court_of(const World& w, const std::string& id) {
    if (find_faction(w, id)) return id;
    if (const Region* r = find_region(w, id)) return r->owner;
    if (const Person* x = find_person(w, id)) return x->faction;
    for (auto& d : w.dead_people) if (d.id == id) return d.faction;
    for (auto& g : w.graveyard) if (g.id == id) return id;
    return "";
}

std::string race_of(const World& w, const std::string& id) {
    if (const Faction* f = find_faction(w, id)) return f->race;
    if (const Person* x = find_person(w, id)) return x->race;
    for (auto& d : w.dead_people) if (d.id == id) return d.race;
    for (auto& g : w.graveyard) if (g.id == id) return g.race;
    if (const Region* r = find_region(w, id)) if (const Faction* f = find_faction(w, r->owner)) return f->race;
    return "";
}

int valley_era(const World& w) {
    int e = 0;
    for (auto& f : w.factions) e = std::max(e, f.era);
    return e;
}

// the side that took land (a region_taken among the causes); a war that wore out was held by the defender; a war
// ended by a collapse was won by the side still standing
std::string war_winner(const World& w, const Event& ended) {
    for (int r : ended.cause.refs)
        if (const Event* x = sift_event(w, r))
            if (x->kind == "region_taken" && (x->subject == ended.subject || x->subject == ended.object)) return x->subject;
    if (ended.tags.count("exhaustion")) return ended.object;
    if (ended.tags.count("submission")) return ended.subject;
    if (ended.tags.count("collapse")) return find_faction(w, ended.subject) ? ended.subject : ended.object;
    return "";
}

std::string protagonist(const SiftPattern& p, const SiftThread& t) {
    const std::string& v = p.emit_subject;
    if (v.empty() || v[0] != '?') return v;
    auto it = t.cast.find(v);
    return it == t.cast.end() ? "" : it->second;
}

}
