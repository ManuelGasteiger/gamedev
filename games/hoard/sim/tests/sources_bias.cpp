#include "fixtures.h"
#include "ages/memory_tiers.h"
#include "sift/sift.h"

using namespace ages;

int main() {
    LoadResult r = load_or_die(1);
    World& w = r.world;
    const Params& p = r.params;
    w.chronicle.clear();
    w.sift = SiftState();
    w.sift.on = true;
    Faction orc = *find_faction(w, "dorun");
    orc.id = "gruk";
    orc.race = "orc";
    orc.regions.clear();
    w.factions.push_back(orc);
    Faction& dwarf = *find_faction(w, "dorun");
    Faction& human = *find_faction(w, "aldmere");
    for (Faction* f : {&dwarf, &human}) { f->dread = Tiers(); f->awe = Tiers(); f->tags.clear(); }
    dwarf.tags.insert("grudge:gruk");
    dwarf.culture.traits.openness = 0.3;
    human.culture.traits.openness = 0.8;

    w.year = 400;
    int sack = emit(w, "region_sacked", "gruk", "dorun", 0.5);
    SiftThread t;
    t.id = 1;
    t.pattern = "test";
    t.cast = {{"$victim", "dorun"}, {"$witness", "aldmere"}};
    t.events = {sack};
    t.years = {400};
    t.state = "closed";
    t.opened = t.advanced = t.ended = 400;
    w.sift.threads.push_back(t);

    Source d = make_source(w, p, 1, "dorun");
    Source h = make_source(w, p, 1, "aldmere");
    CHECK(d.blame == "gruk" && d.stated_cause == "grudge:gruk", "the dwarves lay the sack on the orcs they hold a grudge against");
    CHECK(h.blame == "gruk" && h.stated_cause == "true", "the honest humans record who did it, as the cause it was");
    std::printf("sources_bias ok\n");
    return 0;
}
