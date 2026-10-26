#include "ages.h"
#include "chronicle.h"
#include "hooks.h"
#include "importance.h"
#include "offers.h"
#include "rules/slayers.h"
#include "rules/world_rules.h"
#include "step.h"
#include "talents.h"
#include "world_util.h"
#include "wounds.h"

namespace ages {

// v0.4: a sleep ends the age, so wrecked defences stand again
static void rebuild_defences(World& w) {
    for (auto& r : w.regions) r.tags.erase("defences_wrecked");
}

static std::map<std::string, double> growth_since(const std::map<std::string, double>& before, const Dragon& d) {
    std::map<std::string, double> out;
    for (auto& kv : d.growth) {
        auto it = before.find(kv.first);
        double gained = kv.second - (it == before.end() ? 0 : it->second);
        if (gained > 0) out[kv.first] = gained;
    }
    return out;
}

static void note_ranks(World& w, const Params& p, const std::map<std::string, int>& before) {
    for (auto& kv : all_ranks(w.dragon, p.talents)) {
        auto it = before.find(kv.first);
        if (kv.second > (it == before.end() ? 0 : it->second)) emit(w, "rank_gained", "dragon", kv.first, kv.second, {}, because("growth"));
    }
}

// 17.4: a sleep of `years` runs decade steps plus one remainder step of the leftover years (a sleep under a decade is a
// single short step). When the growth thresholds for the next phase are met, a sleep at least as long as the phase's
// long window is a moult: digestion doubles and the dragon wakes one phase up, stiff for its first raid.
// v0.4: wounds heal step by step; a slain dragon does not sleep.
SleepResult sleep(World& w, const Params& p, int years) {
    const Constants& c = p.c;
    SleepResult res;
    if (dragon_dead(w)) { res.wake_reason = "dragon_slain"; return res; }
    if (years < 1) years = 1;
    size_t start = w.chronicle.size();
    std::map<std::string, double> growth_before = w.dragon.growth;
    std::map<std::string, int> ranks_before = all_ranks(w.dragon, p.talents);
    double wounds_before = w.dragon.wounds;
    double wounded_share = wounds_before / std::max(1.0, health_max(w, p));
    w.wake_reason.clear();
    w.sleep_started = w.year;
    w.moulting = years >= long_sleep(w.dragon, c) && ready_to_moult(w.dragon, c);
    w.dragon.tags.erase("stiff");
    rebuild_defences(w);
    expire_hooks(w, p);   // v0.4 hooks (S3): what the raid left unanswered
    int slept = 0, remaining = years;
    double atrophy = 0;
    emit(w, "sleep_begin", "dragon", "", years, w.moulting ? Tags{"moult"} : Tags{}, {}, "", false);
    while (remaining > 0) {
        if (has_tag(w.tags, "dragon_slain")) break;
        int dt = remaining >= c.step_years ? c.step_years : remaining;
        step(w, p, dt);
        slept += dt;
        remaining -= dt;
        atrophy += hunger_step(w, p, digest_hoard(w, p, dt));
        heal(w, p, dt);
        if (!w.wake_reason.empty()) break;
    }
    storyteller_wake(w, p, slept, true);   // v0.4 world (S2)
    res.wake_reason = w.wake_reason.empty() ? "planned" : w.wake_reason;
    res.years_slept = slept;
    if (atrophy > 0) emit(w, "dragon_atrophied", "dragon", "", atrophy, {}, because("hunger", {}, {{"hunger", w.dragon.hunger}}));
    if (w.moulting && slept >= long_sleep(w.dragon, c)) {
        w.dragon.phase += 1;
        w.dragon.tags.insert("stiff");
        emit(w, "dragon_moulted", "dragon", "", w.dragon.phase, {},
             because("moult", {}, {{"size", w.dragon.size}, {"knowledge", w.dragon.knowledge}, {"magic", w.dragon.magic}}));
    }
    w.moulting = false;
    w.dragon.magic += effect(w, p, "magic_per_sleep_add");
    note_ranks(w, p, ranks_before);
    adapt_after_sleep(w, p, wounded_share, start);
    emit(w, "wake", "dragon", res.wake_reason, slept, {}, {}, "", false);
    score_events(w, p);
    if (!has_tag(w.tags, "dragon_slain")) generate_offers(w, p);
    if (!has_tag(w.tags, "dragon_slain")) res.hooks = generate_hooks(w, p);   // v0.4 hooks (S3): after everything the sleep brought
    res.chronicle.assign(w.chronicle.begin() + static_cast<std::ptrdiff_t>(start), w.chronicle.end());
    res.growth_gained = growth_since(growth_before, w.dragon);
    res.healed = std::max(0.0, wounds_before - w.dragon.wounds);
    return res;
}

uint64_t chronicle_hash(const World& w) { return hash_events(w.chronicle); }

std::string chronicle_to_json(const std::vector<Event>& events) { return events_to_json(events); }

}
