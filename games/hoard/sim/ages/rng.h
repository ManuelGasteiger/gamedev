#pragma once
#include <cstdint>
#include <string>
#include "core/hash.h"

namespace ages {

inline uint64_t splitmix64(uint64_t& s) {
    uint64_t z = (s += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

struct Rng {
    uint64_t state = 0;
    uint64_t inc = 0;

    void seed(uint64_t seed) {
        uint64_t s = seed;
        uint64_t a = splitmix64(s);
        uint64_t b = splitmix64(s);
        inc = (b << 1u) | 1u;
        state = 0;
        next_u32();
        state += a;
        next_u32();
    }

    uint32_t next_u32() {
        uint64_t old = state;
        state = old * 6364136223846793005ull + inc;
        uint32_t xorshifted = static_cast<uint32_t>(((old >> 18u) ^ old) >> 27u);
        uint32_t rot = static_cast<uint32_t>(old >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((32u - rot) & 31u));
    }

    double uniform() {
        return static_cast<double>(next_u32() >> 8) * (1.0 / 16777216.0);
    }

    bool chance(double p) { return uniform() < p; }

    int range(int lo, int hi) {
        if (hi <= lo) return lo;
        uint32_t span = static_cast<uint32_t>(hi - lo);
        return lo + static_cast<int>(next_u32() % span);
    }
};

// a keyed sub-stream: a new draw in one rule no longer shifts every later draw of the world stream
inline Rng rng_for(uint64_t seed, const std::string& tag, const std::string& id) {
    det::Hasher h;
    h.u64(seed);
    h.str(tag);
    h.str(id);
    Rng r;
    r.seed(h.done());
    return r;
}

}
