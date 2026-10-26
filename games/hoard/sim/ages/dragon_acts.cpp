#include "dragon_acts.h"
#include "defences.h"
#include "talents.h"
#include "world_util.h"
#include "wounds.h"

namespace ages {

bool harmful_act(const std::string& type) { return type == "burn" || type == "take" || type == "kill" || type == "take_hostage"; }

double strike_damage(const World& w, const Params& p, const Act& a, Tags* defences) {
    if (!harmful_act(a.type)) return 0;
    const Region* r = act_region(w, a);
    if (!r) return 0;
    double damage = 0;
    for (auto& v : region_defences(w, p, *r)) {
        if (v.per_act <= 0) continue;
        damage += v.per_act * armor_against(w, p, *v.def);
        if (defences) defences->insert(v.def->id);
    }
    return damage;
}

void strike(World& w, const Params& p, const Act& a) {
    const Region* r = harmful_act(a.type) ? act_region(w, a) : nullptr;
    if (!r) return;
    Tags ids;
    double damage = strike_damage(w, p, a, &ids);
    std::string id = r->id;
    wound_dragon(w, p, r->owner, id, damage, ids);
    if (a.type == "burn" && !dragon_dead(w) && effect(w, p, "tower_breaker") > 0) find_region(w, id)->tags.insert("defences_wrecked");
}

void passive_wound(World& w, const Params& p, const Act& a) {
    const DefenceDef* d = nullptr;
    for (auto& x : p.defences.defences) if (x.id == a.item) d = &x;
    std::string court = a.target;
    if (court.empty()) if (const Region* r = find_region(w, a.region)) court = r->owner;
    double armor = d ? armor_against(w, p, *d) : effect(w, p, "armor_mult");
    wound_dragon(w, p, court, a.region, std::max(0.0, a.magnitude) * armor, a.item.empty() ? Tags{} : Tags{a.item});
}

double warded_burn(const World& w, const Params& p, const Act& a, double m) {
    const Region* r = act_region(w, a);
    return r ? m * burn_mult_at(w, p, *r) : m;
}

void after_burn(World& w, const Params& p, const Faction& f, const Act& a, double m) {
    w.dragons[kPlayer].hoard.embers += m * effect(w, p, "ember_mult");
    double spread = effect(w, p, "burn_spread");
    std::vector<std::string> burned;
    if (Region* r = find_region(w, a.region)) {
        burned.push_back(r->id);
        if (spread > 0)
            for (auto& n : r->neighbours) {
                Region* nr = find_region(w, n);
                if (!nr || nr->owner != f.id) continue;
                nr->burn = clamp01(nr->burn + m * spread);
                burned.push_back(n);
            }
    } else burned = f.regions;
    if (effect(w, p, "burn_linger") > 0)
        for (auto& id : burned) if (Region* x = find_region(w, id)) x->tags.insert("clinging_fire");
}

double molten_gold_mult(const World& w, const Params& p, const Faction& f, const Act& a) {
    double mult = effect(w, p, "molten_gold");
    if (mult <= 0) return 1;
    double burn = 0;
    if (const Region* r = find_region(w, a.region)) burn = r->burn;
    else for (auto& id : f.regions) if (const Region* x = find_region(w, id)) burn = std::max(burn, x->burn);
    return burn >= p.c.molten_gold_burn ? mult : 1;
}

}
