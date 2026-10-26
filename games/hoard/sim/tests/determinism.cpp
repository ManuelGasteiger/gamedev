#include "fixtures.h"

using namespace ages;

static uint64_t run(uint64_t seed) {
    LoadResult r = load_or_die(seed);
    Scenario s;
    s.first_acts = sample_log();
    s.years = 1000;
    s.sleep = 100;
    run_scenario(r.world, r.params, s);
    return chronicle_hash(r.world);
}

int main() {
    uint64_t a = run(7), b = run(7), c = run(8);
    CHECK(a == b, "same seed and act log must give the same chronicle hash");
    CHECK(a != c, "different seed must give a different chronicle hash");
    std::printf("determinism ok: %016llx != %016llx\n", static_cast<unsigned long long>(a), static_cast<unsigned long long>(c));
    return 0;
}
