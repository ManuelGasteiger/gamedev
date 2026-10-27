#include <cmath>
#include "fixtures.h"
#include "ages/memory_tiers.h"

using namespace ages;

static bool memory_has(const Faction& f, const char* tag) {
    for (auto& m : f.memory) if (m.tags.count(tag)) return true;
    return false;
}

static void betrayal() {
    LoadResult r = load_or_die(1);
    World& w = r.world;
    w.oaths.push_back(make_oath("aldmere", "no_harm"));
    Faction* f = find_faction(w, "aldmere");
    remember(*f, r.params.c, 0, "gift", 1.0, 0.6, {"gift"});
    apply_acts(w, r.params, {make_act("burn", "aldmere", 0.4, "east_vale")});
    CHECK(memory_has(*f, "betrayal"), "burn under no_harm oath must leave a betrayal memory");
    CHECK(f->faith < 0.05, "faith must collapse below 0.05 on betrayal");
    sleep(w, r.params, 10);
    CHECK(w.oaths[0].status == "breached_by_dragon", "oath must be breached_by_dragon after the burn");
    bool ev = false;
    for (auto& e : w.chronicle) if (e.kind == "oath_breached_by_dragon") ev = true;
    CHECK(ev, "chronicle must contain oath_breached_by_dragon");
}

static double defend_faith(bool losing) {
    LoadResult r = load_or_die(1);
    World& w = r.world;
    Faction* a = find_faction(w, "aldmere");
    Faction* v = find_faction(w, "vessary");
    a->at_war_with.push_back("vessary");
    v->at_war_with.push_back("aldmere");
    War war;
    war.a = "vessary";
    war.b = "aldmere";
    w.wars.push_back(war);
    if (losing) {
        transfer_region(w, "east_marches", "aldmere", "vessary");
        emit(w, "region_taken", "vessary", "east_marches", 0, {"aldmere"});
    }
    Act d = make_act("defend", "aldmere", 1.0);
    d.against = "vessary";
    apply_acts(w, r.params, {d});
    return a->faith;
}

static void strong_vs_weak() {
    LoadResult strong = load_or_die(1);
    LoadResult weak = load_or_die(1);
    Faction* s = find_faction(strong.world, "aldmere");
    Faction* k = find_faction(weak.world, "aldmere");
    s->wealth = 2000;
    s->cohesion = 1.0;
    k->wealth = 1;
    k->cohesion = 0.3;
    double th = threat(strong.world.dragons[kPlayer], strong.params.c);
    CHECK(power(*s, strong.params.c) >= 0.5 * th, "fixture: strong faction must have power >= 0.5 threat");
    CHECK(power(*k, weak.params.c) < 0.5 * th, "fixture: weak faction must have power < 0.5 threat");
    apply_acts(strong.world, strong.params, {make_act("burn", "aldmere", 0.5, "east_vale")});
    apply_acts(weak.world, weak.params, {make_act("burn", "aldmere", 0.5, "east_vale")});
    CHECK(s->anti_dragon > 0, "strong faction must gain anti_dragon progress when burned");
    CHECK(dread_eff(*s) < dread_eff(*k), "strong faction must dread less than a weak one for the same burn");
    CHECK(k->anti_dragon == 0, "weak faction gains no anti_dragon from a burn");
    // item 02: its research then runs at the dread factor and is paid for out of its wealth
    s->era = std::max(s->era, strong.params.c.anti_dragon_min_era);
    s->at_war_with.clear();
    double wealth = s->wealth, anti = s->anti_dragon;
    rule_era(strong.world, strong.params);
    double progress = s->anti_dragon - anti;
    CHECK(progress > 0, "a strong court at research era must progress against the dragon");
    CHECK(std::fabs((wealth - s->wealth) - strong.params.c.mem.research_spend * progress) < 1e-9, "research must cost research_spend wealth per unit of progress");
}

int main() {
    betrayal();
    double losing = defend_faith(true), steady = defend_faith(false);
    CHECK(losing > steady, "defend while losing must raise faith more than defend while not losing");
    strong_vs_weak();
    std::printf("reactions ok (defend losing %.2f vs steady %.2f)\n", losing, steady);
    return 0;
}
