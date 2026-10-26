#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "data.h"
#include "state.h"

namespace ages {

inline double clamp01(double v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

inline Faction* find_faction(World& w, const std::string& id) {
    for (auto& f : w.factions) if (f.id == id) return &f;
    return nullptr;
}
inline const Faction* find_faction(const World& w, const std::string& id) {
    for (auto& f : w.factions) if (f.id == id) return &f;
    return nullptr;
}
inline Region* find_region(World& w, const std::string& id) {
    for (auto& r : w.regions) if (r.id == id) return &r;
    return nullptr;
}
inline const Region* find_region(const World& w, const std::string& id) {
    for (auto& r : w.regions) if (r.id == id) return &r;
    return nullptr;
}

inline bool has_tag(const Tags& t, const std::string& s) { return t.count(s) > 0; }

inline bool contains(const std::vector<std::string>& v, const std::string& s) {
    return std::find(v.begin(), v.end(), s) != v.end();
}
inline void erase_value(std::vector<std::string>& v, const std::string& s) {
    v.erase(std::remove(v.begin(), v.end(), s), v.end());
}

inline double era_mult(const Constants& c, int era) {
    if (era < 0) era = 0;
    if (era >= static_cast<int>(c.era_mult.size())) era = static_cast<int>(c.era_mult.size()) - 1;
    return c.era_mult[static_cast<size_t>(era)];
}

inline double power(const Faction& f, const Constants& c) {
    return std::sqrt(std::max(0.0, f.wealth + f.eff.vassal_wealth)) * era_mult(c, f.era) * f.cohesion;
}

inline std::string dominant_race(const Faction& f) {
    std::string best = f.race;
    double share = -1;
    for (auto& kv : f.pop) if (kv.second > share) { share = kv.second; best = kv.first; }
    return best;
}

inline std::string overlord_of(const Faction& f) {
    for (auto& t : f.tags) if (t.rfind("vassal_of:", 0) == 0) return t.substr(10);
    return "";
}

inline bool is_vassal(const Faction& f) { return !overlord_of(f).empty(); }

// own regions plus the regions of direct vassals
inline size_t realm_regions(const World& w, const Faction& f) {
    size_t n = f.regions.size();
    for (auto& g : w.factions) if (overlord_of(g) == f.id) n += g.regions.size();
    return n;
}

inline bool allied(const Faction& a, const Faction& b) { return has_tag(a.tags, "ally:" + b.id) || has_tag(b.tags, "ally:" + a.id); }

inline bool home_terrain(const Culture& c, const Region& r) {
    if (contains(c.temperament.home_terrain, r.terrain)) return true;
    return has_tag(r.tags, "wasteland") && contains(c.temperament.home_terrain, "wasteland");
}

inline double race_affinity(const Params& p, const std::string& a, const std::string& b) {
    auto ia = p.race_affinity.find(a);
    if (ia == p.race_affinity.end()) return 0;
    auto ib = ia->second.find(b);
    return ib == ia->second.end() ? 0 : ib->second;
}

inline double trait_distance(const Traits& a, const Traits& b) {
    return (std::fabs(a.authority - b.authority) + std::fabs(a.openness - b.openness) +
            std::fabs(a.militarism - b.militarism) + std::fabs(a.piety - b.piety)) / 4.0;
}

// 0.5 * pop-weighted race affinity + 0.5 * (1 - 2 * mean absolute trait distance), both halves in -1..1
inline double affinity(const Params& p, const Faction& a, const Faction& b) {
    double race = 0;
    for (auto& ra : a.pop)
        for (auto& rb : b.pop) race += ra.second * rb.second * race_affinity(p, ra.first, rb.first);
    return 0.5 * race + 0.5 * (1.0 - 2.0 * trait_distance(a.culture.traits, b.culture.traits));
}

inline double threat(const Dragon& d, const Constants& c) {
    return (c.threat_base + d.size * c.threat_k_size + d.magic * c.threat_k_magic) * c.threat_scale;
}

inline bool at_war(const Faction& f, const std::string& other) { return contains(f.at_war_with, other); }

// the one definition of "losing": at war, and either invaded within losing_window_decades or facing a stronger enemy
inline bool losing(const World& w, const Constants& c, const Faction& f) {
    if (f.at_war_with.empty()) return false;
    if (f.last_invaded > w.year - c.losing_window_decades * c.step_years) return true;
    double mine = power(f, c);
    for (auto& e : f.at_war_with) {
        const Faction* o = find_faction(w, e);
        if (o && power(*o, c) > mine) return true;
    }
    return false;
}

inline bool adjacent(const World& w, const Faction& a, const Faction& b) {
    for (auto& ra : a.regions) {
        const Region* r = find_region(w, ra);
        if (!r) continue;
        for (auto& n : r->neighbours)
            if (contains(b.regions, n)) return true;
    }
    return false;
}

// factions bordering the realm (the faction plus its vassals), vassals included
inline std::vector<std::string> realm_neighbours(const World& w, const Faction& h) {
    std::vector<std::string> out;
    for (auto& g : w.factions) {
        if (g.id == h.id) continue;
        bool near = overlord_of(g) == h.id || adjacent(w, h, g);
        if (!near)
            for (auto& v : w.factions)
                if (overlord_of(v) == h.id && adjacent(w, v, g)) { near = true; break; }
        if (near) out.push_back(g.id);
    }
    return out;
}

inline void sort_by_id(World& w) {
    std::sort(w.factions.begin(), w.factions.end(), [](const Faction& a, const Faction& b) { return a.id < b.id; });
    std::sort(w.regions.begin(), w.regions.end(), [](const Region& a, const Region& b) { return a.id < b.id; });
}

// years of this step over the decade: every per-decade rate in the fast rules is multiplied by it (exactly 1 at dt = 10)
inline double dts(const World& w, const Constants& c) { return static_cast<double>(w.dt) / static_cast<double>(c.step_years); }

inline std::string war_thread(const War& x) { return x.thread.empty() ? "war:" + x.a + ":" + x.b + ":" + std::to_string(x.started) : x.thread; }

inline void open_thread(World& w, const std::string& id) {
    if (id.empty() || contains(w.threads_open, id)) return;
    w.threads_open.push_back(id);
    std::sort(w.threads_open.begin(), w.threads_open.end());
}
inline void close_thread(World& w, const std::string& id) { erase_value(w.threads_open, id); }

inline Cause because(const std::string& text_key, std::vector<int> refs = {}, std::map<std::string, double> params = {}) {
    Cause c;
    c.text_key = text_key;
    for (int r : refs) if (r > 0) c.refs.push_back(r);
    std::sort(c.refs.begin(), c.refs.end());
    c.refs.erase(std::unique(c.refs.begin(), c.refs.end()), c.refs.end());
    c.params = params;
    return c;
}

inline bool contains_ref(const Cause& c, int ref) { return std::find(c.refs.begin(), c.refs.end(), ref) != c.refs.end(); }

inline void add_ref(Cause& c, int ref) {
    if (ref <= 0 || std::find(c.refs.begin(), c.refs.end(), ref) != c.refs.end()) return;
    c.refs.push_back(ref);
    std::sort(c.refs.begin(), c.refs.end());
}

// the newest event of `kind` (any kind when empty) about `subject` (any when empty) and `object` (any when empty),
// carrying `tag` (any when empty), not older than `since`; 0 when there is none
inline int last_event_id(const World& w, const std::string& kind, const std::string& subject = "",
                         const std::string& object = "", const std::string& tag = "", int since = -1000000) {
    for (size_t i = w.chronicle.size(); i > 0; --i) {
        const Event& e = w.chronicle[i - 1];
        if (e.year < since) return 0;
        if (!kind.empty() && e.kind != kind) continue;
        if (!subject.empty() && e.subject != subject) continue;
        if (!object.empty() && e.object != object) continue;
        if (!tag.empty() && !e.tags.count(tag)) continue;
        return e.id;
    }
    return 0;
}

inline const Event* find_event(const World& w, int id) {
    if (id <= 0) return nullptr;
    for (size_t i = w.chronicle.size(); i > 0; --i)
        if (w.chronicle[i - 1].id == id) return &w.chronicle[i - 1];
    return nullptr;
}

// returns the new event's id; sleep bookkeeping (sleep_begin, wake) passes numbered = false and gets id 0, so
// the same history in one sleep or ten gives the same ids
inline int emit(World& w, const std::string& kind, const std::string& subject, const std::string& object = "",
                double magnitude = 0, const Tags& tags = {}, const Cause& cause = {}, const std::string& thread = "",
                bool numbered = true) {
    Event e;
    e.id = numbered ? w.next_event_id++ : 0;
    e.year = w.year;
    e.kind = kind;
    e.subject = subject;
    e.object = object;
    e.magnitude = magnitude;
    e.text_key = "ev." + kind;
    e.tags = tags;
    e.cause = cause;
    e.thread = thread;
    w.chronicle.push_back(e);
    return e.id;
}

// copies of the events of this step so far, newest first: the rules read them to name causes while emitting more
inline std::vector<Event> events_this_step(const World& w) {
    std::vector<Event> out;
    for (size_t i = w.chronicle.size(); i > 0; --i) {
        const Event& e = w.chronicle[i - 1];
        if (e.year != w.year) break;
        out.push_back(e);
    }
    return out;
}

inline Person* find_person(World& w, const std::string& id) {
    for (auto& p : w.people) if (p.id == id) return &p;
    return nullptr;
}
inline const Person* find_person(const World& w, const std::string& id) {
    for (auto& p : w.people) if (p.id == id) return &p;
    return nullptr;
}
inline Person* find_person_role(World& w, const std::string& faction, const std::string& role) {
    for (auto& p : w.people) if (p.faction == faction && p.role == role) return &p;
    return nullptr;
}
inline const Person* find_person_role(const World& w, const std::string& faction, const std::string& role) {
    for (auto& p : w.people) if (p.faction == faction && p.role == role) return &p;
    return nullptr;
}
inline bool is_person_id(const World& w, const std::string& id) {
    if (find_person(w, id)) return true;
    for (auto& d : w.dead_people) if (d.id == id) return true;
    return false;
}
inline Known* find_known(World& w, const std::string& id) {
    for (auto& k : w.dragons[kPlayer].known_people) if (k.id == id) return &k;
    return nullptr;
}
inline const Known* find_known(const World& w, const std::string& id) {
    for (auto& k : w.dragons[kPlayer].known_people) if (k.id == id) return &k;
    return nullptr;
}

// the dragon meets a mortal (or deepens an acquaintance): the ledger entry gains `delta` relationship, clamped to -1..1
inline Known& meet(World& w, const std::string& id, const std::string& how, double delta = 0) {
    Known* k = find_known(w, id);
    if (!k) {
        Known n;
        n.id = id;
        n.met = w.year;
        n.how = how;
        w.dragons[kPlayer].known_people.push_back(n);
        std::sort(w.dragons[kPlayer].known_people.begin(), w.dragons[kPlayer].known_people.end(), [](const Known& a, const Known& b) { return a.id < b.id; });
        k = find_known(w, id);
    }
    k->relationship = std::max(-1.0, std::min(1.0, k->relationship + delta));
    return *k;
}

// 17.4: thresholds per phase (size, knowledge, magic); each axis contributes its share of the threshold, 1 opens the phase.
// v0.4: with phase_points the score is the total growth over the phase's points
inline double phase_score(const Dragon& d, const Constants& c, int phase) {
    if (!c.phase_points.empty()) {
        size_t k = static_cast<size_t>(phase - 2);
        if (phase < 2 || k >= c.phase_points.size()) return 0;
        double total = 0;
        for (auto& kv : d.growth) total += kv.second;
        return c.phase_points[k] > 0 ? total / c.phase_points[k] : 1;
    }
    size_t i = static_cast<size_t>(phase - 2) * 3;
    if (phase < 2 || i + 2 >= c.phase_thresholds.size()) return 0;
    double s = c.phase_thresholds[i], k = c.phase_thresholds[i + 1], m = c.phase_thresholds[i + 2];
    return (s > 0 ? d.size / s : 0) + (k > 0 ? d.knowledge / k : 0) + (m > 0 ? d.magic / m : 0);
}

// 17.4: the phase the dragon's growth qualifies for; the sum of each axis over its threshold reaching 1 opens the next phase
inline int eligible_phase(const Dragon& d, const Constants& c) {
    int phase = 1;
    while (phase < 5 && phase_score(d, c, phase + 1) >= 1.0) phase += 1;
    return phase;
}

inline int phase_index(const Dragon& d) { return d.phase < 1 ? 0 : (d.phase > 5 ? 4 : d.phase - 1); }
inline int short_sleep(const Dragon& d, const Constants& c) { return c.phase_short_sleep[static_cast<size_t>(phase_index(d))]; }
inline int long_sleep(const Dragon& d, const Constants& c) { return c.phase_long_sleep[static_cast<size_t>(phase_index(d))]; }
inline bool ready_to_moult(const Dragon& d, const Constants& c) { return eligible_phase(d, c) > d.phase; }

inline void recompute_fear_faith(Faction& f, const Constants& c) {
    double fear = 0, faith = 0;
    bool blocked = false;
    for (auto& m : f.memory) {
        fear += m.weight * m.fear;
        faith += m.weight * m.faith;
        if (has_tag(m.tags, "betrayal") && m.weight > c.betrayal_block_weight) blocked = true;
    }
    f.fear = clamp01(fear);
    double nf = clamp01(faith);
    f.faith = blocked ? std::min(nf, f.faith) : nf;
}

inline void remember(Faction& f, const Constants& c, int year, const std::string& type, double magnitude,
                     double fear_delta, double faith_delta, const Tags& tags) {
    MemoryEntry m;
    m.year = year;
    m.type = type;
    m.magnitude = magnitude;
    m.weight = 1;
    m.fear = fear_delta;
    m.faith = faith_delta;
    m.tags = tags;
    f.memory.push_back(m);
    recompute_fear_faith(f, c);
}

inline void set_faith(Faction& f, const Constants& c, int year, const std::string& type, double magnitude,
                      double target, double fear_delta, const Tags& tags) {
    double current = 0;
    for (auto& m : f.memory) current += m.weight * m.faith;
    remember(f, c, year, type, magnitude, fear_delta, target - current, tags);
    f.faith = target;
}

inline void add_modifier(World& w, const std::string& source, const std::string& target, const std::string& var,
                         const std::string& op, double value, int decades) {
    Modifier m;
    m.source = source;
    m.target = target;
    m.var = var;
    m.op = op;
    m.value = value;
    m.from = w.year;
    m.until = w.year + decades * 10;
    w.modifiers.push_back(m);
}

inline FactionRecord record_of(const Faction& f, int died) {
    FactionRecord r;
    r.id = f.id;
    r.race = f.race;
    r.born = f.born;
    r.died = died;
    r.max_regions = f.max_regions;
    r.max_realm = f.max_realm;
    r.era_years = f.era_years;
    return r;
}

inline void end_wars_of(World& w, const std::string& id) {
    for (auto& f : w.factions) erase_value(f.at_war_with, id);
    w.wars.erase(std::remove_if(w.wars.begin(), w.wars.end(), [&](const War& x) { return x.a == id || x.b == id; }),
                 w.wars.end());
}

}
