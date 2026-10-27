#include <cmath>
#include "fixtures.h"
#include "ages/memory_tiers.h"

using namespace ages;

// a court with no oath, no faith, no records and no monument, weak against the dragon: the owner's base row
static Faction& neutral(World& w) {
    Faction& f = *find_faction(w, "aldmere");
    f.wealth = 1;
    f.cohesion = 0.3;
    f.faith = 0;
    f.memory.clear();
    f.monuments.clear();
    f.dread = Tiers{};
    f.awe = Tiers{};
    w.people.erase(std::remove_if(w.people.begin(), w.people.end(), [](const Person& x) { return x.faction == "aldmere" && (x.role == "sage" || x.role == "historian"); }), w.people.end());
    for (auto& rid : f.regions) find_region(w, rid)->tags.erase("library");
    return f;
}

int main() {
    LoadResult r = load_or_die(1);
    World& w = r.world;
    Faction& f = neutral(w);
    apply_acts(w, r.params, {make_act("burn", "aldmere", 0.4, "east_vale")});
    CHECK(std::fabs(f.dread.oral - 0.14) < 1e-9, "one burn 0.4 on a neutral court leaves oral dread 0.14");
    CHECK(f.awe.oral == 0, "the awe loss of a burn clamps at 0");

    LoadResult r2 = load_or_die(1);
    World& w2 = r2.world;
    Faction& g = neutral(w2);
    g.awe.oral = 0.5;
    double a0 = g.awe.oral;
    apply_acts(w2, r2.params, {make_act("burn", "aldmere", 0.4, "east_vale")});
    double a1 = g.awe.oral, loss1 = a0 - a1;
    w2.year += 10;
    apply_acts(w2, r2.params, {make_act("burn", "aldmere", 0.4, "east_vale")});
    double loss2 = a1 - g.awe.oral;
    CHECK(loss1 > 0, "a burn takes awe from a court that had some");
    CHECK(std::fabs(loss2 - 0.5 * loss1 * (1.0 - a1) / (1.0 - a0)) < 1e-9, "a second burn within 30 years takes half the awe (rep), at the court's new awe");
    std::printf("dread_awe_update ok (dread %.3f; awe loss %.4f then %.4f)\n", f.dread.oral, loss1, loss2);
    return 0;
}
