#include "card_ctx.h"
#include "rules.h"
#include "../endings.h"
#include "../world_util.h"
#include "../prophecy.h"
#include "../wounds.h"

// item 04: after the cards, every enabled ending's conditions are evaluated; each holds a timer, and an ending whose
// conditions all held for their hold_years is ready (world tag ending_ready:<id>, events ending_ready and ending_lapsed)

namespace ages {

void advance_hold(CondHold& h, bool met, int year, const EndingCond& c) {
    h.met = met;
    if (met) {
        if (h.held_since < 0) h.held_since = year;
        h.last_met = year;
        h.ever_met = true;
        h.best_years = std::max(h.best_years, year - h.held_since);
    } else if (h.held_since >= 0 && year - h.last_met > c.lapse_years) {
        h.held_since = -1;
    }
}

int years_held(const CondHold& h, int year) { return h.held_since < 0 ? 0 : year - h.held_since; }

bool cond_held(const CondHold& h, int year, const EndingCond& c) { return h.met && h.held_since >= 0 && year - h.held_since >= c.hold_years; }

std::vector<bool> conditions_met(const World& w, const Params& p, const EndingDef& e, std::string* court) {
    std::vector<bool> out;
    std::vector<CardPred> chain;
    for (auto& c : e.conds) {
        if (!c.with_previous) chain.clear();
        chain.insert(chain.end(), c.when.begin(), c.when.end());
        CardCtx ctx;
        ctx.vars["self"] = "world";
        ctx.vars["year"] = std::to_string(w.year);
        bool met = card_holds(w, p, chain, ctx);
        out.push_back(met);
        if (met && court && ctx.vars.count("court")) *court = ctx.vars["court"];
    }
    return out;
}

const EndingState* ending_state(const World& w, const std::string& id) {
    for (auto& s : w.endings) if (s.id == id) return &s;
    return nullptr;
}

bool ending_ready(const World& w, const std::string& id) {
    const EndingState* s = ending_state(w, id);
    return s && s->ready;
}

bool run_ended(const World& w) { return !w.ended.empty(); }

namespace {

double best_count(const World& w, const Params& p, const EndingCond& c) {
    CardCtx ctx;
    ctx.vars["self"] = "world";
    for (auto& q : c.when) if (q.type == "count") return card_count(w, p, q, ctx);
    return 0;
}

}

void rule_endings(World& w, const Params& p) {
    refresh_prophecy(w, p);
    if (run_ended(w) || dragon_dead(w) || has_tag(w.tags, "egg")) return;
    for (auto& e : p.endings.endings) {
        if (!e.enabled) continue;
        EndingState* s = nullptr;
        for (auto& x : w.endings) if (x.id == e.id) s = &x;
        if (!s) {
            w.endings.push_back(EndingState());
            s = &w.endings.back();
            s->id = e.id;
        }
        s->conds.resize(e.conds.size());
        std::string court;
        std::vector<bool> met = conditions_met(w, p, e, &court);
        bool ready = !e.conds.empty();
        for (size_t i = 0; i < e.conds.size(); ++i) {
            CondHold& h = s->conds[i];
            advance_hold(h, met[i], w.year, e.conds[i]);
            h.best_value = std::max(h.best_value, best_count(w, p, e.conds[i]));
            ready = ready && cond_held(h, w.year, e.conds[i]);
        }
        if (ready) s->court = court;
        if (ready == s->ready) continue;
        s->ready = ready;
        std::string tag = "ending_ready:" + e.id;
        if (ready) {
            w.tags.insert(tag);
            emit(w, "ending_ready", e.id, court, 0, {e.id}, because("ending_ready"));
        } else {
            w.tags.erase(tag);
            emit(w, "ending_lapsed", e.id, s->court, 0, {e.id}, because("ending_lapsed"));
            s->court.clear();
        }
    }
}

}
