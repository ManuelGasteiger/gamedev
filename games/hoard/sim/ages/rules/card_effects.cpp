#include "card_ctx.h"
#include "rules.h"
#include "../names.h"
#include "../world_util.h"

// card effects ("do" in data/events): tag, untag, field, modifier, memory, war, spawn_person, crown, split,
// spawn_faction, refugees, wealth_transfer, ledger (the dragon's relationship to a person), offer. "on" names a variable, or all | neighbours | allies (of "of",
// default self, optionally only of "race"). Spawned entities bind the variable named by "bind".

namespace ages {

namespace {

std::string str(const CardEffect& e, const char* k, const std::string& fallback = "") {
    auto it = e.s.find(k);
    return it == e.s.end() ? fallback : it->second;
}
double num(const CardEffect& e, const char* k, double fallback = 0) {
    auto it = e.n.find(k);
    return it == e.n.end() ? fallback : it->second;
}
bool has(const CardEffect& e, const char* k) { return e.n.count(k) > 0; }

std::vector<std::string> targets(const World& w, const Params& p, const CardEffect& e, const CardCtx& ctx) {
    std::string on = str(e, "on", "self"), race = str(e, "race");
    if (on != "all" && on != "neighbours" && on != "allies") return {var_of(ctx, on)};
    const Faction* of = find_faction(w, var_of(ctx, str(e, "of", "self")));
    std::vector<std::string> out;
    for (auto& f : w.factions) {
        if (!race.empty() && dominant_race(f) != race) continue;
        if (on != "all") {
            if (!of || f.id == of->id || !adjacent(w, *of, f)) continue;
            if (on == "allies" && !(allied(*of, f) || affinity(p, *of, f) > p.c.affinity_alliance)) continue;
        }
        out.push_back(f.id);
    }
    return out;
}

Cause card_cause(const CardCtx& ctx, const std::string& key = "") {
    return because(key.empty() ? ctx.card->cause : key, {ctx.event});
}

void faction_field(World& w, const Params& p, Faction& f, const CardEffect& e, const std::string& source) {
    const Constants& c = p.c;
    std::string k = str(e, "field");
    auto shift = [&](double& v, bool unit) {
        if (has(e, "set")) v = num(e, "set");
        if (has(e, "mul")) v *= num(e, "mul");
        if (has(e, "add")) v += num(e, "add");
        if (has(e, "min")) v = std::max(v, num(e, "min"));
        if (has(e, "max")) v = std::min(v, num(e, "max"));
        if (unit) v = clamp01(v);
        else if (v < 0) v = 0;
    };
    Traits& t = f.culture.traits;
    if (k == "wealth") shift(f.wealth, false);
    else if (k == "cohesion") shift(f.cohesion, true);
    else if (k == "anti_dragon") shift(f.anti_dragon, false);
    else if (k == "era_progress") shift(f.era_progress, false);
    else if (k == "authority") shift(t.authority, true);
    else if (k == "openness") shift(t.openness, true);
    else if (k == "militarism") shift(t.militarism, true);
    else if (k == "piety") shift(t.piety, true);
    else if (k == "faith" || k == "fear") {
        // fear and faith are memories: a set or a floor becomes the entry that moves them there
        double now = k == "faith" ? f.faith : f.fear, want = now;
        shift(want, true);
        Tags tags(e.tags.begin(), e.tags.end());
        if (k == "faith" && (has(e, "set") || has(e, "mul"))) set_faith(f, c, w.year, source, 1.0, want, 0, tags);
        else if (want != now) remember(f, c, w.year, source, 1.0, k == "fear" ? want - now : 0, k == "faith" ? want - now : 0, tags);
    }
}

void field(World& w, const Params& p, const CardEffect& e, const std::string& id, const std::string& source) {
    if (Faction* f = find_faction(w, id)) { faction_field(w, p, *f, e, source); return; }
    std::string k = str(e, "field");
    double* v = nullptr;
    if (Region* r = find_region(w, id)) { if (k == "burn") v = &r->burn; }
    else if (id == "dragon") { if (k == "gold") v = &w.dragons[kPlayer].hoard.gold; else if (k == "hunger") v = &w.dragons[kPlayer].hunger; }
    else if (id == "world" && k == "magic") v = &w.magic;   // v0.4 world (S2)
    if (!v) return;
    if (has(e, "set")) *v = num(e, "set");
    if (has(e, "mul")) *v *= num(e, "mul");
    if (has(e, "add")) *v += num(e, "add");
    if (k != "gold") *v = clamp01(*v);
    else if (*v < 0) *v = 0;
}

void war(World& w, const CardEffect& e, CardCtx& ctx, const std::string& id) {
    Faction* a = find_faction(w, id);
    Faction* b = find_faction(w, var_of(ctx, str(e, "against", "other")));
    if (!a || !b || a->id == b->id || at_war(*a, b->id) || is_vassal(*a) || overlord_of(*b) == a->id) return;
    start_war(w, *a, *b, str(e, "reason", ctx.card->id), card_cause(ctx));
}

void spawn_person(World& w, const Params& p, const CardEffect& e, CardCtx& ctx, const std::string& id) {
    const Faction* f = find_faction(w, id);
    if (!f) return;
    std::string role = str(e, "role", "hero");
    Person x = new_adult(w, p, dominant_race(*f), f->id, role);
    for (auto& t : e.tags) x.tags.insert(subst(ctx, t));
    add_person(w, x);
    Tags tags = {role};
    for (auto& t : e.tags) tags.insert(subst(ctx, t));
    emit(w, "person_rose", x.id, f->id, x.traits.bold, tags, card_cause(ctx));
    ctx.vars[str(e, "bind", "person")] = x.id;
}

void crown(World& w, const Params& p, const CardEffect& e, CardCtx& ctx, const std::string& id) {
    Faction* f = find_faction(w, id);
    std::string who = var_of(ctx, str(e, "person", "person"));
    if (f && find_person(w, who)) crown_person(w, p, *f, who, str(e, "how", ctx.card->id), card_cause(ctx));
}

// puts the new faction into the world on `region` and binds it; a split leaves the parent without invading it, a
// return takes the region from its holder by force
std::string add_faction(World& w, CardCtx& ctx, const CardEffect& e, Faction s, const std::string& region, const std::string& holder, bool invasion) {
    s.regions.clear();
    for (auto& t : e.tags) s.tags.insert(subst(ctx, t));
    std::string id = s.id;
    w.factions.push_back(s);
    sort_by_id(w);
    if (invasion) transfer_region(w, region, holder, id);
    else {
        if (Faction* f = find_faction(w, holder)) { erase_value(f->regions, region); f->last_region_lost = w.year; }
        transfer_region(w, region, "", id);
    }
    ctx.vars[str(e, "bind", "new")] = id;
    return id;
}

// a successor on one of the faction's outer regions (the last in id order that is not its best land), at war with it
void split(World& w, const Params& p, const CardEffect& e, CardCtx& ctx, const std::string& id) {
    Faction* f = find_faction(w, id);
    if (!f || f->regions.size() < 2) return;
    std::string best, pick;
    double top = -1;
    for (auto& rid : f->regions) if (const Region* r = find_region(w, rid)) if (r->land > top) { top = r->land; best = rid; }
    for (auto& rid : f->regions) if (rid != best) pick = rid;
    Faction parent = *f;
    double share = parent.wealth / static_cast<double>(parent.regions.size());
    f->wealth -= share;
    Faction s = found_faction(w, p, parent.race, pick, share, p.c.successor_cohesion, parent.era);
    s.culture = parent.culture;
    s.pop = parent.pop;
    s.fear = parent.fear;
    s.faith = parent.faith;
    s.memory = parent.memory;
    s.anti_dragon = parent.anti_dragon;
    s.tags.insert("successor_of:" + parent.id);
    bool fight = num(e, "war", 1) > 0;
    if (fight) s.tags.insert("civil_war");
    std::string thread = "civil_war:" + parent.id + ":" + std::to_string(w.year);
    std::string sid = add_faction(w, ctx, e, s, pick, parent.id, false);
    emit(w, "successor_founded", sid, pick, 0, {parent.id}, card_cause(ctx), thread);
    if (!fight) return;
    Faction* fs = find_faction(w, sid);
    Faction* fp = find_faction(w, parent.id);
    fp->tags.insert("civil_war");
    War& x = start_war(w, *fs, *fp, "civil_war", card_cause(ctx));
    x.thread = thread;
    w.chronicle.back().thread = thread;
}

// a people that once lived here takes the region back from its holder and, with war_with, fights it
void spawn_faction(World& w, const Params& p, const CardEffect& e, CardCtx& ctx) {
    std::string race = subst(ctx, str(e, "race"));
    std::string region = var_of(ctx, str(e, "region", "region"));
    const Region* r = find_region(w, region);
    if (!r || r->lair || !p.races.count(race) || has_tag(w.tags, "extinct:" + race)) return;
    std::string holder = r->owner;
    const Faction* h = find_faction(w, holder);
    int era = h ? h->era : static_cast<int>(num(e, "era"));
    Faction s = found_faction(w, p, race, region, num(e, "wealth", p.c.remnant_refound_wealth), num(e, "cohesion", p.c.remnant_refound_cohesion), era);
    std::string sid = add_faction(w, ctx, e, s, region, holder, true);
    w.remnants.erase(std::remove_if(w.remnants.begin(), w.remnants.end(), [&](const Remnant& x) { return x.race == race; }), w.remnants.end());
    emit(w, "race_returned", sid, region, 0, {race, "exiles"}, card_cause(ctx));
    Faction* fs = find_faction(w, sid);
    Faction* fh = find_faction(w, holder);
    if (fs && fh && !str(e, "war_with").empty()) start_war(w, *fs, *fh, "restoration_war", card_cause(ctx));
}

// part of the people flees to the neighbour that likes them best; the host is bound as $host
void refugees_to(World& w, const Params& p, const CardEffect& e, CardCtx& ctx, const std::string& id) {
    const Faction* from = find_faction(w, id);
    if (!from) return;
    Faction* host = nullptr;
    double best = 0;
    for (auto& g : w.factions) {
        if (g.id == from->id || !adjacent(w, *from, g)) continue;
        double aff = affinity(p, *from, g);
        if (aff > 0 && (!host || aff > best)) { host = &g; best = aff; }
    }
    if (!host) return;
    double total = from->wealth + host->wealth;
    double share = num(e, "share", 0.4);
    shift_pop(*host, p, from->pop, total > 0 ? share * from->wealth / total : share * 0.5);
    ctx.vars[str(e, "bind", "host")] = host->id;
}

void wealth_transfer(World& w, const CardEffect& e, CardCtx& ctx) {
    std::string from = var_of(ctx, str(e, "from", "self")), to = var_of(ctx, str(e, "to", "other"));
    double* src = from == "dragon" ? &w.dragons[kPlayer].hoard.gold : nullptr;
    double* dst = to == "dragon" ? &w.dragons[kPlayer].hoard.gold : nullptr;
    if (Faction* f = find_faction(w, from)) src = &f->wealth;
    if (Faction* f = find_faction(w, to)) dst = &f->wealth;
    if (!src) return;
    double amount = *src * num(e, "share");
    *src -= amount;
    if (dst) *dst += amount * num(e, "keep", 1);
}

}

void write_tag(World& w, const std::string& target, const std::string& tag, int decades, bool inherit, int event) {
    Tags* tags = tags_of(w, target);
    if (!tags || tag.empty()) return;
    tags->insert(tag);
    int until = decades > 0 ? w.year + decades * 10 : 1000000000;
    for (auto& t : w.timed_tags)
        if (t.target == target && t.tag == tag) { t.until = std::max(t.until, until); t.event = event; t.inherit = t.inherit || inherit; return; }
    TimedTag t;
    t.on = find_faction(w, target) ? "faction" : (find_region(w, target) ? "region" : (find_person(w, target) ? "person" : target));
    t.target = target;
    t.tag = tag;
    t.from = w.year;
    t.until = until;
    t.event = event;
    t.inherit = inherit;
    w.timed_tags.push_back(t);
}

void remove_tags(World& w, const std::string& target, const std::string& pattern) {
    bool wild = pattern.size() > 1 && pattern.back() == '*';
    std::string prefix = wild ? pattern.substr(0, pattern.size() - 1) : pattern;
    auto hit = [&](const std::string& t) { return wild ? t.rfind(prefix, 0) == 0 : t == pattern; };
    if (Tags* tags = tags_of(w, target))
        for (auto it = tags->begin(); it != tags->end();) it = hit(*it) ? tags->erase(it) : std::next(it);
    w.timed_tags.erase(std::remove_if(w.timed_tags.begin(), w.timed_tags.end(), [&](const TimedTag& t) { return t.target == target && hit(t.tag); }),
                       w.timed_tags.end());
}

void apply_card_effect(World& w, const Params& p, const CardEffect& e, CardCtx& ctx) {
    if (!e.when.empty()) {
        CardCtx probe = ctx;
        if (!card_holds(w, p, e.when, probe)) return;
    }
    std::string source = "card." + ctx.card->id;
    const std::string& t = e.type;
    if (t == "spawn_faction") { spawn_faction(w, p, e, ctx); return; }
    if (t == "wealth_transfer") { wealth_transfer(w, e, ctx); return; }
    if (world_effect(w, p, e, ctx)) return;   // v0.4 world (S2)
    for (auto& id : targets(w, p, e, ctx)) {
        if (id.empty()) continue;
        if (t == "tag") write_tag(w, id, subst(ctx, str(e, "tag")), static_cast<int>(num(e, "decades")), num(e, "inherit") > 0, ctx.event);
        else if (t == "untag") remove_tags(w, id, subst(ctx, str(e, "tag")));
        else if (t == "field") field(w, p, e, id, source);
        else if (t == "modifier") add_modifier(w, source, id, str(e, "var"), str(e, "op", "mul"), num(e, "value", 1), static_cast<int>(num(e, "decades", 10)));
        else if (t == "memory") {
            if (Faction* f = find_faction(w, id)) {
                Tags tags;
                for (auto& x : e.tags) tags.insert(subst(ctx, x));
                remember(*f, p.c, w.year, str(e, "type", source), num(e, "magnitude", 1), num(e, "fear"), num(e, "faith") * (num(e, "faith") > 0 ? f->eff.faith_gain : 1), tags);
            }
        } else if (t == "war") war(w, e, ctx, id);
        else if (t == "spawn_person") spawn_person(w, p, e, ctx, id);
        else if (t == "crown") crown(w, p, e, ctx, id);
        else if (t == "split") split(w, p, e, ctx, id);
        else if (t == "refugees") refugees_to(w, p, e, ctx, id);
        else if (t == "ledger") meet(w, id, str(e, "how", ctx.card->id), num(e, "delta"));
        else if (t == "offer") write_tag(w, id, "asks:" + str(e, "kind") + ":" + var_of(ctx, str(e, "target", "self")), static_cast<int>(num(e, "decades", 10)), false, ctx.event);
    }
}

}
