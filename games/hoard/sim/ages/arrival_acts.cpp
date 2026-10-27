#include "arrivals.h"
#include "ages.h"
#include "hooks.h"
#include "unrecorded.h"
#include "world_util.h"
#include "wounds.h"

// item 06: receive (pilgrims, refugees and the other visitors who wait at the lair) and hunt

namespace ages {

bool receive_outcome(World& w, const Params& p, const Arrival& arrival, const std::string& outcome) {
    Arrival a = arrival;
    const ArrivalDef* d = arrival_def(p, a.kind);
    if (!d) return false;
    auto answer = d->answers.find(outcome);
    if (answer == d->answers.end()) return false;
    Act act;
    act.type = "receive";
    act.target = a.court;
    act.item = outcome;
    act.magnitude = 1.0;
    apply_acts(w, p, {act});
    std::map<std::string, double> reward = answer->second;
    if (reward.erase("settle_slopes")) {
        for (auto& r : w.regions) if (r.lair) r.tags.insert("slopes_settled");
    }
    apply_reward(w, p, a.court, "", reward);
    if (outcome != "eat") if (Faction* f = find_faction(w, a.court)) learn_lair(w, *f, true);
    arrival_consume(w, a.id);
    emit(w, "arrival_answered", a.court, a.kind, 0, {outcome}, because("arrival_answered", {last_event_id(w, "act_receive", a.court, "", "", w.year)}));
    w.chronicle.back().text_key = "ev.arrival_" + outcome;
    return true;
}

bool receive_unrecorded(World& w, const Params& p, const std::string& id, const std::string& outcome, std::string& reason) {
    if (dragon_dead(w)) { reason = "dragon_slain"; return false; }
    const Arrival* a = nullptr;
    for (auto& x : w.arrivals) if (x.id == id) a = &x;
    if (!a) { reason = "no_arrival"; return false; }
    const ArrivalDef* d = arrival_def(p, a->kind);
    if (!d || !d->answers.count(outcome)) { reason = "no_outcome"; return false; }
    for (auto& h : w.hooks) {
        const HookDef* t = hook_def(p, h);
        if (t && t->arrival == a->kind && h.faction == a->court) return answer_hook_unrecorded(w, p, h.id, outcome, reason);
    }
    return receive_outcome(w, p, *a, outcome);
}

// food from the lair slopes or from land nobody holds; no court sees it
bool hunt_unrecorded(World& w, const Params& p, const std::string& region, std::string& reason) {
    if (dragon_dead(w)) { reason = "dragon_slain"; return false; }
    const Region* r = find_region(w, region);
    if (!r) { reason = "no_region"; return false; }
    if (!r->lair && !r->owner.empty()) { reason = "not_wild"; return false; }
    double food = p.arrivals.hunt_food * (r->lair && has_tag(r->tags, "slopes_settled") ? p.arrivals.slopes_hunt_mult : 1.0);
    w.dragons[kPlayer].hoard.food += food;
    Act a;
    a.year = w.year;
    a.type = "hunt";
    a.target = region;
    a.region = region;
    w.acts.push_back(a);
    emit(w, "act_hunt", region, "", food);
    return true;
}

}
