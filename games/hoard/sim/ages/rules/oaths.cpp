#include "rules.h"
#include "../oaths.h"
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
    return because("faded_memory", {}, {{"fear", f.fear}});
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
            if (clause_holds(w, p, o, cl, window_from, by_dragon)) continue;
            Cause cause = breach_cause(w, *f, cl, by_dragon, window_from);
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
