#include "fixtures.h"

using namespace ages;

int main() {
    LoadResult r = load_or_die(3);
    World& w = r.world;
    Oath o = make_oath("aldmere", "defend");
    w.oaths.push_back(o);
    Faction* a = find_faction(w, "aldmere");
    Faction* v = find_faction(w, "vessary");
    remember(*a, r.params.c, 0, "gift", 1.0, 0, 0.7, {"gift"});
    a->cohesion = 0.3;
    a->wealth = 5;
    v->wealth = 5000;
    v->cohesion = 1.0;
    double faith_before = a->faith;

    bool breached = false, invaded = false;
    for (int decade = 0; decade < 30 && !breached; ++decade) {
        SleepResult res = sleep(w, r.params, 10);
        for (auto& e : res.chronicle) {
            if (e.kind == "oath_breached_by_dragon") breached = true;
            if (e.kind == "region_taken" && e.tags.count("aldmere")) invaded = true;
        }
    }
    CHECK(invaded, "fixture: vessary must have taken a region from aldmere during the sleep");
    CHECK(breached, "sleeping through an invasion must breach the defend oath");
    CHECK(w.oaths[0].status == "breached_by_dragon", "oath status must be breached_by_dragon");
    bool betrayal = false;
    double faith_after = 1;
    for (auto& f : w.factions) {
        if (f.id != "aldmere" && !f.tags.count("successor_of:aldmere")) continue;
        for (auto& m : f.memory) if (m.tags.count("betrayal")) betrayal = true;
        faith_after = std::min(faith_after, f.faith);
    }
    CHECK(betrayal, "stakes: betrayal memory must be applied to the betrayed party");
    CHECK(faith_after < faith_before, "stakes: faith must have dropped");
    std::printf("oath_breach_sleep ok (faith %.2f -> %.2f)\n", faith_before, faith_after);
    return 0;
}
