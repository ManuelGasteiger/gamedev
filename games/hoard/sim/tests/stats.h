#pragma once
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include "fixtures.h"

struct SeedStats {
    std::map<std::string, std::vector<double>> lifetimes;
    std::map<std::string, int> first_era2;
    bool empty_valley = false;
    double orc_share = 0;
    double total_wealth = 0;
    std::vector<ages::FactionRecord> records;
    std::map<std::string, int> race_regions;    // by dominant race
    std::map<std::string, bool> alive;          // dominant race has a living faction
    std::map<std::string, double> remnants;
    int hegemonies_fallen = 0;
    int hunger_wakes = 0;
};

inline SeedStats collect(const ages::World& w) {
    SeedStats s;
    std::vector<ages::FactionRecord> all = w.graveyard;
    for (auto& f : w.factions) all.push_back(ages::record_of(f, w.year));
    for (auto& r : all) {
        s.lifetimes[r.race].push_back(r.died - r.born);
        if (r.era_years.size() > 2) {
            auto it = s.first_era2.find(r.race);
            if (it == s.first_era2.end() || r.era_years[2] < it->second) s.first_era2[r.race] = r.era_years[2];
        }
    }
    int owned = 0, orc = 0;
    for (auto& r : w.regions) {
        if (r.owner.empty()) continue;
        owned += 1;
        const ages::Faction* f = ages::find_faction(w, r.owner);
        if (!f) continue;
        if (f->race == "orc") orc += 1;
        s.race_regions[ages::dominant_race(*f)] += 1;
    }
    s.empty_valley = owned == 0;
    s.orc_share = static_cast<double>(orc) / static_cast<double>(w.regions.size());
    for (auto& f : w.factions) {
        s.total_wealth += f.wealth;
        s.alive[ages::dominant_race(f)] = true;
    }
    for (auto& r : w.remnants) s.remnants[r.race] = r.strength;
    for (auto& e : w.chronicle) {
        if (e.kind == "hegemony_ended") s.hegemonies_fallen += 1;
        if (e.kind == "wake" && e.object == "hunger") s.hunger_wakes += 1;
    }
    s.records = all;
    return s;
}

inline double median(std::vector<double> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}
