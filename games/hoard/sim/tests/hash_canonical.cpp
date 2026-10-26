#include <limits>
#include "check.h"
#include "fixtures.h"
#include "core/hash.h"
#include "ages/state_hash.h"

static uint64_t of(double v) { det::Hasher h; h.f64(v); return h.done(); }

int main() {
    CHECK(of(0.0) == of(-0.0), "-0 and +0 must hash equal");
    CHECK(of(1.0) != of(std::nextafter(1.0, 2.0)), "last-bit change must change the hash");
    det::Hasher a, b;
    a.f64(1.0);
    b.f64(std::numeric_limits<double>::quiet_NaN());
    CHECK(a.done() != b.done() && b.non_finite == 1, "NaN must change the hash and be counted");
    det::Hasher s1, s2;
    s1.str("ab"); s1.str("c");
    s2.str("a"); s2.str("bc");
    CHECK(s1.done() != s2.done(), "string framing");

    ages::LoadResult w1 = load_or_die(42), w2 = load_or_die(42);
    CHECK(ages::state_hash(w1.world).root == ages::state_hash(w2.world).root, "same world, same hash");
    w2.world.factions[0].wealth += 1e-12;
    ages::StateHash h1 = ages::state_hash(w1.world), h2 = ages::state_hash(w2.world);
    CHECK(h1.root != h2.root, "a tiny wealth change must change the root");
    int changed = 0;
    for (size_t i = 0; i < h1.parts.size(); ++i) if (h1.parts[i].second != h2.parts[i].second) ++changed;
    CHECK(changed == 1, "only the factions part may change");
    return 0;
}
