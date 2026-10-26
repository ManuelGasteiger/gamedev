#include "card_ctx.h"
#include "rules.h"
#include "world_rules.h"
#include "../world_util.h"

// the S2 card effects (data/events): arrive (a people from beyond the rim settles or invades), spawn_undead (the dead
// take a region), terrain (a cataclysm changes a region's terrain and its land), die_out (a people's last court is
// gone for good), undying (a ruler stops dying)

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

std::string pick_race(World& w, const Params& p, const std::map<std::string, double>& table) {
    double total = 0;
    for (auto& kv : table) if (kv.second > 0 && p.races.count(kv.first) && !has_tag(w.tags, "extinct:" + kv.first)) total += kv.second;
    if (total <= 0) return "";
    double roll = w.rng.uniform() * total;
    std::string last;
    for (auto& kv : table) {
        if (kv.second <= 0 || !p.races.count(kv.first) || has_tag(w.tags, "extinct:" + kv.first)) continue;
        last = kv.first;
        if (roll < kv.second) return last;
        roll -= kv.second;
    }
    return last;
}

// an empty rim region of the newcomers' home terrain, else any empty rim region, else a rim region of the weakest court
// that holds more than one (taken by force)
std::string rim_region(World& w, const Params& p, const Culture& c, double weak, std::string& holder) {
    std::vector<std::string> home, empty;
    const Faction* prey = nullptr;
    std::string prey_region;
    for (auto& id : p.genesis.rim) {
        const Region* r = find_region(w, id);
        if (!r || r->lair) continue;
        if (r->owner.empty()) { (home_terrain(c, *r) ? home : empty).push_back(id); continue; }
        const Faction* f = find_faction(w, r->owner);
        if (f && f->regions.size() > 1 && f->cohesion < weak && (!prey || f->cohesion < prey->cohesion)) { prey = f; prey_region = id; }
    }
    std::vector<std::string>& pool = home.empty() ? empty : home;
    if (!pool.empty()) return pool[static_cast<size_t>(w.rng.range(0, static_cast<int>(pool.size())))];
    if (prey) holder = prey->id;
    return prey_region;
}

// puts the faction on the region (from its holder by force) and binds it
std::string settle(World& w, CardCtx& ctx, const CardEffect& e, Faction f, const std::string& region, const std::string& holder, const std::string& reason) {
    for (auto& t : e.tags) f.tags.insert(subst(ctx, t));
    f.regions.clear();
    std::string id = f.id;
    w.factions.push_back(f);
    sort_by_id(w);
    transfer_region(w, region, holder, id);
    if (Region* r = find_region(w, region)) r->tags.erase("razed");
    ctx.vars[str(e, "bind", "new")] = id;
    ctx.vars["region"] = region;
    Faction* nf = find_faction(w, id);
    Faction* h = find_faction(w, holder);
    if (nf && h) start_war(w, *nf, *h, reason, because(ctx.card->cause, {ctx.event}));
    return id;
}

void arrive(World& w, const Params& p, const CardEffect& e, CardCtx& ctx) {
    std::string race = pick_race(w, p, e.table);
    if (race.empty()) return;
    std::string holder;
    std::string region = rim_region(w, p, p.races.at(race).culture, num(e, "weak_cohesion", 0.5), holder);
    if (region.empty()) return;
    int era = 0;
    for (auto& f : w.factions) era = std::max(era, f.era);
    era = std::max(0, era - static_cast<int>(num(e, "era_lag", 1)));
    Faction f = found_faction(w, p, race, region, num(e, "wealth", 40), num(e, "cohesion", 0.85), era);
    ctx.vars["race"] = race;
    settle(w, ctx, e, f, region, holder, "invasion");
}

void spawn_undead(World& w, const Params& p, const CardEffect& e, CardCtx& ctx) {
    std::string region = var_of(ctx, str(e, "region", "region"));
    const Region* r = find_region(w, region);
    if (!r || r->lair || !p.races.count("undead")) return;
    std::string holder = r->owner;
    const Faction* h = find_faction(w, holder);
    if (h && h->regions.size() < 2) return;
    double wealth = num(e, "wealth", 40) + num(e, "wealth_per_magic", 0) * w.magic;
    Faction f = found_faction(w, p, "undead", region, wealth, num(e, "cohesion", 0.8), static_cast<int>(num(e, "era", 1)));
    settle(w, ctx, e, f, region, holder, "the_dead_walk");
}

// the last court of a people dwindles away: it leaves the world, its land lies empty in ruins, the people is extinct
void die_out(World& w, const Params& p, const CardEffect& e, CardCtx& ctx) {
    Faction* f = find_faction(w, var_of(ctx, str(e, "on", "self")));
    if (!f) return;
    std::string race = dominant_race(*f);
    std::vector<std::string> regions = f->regions;
    remove_faction(w, f->id);
    for (auto& rid : regions)
        if (Region* r = find_region(w, rid)) { r->tags.insert("ruins"); r->tags.insert("razed"); r->empty_years = 0; }
    extinguish(w, p, race);
}

void terrain(World& w, const Params& p, const CardEffect& e, CardCtx& ctx) {
    Region* r = find_region(w, var_of(ctx, str(e, "on", "region")));
    std::string to = str(e, "to");
    if (!r || r->lair || to.empty()) return;
    r->terrain = to;
    auto td = p.genesis.terrain.find(to);
    if (td != p.genesis.terrain.end()) r->land = std::min(r->land, td->second.land);
    r->burn = clamp01(r->burn + num(e, "burn"));
    r->tags.insert("cataclysm");
}

}

bool world_effect(World& w, const Params& p, const CardEffect& e, CardCtx& ctx) {
    const std::string& t = e.type;
    if (t == "arrive") arrive(w, p, e, ctx);
    else if (t == "spawn_undead") spawn_undead(w, p, e, ctx);
    else if (t == "terrain") terrain(w, p, e, ctx);
    else if (t == "die_out") die_out(w, p, e, ctx);
    else if (t == "undying") {
        if (Faction* f = find_faction(w, var_of(ctx, str(e, "on", "self")))) make_undying(w, p, *f, because(ctx.card->cause, {ctx.event}), ctx.event);
    } else return false;
    return true;
}

}
