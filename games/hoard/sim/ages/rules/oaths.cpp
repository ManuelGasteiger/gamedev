#include "rules.h"
#include "../memory_tiers.h"
#include "../oaths.h"
#include "../talents.h"
#include "../world_util.h"

namespace ages {

// why a clause failed: the dragon's harm or the invasion it slept through, or on their side a faded memory or an
// emptied treasury (plague, lost war)
static Cause breach_cause(const World& w, const Faction& f, const Clause& cl, bool by_dragon, int window_from) {
    if (by_dragon) {
        if (cl.pred == "defend") return because("slept_through_invasion", {last_event_id(w, "region_taken", "", "", f.id, window_from)});
        if (cl.pred == "hostage") return because("line_ended", {last_event_id(w, "act_release", f.id)});
        int harm = 0;
        for (const char* k : {"act_burn", "act_kill", "act_take"}) harm = std::max(harm, last_event_id(w, k, f.id));
        return because("dragon_harm", {harm});
    }
    if (cl.pred == "tribute") {
        int emptied = std::max(last_event_id(w, "plague", f.id), std::max(last_event_id(w, "region_taken", "", "", f.id), last_event_id(w, "act_take", f.id)));
        if (f.faith <= 0 || (emptied && find_event(w, emptied)->year > w.year - 100)) return because(f.faith > 0 ? "treasury_empty" : "faded_memory", {emptied}, {{"wealth", f.wealth}, {"faith", f.faith}});
        return because("faded_memory", {}, {{"faith", f.faith}});
    }
    if (f.era >= 2) return because("era_outgrew", {last_event_id(w, "era_advance", f.id)}, {{"era", static_cast<double>(f.era)}});
    return because("faded_memory", {}, {{"dread", dread_eff(f)}});
}

// an oath sworn in dread and not in awe chafes (act-api 6.4): it breaks by chance, oath_breach_rate per decade times
// 1 - stability, where stability = 1 - 0.8 D (1 - A) and an awed court (A >= cult_awe) holds it half again as well;
// a keyed draw, so the world stream does not move. Dragon-king (oath_steadfast) is never resented
static bool breaks_unkept(const World& w, const Params& p, const Oath& o, const Faction& f) {
    if (effect(w, p, "oath_steadfast") > 0) return false;
    double unstable = 1.0 - oath_stability(f, p.c.mem);
    if (awe_eff(f) >= p.c.mem.cult_awe) unstable /= 1.5;
    double chance = p.c.oath_breach_rate * unstable * dts(w, p.c);
    if (chance <= 0) return false;
    Rng r = rng_for(w.seed, "oath_breach", o.id + ":" + std::to_string(w.year));
    return r.chance(chance);
}

void rule_oaths(World& w, const Params& p) {
    int window_from = w.year - w.dt;
    for (auto& o : w.oaths) {
        if (o.status != "active") continue;
        std::string mortal = o.parties[0] == "dragon" ? o.parties[1] : o.parties[0];
        int sworn = last_event_id(w, "oath_sworn", o.id);
        std::string thread = "oath:" + o.id;
        const Faction* f = find_faction(w, mortal);
        if (!f) {
            o.status = "expired";
            emit(w, "oath_expired", o.id, mortal, 0, {"party_gone"}, because("party_gone", {sworn, std::max(last_event_id(w, "collapse", mortal), last_event_id(w, "faction_destroyed", mortal))}), thread);
            continue;
        }
        if (o.expires > 0 && w.year >= o.expires) { o.status = "expired"; emit(w, "oath_expired", o.id, mortal, 0, {}, because("term_ended", {sworn}), thread); continue; }
        for (auto& cl : o.clauses) {
            bool by_dragon = false;
            bool resented = false;
            if (clause_holds(w, p, o, cl, window_from, by_dragon)) {
                resented = (cl.pred == "no_walls" || cl.pred == "no_settle") && breaks_unkept(w, p, o, *f);
                if (!resented) continue;
            }
            Cause cause = resented ? because("resentment", {}, {{"dread", dread_eff(*f)}, {"awe", awe_eff(*f)}}) : breach_cause(w, *f, cl, by_dragon, window_from);
            add_ref(cause, sworn);
            if (by_dragon) {
                o.status = "breached_by_dragon";
                emit(w, "oath_breached_by_dragon", o.id, mortal, 0, {cl.pred}, cause, thread);
                apply_oath_effect(w, p, o.breach_by_dragon, mortal);
            } else {
                o.status = "breached_by_them";
                emit(w, "oath_breached_by_them", o.id, mortal, 0, {cl.pred}, cause, thread);
                apply_oath_effect(w, p, o.breach_by_them, mortal);
            }
            break;
        }
    }
}

}
