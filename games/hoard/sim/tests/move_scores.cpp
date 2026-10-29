#include "fixtures.h"
#include "ages/moves.h"
#include "ages/person_record.h"

using namespace ages;

namespace {

Person person(const char* id, std::vector<std::string> traits) {
    Person x;
    x.id = id;
    x.race = "human";
    x.role = "hero";
    x.traits.list = traits;
    return x;
}

PersonMemory entry(const char* ev, int sev, double dread, double awe, double debt) {
    PersonMemory m;
    m.ev = ev;
    m.sev = sev;
    m.dread = dread;
    m.awe = awe;
    m.debt = debt;
    return m;
}

double score(const World& w, const Params& p, const Person& x, const char* move) { return move_score(w, p, x, p.moves.moves.at(move)); }

}

int main() {
    LoadResult r = load_or_die(1);
    const Params& p = r.params;
    const World& w = r.world;
    CHECK(p.moves.moves.size() == 5, "five scored moves in data/moves.json");

    Person heir = person("h", {"curious", "ambitious"});
    CHECK(score(w, p, heir, "teach") == 5, "Teach on curious + ambitious = 5");
    CHECK(score(w, p, heir, "corrupt") == 4, "Corrupt on curious + ambitious = 4");
    CHECK(score(w, p, heir, "bargain") == 2, "Bargain on curious + ambitious = 2");
    CHECK(score(w, p, heir, "threaten") == -1, "Threaten on curious + ambitious = -1");
    CHECK(score(w, p, heir, "court") == 6, "Court on curious + ambitious = 6");

    Person king = person("k", {"craven"});
    king.mem.push_back(entry("act_burn", 3, 1, 0, 0));
    CHECK(axes(p, king).dread == 3, "a witnessed sev 3 burn is dread 3");
    CHECK(score(w, p, king, "threaten") == 7, "Threaten on craven with dread 3 = 7");
    CHECK(score(w, p, king, "bargain") == 6, "Bargain on craven with dread 3 = 6");
    king.mem.push_back(entry("act_burn", 3, 1, 0, 0));
    CHECK(score(w, p, king, "threaten") == 8, "dread counts up to +4");

    Person taught = heir;
    taught.mem.push_back(entry("taught", 2, 0, 1, 0.5));
    CHECK(axes(p, taught).debt == 1 && axes(p, taught).awe == 2, "taught: awe +2, debt +1");
    CHECK(score(w, p, taught, "corrupt") == 5, "Corrupt after Teach: ambitious 3 + curious 1 + debt 1 = 5");
    CHECK(score(w, p, taught, "teach") == 5 + 2 + 1 + 1, "Teach again: awe 2, debt 1, one earlier success");
    taught.mem.push_back(entry("mercy", 1, 0, 1, 2));
    CHECK(score(w, p, taught, "bargain") == 2 + 3 + 2, "Bargain: debt 3 (cap 3) and one mercy (+2)");
    for (int i = 0; i < 4; ++i) taught.mem.push_back(entry("mercy", 1, 0, 0, 0));
    CHECK(score(w, p, taught, "bargain") == 2 + 3 + 6, "the memory term is clamped at +6");
    Person betrayed = person("b", {"honest"});
    betrayed.mem.push_back(entry("betrayal", 1, 0, 0, 0));
    CHECK(score(w, p, betrayed, "teach") == 1 - 3, "one betrayal costs 3");

    Person proud = person("p", {"proud", "devout"});
    CHECK(score(w, p, proud, "threaten") == -6, "Threaten on proud + devout = -6");
    std::printf("move_scores ok\n");
    return 0;
}
