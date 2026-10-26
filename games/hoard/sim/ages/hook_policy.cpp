#include "ages.h"
#include "dragon_acts.h"
#include "hooks.h"
#include "world_util.h"
#include "wounds.h"

// v0.4 hooks (S3): a scripted policy answers the open hooks whose first allowed act it can perform: a synthesised act
// of hook_magnitude at the hook's region (a take carries the policy's payload), a kill_person, or the policy's choice;
// an answer is taken only while the wounds after it (the defences' strike and the fight) stay under hook_wound_cap of health

namespace ages {

namespace {

std::vector<std::string> everyone(const World& w) {
    std::vector<std::string> ids;
    for (auto& f : w.factions) ids.push_back(f.id);
    return ids;
}

std::string first_allowed(const Hook& h, const Policy& policy) {
    for (auto& want : h.acts) if (contains(policy.hook_acts, want)) return want;
    if (!h.choices.empty() && contains(policy.hook_acts, "answer") && contains(h.choices, policy.hook_choice)) return "answer";
    return "";
}

// regions between the lair and the hook's region (the lair itself is 0)
int hops_from_lair(const World& w, const std::string& region) {
    std::vector<std::string> frontier = {lair_region(w)}, seen = frontier;
    for (int d = 0; !frontier.empty(); ++d) {
        if (contains(frontier, region)) return d;
        std::vector<std::string> next;
        for (auto& id : frontier)
            if (const Region* r = find_region(w, id))
                for (auto& n : r->neighbours) if (!contains(seen, n)) { seen.push_back(n); next.push_back(n); }
        frontier = next;
    }
    return 1000;
}

// the act that answers the hook: of hook_magnitude at its region (a take carries the policy's payload, a kill is the
// army), a defence of the other side, or the kill of the named person at the court's seat
Act answer_act(const World& w, const Policy& policy, const Hook& h, const std::string& verb) {
    Act a;
    bool defend = verb == "defend";
    a.type = verb == "kill_person" ? "kill" : verb;
    a.target = defend ? h.other : h.faction;
    a.against = defend ? h.faction : "";
    a.region = defend || verb == "kill_person" ? "" : h.region;
    a.item = verb == "kill_person" ? h.person : "";
    a.magnitude = defend ? 1.0 : policy.hook_magnitude;
    if (verb == "take") a.payload = policy.hook_payload;
    if (verb == "kill") a.payload["soldiers"] = 1;
    a.witnesses = everyone(w);
    return a;
}

// within reach, and the wounds after the defences' strike and the fight stay under hook_wound_cap of health (wounds
// carried into the sleep make a slaying easier)
bool affordable(const World& w, const Params& p, const Policy& policy, const Hook& h, const Act& a) {
    if (policy.hook_reach > 0 && hops_from_lair(w, h.region) > policy.hook_reach) return false;
    double damage = strike_damage(w, p, a) + hook_fight_damage(w, p, h);
    return damage <= 0 || w.dragons[kPlayer].wounds + damage <= policy.hook_wound_cap * health_max(w, p);
}

}

void answer_hooks(World& w, const Params& p, const Policy& policy) {
    if (policy.hook_acts.empty()) return;
    std::vector<Hook> open = w.hooks;   // the most pressing first (template priority)
    auto priority = [&](const Hook& h) { const HookDef* t = hook_def(p, h); return t ? t->priority : 0.0; };
    std::stable_sort(open.begin(), open.end(), [&](const Hook& a, const Hook& b) { return priority(a) > priority(b); });
    int answered = 0;
    for (auto& h : open) {
        if (dragon_dead(w) || (policy.hook_max > 0 && answered >= policy.hook_max)) break;
        std::string verb = first_allowed(h, policy), reason;
        if (verb.empty()) continue;
        Act a = answer_act(w, policy, h, verb);
        if (verb != "answer" && (a.target.empty() || !affordable(w, p, policy, h, a))) continue;
        if (verb == "answer" && !affordable(w, p, policy, h, Act())) continue;
        answered += 1;
        if (verb == "answer") answer_hook(w, p, h.id, policy.hook_choice, reason);
        else if (verb == "kill_person") kill_person(w, p, h.person, reason);
        else apply_acts(w, p, {a});
    }
}

}
