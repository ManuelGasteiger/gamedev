#include "arrivals.h"
#include "defences.h"
#include "memory_tiers.h"
#include "rules/slayers.h"
#include "talents.h"
#include "world_util.h"

namespace ages {

namespace {

std::string lair_id(const World& w) {
    for (auto& r : w.regions) if (r.lair) return r.id;
    return "";
}

bool any_oath(const World& w, const std::string& court) {
    for (auto& o : w.oaths) if (o.status == "active" && (o.parties[0] == court || o.parties[1] == court)) return true;
    return false;
}

double oral_eff(const Faction& f) { return 1.0 - (1.0 - f.dread.oral) * (1.0 - f.awe.oral); }

int memory_rank(const Faction& f, const MemoryConstants& m) {
    std::string t = memory_tier(f, m);
    return t == "monumental" ? 3 : (t == "written" ? 2 : (t == "oral" ? 1 : 0));
}

bool gold_rumour(const World& w, const Params& p, const Faction& f) {
    if (w.dragons[kPlayer].hoard.gold >= p.arrivals.lure_gold) return true;
    for (size_t i = w.acts.size(); i > 0; --i) {
        const Act& a = w.acts[i - 1];
        if (a.year < w.year - p.arrivals.rumour_years) break;
        if ((a.type == "take" || a.type == "show") && contains(a.witnesses, f.id)) return true;
    }
    return false;
}

bool taught(const World& w, const Faction& f) {
    for (auto& m : w.modifiers) if (m.source == "dragon.teach" && m.target == f.id && m.until > w.year) return true;
    return false;
}

bool relative_held(const World& w, const Faction& f) {
    for (auto& id : w.dragons[kPlayer].hostages) if (const Person* x = find_person(w, id)) if (x->faction == f.id) return true;
    return false;
}

bool has_library(const World& w, const Faction& f) {
    for (auto& rid : f.regions) if (const Region* r = find_region(w, rid)) if (has_tag(r->tags, "library")) return true;
    return false;
}

bool has_tag_prefix(const Tags& t, const std::string& prefix) {
    for (auto& x : t) if (x.rfind(prefix, 0) == 0) return true;
    return false;
}

// a numeric or boolean value per gate name; false for a name outside the closed list
bool value_of(const GateCtx& g, const std::string& n, double& v) {
    const World& w = g.w;
    const Faction& f = g.f;
    const Constants& c = g.p.c;
    auto b = [&](bool x) { v = x ? 1 : 0; return true; };
    if (n == "faith_share") { v = f.faith; return true; }   // until item 12 gives courts a cult's share
    if (n == "awe_eff") { v = awe_eff(f); return true; }
    if (n == "dread_eff") { v = dread_eff(f); return true; }
    if (n == "dread_over_awe") { v = dread_eff(f) - awe_eff(f); return true; }
    if (n == "cohesion") { v = f.cohesion; return true; }
    if (n == "era") { v = f.era; return true; }
    if (n == "piety") { v = f.culture.traits.piety; return true; }
    if (n == "awe_or_oath") { v = any_oath(w, f.id) ? 1.0 : awe_eff(f); return true; }
    if (n == "knows_lair") return b(f.lair_known >= 0);
    if (n == "knows_lair_exact") return b(f.lair_known >= 0 && f.lair_exact);
    if (n == "lair_on_record") return b(memory_rank(f, c.mem) >= 2 || (f.lair_known >= 0 && f.lair_exact));
    if (n == "route_open") return b(route_open(w, f));
    if (n == "motive") return b(g.motive);
    if (n == "debt") return b(any_oath(w, f.id));
    if (n == "envoy_killed") { auto it = w.arrival_fired.find(f.id + "|envoy_killed"); return b(it != w.arrival_fired.end() && w.year - it->second < g.p.arrivals.envoy_kill_generations * 30); }
    if (n == "gold_rumour") return b(gold_rumour(w, g.p, f));
    if (n == "thief_guard") return b(effect(w, g.p, "thief_guard") > 0);
    if (n == "slayer_order") return b(has_tag(f.tags, "slayers"));
    if (n == "band_ready") return b(slayer_stage(w, f.id) == "slayers_ready" || has_tag(w.tags, "slayers_coming:" + f.id));
    if (n == "taught") return b(taught(w, f));
    if (n == "library") return b(has_library(w, f));
    if (n == "relative_held") return b(relative_held(w, f));
    if (n == "lost_region") return b(f.last_region_lost >= w.year - g.p.arrivals.refugee_years);
    if (n == "lair_wasteland") { const Region* r = find_region(w, lair_id(w)); return b(r && has_tag(r->tags, "wasteland")); }
    if (n == "prophecy_or_cult") return b(has_tag(f.tags, "prophecy") || awe_eff(f) >= c.mem.cult_awe);   // until item 04 and item 12
    if (n == "false_dragon") return b(has_tag_prefix(f.tags, "false_dragon:"));
    return false;
}

}

std::string gate_key(const ArrivalGateDef& d) { return d.has_lte && !d.has_gte ? d.gate + "_low" : d.gate; }

bool gate_holds(const GateCtx& g, const ArrivalGateDef& d) {
    double v = 0;
    if (!value_of(g, d.gate, v)) return false;
    bool bounded = d.has_gte || d.has_lte || d.has_gt || d.has_lt;
    bool ok = bounded ? (!d.has_gte || v >= d.gte) && (!d.has_lte || v <= d.lte) && (!d.has_gt || v > d.gt) && (!d.has_lt || v < d.lt) : v != 0;
    if (ok && !d.unless.empty()) {
        ArrivalGateDef u;
        u.gate = d.unless;
        if (gate_holds(g, u)) ok = false;
    }
    return ok;
}

int lair_distance(const World& w, const std::string& region) {
    std::string lair = lair_id(w);
    if (lair.empty() || region.empty()) return 1000;
    std::vector<std::string> frontier = {lair}, seen = frontier;
    for (int d = 0; !frontier.empty(); ++d) {
        if (contains(frontier, region)) return d;
        std::vector<std::string> next;
        for (auto& id : frontier)
            if (const Region* r = find_region(w, id))
                for (auto& n : r->neighbours) if (!contains(seen, n)) { seen.push_back(n); next.push_back(n); }
        frontier = next;
    }
    return 1000;
}

void learn_lair(World& w, Faction& f, bool exact) {
    f.lair_known = w.year;
    if (exact) f.lair_exact = true;
}

// a witnessed act within sight_regions of the lair, a show at a capital, a hostage sent home
void note_lair_sight(World& w, const Params& p, const Act& a) {
    const Region* at = act_region(w, a);
    bool near = at && lair_distance(w, at->id) <= p.arrivals.sight_regions;
    for (auto& id : a.witnesses)
        if (Faction* f = find_faction(w, id)) if (near || (a.type == "show" && id == a.target)) learn_lair(w, *f, false);
    if (a.type == "release") if (Faction* f = find_faction(w, a.target)) learn_lair(w, *f, true);
}

void refresh_lair_knowledge(World& w, const Params& p) {
    for (auto& f : w.factions) {
        if (f.lair_known < 0) continue;
        if (memory_rank(f, p.c.mem) >= 2 || oral_eff(f) >= p.arrivals.oral_floor || w.year - f.lair_known < p.arrivals.lair_memory_years) continue;
        f.lair_known = -1;
        f.lair_exact = false;
    }
}

// false while an orc court or wasteland sits on every path from the court to the lair region
bool route_open(const World& w, const Faction& f) {
    std::string lair = lair_id(w);
    if (lair.empty() || f.regions.empty()) return false;
    std::vector<std::string> frontier = f.regions, seen = frontier;
    while (!frontier.empty()) {
        std::vector<std::string> next;
        for (auto& id : frontier) {
            if (id == lair) return true;
            const Region* r = find_region(w, id);
            if (!r) continue;
            for (auto& n : r->neighbours) {
                if (contains(seen, n)) continue;
                const Region* nr = find_region(w, n);
                if (!nr) continue;
                if (n != lair) {
                    if (has_tag(nr->tags, "wasteland")) continue;
                    const Faction* o = nr->owner.empty() ? nullptr : find_faction(w, nr->owner);
                    if (o && o->race == "orc" && o->id != f.id) continue;
                }
                seen.push_back(n);
                next.push_back(n);
            }
        }
        frontier = next;
    }
    return false;
}

}
