#include <algorithm>
#include "candidate.h"
#include "sift.h"
#include "sift_util.h"
#include "ages/memory_tiers.h"
#include "ages/world_util.h"

// research-story-sifting.md section 6: one true thread, many tellings. Labov's transforms as rules over the match's
// events and the court's state; contradictions are computed from the transforms, never written.

namespace ages {

namespace {

std::vector<int> kernel_of(const World& w, int match) {
    for (auto& t : w.sift.threads) if (t.id == match) return t.events;
    std::string tag = "match:" + std::to_string(match);
    for (auto& e : w.sift.compounds) if (e.thread == tag) return e.cause.refs;
    return {};
}

std::string agent(const World& w, const Event& e) { return is_dragon_act(e) ? "dragon" : court_of(w, e.subject); }

// the court it fought last, else the dragon if it fears it at all
std::string top_grudge(const World& w, const std::string& court) {
    for (size_t i = w.chronicle.size(); i > 0; --i) {
        const Event& e = w.chronicle[i - 1];
        if (e.kind != "war_started" && e.kind != "old_enemies" && e.kind != "old_enemies_again") continue;
        if (e.subject == court && !e.object.empty()) return e.object;
        if (e.object == court && !e.subject.empty()) return e.subject;
    }
    const Faction* f = find_faction(w, court);
    return f && dread_eff(*f) > 0 ? "dragon" : "";
}

std::string enemy_now(const World& w, const std::string& court) {
    const Faction* f = find_faction(w, court);
    if (!f || f->at_war_with.empty()) return "";
    return *f->at_war_with.begin();
}

}

Source make_source(const World& w, const Params& p, int match, const std::string& court) {
    const SiftConstants& k = p.sift.c;
    Source s;
    s.match = match;
    s.court = court;
    std::vector<int> kernel = kernel_of(w, match);
    if (kernel.empty()) return s;
    std::vector<int> all = kernel;
    for (int id : because_chain(w, p, kernel.front(), k.because_hops)) if (std::find(all.begin(), all.end(), id) == all.end()) all.push_back(id);
    std::sort(all.begin(), all.end());
    const Faction* f = find_faction(w, court);
    std::string tier = f ? memory_tier(*f, p.c.mem) : "";
    s.medium = tier.empty() ? "oral" : tier;
    s.start = all.front();
    for (int id : all)
        if (const Event* e = sift_event(w, id)) if (agent(w, *e) != court) { s.start = id; break; }
    for (int id : all) {
        const Event* e = sift_event(w, id);
        if (!e || id < s.start) continue;
        bool own = agent(w, *e) == court;
        bool harm = own && !e->object.empty() && court_of(w, e->object) != court && k.harm.count(e->kind);
        if (harm) s.omit.push_back(id);
        else {
            s.printed.push_back(id);
            if (own) s.passive.push_back(id);
        }
    }
    s.blame = f && dread_eff(*f) >= k.blame_dread ? "dragon" : top_grudge(w, court);
    if (s.medium == "oral") {
        s.hedge = 1;
        const Event* last = sift_event(w, kernel.back());
        int gens = last ? (w.year - last->year) / std::max(1, k.generation_years) : 0;
        std::string enemy = enemy_now(w, court);
        for (int g = 0; g < gens && !enemy.empty(); ++g) {
            Rng r = rng_for(w.seed, "source_blame", std::to_string(match) + ":" + court + ":" + std::to_string(g));
            if (r.chance(k.oral_reroll)) s.blame = enemy;
        }
    } else if (s.medium == "monumental") {
        int best = 0;
        double top = -1;
        for (int id : s.printed)
            if (const Event* e = sift_event(w, id)) { double r = reportable(w, p, *e); if (r > top) { top = r; best = id; } }
        for (int id : s.printed) if (id != best) s.omit.push_back(id);
        std::sort(s.omit.begin(), s.omit.end());
        s.printed = best ? std::vector<int>{best} : std::vector<int>{};
        s.passive.clear();
    }
    s.stated_cause = s.blame.empty() ? "" : "grudge:" + s.blame;
    s.archetype = s.medium == "oral" ? "teller" : (s.medium == "written" ? "annalist" : "inscription");
    return s;
}

bool sources_conflict(const World& w, const Source& a, const Source& b) {
    if (a.match != b.match) return false;
    if (a.blame != b.blame) return true;
    if (a.start != b.start) {
        int lo = std::min(a.start, b.start), hi = std::max(a.start, b.start);
        for (auto& e : w.chronicle) if (e.id >= lo && e.id < hi && is_dragon_act(e)) return true;
    }
    for (int id : a.omit) if (std::find(b.printed.begin(), b.printed.end(), id) != b.printed.end()) return true;
    for (int id : b.omit) if (std::find(a.printed.begin(), a.printed.end(), id) != a.printed.end()) return true;
    return false;
}

}
