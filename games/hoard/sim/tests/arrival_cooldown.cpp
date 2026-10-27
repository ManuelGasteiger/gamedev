#include "fixtures.h"
#include "ages/arrivals.h"
#include "ages/memory_tiers.h"

using namespace ages;

namespace {

const Arrival* envoy_of(const World& w, const std::string& court) {
    for (auto& a : w.arrivals) if (a.kind == "envoy" && a.court == court) return &a;
    return nullptr;
}

bool same_cause(const World& w, const std::string& court, const Arrival& first) {
    for (auto& a : w.arrivals)
        if (a.kind == "envoy" && a.court == court && a.offer_kind == first.offer_kind && a.motive == first.motive) return true;
    return false;
}

}

int main() {
    for (uint64_t seed = 1; seed <= 30; ++seed) {
        LoadResult r = load_or_die(seed);
        World& w = r.world;
        for (auto& f : w.factions) {
            f.lair_known = w.year;
            f.lair_exact = true;
            f.cohesion = 0.9;
            f.faith = 0.95;
            f.dread = Tiers{};
            f.awe = Tiers{};
            set_eff(f.dread, 0.1);
        }
        rule_arrivals(w, r.params);
        for (auto& f : w.factions) {
            const Arrival* e = envoy_of(w, f.id);
            if (!e) continue;
            Arrival first = *e;
            for (int later : {20, 60, 99}) {
                w.arrivals.clear();
                w.year = first.year + later;
                for (auto& g : w.factions) { g.lair_known = w.year; g.lair_exact = true; }
                rule_arrivals(w, r.params);
                CHECK(!same_cause(w, f.id, first), "the same court, envoy and motive do not repeat within 100 years");
            }
            w.arrivals.clear();
            w.year = first.year + 101;
            for (auto& g : w.factions) { g.lair_known = w.year; g.lair_exact = true; }
            rule_arrivals(w, r.params);
            CHECK(same_cause(w, f.id, first), "after the cooldown the same cause may come again");
            std::printf("arrival_cooldown ok (seed %llu, %s, %s)\n", static_cast<unsigned long long>(seed), f.id.c_str(), first.offer_kind.c_str());
            return 0;
        }
    }
    CHECK(false, "no seed in 1..30 produced an envoy to test");
    return 1;
}
