#include <cmath>
#include "fixtures.h"
#include "ages/memory_tiers.h"

using namespace ages;

int main() {
    LoadResult r = load_or_die(1);
    World& w = r.world;
    const MemoryConstants& m = r.params.c.mem;
    Faction& f = *find_faction(w, "aldmere");
    f.memory.clear();
    f.monuments.clear();
    f.dread = Tiers{};
    f.awe = Tiers{};
    w.people.erase(std::remove_if(w.people.begin(), w.people.end(), [](const Person& x) { return x.faction == "aldmere" && (x.role == "sage" || x.role == "historian"); }), w.people.end());
    find_region(w, "east_vale")->tags.insert("library");
    apply_acts(w, r.params, {make_act("burn", "aldmere", 0.4, "east_marches")});
    CHECK(f.dread.oral > 0 && std::fabs(f.dread.written - m.written_deposit * f.dread.oral) < 1e-9, "a court with a library writes down written_deposit of what it tells");

    double written = f.dread.written;
    Act lib = make_act("burn", "aldmere", 0.1, "east_vale");
    lib.item = "library";
    apply_acts(w, r.params, {lib});
    CHECK(std::fabs(f.dread.written - written * m.library_burn) < 1e-9, "burn(library) leaves library_burn of the written stock");

    f.dread.written = 0.5;
    w.dt = 10;
    for (int i = 0; i < 25; ++i) decay_tiers(w, r.params, f);
    CHECK(std::fabs(f.dread.written - 0.25) < 1e-6, "250 years without acts halve the written stock");
    std::printf("memory_tiers ok (written after library %.4f, after 250 years %.4f)\n", written * m.library_burn, f.dread.written);
    return 0;
}
