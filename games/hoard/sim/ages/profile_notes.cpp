#include <algorithm>
#include "endings.h"
#include "memory_tiers.h"
#include "names.h"
#include "profile.h"
#include "rules/slayers.h"
#include "world_util.h"
#include "wounds.h"

// item 04: what a run teaches the profile. Atoms only ever rise (rumor < witnessed < used): a condition is witnessed
// when a briefing shows it true, a rumor when one of its routes told of it, used when an ending fired with it.

namespace ages {

namespace {

int rank(const std::string& s) { return s == "used" ? 3 : s == "witnessed" ? 2 : s == "rumor" ? 1 : 0; }

void raise(Profile& pr, const std::string& ending, const std::string& cond, const std::string& state, const std::string& how) {
    Atom& a = pr.atoms[ending][cond];
    if (rank(state) <= rank(a.state)) return;
    a.state = state;
    a.how = how;
}

bool has_route(const EndingCond& c, const std::string& via) {
    for (auto& r : c.rumors) if (r.via == via) return true;
    return false;
}

int waking_index(const World& w) {
    int n = 0;
    for (auto& e : w.chronicle) if (e.kind == "wake") n += 1;
    return n;
}

void see(Profile& pr, const std::string& id, const std::string& kind, const World& w, int era) {
    if (pr.bestiary.count(id)) return;
    pr.bestiary[id] = {kind, waking_index(w), w.year - w.genesis.hatched_year, era};
}

double threshold(const EndingCond& c) {
    for (auto& q : c.when) if (q.type == "count") return std::max(1.0, q.value);
    return 1;
}

}

bool atom_known(const Profile& pr, const std::string& ending, const std::string& cond) {
    auto e = pr.atoms.find(ending);
    if (e == pr.atoms.end()) return false;
    auto c = e->second.find(cond);
    return c != e->second.end() && rank(c->second.state) > 0;
}

void note_hatching(Profile& pr, const World& w, const Params& p) {
    for (auto& l : w.prophecy.lines) {
        GlossaryEntry& g = pr.glossary[l.key];
        g.ending = w.prophecy.ending;
        g.cond = l.cond;
    }
    for (auto& e : p.endings.endings) {
        if (!e.enabled) continue;
        for (auto& c : e.conds) {
            if (has_route(c, "prophecy") && e.id == w.prophecy.ending)
                for (auto& l : w.prophecy.lines) if (l.cond == c.id) raise(pr, e.id, c.id, "rumor", "prophecy");
            if (has_route(c, "legend"))
                for (auto& l : pr.legends) if (l.ending == e.id && has_tag(w.tags, "legend:" + l.id)) raise(pr, e.id, c.id, "rumor", "legend");
        }
    }
}

void note_waking(Profile& pr, const World& w, const Params& p) {
    if (dragon_dead(w)) return;
    std::set<std::string> told;
    for (auto& e : w.chronicle) if (e.year > w.sleep_started && e.id > 0) told.insert(e.kind);
    for (auto& e : p.endings.endings) {
        if (!e.enabled) continue;
        std::vector<bool> met = conditions_met(w, p, e);
        for (size_t i = 0; i < e.conds.size(); ++i) {
            const EndingCond& c = e.conds[i];
            if (met[i]) raise(pr, e.id, c.id, "witnessed", "briefing");
            for (auto& r : c.rumors)
                if (r.via == "event")
                    for (auto& k : r.kinds) if (told.count(k)) raise(pr, e.id, c.id, "rumor", "event:" + k);
        }
    }
    for (auto& e : w.chronicle) {
        if (e.year <= w.sleep_started || e.id <= 0) continue;
        const Faction* f = find_faction(w, e.subject);
        int era = f ? f->era : 0;
        if (e.kind == "slayers_founded" || e.kind == "slayer_order") see(pr, "order:" + e.kind, "order", w, era);
        if (e.kind == "dragon_wounded")
            for (auto& t : e.tags)
                for (auto& d : p.defences.defences) if (d.id == t) see(pr, "defence:" + t, "defence", w, era);
    }
}

std::vector<std::string> closest_conditions(const World& w, const Params& p) {
    struct Near { double progress; size_t order; std::string id; };
    std::vector<Near> near;
    size_t order = 0;
    for (auto& e : p.endings.endings) {
        if (!e.enabled) continue;
        const EndingState* s = ending_state(w, e.id);
        for (size_t i = 0; i < e.conds.size(); ++i, ++order) {
            const EndingCond& c = e.conds[i];
            CondHold h = s && i < s->conds.size() ? s->conds[i] : CondHold();
            if (cond_held(h, w.year, c)) continue;
            double progress = h.ever_met ? 0.5 + 0.5 * (c.hold_years > 0 ? std::min(1.0, h.best_years / static_cast<double>(c.hold_years)) : 1.0)
                                         : 0.5 * std::min(1.0, h.best_value / threshold(c));
            near.push_back({progress, order, e.id + "." + c.id});
        }
    }
    std::stable_sort(near.begin(), near.end(), [](const Near& a, const Near& b) { return a.progress > b.progress || (a.progress == b.progress && a.order < b.order); });
    std::vector<std::string> out;
    for (auto& n : near) {
        if (out.size() >= 3 || (n.progress <= 0 && !out.empty())) break;
        out.push_back(n.id);
    }
    return out;
}

bool myth_qualifies(const World& w, const Params& p) {
    const EndingDef* e = ending_def(p.endings, "myth");
    if (!e || !e->enabled || !dragon_dead(w)) return false;
    std::vector<bool> met = conditions_met(w, p, *e);
    return !met.empty() && std::all_of(met.begin(), met.end(), [](bool b) { return b; });
}

LegendEntry myth_entry(const World& w, const Params& p) {
    LegendEntry l;
    l.world_seed = w.seed;
    l.hatched = w.genesis.hatched_year;
    l.died = w.year;
    l.reign = w.year - w.genesis.hatched_year;
    l.id = "L" + std::to_string(w.seed) + "_" + std::to_string(w.year);
    for (auto& r : w.regions) if (r.lair) l.name = display_name(w, r.id);
    for (size_t i = w.chronicle.size(); i > 0; --i)
        if (w.chronicle[i - 1].kind == "dragon_slain") {
            const Event& e = w.chronicle[i - 1];
            l.killer = e.subject;
            l.slain_by_order = e.cause.text_key != "defences";
            break;
        }
    std::map<std::string, int> tiers;
    for (auto& f : w.factions) {
        l.dread += dread_eff(f);
        l.awe += awe_eff(f);
        std::string t = memory_tier(f, p.c.mem);
        if (!t.empty()) tiers[t] += 1;
    }
    l.tier = "oral";
    int best = 0;
    for (const char* t : {"oral", "written", "monumental"}) if (tiers[t] >= best && tiers[t] > 0) { best = tiers[t]; l.tier = t; }
    std::vector<Deed> deeds;
    for (auto& a : w.acts) {
        if (a.type == "exit" || a.target.empty()) continue;
        deeds.push_back({a.type, a.target, a.year, a.magnitude * static_cast<double>(a.witnesses.size())});
    }
    std::stable_sort(deeds.begin(), deeds.end(), [](const Deed& a, const Deed& b) { return a.weight > b.weight; });
    if (deeds.size() > static_cast<size_t>(p.endings.myth_deeds)) deeds.resize(static_cast<size_t>(p.endings.myth_deeds));
    l.deeds = deeds;
    return l;
}

void note_death(Profile& pr, const World& w, const Params& p) {
    if (!dragon_dead(w)) return;
    for (auto& id : closest_conditions(w, p)) {
        size_t dot = id.find('.');
        std::string ending = id.substr(0, dot), cond = id.substr(dot + 1);
        const EndingDef* e = ending_def(p.endings, ending);
        for (auto& c : e->conds) if (c.id == cond && has_route(c, "death_report")) raise(pr, ending, cond, "rumor", "death_report");
    }
    if (!myth_qualifies(w, p)) return;
    LegendEntry l = myth_entry(w, p);
    for (auto& x : pr.legends) if (x.id == l.id) return;
    pr.legends.push_back(l);
    if (const EndingDef* e = ending_def(p.endings, "myth"))
        for (auto& c : e->conds) raise(pr, e->id, c.id, "used", "myth");
}

void note_exit(Profile& pr, const World& w, const Params& p) {
    if (w.ended.empty()) return;
    for (auto& e : p.endings.endings) {
        if (e.exit_kind != w.ended) continue;
        for (auto& c : e.conds) raise(pr, e.id, c.id, "used", "exit");
        for (auto& g : pr.glossary) if (g.second.ending == e.id) g.second.confirmed = true;
    }
}

}
