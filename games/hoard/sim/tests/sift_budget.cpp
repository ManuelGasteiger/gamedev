#include "fixtures.h"
#include "sift/sift.h"

using namespace ages;

// a sleep of `years` holding more lines than any budget: twelve tier-4 events on different courts and two tier-5 ones
static void check_sleep(int years, int expect) {
    LoadResult r = load_or_die(1);
    World& w = r.world;
    w.chronicle.clear();
    w.sift = SiftState();
    w.year = 1000;
    emit(w, "sleep_begin", "dragon", "", years, {}, {}, "", false);
    for (int i = 0; i < 12; ++i) {
        w.year = 1000 + (years * i) / 12;
        emit(w, "slayers_founded", "court" + std::to_string(i));
    }
    w.year = 1000 + years;
    emit(w, "faction_destroyed", "lost_a");
    emit(w, "faction_destroyed", "lost_b");
    std::vector<ThreadLine> lines = briefing_lines(w, r.params, 1000);
    int body = 0, heads = 0, tier5 = 0;
    for (auto& l : lines) {
        if (l.kind == "headline") ++heads;
        if (l.tier >= 5) ++tier5;
        if (l.kind != "headline" && l.tier < 5) ++body;
    }
    std::printf("%d years: %d lines, headline %d, tier 5 %d\n", years, body, heads, tier5);
    CHECK(line_budget(years) == expect, "L(y)");
    CHECK(body == expect, "the lines fill the budget exactly");
    CHECK(heads == 1, "plus one headline");
    CHECK(tier5 == 2, "plus every tier-5 line");
}

int main() {
    check_sleep(1, 3);
    check_sleep(10, 5);
    check_sleep(100, 7);
    check_sleep(1000, 9);
    std::printf("sift_budget ok\n");
    return 0;
}
