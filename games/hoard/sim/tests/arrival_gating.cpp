#include "fixtures.h"
#include "ages/arrivals.h"
#include "ages/memory_tiers.h"

using namespace ages;

namespace {

void prepare(World& w, double awe, double dread) {
    Faction& f = *find_faction(w, "aldmere");
    f.memory.clear();
    f.monuments.clear();
    f.dread = Tiers{};
    f.awe = Tiers{};
    set_eff(f.awe, awe);
    set_eff(f.dread, dread);
    f.faith = 0.8;
    f.cohesion = 0.9;
    f.lair_known = w.year;
    f.lair_exact = false;
    w.arrivals.clear();
    w.arrival_fired.clear();
}

bool shrine(const World& w) {
    for (auto& e : w.chronicle) if (e.kind == "foothill_shrine" && e.subject == "aldmere") return true;
    return false;
}

}

int main() {
    LoadResult r = load_or_die(1);
    World loved = r.world;
    World feared = r.world;
    prepare(loved, 0.2, 0.0);
    prepare(feared, 0.2, 0.6);
    rule_arrivals(loved, r.params);
    rule_arrivals(feared, r.params);

    const Arrival* pilgrims = arrival_for(loved, "pilgrim", "aldmere");
    CHECK(pilgrims, "a court with awe_eff 0.2 that knows the lair sends pilgrims");
    CHECK(!pilgrims->because.empty(), "the arrival names the clauses that let it through");
    CHECK(!arrival_for(feared, "pilgrim", "aldmere"), "the same court at dread_eff 0.6 sends no pilgrims");
    CHECK(shrine(feared), "a foothill shrine event fires instead");
    CHECK(!shrine(loved), "no shrine where the pilgrims came");
    std::printf("arrival_gating ok (because: %zu clauses)\n", pilgrims->because.size());
    return 0;
}
