#include "act_kinds.h"
#include "act_gates.h"
#include "arrivals.h"
#include "chron.h"
#include "endings.h"
#include "unrecorded.h"
#include "world_util.h"
#include "wounds.h"

// item 09: acts that existed as API verbs before. The act calls the verb, the verb logs it; the act's source and
// answers are carried onto the logged record.

namespace ages {

namespace {

std::string arg(const Act& a, const char* k) {
    auto it = a.args.find(k);
    return it == a.args.end() ? "" : it->second;
}

bool cast_type(const std::string& t) {
    return t == "bless" || t == "ward" || t == "curse" || t == "incite" || t == "raise_undead";
}

std::string cast(World& w, const Params& p, const Act& a, const std::string& spell) {
    std::string target = a.target_kind == "region" ? "" : a.target;
    CastResult r = cast_spell_effect(w, p, spell, target, a.region, a.against);
    if (!r.cast) return r.reason;
    Act logged = cast_act(w, r);
    logged.source = a.source.empty() ? logged.source : a.source;
    logged.answers = a.answers;
    for (auto& kv : a.args) logged.args[kv.first] = kv.second;
    if (!a.item.empty()) { logged.item = a.item; logged.target_kind = a.target_kind; }
    apply_logged_acts(w, p, {logged});
    return "";
}

std::string swear(World& w, const Params& p, const Act& a) {
    std::string want = arg(a, "answers_offer");
    for (size_t i = 0; i < w.offers.size(); ++i)
        if (w.offers[i].id == want || (want.empty() && w.offers[i].from_faction == a.target))
            return accept_offer_unrecorded(w, p, i) ? "" : "swear.refused";
    return "swear.no_offer";
}

std::string demand(World& w, const Params& p, const Act& a) {
    size_t before = w.acts.size();
    DemandResult r = demand_oath_unrecorded(w, p, a.target, arg(a, "clause"), a.args);
    return w.acts.size() > before ? "" : r.reason;   // a refused demand is still an act: the court hardens
}

std::string pick(World& w, const Params& p, const Act& a) {
    if (a.payload.count("skip") || a.payload.count("reroll")) return "pick.unsupported";
    std::string choice = arg(a, "choice"), reason;
    if (choice.empty()) return "act.malformed|field=payload.choice";
    return pick_talent_unrecorded(w, p, choice, reason) ? "" : reason;
}

std::string receive_act(World& w, const Params& p, const Act& a) {
    std::string who = arg(a, "who"), reason;
    if (who.empty())
        for (auto& x : w.arrivals) if (a.target.empty() || x.court == a.target) { who = x.id; break; }
    return receive_unrecorded(w, p, who, arg(a, "outcome"), reason) ? "" : reason;
}

std::string verb(World& w, const Params& p, const Act& a) {
    std::string reason;
    const std::string& t = a.type;
    if (t == "hunt") return hunt_unrecorded(w, p, a.region.empty() ? a.target : a.region, reason) ? "" : reason;
    if (t == "receive") return receive_act(w, p, a);
    if (t == "exit") return exit_unrecorded(w, p, arg(a, "kind"), reason) ? "" : reason;
    if (t == "commune") return commune_unrecorded(w, p, a.item, arg(a, "answer"), reason) ? "" : reason;
    if (t == "keep") return keep_person_unrecorded(w, p, a.item, "historian", reason) ? "" : reason;
    if (t == "kill") return kill_person_unrecorded(w, p, a.item, reason, nullptr) ? "" : reason;
    if (t == "terraform") { TerraformResult r = terraform_unrecorded(w, p, a.region, arg(a, "kind")); return r.done ? "" : r.reason; }
    if (t == "demand") return demand(w, p, a);
    if (t == "swear") return swear(w, p, a);
    if (t == "pick_talent") return pick(w, p, a);
    if (t == "sleep") {
        if (dragon_dead(w)) return "dragon_slain";
        if (run_ended(w)) return "run_ended";
        sleep_unrecorded(w, p, static_cast<int>(a.payload.at("years")));
        return "";
    }
    if (t == "teach" || t == "corrupt" || cast_type(t)) return cast(w, p, a, t);
    return "act.unknown|type=" + t;
}

}

bool act_routed(const Params& p, const Act& a) {
    if (a.type == "kill") return a.target_kind == "person";
    if (a.type == "keep") return arg(a, "role") == "historian";
    if (a.type == "teach" || a.type == "corrupt") return a.item.empty();   // on a faction: the spell
    const ActRule* r = act_rule(p, a.type);
    return r && !r->via.empty();
}

std::string apply_routed(World& w, const Params& p, const Act& a) {
    size_t before = w.acts.size();
    std::string why = verb(w, p, a);
    if (why.empty() && w.acts.size() > before) {
        for (size_t i = before; i < w.acts.size(); ++i) {
            Act& x = w.acts[i];
            if (x.type != a.type) continue;
            if (!a.source.empty()) x.source = a.source;
            if (!a.answers.empty()) x.answers = a.answers;
            break;
        }
    }
    return why;
}

}
