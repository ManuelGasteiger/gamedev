#include <cstdlib>
#include "ages.h"
#include "defences.h"
#include <algorithm>
#include "endings.h"
#include "hooks.h"
#include "talents.h"
#include "world_util.h"

namespace ages {

static std::string largest_region(const World& w, const Faction& f) {
    std::string best;
    double best_land = -1;
    for (auto& rid : f.regions) {
        const Region* r = find_region(w, rid);
        if (r && r->land > best_land) { best_land = r->land; best = rid; }
    }
    return best;
}

bool inside_ring(const World& w, const Params& p, const Act& a) {
    const Region* r = act_region(w, a);
    if (!r) return false;
    for (auto& v : region_defences(w, p, *r))
        if (v.def && v.def->id != "levies" && (v.per_act > 0 || v.passive > 0)) return true;
    return false;
}

static int last_sleep_begin(const World& w) {
    for (size_t i = w.chronicle.size(); i > 0; --i)
        if (w.chronicle[i - 1].kind == "sleep_begin") return w.chronicle[i - 1].year;
    return 0;
}

static std::vector<std::string> attackers_of(const World& w, const Params& p, const Faction& f) {
    std::vector<std::string> attackers;
    if (is_vassal(f)) attackers.push_back(overlord_of(f));
    for (auto& war : w.wars) {
        if (war.b != f.id) continue;
        attackers.push_back(war.a);
        for (auto& ally : war.allies_a) attackers.push_back(ally);
    }
    int since = last_sleep_begin(w);
    for (auto& e : w.chronicle)
        if (e.year >= since && e.kind == "region_taken" && e.tags.count(f.id) && !contains(attackers, e.subject))
            attackers.push_back(e.subject);
    if (!attackers.empty()) { std::sort(attackers.begin(), attackers.end()); return attackers; }
    std::string rival;
    double best = -1;
    for (auto& g : w.factions) {
        if (g.id == f.id || !adjacent(w, f, g) || at_war(f, g.id)) continue;
        double pw = power(g, p.c);
        if (pw > best) { best = pw; rival = g.id; }
    }
    if (!rival.empty()) attackers.push_back(rival);
    return attackers;
}

static const Faction* resolve_lineage(const World& w, const std::string& id) {
    if (const Faction* f = find_faction(w, id)) return f;
    const Faction* best = nullptr;
    for (auto& f : w.factions) {
        if (!has_tag(f.tags, "successor_of:" + id)) continue;
        if (!best || f.regions.size() > best->regions.size() || (f.regions.size() == best->regions.size() && f.wealth > best->wealth)) best = &f;
    }
    return best;
}

static std::vector<std::string> everyone(const World& w) {
    std::vector<std::string> ids;
    for (auto& f : w.factions) ids.push_back(f.id);
    return ids;
}

// keyword targets for scripted "player-like" policies; ties broken by id order (factions are sorted by id)
static std::vector<std::string> keyword_targets(const World& w, const Params& p, const std::string& key) {
    std::vector<std::string> ids;
    if (key == "each") return everyone(w);
    if (key == "has_slayers") {
        for (auto& f : w.factions) if (has_tag(f.tags, "slayers")) ids.push_back(f.id);
        return ids;
    }
    if (key == "weakest_human" || key == "richest") {
        const Faction* best = nullptr;
        for (auto& f : w.factions) {
            if (key == "weakest_human") {
                if (f.race != "human") continue;
                if (!best || power(f, p.c) < power(*best, p.c)) best = &f;
            } else if (!best || f.wealth > best->wealth) best = &f;
        }
        if (best) ids.push_back(best->id);
        return ids;
    }
    return {key};
}

static void expand(const World& w, const Params& p, const PolicyAct& t, std::vector<Act>& out) {
    std::vector<std::string> targets = keyword_targets(w, p, t.target);
    for (auto& id : targets) {
        const Faction* f = resolve_lineage(w, id);
        if (!f) continue;
        Act a;
        a.type = t.type;
        a.target = f->id;
        a.magnitude = t.magnitude;
        a.region = t.region == "largest" ? largest_region(w, *f) : t.region;
        a.payload = t.payload_share;
        a.witnesses = everyone(w);
        if (t.against != "any_attacker") { a.against = t.against; out.push_back(a); continue; }
        for (auto& att : attackers_of(w, p, *f)) { a.against = att; out.push_back(a); }
    }
}

std::vector<Act> resolve_policy(const World& w, const Params& p, const Policy& policy, int sleep_index,
                                const std::string& data_dir) {
    std::vector<Act> acts;
    if (!policy.cycle.empty()) {
        Policy sub;
        std::string err;
        const std::string& name = policy.cycle[static_cast<size_t>(sleep_index) % policy.cycle.size()];
        if (!load_policy(data_dir, name, sub, err) || !sub.cycle.empty()) return acts;
        return resolve_policy(w, p, sub, sleep_index, data_dir);
    }
    if (policy.act_wakings > 0 && sleep_index >= policy.act_wakings) return acts;
    for (auto& t : policy.acts) expand(w, p, t, acts);
    if (policy.avoid_rings)
        acts.erase(std::remove_if(acts.begin(), acts.end(), [&](const Act& a) { return inside_ring(w, p, a); }), acts.end());
    return acts;
}

}

namespace ages {

// v0.4 (S1): a policy with a talent list takes every open node, choosing the option listed first (else the first one)
static void pick_talents(World& w, const Params& p, const Policy& policy) {
    if (policy.talents.empty()) return;
    auto rank = [&](const std::string& id) { return std::find(policy.talents.begin(), policy.talents.end(), id) - policy.talents.begin(); };
    for (bool picked = true; picked;) {
        picked = false;
        for (auto& pk : pending_picks(w, p)) {
            if (!pk.open) continue;
            std::string choice = pk.options.front(), reason;
            for (auto& id : pk.options) if (rank(id) < rank(choice)) choice = id;
            if ((picked = pick_talent(w, p, choice, reason))) break;
        }
    }
}

// v0.4 (S1): sleep_cycle entries are "short" | "long" (the phase's windows) or years, cycled; min_sleep is the floor
static int policy_sleep(const World& w, const Params& p, const Policy& policy, int index, int fallback) {
    if (policy.sleep_cycle.empty() && policy.sleep_open.empty()) return fallback;
    size_t open = policy.sleep_open.size();
    const std::string& s = static_cast<size_t>(index) < open ? policy.sleep_open[static_cast<size_t>(index)]
                         : policy.sleep_cycle.empty() ? policy.sleep_open.back()
                         : policy.sleep_cycle[(static_cast<size_t>(index) - open) % policy.sleep_cycle.size()];
    int years = s == "short" ? short_sleep(w.dragons[kPlayer], p.c) : s == "long" ? long_sleep(w.dragons[kPlayer], p.c) : std::atoi(s.c_str());
    return std::max(1, std::max(years, policy.min_sleep));
}

void run_scenario(World& w, const Params& p, const Scenario& s) {
    int sleep_index = 0;
    int remaining = s.years;
    while (remaining > 0 && !has_tag(w.tags, "dragon_slain") && !run_ended(w)) {
        if (sleep_index == 0 && !s.first_acts.empty()) apply_acts(w, p, s.first_acts);
        if (s.policy) {
            pick_talents(w, p, *s.policy);
            std::vector<Act> acts = resolve_policy(w, p, *s.policy, sleep_index, s.data_dir);
            if (s.at_waking) s.at_waking(w, acts, sleep_index);
            apply_acts(w, p, acts);
            answer_hooks(w, p, *s.policy);   // v0.4 hooks (S3)
        }
        int years = s.policy ? policy_sleep(w, p, *s.policy, sleep_index, s.sleep) : s.sleep;
        if (s.adjust_sleep) years = s.adjust_sleep(years, sleep_index);
        if (s.before_sleep) s.before_sleep(w, sleep_index);
        int span = std::min(remaining, years);
        SleepResult r = sleep(w, p, span);
        if (s.after_sleep) s.after_sleep(w);
        remaining -= std::max(r.years_slept, std::min(span, p.c.step_years));
        sleep_index += 1;
    }
}

}
