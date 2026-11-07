#include "arrivals.h"
#include "core/detmath.h"
#include "defences.h"
#include "memory_tiers.h"
#include "oaths.h"
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
    if (n == "knows_lair") return b(lair_level(f, g.p) >= 1);
    if (n == "knows_lair_exact") return b(lair_level(f, g.p) >= 2);
    if (n == "lair_on_record")   // the lair written down, or written memory and the way roughly known, or exact
        return b(f.lair_written >= g.p.arrivals.lair_rough || (memory_rank(f, c.mem) >= 2 && lair_level(f, g.p) >= 1) || lair_level(f, g.p) >= 2);
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
    if (n == "sworn_pilgrimage") return b(active_oath_with_pred(w, f.id, "pilgrimage") != nullptr);   // item 09: demand(pilgrimage)
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

namespace {

void raise(double& x, double gain, double cap) {
    if (x < cap) x = std::min(cap, x + gain * (1.0 - x));
}

}

void learn_lair(World& w, const Params& p, Faction& f, const std::string& evidence, double scale) {
    const ArrivalBook& b = p.arrivals;
    auto e = b.lair_evidence.find(evidence);
    if (e == b.lair_evidence.end() || scale <= 0) return;
    auto c = b.lair_caps.find(evidence);
    double cap = c == b.lair_caps.end() ? 1.0 : c->second;
    raise(f.lair_oral, e->second * scale, cap);
    if (keeps_records(w, f)) raise(f.lair_written, e->second * scale * b.lair_written_share, cap);
}

double lair_lore(const Faction& f) { return std::max(f.lair_oral, f.lair_written); }

int lair_level(const Faction& f, const Params& p) {
    double x = lair_lore(f);
    return x >= p.arrivals.lair_exact ? 2 : (x >= p.arrivals.lair_rough ? 1 : 0);
}

// a flight home is seen by the witnesses of an act within sight_regions of the lair and by every court with land within
// sight_regions of it, halved per region of distance; the closest sighting of a waking counts once. A show or a
// receive is no flight; a released hostage lived in the lair
void note_lair_sight(World& w, const Params& p, const Act& a) {
    if (a.type == "release") if (Faction* f = find_faction(w, a.target)) learn_lair(w, p, *f, "hostage");
    if (contains(p.arrivals.no_flight, a.type)) return;
    const Region* at = act_region(w, a);
    int act_d = at ? lair_distance(w, at->id) : 1000;
    auto seen = [&](int dist) { double s = 1; for (int i = 0; i < dist; ++i) s *= p.arrivals.flight_falloff; return s; };
    for (auto& f : w.factions) {
        int d = contains(a.witnesses, f.id) ? act_d : 1000;
        for (auto& rid : f.regions) d = std::min(d, lair_distance(w, rid));
        if (d > p.arrivals.sight_regions) continue;
        auto year = w.arrival_fired.find(f.id + "|sight");
        double before = year != w.arrival_fired.end() && year->second == w.year ? seen(w.arrival_fired[f.id + "|sight_d"]) : 0;
        if (seen(d) <= before) continue;
        learn_lair(w, p, f, "flight", seen(d) - before);
        w.arrival_fired[f.id + "|sight"] = w.year;
        w.arrival_fired[f.id + "|sight_d"] = d;
    }
}

void fade_lair_knowledge(World& w, const Params& p) {
    double written = det::half_life(w.dt, p.c.mem.written_halflife);
    for (auto& f : w.factions) {
        if (f.lair_oral <= 0 && f.lair_written <= 0) continue;
        double hl = halflife_of(w, p, f, "awe", "oral");
        f.lair_oral *= hl > 0 ? det::half_life(w.dt, hl) : 1.0;
        f.lair_written *= written;
        if (f.lair_oral < 1e-3) f.lair_oral = 0;
        if (f.lair_written < 1e-3) f.lair_written = 0;
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
