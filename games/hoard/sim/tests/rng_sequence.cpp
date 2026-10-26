#include "check.h"
#include "ages/rng.h"

int main() {
    ages::Rng r;
    r.seed(42);
    const uint32_t world[] = {3508393247u, 2846903365u, 3050928809u, 2850731726u};
    for (uint32_t v : world) CHECK(r.next_u32() == v, "world.rng sequence for seed 42 changed");
    CHECK(r.range(0, 1000) == 665 && r.range(5, 9) == 8, "range sequence changed");
    ages::Rng k = ages::rng_for(42, "cards", "aldmere");
    const uint32_t keyed[] = {3496724043u, 1639079990u, 77465609u, 2031601185u};
    for (uint32_t v : keyed) CHECK(k.next_u32() == v, "rng_for sequence changed");
    return 0;
}
