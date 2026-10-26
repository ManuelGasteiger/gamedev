#include "rules.h"
#include "../world_util.h"

namespace ages {

static void apply(double& v, const Modifier& m) {
    if (m.op == "mul") v *= m.value;
    else v += m.value;
}

static void trait_effects(Faction& f, const Constants& c) {
    const Traits& t = f.culture.traits;
    const Temperament& tp = f.culture.temperament;
    f.eff.cohesion_decay *= 1.0 - c.trait_authority_decay * t.authority;
    f.eff.war_chance = tp.war_appetite * (c.trait_mult_base + t.militarism);
    f.eff.anti_dragon_rate = c.trait_mult_base + t.militarism;
    f.eff.diffusion = c.trait_mult_base + t.openness;
    f.eff.faith_gain = c.trait_mult_base + t.piety;
    f.eff.settle_chance = tp.expansion;
    f.eff.succession_bonus = t.authority > c.trait_authority_high ? c.trait_succession_halved : 1.0;
}

// 17.2: while a sage or priest who witnessed one of the dragon's acts lives, the faction's memory decays at half speed
static bool sage_remembers(const World& w, const Faction& f) {
    for (auto& x : w.people)
        if (x.faction == f.id && (x.role == "sage" || x.role == "priest") && x.memory_of_dragon > 0) return true;
    return false;
}

static void vassal_wealth(World& w, const Constants& c) {
    for (auto& f : w.factions) f.eff.vassal_wealth = 0;
    for (auto& f : w.factions) {
        std::string lord = overlord_of(f);
        if (lord.empty()) continue;
        if (Faction* o = find_faction(w, lord)) o->eff.vassal_wealth += f.wealth * c.vassal_wealth_weight;
    }
}

void rule_modifiers(World& w, const Params& p) {
    const Constants& c = p.c;
    for (auto& m : w.modifiers)
        if (m.until <= w.year && m.source.rfind("dragon.", 0) == 0) emit(w, "spell_expired", m.source.substr(7), m.target);
    w.modifiers.erase(std::remove_if(w.modifiers.begin(), w.modifiers.end(),
                                     [&](const Modifier& m) { return m.until <= w.year; }),
                      w.modifiers.end());

    for (auto& f : w.factions) {
        f.eff.growth = f.culture.growth;
        f.eff.cohesion_decay = f.culture.cohesion_decay;
        f.eff.era_rate = f.culture.era_rate;
        f.eff.memory_halflife = f.culture.memory_halflife;
        if (f.eff.memory_halflife > 0 && sage_remembers(w, f)) f.eff.memory_halflife /= c.sage_memory_mult;
        trait_effects(f, c);
        if (f.ruler.cls == "immortal") {
            f.eff.cohesion_decay *= c.immortal_decay_mult;
            f.eff.era_rate *= c.immortal_era_rate_mult;
        }
        if (has_tag(f.tags, "fading")) {
            f.eff.growth *= c.fading_growth_mult;
            f.eff.cohesion_decay *= c.fading_decay_mult;
        }
        for (auto& m : w.modifiers) {
            if (m.target != f.id && m.target != "*") continue;
            if (m.var == "growth") apply(f.eff.growth, m);
            else if (m.var == "cohesion_decay") apply(f.eff.cohesion_decay, m);
            else if (m.var == "era_rate") apply(f.eff.era_rate, m);
            else if (m.var == "war_chance") apply(f.eff.war_chance, m);
            else if (m.var == "anti_dragon_rate") apply(f.eff.anti_dragon_rate, m);
            else if (m.var == "faith_gain") apply(f.eff.faith_gain, m);
        }
        if (f.eff.growth < 0) f.eff.growth = 0;
        if (f.eff.cohesion_decay < 0) f.eff.cohesion_decay = 0;
        if (f.eff.war_chance < 0) f.eff.war_chance = 0;
    }
    vassal_wealth(w, c);

    for (auto& r : w.regions) {
        r.eff_land = r.land;
        if (const Faction* o = r.owner.empty() ? nullptr : find_faction(w, r.owner))
            r.eff_land *= home_terrain(o->culture, r) ? c.home_land_mult : o->culture.temperament.off_home_land;
        r.eff_invasion = 1.0;
        if (has_tag(r.tags, "dug_in")) r.eff_invasion *= c.dug_in_invasion_mult;
        for (auto& m : w.modifiers) {
            if (m.target != r.id && m.target != "*") continue;
            if (m.var == "land") apply(r.eff_land, m);
            else if (m.var == "invasion_chance") apply(r.eff_invasion, m);
        }
        r.eff_land = clamp01(r.eff_land);
        if (r.eff_invasion < 0) r.eff_invasion = 0;
    }
}

}
