#include <algorithm>
#include "candidate.h"
#include "sift.h"
#include "sift_util.h"
#include "ages/importance.h"
#include "ages/world_util.h"

// research-story-sifting.md section 5: the thread score, features on 0..1 from the log and the dry-run tables

namespace ages {

namespace {

double importance_of(const World& w, const Params& p, const Event& e) { return e.importance >= 0 ? e.importance : event_importance(w, p, e); }

double magnitude_pct(const Params& p, const Event& e) {
    auto it = p.sift.magnitudes.find(e.kind);
    if (it == p.sift.magnitudes.end() || it->second.empty()) return 0.5;
    size_t below = 0;
    for (double d : it->second) if (e.magnitude >= d) ++below;
    return static_cast<double>(below) / static_cast<double>(it->second.size());
}

bool descends_from_act(const World& w, const Event& e, int hops) {
    std::vector<int> frontier = e.cause.refs;
    for (int h = 0; h < hops && !frontier.empty(); ++h) {
        std::vector<int> next;
        for (int id : frontier)
            if (const Event* x = sift_event(w, id)) {
                if (is_dragon_act(*x)) return true;
                next.insert(next.end(), x->cause.refs.begin(), x->cause.refs.end());
            }
        frontier.swap(next);
    }
    return false;
}

bool involves(const World& w, const Event& e, const std::string& who) {
    if (who.empty()) return false;
    return e.subject == who || e.object == who || (!find_person(w, who) && court_of(w, e.subject) == who);
}

double proximity(const World& w, const Candidate& c) {
    std::vector<std::string> ids;
    if (c.thread) for (auto& kv : c.thread->cast) ids.push_back(kv.second);
    else for (int id : c.events) if (const Event* e = sift_event(w, id)) { ids.push_back(e->subject); ids.push_back(e->object); }
    if (!c.protagonist.empty()) ids.push_back(c.protagonist);
    std::string lair, lair_owner;
    std::vector<std::string> near;
    for (auto& r : w.regions) if (r.lair) { lair = r.id; lair_owner = r.owner; near = r.neighbours; }
    double best = 0.2;
    for (auto& id : ids) {
        if (id.empty() || id == "dragon") continue;
        std::string court = court_of(w, id);
        if (id == lair || (!lair_owner.empty() && court == lair_owner) || find_known(w, id)) return 1.0;
        for (auto& o : w.oaths) if (o.status == "active" && (o.parties[1] == court || o.parties[1] == id)) return 1.0;
        for (auto& n : near) {
            const Region* r = find_region(w, n);
            if (n == id || (r && !r->owner.empty() && r->owner == court)) best = std::max(best, 0.5);
        }
    }
    return best;
}

double surprise(const World& w, const Params& p, const Candidate& c) {
    if (!c.thread || !c.thread->emitted) return 0.5;
    const Event* e = sift_event(w, c.thread->emitted);
    if (!e) return 0.5;
    std::string era;
    for (auto& t : e->tags) if (t.rfind("era:", 0) == 0) era = t.substr(4);
    auto it = p.sift.rates.find(c.thread->pattern + "|" + era);
    if (it == p.sift.rates.end()) return 0.5;
    auto n = it->second.find("n");
    if (n == it->second.end() || n->second <= 0) return 0.5;
    double sum = 0;
    int k = 0;
    for (auto& t : e->tags) {
        if (t.rfind("p:", 0) != 0) continue;
        auto r = it->second.find(t);
        if (r == it->second.end()) continue;   // merged into a property it co-occurs with, or never counted
        sum += r->second / n->second;
        ++k;
    }
    return k ? clamp01(1.0 - sum / k) : 0.5;
}

int reversal_count(const World& w, const Params& p, const Candidate& c) {
    int last = 0, changes = 0;
    for (int id : c.events) {
        const Event* e = sift_event(w, id);
        if (!e) continue;
        int s = 0;
        if (e->kind == "war_ended") {   // the winner gains, the loser loses
            std::string won = war_winner(w, *e);
            if (!won.empty() && (e->subject == c.protagonist || e->object == c.protagonist)) s = won == c.protagonist ? 1 : -1;
        } else {
            auto it = p.sift.c.fortune.find(e->kind);
            if (it == p.sift.c.fortune.end()) continue;
            if (e->subject == c.protagonist) s = it->second;
            else if (e->object == c.protagonist) s = -it->second;
        }
        if (s == 0) continue;
        if (last != 0 && s != last) ++changes;
        last = s;
    }
    return changes;
}

}

double reportable(const World& w, const Params& p, const Event& e) { return importance_of(w, p, e) * (0.5 + 0.5 * magnitude_pct(p, e)); }

Features features(const World& w, const Params& p, const Candidate& c) {
    const SiftConstants& k = p.sift.c;
    Features f;
    std::vector<const Event*> evs;
    for (int id : c.events) if (const Event* e = sift_event(w, id)) evs.push_back(e);
    for (auto* e : evs) {
        if (is_dragon_act(*e)) f.dragon = 1;
        else if (f.dragon < 0.6 && descends_from_act(w, *e, k.dragon_hops)) f.dragon = 0.6;
        f.stakes = std::max(f.stakes, reportable(w, p, *e));
        if (k.irreversible.count(e->kind)) f.irreversible = 1;
    }
    if (evs.size() > 1) {
        int links = 0;
        for (size_t j = 1; j < evs.size(); ++j)
            for (size_t i = 0; i < j; ++i) if (contains_ref(evs[j]->cause, evs[i]->id)) ++links;
        f.causal = clamp01(static_cast<double>(links) / static_cast<double>(evs.size() - 1));
    }
    if (!evs.empty() && !c.protagonist.empty()) {
        int n = 0;
        for (auto* e : evs) if (involves(w, *e, c.protagonist)) ++n;
        f.continuity = static_cast<double>(n) / static_cast<double>(evs.size());
        if (w.sift.named.count(c.protagonist)) f.continuity = std::min(1.0, f.continuity + 0.5);
    }
    f.proximity = proximity(w, c);
    f.surprise = surprise(w, p, c);
    f.reversal_count = reversal_count(w, p, c);
    const std::vector<double>& rs = k.reversal_score;
    if (!rs.empty()) f.reversals = rs[std::min(static_cast<size_t>(f.reversal_count), rs.size() - 1)];
    return f;
}

double weighted(const SiftConstants& k, const Features& f) {
    const SiftWeights& x = k.w;
    return x.dragon * f.dragon + x.stakes * f.stakes + x.causal * f.causal + x.continuity * f.continuity + x.proximity * f.proximity +
           x.surprise * f.surprise + x.reversals * f.reversals + x.irreversible * f.irreversible;
}

double thread_score(const World& w, const Params& p, const SiftThread& t) {
    Candidate c;
    c.thread = &t;
    c.pattern = find_pattern(p, t.pattern);
    c.events = t.events;
    if (c.pattern) c.protagonist = protagonist(*c.pattern, t);
    return weighted(p.sift.c, features(w, p, c));
}

}
