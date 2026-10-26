#pragma once
#include <cstdint>
#include <cstring>
#include <string>

namespace det {

inline uint64_t mix(uint64_t z) {
    z += 0x9e3779b97f4a7c15ull;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

struct Hasher {
    uint64_t h = 0x243f6a8885a308d3ull;
    uint64_t non_finite = 0;

    void u64(uint64_t v) { h = mix(h ^ v); }
    void i64(int64_t v) { u64(static_cast<uint64_t>(v)); }
    void f64(double v) {
        if (v != v || v - v != 0) { ++non_finite; u64(0x7ff8000000000000ull + non_finite); return; }
        if (v == 0) v = 0;   // -0 hashes as +0
        uint64_t b;
        std::memcpy(&b, &v, 8);
        u64(b);
    }
    void str(const std::string& s) {
        u64(s.size());
        uint64_t w = 0;
        size_t n = 0;
        for (unsigned char c : s) {
            w = (w << 8) | c;
            if (++n == 8) { u64(w); w = 0; n = 0; }
        }
        if (n) u64(w);
    }
    void tag(uint64_t t) { u64(0x7a67000000000000ull ^ t); }
    uint64_t done() const { return mix(h); }
};

}
