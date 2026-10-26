#include "pop.h"
#include "world_util.h"

namespace ages {

static void normalise(Pop& pop) {
    double sum = 0;
    for (auto& kv : pop) sum += kv.second;
    if (sum <= 0) return;
    for (auto& kv : pop) kv.second /= sum;
    for (auto it = pop.begin(); it != pop.end();) it = it->second < 1e-6 ? pop.erase(it) : std::next(it);
}

void blend_culture(Faction& f, const Params& p) {
    Culture& c = f.culture;
    double lifespan = 0, growth = 0, decay = 0, era_rate = 0, memory_rate = 0, war = 0, expansion = 0, off_home = 0;
    for (auto& kv : f.pop) {
        auto race = p.races.find(kv.first);
        if (race == p.races.end()) continue;
        const Culture& r = race->second.culture;
        lifespan += kv.second * r.lifespan;
        growth += kv.second * r.growth;
        decay += kv.second * r.cohesion_decay;
        era_rate += kv.second * r.era_rate;
        memory_rate += kv.second * (r.memory_halflife > 0 ? 1.0 / r.memory_halflife : 0.0);
        war += kv.second * r.temperament.war_appetite;
        expansion += kv.second * r.temperament.expansion;
        off_home += kv.second * r.temperament.off_home_land;
    }
    c.lifespan = lifespan;
    c.growth = growth;
    c.cohesion_decay = decay;
    c.era_rate = era_rate;
    c.memory_halflife = memory_rate > 0 ? 1.0 / memory_rate : 0.0;
    c.temperament.war_appetite = war;
    c.temperament.expansion = expansion;
    c.temperament.off_home_land = off_home;
    auto dom = p.races.find(dominant_race(f));
    if (dom != p.races.end()) {
        c.temperament.home_terrain = dom->second.culture.temperament.home_terrain;
        c.temperament.settles_off_home = dom->second.culture.temperament.settles_off_home;
    }
}

void shift_pop(Faction& f, const Params& p, const Pop& toward, double amount) {
    if (amount <= 0) return;
    for (auto& kv : f.pop) kv.second *= 1.0 - amount;
    for (auto& kv : toward) f.pop[kv.first] += amount * kv.second;
    normalise(f.pop);
    blend_culture(f, p);
}

void blend_traits(Traits& t, const Traits& toward, double amount) {
    t.authority += (toward.authority - t.authority) * amount;
    t.openness += (toward.openness - t.openness) * amount;
    t.militarism += (toward.militarism - t.militarism) * amount;
    t.piety += (toward.piety - t.piety) * amount;
}

void refugees(World& w, const Params& p, const Faction& from, double share) {
    Faction* best = nullptr;
    double best_aff = 0;
    for (auto& g : w.factions) {
        if (g.id == from.id || !adjacent(w, from, g)) continue;
        double aff = affinity(p, from, g);
        if (aff > 0 && (!best || aff > best_aff)) { best = &g; best_aff = aff; }
    }
    if (!best) return;
    double total = from.wealth + best->wealth;
    double amount = total > 0 ? share * from.wealth / total : share * 0.5;
    shift_pop(*best, p, from.pop, amount);
}

}
