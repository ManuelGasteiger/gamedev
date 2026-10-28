#include "card_ctx.h"
#include "rules.h"
#include "world_rules.h"
#include "../world_util.h"

// Event cards (doc section 10, catalogue rule 14): data/events/*.json, evaluated in id order after shocks. A card
// fires on a faction, a region or the world when its trigger holds and a w.rng roll of chance x dt/10 passes; its
// chronicle event names the events that satisfied the trigger as cause refs, and the tags it writes carry the tail
// to later cards. Tags expire from data; inheritable ones pass to the successors founded this step.

namespace ages {

namespace {

void expire(World& w) {
    std::vector<TimedTag> keep;
    for (auto& t : w.timed_tags) {
        if (t.until > w.year) { keep.push_back(t); continue; }
        if (Tags* tags = tags_of(w, t.target)) tags->erase(t.tag);
    }
    w.timed_tags.swap(keep);
}

// a collapse or a split this step: the successor carries its parent's inheritable tags; tags of factions that are gone
// and have no heir this step are dropped
void inherit(World& w) {
    std::vector<TimedTag> born;
    for (const Event& e : events_this_step(w)) {
        if (e.kind != "successor_founded" || e.tags.empty()) continue;
        const std::string& parent = *e.tags.begin();
        for (auto& t : w.timed_tags)
            if (t.target == parent && t.inherit && t.on == "faction") { TimedTag c = t; c.target = e.subject; born.push_back(c); }
    }
    for (auto& c : born) {
        if (Faction* f = find_faction(w, c.target)) f->tags.insert(c.tag);
        if (!timed_tag(w, c.target, c.tag)) w.timed_tags.push_back(c);
    }
    w.timed_tags.erase(std::remove_if(w.timed_tags.begin(), w.timed_tags.end(),
                                      [&](const TimedTag& t) { return (t.on == "faction" && !find_faction(w, t.target)) || (t.on == "person" && !find_person(w, t.target)); }),
                       w.timed_tags.end());
}

std::vector<std::string> selves(const World& w, const CardDef& card) {
    std::vector<std::string> out;
    if (card.scope == "world") out.push_back("world");
    else if (card.scope == "region") for (auto& r : w.regions) out.push_back(r.id);
    else for (auto& f : w.factions) out.push_back(f.id);
    return out;
}

bool in_era(const World& w, const CardDef& card, const std::string& self) {
    const Faction* f = find_faction(w, self);
    if (!f) if (const Region* r = find_region(w, self)) f = find_faction(w, r->owner);
    if (!f) return card.scope == "world" || (card.era_min <= 0 && card.scope == "region");
    return f->era >= card.era_min && f->era <= card.era_max;
}

const CardBranch* pick_branch(World& w, const Params& p, const CardDef& card, CardCtx& ctx) {
    if (card.branches.empty()) return nullptr;
    for (auto& b : card.branches) {
        if (b.when.empty()) continue;
        CardCtx probe = ctx;
        if (card_holds(w, p, b.when, probe)) { ctx = probe; return &b; }
    }
    std::vector<double> weight;
    double total = 0;
    for (auto& b : card.branches) {
        double wt = b.weight;
        for (auto& bo : b.boost) {
            CardCtx probe = ctx;
            if (card_holds(w, p, bo.when, probe)) wt = (wt + bo.add) * bo.mult;
        }
        weight.push_back(b.when.empty() ? wt : 0);
        total += weight.back();
    }
    if (total <= 0) return nullptr;
    double roll = w.rng.uniform() * total;
    for (size_t i = 0; i < card.branches.size(); ++i) {
        if (!card.branches[i].when.empty() || weight[i] <= 0) continue;
        if (roll < weight[i]) return &card.branches[i];
        roll -= weight[i];
    }
    return nullptr;
}

void fire(World& w, const Params& p, const CardDef& card, CardCtx& ctx) {
    const CardBranch* branch = pick_branch(w, p, card, ctx);
    Tags tags;
    for (auto& t : card.tags) tags.insert(subst(ctx, t));
    if (branch && !branch->tag.empty()) tags.insert(subst(ctx, branch->tag));
    std::sort(ctx.refs.begin(), ctx.refs.end());
    std::map<std::string, double> params;
    if (!ctx.refs.empty()) if (const Event* old = find_event(w, ctx.refs.front())) params["years"] = w.year - old->year;
    std::string object = subst(ctx, card.object);
    std::string kind = card.kind.empty() ? card.id : card.kind;
    ctx.event = emit(w, kind, subst(ctx, card.subject), object, 0, tags, because(card.cause, ctx.refs, params), subst(ctx, card.thread));
    w.chronicle.back().text_key = "ev." + (branch && !branch->text_key.empty() ? branch->text_key : card.text_key);
    for (auto& e : card.effects) apply_card_effect(w, p, e, ctx);
    if (branch) for (auto& e : branch->effects) apply_card_effect(w, p, e, ctx);
    // v0.4 world (S2): subject, object and tags may name what the effects bound ($new, $race)
    Tags after;
    for (auto& t : card.tags) after.insert(subst(ctx, t));
    if (branch && !branch->tag.empty()) after.insert(subst(ctx, branch->tag));
    for (auto& e : w.chronicle) if (e.id == ctx.event) { e.subject = subst(ctx, card.subject); e.object = subst(ctx, card.object); e.tags = after; }
}

}

void rule_cards(World& w, const Params& p) {
    expire(w);
    inherit(w);
    if (p.cards.empty()) return;
    // years since the dragon last acted count from its hatching (year 0 on the fixed start): the valley knew of it from then
    std::map<std::string, int> last_act;
    int any_act = w.genesis.hatched_year;
    for (auto& e : w.chronicle)
        if (e.id > 0 && e.kind.rfind("act_", 0) == 0) { last_act[e.subject] = e.year; any_act = e.year; }
    double k = dts(w, p.c);
    for (auto& card : p.cards) {
        if (w.dragons[kPlayer].phase < card.phase_min || w.dragons[kPlayer].phase > card.phase_max) continue;
        if (card.needs_dragon && (has_tag(w.tags, "egg") || !w.ended.empty())) continue;   // the egg has not hatched, or the run ended (item 04)
        for (auto& self : selves(w, card)) {
            if (self != "world" && !find_faction(w, self) && !find_region(w, self)) continue;
            if (!in_era(w, card, self)) continue;
            std::string key = card.id + "|" + self;
            auto fired = w.card_fired.find(key);
            if (fired != w.card_fired.end() && (card.once || w.year - fired->second < card.cooldown * p.c.step_years)) continue;
            CardCtx ctx;
            ctx.card = &card;
            ctx.last_act = &last_act;
            ctx.any_act = any_act;
            ctx.vars["self"] = self;
            ctx.vars["year"] = std::to_string(w.year);
            if (const Region* r = find_region(w, self)) { ctx.vars["region"] = self; ctx.vars["owner"] = r->owner; }
            if (!card_holds(w, p, card.trigger, ctx)) continue;
            double chance = card.chance * k;
            for (auto& m : card.chance_mult) {
                CardCtx probe = ctx;
                if (!card_holds(w, p, m.when, probe)) continue;
                chance *= m.mult;
                add_refs(ctx, probe.refs);
            }
            if (card.major) chance *= storyteller_major_mult(w, p);   // v0.4 world (S2)
            if (card.keyed) {   // item 04
                Rng r = rng_for(w.seed, card.id, self + "@" + std::to_string(w.year));
                if (!r.chance(chance)) continue;
            } else if (!w.rng.chance(chance)) continue;
            w.card_fired[key] = w.year;
            fire(w, p, card, ctx);
        }
    }
}

// v0.4 world (S2)
bool fire_card_now(World& w, const Params& p, const CardDef& card) {
    if (w.dragons[kPlayer].phase < card.phase_min || w.dragons[kPlayer].phase > card.phase_max || (card.needs_dragon && (has_tag(w.tags, "egg") || !w.ended.empty()))) return false;
    std::map<std::string, int> last_act;
    int any_act = w.genesis.hatched_year;
    for (auto& e : w.chronicle)
        if (e.id > 0 && e.kind.rfind("act_", 0) == 0) { last_act[e.subject] = e.year; any_act = e.year; }
    std::vector<CardCtx> holding;
    for (auto& self : selves(w, card)) {
        if (self != "world" && !find_faction(w, self) && !find_region(w, self)) continue;
        if (!in_era(w, card, self)) continue;
        auto fired = w.card_fired.find(card.id + "|" + self);
        if (fired != w.card_fired.end() && (card.once || w.year - fired->second < card.cooldown * p.c.step_years)) continue;
        CardCtx ctx;
        ctx.card = &card;
        ctx.last_act = &last_act;
        ctx.any_act = any_act;
        ctx.vars["self"] = self;
        ctx.vars["year"] = std::to_string(w.year);
        if (const Region* r = find_region(w, self)) { ctx.vars["region"] = self; ctx.vars["owner"] = r->owner; }
        if (card_holds(w, p, card.trigger, ctx)) holding.push_back(ctx);
    }
    if (holding.empty()) return false;
    CardCtx ctx = holding[static_cast<size_t>(w.rng.range(0, static_cast<int>(holding.size())))];
    ctx.last_act = &last_act;
    w.card_fired[card.id + "|" + ctx.vars["self"]] = w.year;
    fire(w, p, card, ctx);
    return true;
}

}
