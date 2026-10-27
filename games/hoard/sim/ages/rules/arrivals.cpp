#include "rules.h"
#include "../arrivals.h"
#include "../memory_tiers.h"
#include "../world_util.h"
#include "../wounds.h"

// item 06: per court per tick, every kind of data/arrivals.json is tried; the most specific kind whose requires all
// hold, none of whose blocked_when holds and that is off cooldown comes to the lair. One visitor per court per
// decade (a kind may waive that), no draw from the world stream.

namespace ages {

namespace {

struct Pick {
    const ArrivalDef* def = nullptr;
    EnvoyDraft draft;
    std::vector<std::string> because;
    std::string cause;
};

std::string key_of(const std::string& court, const std::string& kind, const std::string& cause) { return court + "|" + kind + "|" + cause; }

bool cooling(const World& w, const std::string& key, int years) {
    auto it = w.arrival_fired.find(key);
    return years > 0 && it != w.arrival_fired.end() && w.year - it->second < years;
}

const Person* pick_person(const World& w, const Faction& f, const std::string& role) {
    if (role.empty()) return nullptr;
    const Person* best = nullptr;
    for (auto& x : w.people) {
        if (x.faction != f.id || has_tag(x.tags, "hostage")) continue;
        if (role == "hero" ? x.role == "hero" && (!best || x.traits.bold > best->traits.bold) : x.role == role && !best) best = &x;
    }
    if (!best && role == "ruler") best = find_person_role(w, f.id, "heir");
    return best;
}

bool try_kind(World& w, const Params& p, const Faction& f, const ArrivalDef& d, Pick& out) {
    GateCtx g{w, p, f};
    EnvoyDraft draft;
    std::vector<std::string> clauses;
    for (auto& r : d.requires_) {
        if (r.gate == "motive") {
            for (auto& e : envoy_drafts(w, p, f))
                if (!cooling(w, key_of(f.id, d.kind, e.kind), d.cooldown_years)) { draft = e; break; }
            g.motive = !draft.kind.empty();
        }
        if (!gate_holds(g, r)) return false;
        clauses.push_back(r.gate == "motive" ? "motive." + draft.motive : gate_key(r));
    }
    std::string cause = !draft.kind.empty() ? draft.kind : (d.cause.empty() ? d.kind : d.cause);
    if (cooling(w, key_of(f.id, d.kind, cause), d.cooldown_years)) return false;
    for (auto& b : d.blocked_when) {
        if (!gate_holds(g, b)) continue;
        std::string shrine = key_of(f.id, d.kind, b.event);
        if (!b.event.empty() && !cooling(w, shrine, d.cooldown_years)) {
            w.arrival_fired[shrine] = w.year;
            w.dragons[kPlayer].faith_pool += b.faith_pool;
            emit(w, b.event, f.id, "", b.faith_pool, {}, because("blocked", {}, {{"dread", dread_eff(f)}, {"awe", awe_eff(f)}}));
        }
        return false;
    }
    out.def = &d;
    out.draft = draft;
    out.because = clauses;
    out.cause = cause;
    return true;
}

}

void rule_arrivals(World& w, const Params& p) {
    if (dragon_dead(w) || p.arrivals.kinds.empty() || w.year < w.genesis.hatched_year) return;
    refresh_lair_knowledge(w, p);
    for (auto& f : w.factions) {
        auto last = w.arrival_fired.find(f.id);
        bool limited = last != w.arrival_fired.end() && w.year - last->second < p.c.step_years;
        Pick best;
        for (auto& d : p.arrivals.kinds) {
            if (limited && !d.ignores_decade_limit) continue;
            if (best.def && d.specificity <= best.def->specificity) continue;
            Pick got;
            if (try_kind(w, p, f, d, got)) best = got;
        }
        if (!best.def) continue;
        const ArrivalDef& d = *best.def;
        Arrival a;
        a.kind = d.kind;
        a.court = f.id;
        a.year = w.year;
        a.id = "arrival_" + f.id + "_" + d.kind + "_" + std::to_string(w.year);
        a.because = best.because;
        const Person* who = best.draft.person.empty() ? pick_person(w, f, d.person_role) : find_person(w, best.draft.person);
        a.person = who ? who->id : "";
        if (d.kind == "envoy") {
            a.offer_kind = best.draft.kind;
            a.target = best.draft.target;
            a.motive = best.draft.motive;
            a.region = best.draft.region;
            a.slay_true = best.draft.slay_true;
            Rng rng = rng_for(w.seed, "arrival_lie", a.id);
            roll_envoy_lie(w, p, f, best.draft, rng, a.stated_reason, a.is_lie);
        }
        w.arrival_fired[f.id] = w.year;
        w.arrival_fired[key_of(f.id, d.kind, best.cause)] = w.year;
        w.arrivals.push_back(a);
        emit(w, "arrival", f.id, d.kind, 0, {}, because("arrival"));
        w.chronicle.back().text_key = "ev.arrival_" + d.kind;
    }
}

}
