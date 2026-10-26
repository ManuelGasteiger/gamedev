#include "defences.h"
#include "memory_tiers.h"
#include "talents.h"
#include "world_util.h"

namespace ages {

namespace {

bool warded(const World& w, const Region& r) {
    for (auto& m : w.modifiers) if (m.target == r.id && m.source == "dragon.ward") return true;
    return false;
}

bool stands(const World& w, const DefenceDef& d, const Faction& f, const Region& r) {
    if (d.ward_modifier && warded(w, r)) return true;
    if (f.era < d.min_era) return false;
    if (!d.races_any.empty() && !contains(d.races_any, dominant_race(f))) return false;
    if (d.min_anti_dragon <= 0 && d.tags_any.empty()) return true;
    if (d.min_anti_dragon > 0 && f.anti_dragon >= d.min_anti_dragon) return true;
    for (auto& t : d.tags_any) if (has_tag(f.tags, t)) return true;
    return false;
}

bool hero_at_court(const World& w, const std::string& faction) {
    for (auto& x : w.people)
        if (x.faction == faction && x.role == "hero" && !has_tag(x.tags, "hostage")) return true;
    return false;
}

}

std::string capital_region(const World& w, const Faction& f) {
    std::string best;
    double top = -1;
    for (auto& rid : f.regions)
        if (const Region* r = find_region(w, rid)) {
            double y = r->eff_land * (1.0 - r->burn);
            if (y > top) { top = y; best = rid; }
        }
    return best;
}

std::vector<DefenceView> region_defences(const World& w, const Params& p, const Region& r) {
    std::vector<DefenceView> out;
    const Faction* f = r.owner.empty() ? nullptr : find_faction(w, r.owner);
    if (!f) return out;
    const DefenceBook& b = p.defences;
    double dread = dread_eff(*f);
    double damp = dread >= p.c.mem.flee_dread ? 1.0 - dread : 1.0;   // the garrisons flee (act-api 6.4)
    bool hero = hero_at_court(w, f->id) && capital_region(w, *f) == r.id;
    bool wrecked = has_tag(r.tags, "defences_wrecked");
    for (auto& d : b.defences) {
        if (!stands(w, d, *f, r)) continue;
        DefenceView v;
        v.def = &d;
        double scale = (1.0 + d.era_scale * f->era) * (d.fearless ? 1.0 : damp);
        v.per_act = d.per_act * scale;
        v.passive = d.passive * scale;
        v.burn_mult = d.burn_mult;
        if (hero && d.per_act > 0) { v.per_act += b.hero_bonus; hero = false; }
        if (wrecked) { v.per_act = 0; v.passive = 0; v.burn_mult = 1; v.wrecked = true; }
        out.push_back(v);
    }
    return out;
}

Unlock next_unlock(const World& w, const Params& p, const Faction& f) {
    Unlock best;
    const Region* seat = find_region(w, capital_region(w, f));
    for (auto& d : p.defences.defences) {
        if (f.era < d.min_era || (!d.races_any.empty() && !contains(d.races_any, dominant_race(f)))) continue;
        if (seat && stands(w, d, f, *seat)) continue;
        double at = -1;
        if (d.min_anti_dragon > f.anti_dragon) at = d.min_anti_dragon;
        if (contains(d.tags_any, "slayers") && p.c.slayers_at > f.anti_dragon && (at < 0 || p.c.slayers_at < at)) at = p.c.slayers_at;
        if (at > 0 && (best.at < 0 || at < best.at)) { best.at = at; best.defence = d.id; }
    }
    return best;
}

const Region* act_region(const World& w, const Act& a) {
    if (const Region* r = find_region(w, a.region)) return r;
    const Faction* f = find_faction(w, a.target);
    return f ? find_region(w, capital_region(w, *f)) : nullptr;
}

double armor_against(const World& w, const Params& p, const DefenceDef& d) {
    double a = effect(w, p, "armor_mult");
    if (d.radius > 0) a *= effect(w, p, "arcane_armor_mult");
    return a;
}

double burn_mult_at(const World& w, const Params& p, const Region& r) {
    if (effect(w, p, "ward_breaker") > 0) return 1;
    double m = 1;
    for (auto& v : region_defences(w, p, r)) m *= v.burn_mult;
    return m;
}

}
