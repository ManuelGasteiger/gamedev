#include <cmath>
#include "fixtures.h"
#include "ages/step.h"

using namespace ages;

int main() {
    LoadResult r = load_or_die(1);
    World& w = r.world;
    const Constants& c = r.params.c;
    Dragon& d = w.dragons[kPlayer];
    d.phase = 2;
    d.hoard = Hoard{};
    d.hoard.gold = 1000;
    d.hoard.books = 10;
    d.growth.clear();
    d.knowledge = 0;
    w.moulting = false;
    w.sleep_started = w.year - 100;

    digest_hoard(w, r.params, 100);

    double window = c.phase_long_sleep[1];
    double floor = c.gold_floor_base * c.floor_phase_mult[1];
    double eaten = c.gold_burn * (1000 - floor);
    double books_left = 10 * std::pow(0.5, 100 / (c.growth_halflife.at("books") * window));
    CHECK(std::fabs(d.hoard.gold - (1000 - eaten)) < 1e-6, "one long window eats gold_burn of the gold above the floor");
    CHECK(std::fabs(d.hoard.books - books_left) < 1e-6, "books keep the remainder of their half-life");
    CHECK(std::fabs(d.growth["might"] - eaten * c.gold_pts) < 1e-6, "Might is gold_pts per gold eaten");
    CHECK(std::fabs(d.growth["lore"] - (10 - books_left) * c.growth_points.at("books")) < 1e-6, "Lore is the digested books at their points");
    std::printf("digestion_consumes ok (gold %.2f, books %.4f, might %.3f, lore %.3f)\n", d.hoard.gold, d.hoard.books, d.growth["might"], d.growth["lore"]);
    return 0;
}
