#include "slayers.h"
#include <cmath>
#include <limits>
#include "rules.h"
#include "../growth.h"
#include "../talents.h"
#include "../world_util.h"
#include "../wounds.h"

namespace ages {

namespace {

const char* const kForming = "slayers_forming";
const char* const kReady = "slayers_ready";

const TimedTag* stage_tag(const World& w, const std::string& faction) {
    for (auto& t : w.timed_tags)
        if (t.on == "faction" && t.target == faction && (t.tag == kForming || t.tag == kReady)) return &t;
    return nullptr;
}

void clear_stage(World& w, Faction& f) {
    f.tags.erase(kForming);
    f.tags.erase(kReady);
    w.timed_tags.erase(std::remove_if(w.timed_tags.begin(), w.timed_tags.end(),
                                      [&](const TimedTag& t) { return t.on == "faction" && t.target == f.id && (t.tag == kForming || t.tag == kReady); }),
                       w.timed_tags.end());
}

void set_stage(World& w, Faction& f, const char* tag, int event) {
    clear_stage(w, f);
    f.tags.insert(tag);
    TimedTag t;
    t.on = "faction";
    t.target = f.id;
    t.tag = tag;
    t.from = w.year;
    t.until = std::numeric_limits<int>::max() / 2;   // held until the band attacks or disperses
    t.event = event;
    w.timed_tags.push_back(t);
}

void forget_warning(World& w, const std::string& tag) {
    w.tags.erase(tag);
    w.timed_tags.erase(std::remove_if(w.timed_tags.begin(), w.timed_tags.end(), [&](const TimedTag& t) { return t.on == "world" && t.tag == tag; }),
                       w.timed_tags.end());
}

const Person* boldest_hero(const World& w, const Faction& f) {
    const Person* lead = nullptr;
    for (auto& x : w.people) if (x.faction == f.id && x.role == "hero" && (!lead || x.traits.bold > lead->traits.bold)) lead = &x;
    return lead;
}

double hoard_value(const Hoard& h) { return std::max(0.0, h.gold + h.books + h.artifacts + h.art); }

// the burn the faction remembers hardest plus the founding of its order; a bold hero leading it raises the odds (17.2)
Cause slay_cause(const World& w, const Faction& f, const Person* lead) {
    int heaviest = 0;
    double top = -1;
    for (auto& m : f.memory)
        if (m.type == "burn" && m.weight * m.magnitude > top) { top = m.weight * m.magnitude; heaviest = last_event_id(w, "act_burn", f.id, "", "", m.year); }
    Cause c = because("slayers", {heaviest, last_event_id(w, "slayers_founded", f.id)}, {{"fear", f.fear}});
    if (lead) { c.params["bold"] = lead->traits.bold; c.text_key = "slayers_led"; }
    return c;
}

// Premonition wakes the dragon instead; the band then waits at the lair (slayers_coming:<court>) and attacks in the
// next sleep without a second warning
bool forewarned(World& w, const Params& p, const Faction& f, const std::string& hero, double odds, const Cause& cause) {
    std::string tag = "slayers_coming:" + f.id;
    if (effect(w, p, "premonition") <= 0 || has_tag(w.tags, tag)) return false;
    w.tags.insert(tag);
    TimedTag t;
    t.on = "world";
    t.target = "world";
    t.tag = tag;
    t.from = w.year;
    t.until = w.year + p.c.premonition_decades * p.c.step_years;
    t.event = emit(w, "slayers_coming", f.id, hero, odds, {}, cause);
    w.timed_tags.push_back(t);
    if (w.wake_reason.empty()) w.wake_reason = "premonition";
    return true;
}

void form(World& w, const Params& p, Faction& f) {
    const Person* lead = boldest_hero(w, f);
    int ev = emit(w, kForming, f.id, lead ? lead->id : "", slayer_strength(w, p, f), {}, slay_cause(w, f, lead));
    set_stage(w, f, kForming, ev);
}

void ready(World& w, const Params& p, Faction& f) {
    const Person* lead = boldest_hero(w, f);
    Cause cause = slay_cause(w, f, lead);
    cause.params["strength"] = slayer_strength(w, p, f);
    add_ref(cause, last_event_id(w, kForming, f.id));
    int ev = emit(w, kReady, f.id, lead ? lead->id : "", slaying_odds(w, p, f), {}, cause);
    set_stage(w, f, kReady, ev);
}

void attempt(World& w, const Params& p, Faction& f) {
    const Constants& c = p.c;
    const Person* lead = boldest_hero(w, f);
    std::string hero = lead ? lead->id : "";
    double odds = slaying_odds(w, p, f);
    Cause cause = slay_cause(w, f, lead);
    add_ref(cause, last_event_id(w, kReady, f.id));
    if (forewarned(w, p, f, hero, odds, cause)) return;
    clear_stage(w, f);
    forget_warning(w, "slayers_coming:" + f.id);
    if (w.rng.chance(odds)) {
        w.tags.insert("dragon_slain");
        w.wake_reason = "dragon_slain";
        emit(w, "dragon_slain", f.id, hero, slayer_strength(w, p, f), {}, cause);
        return;
    }
    f.wealth *= (1.0 - c.slay_fail_wealth_loss);
    remember(f, c, w.year, "slay_failed", 1.0, c.slay_fail_fear, 0, {"slay_failed"});
    w.dragon.tags.insert("rage");
    if (w.wake_reason.empty()) w.wake_reason = "slay_attempt_failed";
    emit(w, "slay_attempt_failed", f.id, hero, odds, {}, cause);
}

}

double adaptation_factor(const World& w, const Params& p) {
    const Constants& c = p.c;
    return std::max(c.adapt_min, std::min(c.adapt_max, c.adapt_base + c.adapt_step * w.dragon.adaptation));
}

double slayer_strength(const World& w, const Params& p, const Faction& f) {
    const Constants& c = p.c;
    double points = c.slay_points_base + c.slay_points_hoard * std::sqrt(hoard_value(w.dragon.hoard)) + c.slay_points_growth * total_growth(w.dragon);
    return points * era_mult(c, f.era) * adaptation_factor(w, p);
}

double slaying_odds(const World& w, const Params& p, const Faction& f) {
    const Constants& c = p.c;
    double odds = clamp01(slayer_strength(w, p, f) / (health_max(w, p) * c.slay_health_div));
    if (const Person* lead = boldest_hero(w, f)) odds = clamp01(odds * (c.hero_slay_bold_base + lead->traits.bold));
    return clamp01(odds * (1.0 + w.dragon.wounds / std::max(1.0, health_max(w, p))) * effect(w, p, "slay_mult"));
}

std::string slayer_stage(const World& w, const std::string& faction) {
    const TimedTag* t = stage_tag(w, faction);
    return t ? t->tag : "";
}

// forming needs the order and fear above slay_fear; the band readies by a per-decade roll at least one waking after it
// formed and attacks in the first step of the sleep after it was ready; it disperses when the order or the fear goes
void slayer_clock(World& w, const Params& p) {
    const Constants& c = p.c;
    double k = dts(w, c);
    for (auto& f : w.factions) {
        bool order = has_tag(f.tags, "slayers");
        bool eligible = order && f.fear > c.slay_fear;
        const TimedTag* t = stage_tag(w, f.id);
        if (!t) { if (eligible) form(w, p, f); continue; }
        if (!order || f.fear < c.slay_lapse_fear) { clear_stage(w, f); continue; }
        if (t->from > w.sleep_started) continue;   // reported this sleep: the next stage waits for a waking
        if (t->tag == kForming) {
            if (eligible && w.rng.chance(c.slay_ready_rate * k)) ready(w, p, f);
            continue;
        }
        attempt(w, p, f);
        if (has_tag(w.tags, "dragon_slain")) return;
    }
}

void disperse_slayers(World& w, Faction& f) {
    clear_stage(w, f);
    forget_warning(w, "slayers_coming:" + f.id);
}

// a near death (a failed slaying, or a sleep begun with more than near_death_wounds of health in wounds) lowers the
// slayers' adaptation; a sleep nobody challenged raises it (faster while it is negative)
void adapt_after_sleep(World& w, const Params& p, double wounds_share_at_start, size_t first_event) {
    const Constants& c = p.c;
    bool failed = false, challenged = false;
    for (size_t i = first_event; i < w.chronicle.size(); ++i) {
        const std::string& k = w.chronicle[i].kind;
        if (k == "slay_attempt_failed") failed = true;
        if (k == "slay_attempt_failed" || k == "slayers_coming") challenged = true;
    }
    double& a = w.dragon.adaptation;
    if (failed || wounds_share_at_start > c.near_death_wounds) a -= c.adapt_loss;
    else if (!challenged) a += a < 0 ? c.adapt_gain_negative : c.adapt_gain;
    double lo = (c.adapt_min - c.adapt_base) / c.adapt_step, hi = (c.adapt_max - c.adapt_base) / c.adapt_step;
    a = std::max(lo, std::min(hi, a));
}

}
