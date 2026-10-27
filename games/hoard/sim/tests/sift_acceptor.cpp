#include "fixtures.h"
#include "sift/sift.h"

using namespace ages;

// one scripted step: the year, the event, then the acceptors as rule_chronicle runs them
static int at(World& w, const Params& p, int year, const std::string& kind, const std::string& subject, const std::string& object = "",
              double m = 0, std::vector<int> refs = {}) {
    w.year = year;
    Cause c;
    c.refs = refs;
    int id = emit(w, kind, subject, object, m, {}, c);
    sift_events(w, p);
    return id;
}

static int stories(const World& w, const std::string& court) {
    int n = 0;
    for (auto& e : w.sift.compounds) if (e.kind == "story.vengeance" && e.subject == court) ++n;
    return n;
}

int main() {
    LoadResult r = load_or_die(1);
    Params& p = r.params;
    std::vector<SiftPattern> keep;
    for (auto& x : p.sift.patterns) if (x.id == "vengeance_for_burning") keep.push_back(x);
    CHECK(keep.size() == 1, "the bank has vengeance_for_burning");
    p.sift.patterns = keep;
    World& w = r.world;
    w.chronicle.clear();
    w.sift = SiftState();

    int a = at(w, p, 100, "act_burn", "aldmere", "east_vale", 0.5);
    int b = at(w, p, 120, "slayers_founded", "aldmere", "", 0, {a});
    at(w, p, 150, "slay_attempt_failed", "aldmere", "", 0.1, {b});
    CHECK(stories(w, "aldmere") == 1, "burn, an order founded because of it, an attack: one story");

    int a2 = at(w, p, 200, "act_burn", "aldmere", "east_vale", 0.5);
    int b2 = at(w, p, 220, "slayers_founded", "aldmere", "", 0, {a2});
    at(w, p, 250, "slay_attempt_failed", "aldmere", "", 0.1, {b2});
    CHECK(stories(w, "aldmere") == 1, "once per court");

    int c = at(w, p, 300, "act_burn", "vessary", "", 0.5);
    at(w, p, 310, "oath_sworn", "oath_1", "vessary");
    int d = at(w, p, 320, "slayers_founded", "vessary", "", 0, {c});
    at(w, p, 330, "slay_attempt_failed", "vessary", "", 0.1, {d});
    CHECK(stories(w, "vessary") == 0, "an oath between the first two clauses kills the match");
    int e = at(w, p, 400, "act_burn", "vessary", "", 0.5);
    int f = at(w, p, 410, "slayers_founded", "vessary", "", 0, {e});
    at(w, p, 420, "slay_attempt_failed", "vessary", "", 0.1, {f});
    CHECK(stories(w, "vessary") == 0, "and bars that court for good");
    std::printf("sift_acceptor ok\n");
    return 0;
}
