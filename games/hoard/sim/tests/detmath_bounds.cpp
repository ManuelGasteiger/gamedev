#include <cmath>
#include <cstdint>
#include "check.h"
#include "core/detmath.h"

static uint64_t state = 0x9e3779b97f4a7c15ull;
static double uniform(double lo, double hi) {
    state = state * 6364136223846793005ull + 1442695040888963407ull;
    return lo + (hi - lo) * static_cast<double>(state >> 11) / 9007199254740992.0;
}

static double rel(double a, double b) { return b == 0 ? std::fabs(a) : std::fabs(a - b) / std::fabs(b); }

int main() {
    double e = 0, l = 0, p = 0, h = 0;
    for (int i = 0; i < 10000; ++i) {
        double x = uniform(-700, 700);
        e = std::fmax(e, rel(det::exp(x), std::exp(x)));
        double y = std::exp(uniform(-300, 300));
        l = std::fmax(l, rel(det::log(y), std::log(y)));
        double a = uniform(1e-6, 1e3), b = uniform(-20, 20);
        p = std::fmax(p, rel(det::pow(a, b), std::pow(a, b)));
        double t = uniform(0, 3000), hl = uniform(1, 5000);
        h = std::fmax(h, rel(det::half_life(t, hl), std::pow(0.5, t / hl)));
    }
    std::printf("max relative error: exp %.2e log %.2e pow %.2e half_life %.2e\n", e, l, p, h);
    CHECK(e < 1e-14, "exp");
    CHECK(l < 1e-15, "log");
    CHECK(p < 1e-12, "pow");
    CHECK(h < 1e-12, "half_life");
    CHECK(det::pow(3, 4) == 81 && det::pow(2, -2) == 0.25 && det::pow(9, 0.5) == 3, "exact cases");
    return 0;
}
