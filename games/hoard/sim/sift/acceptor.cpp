#include <algorithm>
#include <cstdlib>
#include "sift.h"
#include "sift_util.h"
#include "ages/world_util.h"

// research-story-sifting.md section 2: Winnow-style acceptors. On each new event a live partial dies on an unless hit,
// forks when the event fits its next clause (the parent keeps waiting while the event binds new variables), and
// otherwise passes. A completed match emits its compound event, which the higher levels see next.

namespace ages {

namespace {

using Cast = std::map<std::string, std::string>;

bool is_var(const std::string& s) { return !s.empty() && s[0] == '?'; }

struct Acceptor {
    World& w;
    const Params& p;

    const Event* kernel(const SiftThread& t, int clause) const {
        return clause >= 0 && clause < static_cast<int>(t.events.size()) ? sift_event(w, t.events[clause]) : nullptr;
    }

    // "<clause>.<field>[.<attr>]" or a literal
    std::string path(const SiftPattern& pat, const SiftThread& t, const Cast& cast, const std::string& s) const {
        if (is_var(s)) { auto it = cast.find(s); return it == cast.end() ? "" : it->second; }
        size_t dot = s.find('.');
        if (dot == std::string::npos) return s;
        int ci = -1;
        for (size_t i = 0; i < pat.clauses.size(); ++i) if (pat.clauses[i].as == s.substr(0, dot)) ci = static_cast<int>(i);
        const Event* e = kernel(t, ci);
        if (!e) return "";
        std::string rest = s.substr(dot + 1);
        size_t dot2 = rest.find('.');
        std::string field = rest.substr(0, dot2), attr = dot2 == std::string::npos ? "" : rest.substr(dot2 + 1);
        std::string v;
        if (field == "subject") v = e->subject;
        else if (field == "object") v = e->object;
        else if (field == "winner") v = war_winner(w, *e);
        else if (field.rfind("tag:", 0) == 0) {
            for (auto& tag : e->tags) if (tag.rfind(field.substr(4), 0) == 0) { v = tag.substr(field.size() - 4); break; }
        }
        if (attr == "court") return court_of(w, v);
        if (attr == "race") return race_of(w, v);
        return v;
    }

    bool slot(const std::string& want, const std::string& have, Cast& cast) const {
        if (want.empty()) return true;
        if (!is_var(want)) return want == have;
        auto it = cast.find(want);
        if (it != cast.end()) return it->second == have;
        if (have.empty()) return false;
        cast[want] = have;
        return true;
    }

    bool fits(const SiftMatch& m, const Event& e, Cast& cast) const {
        if (std::find(m.kinds.begin(), m.kinds.end(), e.kind) == m.kinds.end()) return false;
        for (auto& t : m.tags_all) if (!e.tags.count(t)) return false;
        for (auto& t : m.tags_none) if (e.tags.count(t)) return false;
        if (!m.tags_any.empty()) {
            bool any = false;
            for (auto& t : m.tags_any) if (e.tags.count(t)) any = true;
            if (!any) return false;
        }
        if (m.has_magnitude && e.magnitude < m.magnitude_gte) return false;
        return slot(m.subject, e.subject, cast) && slot(m.object, e.object, cast);
    }

    // the cast the thread would have with e as its next kernel event, or false
    bool advance(const SiftPattern& pat, const SiftThread& t, const Event& e, Cast& out) const {
        int k = static_cast<int>(t.events.size());
        const SiftClause& c = pat.clauses[static_cast<size_t>(k)];
        if (k > 0) {
            if (c.within_years > 0 && e.year - t.years.back() > c.within_years) return false;
            if (e.year - t.years.front() > pat.window_years) return false;
            if (std::find(t.events.begin(), t.events.end(), e.id) != t.events.end()) return false;
        }
        bool ok = false;
        for (auto& alt : c.alts) {
            Cast cast = t.cast;
            if (fits(alt, e, cast)) { out = cast; ok = true; break; }
        }
        if (!ok) return false;
        if (c.caused_by.clause >= 0) {
            const Event* r = kernel(t, c.caused_by.clause);
            if (!r || !reaches(w, e, r->id, c.caused_by.max_hops)) return false;
        }
        if (c.not_caused_by.clause >= 0) {
            const Event* r = kernel(t, c.not_caused_by.clause);
            if (r && reaches(w, e, r->id, c.not_caused_by.max_hops)) return false;
        }
        if (c.same_thread_as >= 0) {
            const Event* r = kernel(t, c.same_thread_as);
            if (!r || r->thread.empty() || r->thread != e.thread) return false;
        }
        SiftThread probe = t;
        probe.events.push_back(e.id);
        probe.years.push_back(e.year);
        for (auto& b : c.bind) {
            std::string v = path(pat, probe, out, b.second);
            if (v.empty()) return false;
            if (!slot(b.first, v, out)) return false;
        }
        for (auto& wh : c.where) {
            std::string a = path(pat, probe, out, wh.first), b = is_var(wh.second) ? path(pat, probe, out, wh.second) : wh.second;
            if (a.empty() || a != b) return false;
        }
        return !barred(pat, out) && !once_taken(pat, out);
    }

    static std::string key_of(const std::string& pid, const std::vector<std::string>& vars, const Cast& cast) {
        std::string k = pid;
        for (auto& v : vars) {
            auto it = cast.find(v);
            if (it == cast.end()) return "";
            k += "|" + v + "=" + it->second;
        }
        return k;
    }

    static std::vector<std::string> vars_of(const SiftMatch& m) {
        std::vector<std::string> v;
        if (is_var(m.subject)) v.push_back(m.subject);
        if (is_var(m.object)) v.push_back(m.object);
        return v;
    }

    bool barred(const SiftPattern& pat, const Cast& cast) const {
        for (auto& g : pat.unless_forever) {
            std::string k = key_of(pat.id, vars_of(g.m), cast);
            if (!k.empty() && w.sift.barred.count(k)) return true;
        }
        return false;
    }

    bool once_taken(const SiftPattern& pat, const Cast& cast) const {
        if (pat.once_per.empty()) return false;
        std::string k = key_of(pat.id, pat.once_per, cast);
        return !k.empty() && w.sift.once.count(k);
    }

    bool guard_on(const SiftGuard& g, const SiftThread& t) const {
        int stage = static_cast<int>(t.events.size());
        if (g.from < 0) return true;
        return stage > g.from && stage <= g.to;
    }

    // a latent partial that ends was never a thread: it is dropped
    void end(SiftThread& t, const std::string& state, const std::string& why) {
        t.state = t.state == "latent" && state == "cut" ? "dropped" : state;
        t.cut_by = why;
        t.ended = w.year;
    }

    std::string state_of(const SiftPattern& pat, const SiftThread& t) const {
        return t.events.size() * 3 > pat.clauses.size() ? "open" : "latent";
    }

    double magnitude(const SiftPattern& pat, const SiftThread& t) const {
        const std::string& s = pat.emit_magnitude;
        if (s.empty()) return 0;
        auto term = [&](std::string x) {
            while (!x.empty() && x.front() == ' ') x.erase(x.begin());
            while (!x.empty() && x.back() == ' ') x.pop_back();
            size_t dot = x.find(".magnitude");
            if (dot == std::string::npos) return std::strtod(x.c_str(), nullptr);
            for (size_t i = 0; i < pat.clauses.size(); ++i)
                if (pat.clauses[i].as == x.substr(0, dot)) if (const Event* e = kernel(t, static_cast<int>(i))) return e->magnitude;
            return 0.0;
        };
        if (s.rfind("max(", 0) == 0) {
            double m = 0;
            std::string inner = s.substr(4, s.size() - 5);
            size_t from = 0;
            for (size_t comma; (comma = inner.find(',', from)) != std::string::npos; from = comma + 1) m = std::max(m, term(inner.substr(from, comma - from)));
            return std::max(m, term(inner.substr(from)));
        }
        return term(s);
    }

    std::string outcome(const SiftPattern& pat, const SiftThread& t) const {
        const Event* e = kernel(t, pat.outcome_clause);
        if (!e) return "";
        if (pat.outcome_field == "kind") { auto it = pat.outcomes.find(e->kind); return it == pat.outcomes.end() ? "" : it->second; }
        for (auto& o : pat.outcomes) if (e->tags.count(o.first)) return o.second;
        auto it = pat.outcomes.find("*");
        return it == pat.outcomes.end() ? "" : it->second;
    }

    // the properties surprise is computed over (base_rates.json is counted over the same ones)
    Tags properties(const SiftPattern& pat, const SiftThread& t, const std::string& out) const {
        Tags p;
        if (!out.empty()) p.insert("p:outcome=" + out);
        std::string who = protagonist(pat, t);
        std::string race = race_of(w, who);
        if (!race.empty()) p.insert("p:race=" + race);
        int span = t.years.back() - t.years.front();
        p.insert(std::string("p:span=") + (span < 30 ? "short" : (span < 300 ? "mid" : "long")));
        bool dragon = false;
        for (int id : t.events) if (const Event* e = sift_event(w, id)) if (is_dragon_act(*e)) dragon = true;
        p.insert(std::string("p:dragon=") + (dragon ? "1" : "0"));
        return p;
    }

    std::string role_value(const SiftThread& t, const std::string& s) const {
        if (!is_var(s)) return s;
        auto it = t.cast.find(s);
        return it == t.cast.end() ? "" : it->second;
    }

    void complete(const SiftPattern& pat, SiftThread& t) {
        if (!pat.once_per.empty()) w.sift.once.insert(key_of(pat.id, pat.once_per, t.cast));
        t.outcome = outcome(pat, t);
        Event e;
        e.id = w.sift.next_event++;
        e.year = t.years.back();
        e.kind = pat.emit_kind;
        e.subject = role_value(t, pat.emit_subject);
        e.object = role_value(t, pat.emit_object);
        e.magnitude = magnitude(pat, t);
        e.text_key = "pat." + pat.id;
        e.tags = pat.emit_tags;
        if (!t.outcome.empty()) e.tags.insert("outcome:" + t.outcome);
        e.tags.insert("era:" + std::to_string(valley_era(w)));
        for (auto& x : properties(pat, t, t.outcome)) e.tags.insert(x);
        e.cause.refs = t.events;
        e.cause.text_key = "pattern";
        e.thread = "match:" + std::to_string(t.id);
        e.importance = pat.tier / 5.0;
        w.sift.compounds.push_back(e);
        t.emitted = e.id;
        t.state = "closed";
        t.ended = e.year;
        pending.push_back(e.id);
    }

    void cap(const std::string& pid) {
        std::vector<SiftThread*> live;
        for (auto& t : w.sift.threads) if (t.pattern == pid && (t.state == "latent" || t.state == "open")) live.push_back(&t);
        int over = static_cast<int>(live.size()) - p.sift.c.partial_cap;
        if (over <= 0) return;
        // the lowest score: fewest clauses bound, then the stalest
        std::sort(live.begin(), live.end(), [](const SiftThread* a, const SiftThread* b) {
            if (a->events.size() != b->events.size()) return a->events.size() < b->events.size();
            return a->advanced != b->advanced ? a->advanced < b->advanced : a->id < b->id;
        });
        for (int i = 0; i < over; ++i) end(*live[static_cast<size_t>(i)], "cut", "evicted");
    }

    std::map<std::string, std::vector<size_t>> kinds;   // event kind -> the patterns that read it

    void index() {
        for (size_t i = 0; i < p.sift.patterns.size(); ++i) {
            const SiftPattern& pat = p.sift.patterns[i];
            std::set<std::string> ks;
            for (auto& c : pat.clauses) for (auto& a : c.alts) ks.insert(a.kinds.begin(), a.kinds.end());
            for (auto* gs : {&pat.unless, &pat.unless_forever}) for (auto& g : *gs) ks.insert(g.m.kinds.begin(), g.m.kinds.end());
            if (!pat.survives_death) ks.insert("person_died");
            for (auto& k : ks) kinds[k].push_back(i);
        }
    }

    void feed(const Event& e) {
        int level = event_level(e);
        auto hit = kinds.find(e.kind);
        if (hit == kinds.end()) return;
        std::map<std::string, std::vector<size_t>> by_pattern;
        for (size_t i = 0; i < w.sift.threads.size(); ++i) {
            const SiftThread& t = w.sift.threads[i];
            if (t.state == "latent" || t.state == "open") by_pattern[t.pattern].push_back(i);
        }
        std::vector<SiftThread> born;
        std::set<std::string> grew;
        for (size_t pi : hit->second) {
            const SiftPattern& pat = p.sift.patterns[pi];
            if (pat.level <= level) continue;
            for (size_t ti : by_pattern[pat.id]) {
                SiftThread& t = w.sift.threads[ti];
                bool killed = false;
                for (auto& g : pat.unless) {
                    Cast c = t.cast;
                    if (guard_on(g, t) && fits(g.m, e, c) && c == t.cast) { end(t, "cut", "unless"); killed = true; break; }
                }
                for (auto& g : pat.unless_forever) {
                    if (killed) break;
                    Cast c = t.cast;
                    if (fits(g.m, e, c) && c == t.cast) {
                        std::string k = key_of(pat.id, vars_of(g.m), t.cast);
                        if (!k.empty()) w.sift.barred.insert(k);
                        end(t, "cut", "unless_forever");
                        killed = true;
                    }
                }
                if (killed) continue;
                if (!pat.survives_death && e.kind == "person_died") {
                    bool dead = false;
                    for (auto& kv : t.cast) if (kv.second == e.subject) dead = true;
                    if (dead) { end(t, "cut", "died"); continue; }
                }
                Cast cast;
                if (!advance(pat, t, e, cast)) continue;
                SiftThread next = t;
                next.cast = cast;
                next.events.push_back(e.id);
                next.years.push_back(e.year);
                next.advanced = e.year;
                if (cast.size() == t.cast.size()) {   // nothing new bound: advance in place
                    next.id = t.id;
                    t = next;
                    if (t.state == "latent" && state_of(pat, t) == "open") { t.state = "open"; t.opened = e.year; }
                    if (t.events.size() == pat.clauses.size()) complete(pat, t);
                } else {
                    next.id = w.sift.next_thread++;
                    next.shown.clear();
                    next.state = state_of(pat, next);
                    if (next.state == "open") next.opened = t.state == "open" ? t.opened : e.year;
                    born.push_back(next);
                }
            }
            SiftThread root;
            root.pattern = pat.id;
            Cast cast;
            if (advance(pat, root, e, cast)) {
                root.id = w.sift.next_thread++;
                root.cast = cast;
                root.events.push_back(e.id);
                root.years.push_back(e.year);
                root.advanced = e.year;
                root.state = state_of(pat, root);
                if (root.state == "open") root.opened = e.year;
                born.push_back(root);
            }
        }
        for (auto& b : born) {
            w.sift.threads.push_back(b);
            grew.insert(b.pattern);
            const SiftPattern* pat = find_pattern(p, b.pattern);
            if (pat && b.events.size() == pat->clauses.size()) complete(*pat, w.sift.threads.back());
        }
        for (auto& id : grew) cap(id);
    }

    // windows that lapsed with no event to notice it
    void lapse() {
        for (auto& t : w.sift.threads) {
            if (t.state != "latent" && t.state != "open") continue;
            const SiftPattern* pat = find_pattern(p, t.pattern);
            if (!pat) { end(t, "cut", "lapsed"); continue; }
            const SiftClause& c = pat->clauses[t.events.size()];
            if ((c.within_years > 0 && w.year - t.years.back() > c.within_years) || w.year - t.years.front() > pat->window_years) end(t, "cut", "lapsed");
        }
        w.sift.threads.erase(std::remove_if(w.sift.threads.begin(), w.sift.threads.end(), [](const SiftThread& t) { return t.state == "dropped"; }),
                             w.sift.threads.end());
    }

    std::vector<int> pending;

    void run() {
        index();
        size_t from = w.chronicle.size();
        while (from > 0 && (w.chronicle[from - 1].id == 0 || w.chronicle[from - 1].id > w.sift.cursor)) --from;
        for (size_t i = from; i < w.chronicle.size(); ++i) {
            const Event& e = w.chronicle[i];
            if (e.id <= w.sift.cursor) continue;
            feed(e);
            while (!pending.empty()) {
                int c = pending.front();
                pending.erase(pending.begin());
                if (const Event* x = sift_event(w, c)) { Event cc = *x; feed(cc); }
            }
            w.sift.cursor = e.id;
        }
        lapse();
    }
};

}

void sift_events(World& w, const Params& p) {
    if (p.sift.patterns.empty()) return;
    w.sift.on = true;
    Acceptor a{w, p, {}, {}};
    a.run();
}

}
