#include "core/detmath.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace det {

namespace {

constexpr double ln2_hi = 6.93147180369123816490e-01;
constexpr double ln2_lo = 1.90821492927058770002e-10;
constexpr double inv_ln2 = 1.44269504088896338700e+00;
constexpr double sqrt_half = 0.70710678118654752440;

uint64_t bits(double x) { uint64_t u; std::memcpy(&u, &x, 8); return u; }
double from_bits(uint64_t u) { double x; std::memcpy(&x, &u, 8); return x; }

// 2^k for k in [-1022, 1023]
double pow2i(int k) { return from_bits(static_cast<uint64_t>(k + 1023) << 52); }

double ipow(double x, long long n) {
    bool inv = n < 0;
    unsigned long long m = inv ? 0ull - static_cast<unsigned long long>(n) : static_cast<unsigned long long>(n);
    double r = 1;
    while (m) {
        if (m & 1) r *= x;
        x *= x;
        m >>= 1;
    }
    return inv ? 1 / r : r;
}

}

double exp(double x) {
    if (x != x) return x;
    if (x > 709.78) return std::numeric_limits<double>::infinity();
    if (x < -745.13) return 0;
    double k = std::floor(x * inv_ln2 + 0.5);
    double r = (x - k * ln2_hi) - k * ln2_lo;
    // Taylor to r^14 on |r| <= 0.347, Horner form
    double p = 1.0 / 87178291200.0;
    p = p * r + 1.0 / 6227020800.0;
    p = p * r + 1.0 / 479001600.0;
    p = p * r + 1.0 / 39916800.0;
    p = p * r + 1.0 / 3628800.0;
    p = p * r + 1.0 / 362880.0;
    p = p * r + 1.0 / 40320.0;
    p = p * r + 1.0 / 5040.0;
    p = p * r + 1.0 / 720.0;
    p = p * r + 1.0 / 120.0;
    p = p * r + 1.0 / 24.0;
    p = p * r + 1.0 / 6.0;
    p = p * r + 0.5;
    p = p * r + 1.0;
    p = p * r + 1.0;
    int ki = static_cast<int>(k);
    if (ki > 1023) return p * pow2i(1023) * pow2i(ki - 1023);
    if (ki < -1022) return p * pow2i(-1022) * pow2i(ki + 1022);
    return p * pow2i(ki);
}

double log(double x) {
    if (x != x || x < 0) return std::numeric_limits<double>::quiet_NaN();
    if (x == 0) return -std::numeric_limits<double>::infinity();
    if (x == std::numeric_limits<double>::infinity()) return x;
    int e = 0;
    if (x < 2.2250738585072014e-308) { x *= 18014398509481984.0; e -= 54; }   // subnormals: scale by 2^54
    uint64_t u = bits(x);
    e += static_cast<int>((u >> 52) & 0x7ff) - 1023;
    double m = from_bits((u & 0x000fffffffffffffull) | 0x3ff0000000000000ull);
    if (m > 2 * sqrt_half) { m *= 0.5; e += 1; }
    // log m = 2 atanh(s), s = (m - 1) / (m + 1), |s| <= 0.1716
    double s = (m - 1) / (m + 1);
    double s2 = s * s;
    double q = 1.0 / 23;
    q = q * s2 + 1.0 / 21;
    q = q * s2 + 1.0 / 19;
    q = q * s2 + 1.0 / 17;
    q = q * s2 + 1.0 / 15;
    q = q * s2 + 1.0 / 13;
    q = q * s2 + 1.0 / 11;
    q = q * s2 + 1.0 / 9;
    q = q * s2 + 1.0 / 7;
    q = q * s2 + 1.0 / 5;
    q = q * s2 + 1.0 / 3;
    double lm = 2 * s + 2 * s * s2 * q;
    return e * ln2_hi + (e * ln2_lo + lm);
}

double pow(double x, double y) {
    if (y == 0) return 1;
    if (y == 0.5) return std::sqrt(x);
    if (y == std::floor(y) && std::fabs(y) <= 1024) return ipow(x, static_cast<long long>(y));
    if (x == 0) return y > 0 ? 0 : std::numeric_limits<double>::infinity();
    if (x < 0) return std::numeric_limits<double>::quiet_NaN();
    return exp(y * log(x));
}

double half_life(double t, double halflife) {
    return exp(-(t / halflife) * (ln2_hi + ln2_lo));
}

}
