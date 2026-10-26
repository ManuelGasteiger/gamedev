#include "stats.h"

using namespace ages;

int main() {
    const int seeds = 200, years = 3000;
    const char* races[] = {"human", "dwarf", "elf", "orc"};
    std::map<std::string, std::vector<double>> lifetimes;
    int human_era2 = 0, not_empty = 0, every_race = 0, dwarf_alive = 0, elf_alive = 0, no_dominant = 0, humans_largest = 0, hegemony = 0;
    for (int seed = 1; seed <= seeds; ++seed) {
        LoadResult r = load_or_die(static_cast<uint64_t>(seed));
        Scenario s;
        s.years = years;
        s.sleep = 100;
        run_scenario(r.world, r.params, s);
        SeedStats st = collect(r.world);
        for (auto& kv : st.lifetimes) lifetimes[kv.first].insert(lifetimes[kv.first].end(), kv.second.begin(), kv.second.end());
        auto h = st.first_era2.find("human");
        if (h != st.first_era2.end() && h->second <= 1500) human_era2 += 1;
        if (!st.empty_valley) not_empty += 1;

        bool all = true;
        for (auto race : races) all = all && (st.alive[race] || st.remnants[race] > 0.1);
        every_race += all;
        dwarf_alive += st.alive["dwarf"];
        elf_alive += st.alive["elf"];
        int most = 0, regions = static_cast<int>(r.world.regions.size());
        std::string largest;
        bool tie = false;
        for (auto& kv : st.race_regions) {
            if (kv.second > most) { most = kv.second; largest = kv.first; tie = false; }
            else if (kv.second == most) tie = true;
        }
        if (most * 10 <= regions * 7) no_dominant += 1;
        if (largest == "human" && !tie) humans_largest += 1;
        if (st.hegemonies_fallen > 0) hegemony += 1;
    }
    double human = median(lifetimes["human"]), dwarf = median(lifetimes["dwarf"]);
    double elf = median(lifetimes["elf"]), orc = median(lifetimes["orc"]);
    auto share = [&](int n) { return static_cast<double>(n) / seeds; };
    std::printf("baseline over %d seeds x %d years, policy idle\n", seeds, years);
    std::printf("  %-30s %8s %s\n", "metric", "measured", "band");
    std::printf("  %-30s %8.0f [150, 600]\n", "median human lifetime", human);
    std::printf("  %-30s %8.0f [400, 1500]\n", "median dwarf lifetime", dwarf);
    std::printf("  %-30s %8.0f > 1200\n", "median elf lifetime", elf);
    std::printf("  %-30s %8.0f (n=%zu, not asserted)\n", "median orc lifetime", orc, lifetimes["orc"].size());
    std::printf("  %-30s %8.2f >= 0.80\n", "humans era 2 by 1500", share(human_era2));
    std::printf("  %-30s %8.2f >= 0.90\n", "valley not empty at 3000", share(not_empty));
    std::printf("  %-30s %8.2f >= 0.95\n", "every race alive or remnant", share(every_race));
    std::printf("  %-30s %8.2f >= 0.60\n", "dwarves alive at 3000", share(dwarf_alive));
    std::printf("  %-30s %8.2f >= 0.60\n", "elves alive at 3000", share(elf_alive));
    std::printf("  %-30s %8.2f >= 0.85\n", "no race > 70% of regions", share(no_dominant));
    std::printf("  %-30s %8.2f [0.50, 0.90]\n", "humans largest race", share(humans_largest));
    std::printf("  %-30s %8.2f >= 0.60\n", "hegemony rose and fell", share(hegemony));
    std::fflush(stdout);
    CHECK(human >= 150 && human <= 600, "median human lifetime out of band");
    CHECK(dwarf >= 400 && dwarf <= 1500, "median dwarf lifetime out of band");
    CHECK(elf > 1200, "median elf lifetime out of band");
    CHECK(share(human_era2) >= 0.80, "humans reach era 2 by 1500 in too few seeds");
    CHECK(share(not_empty) >= 0.90, "valley empty in too many seeds");
    CHECK(share(every_race) >= 0.95, "a race is gone (no faction, no remnant) in too many seeds");
    CHECK(share(dwarf_alive) >= 0.60, "dwarves have no living faction in too many seeds");
    CHECK(share(elf_alive) >= 0.60, "elves have no living faction in too many seeds");
    CHECK(share(no_dominant) >= 0.85, "one race holds more than 70% of regions in too many seeds");
    CHECK(share(humans_largest) >= 0.50 && share(humans_largest) <= 0.90, "humans largest race share out of band");
    CHECK(share(hegemony) >= 0.60, "a hegemony rose and fell in too few seeds");
    return 0;
}
