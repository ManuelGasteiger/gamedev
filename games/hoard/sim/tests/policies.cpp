#include "stats.h"

using namespace ages;

struct PolicyRun {
    double orc_share = 0, wealth = 0, protected_max_realm = 0;
    int dominant = 0, collapsed_after_dominance = 0, hunger_wakes = 0;
};

static int seed_offset = 0;

static PolicyRun run(const char* name, int seeds) {
    PolicyRun out;
    Policy policy;
    bool has = name != nullptr;
    if (has) {
        std::string err;
        CHECK(load_policy(AGES_DATA_DIR, name, policy, err), err.c_str());
    }
    for (int seed = 1; seed <= seeds; ++seed) {
        LoadResult r = load_or_die(static_cast<uint64_t>(seed + seed_offset));
        World& w = r.world;
        int regions = static_cast<int>(w.regions.size());
        int dominance_year = -1, collapse_year = -1;
        bool hunger = false;
        for (int i = 0; i < 20 && !has_tag(w.tags, "dragon_slain"); ++i) {
            if (has) apply_acts(w, r.params, resolve_policy(w, r.params, policy, i, AGES_DATA_DIR));
            SleepResult res = sleep(w, r.params, 100);
            if (res.wake_reason == "hunger") hunger = true;
            if (dominance_year >= 0 && collapse_year < 0)
                for (auto& e : res.chronicle) if (e.kind == "collapse" && e.subject == "aldmere") collapse_year = e.year;
            const Faction* a = find_faction(w, "aldmere");
            if (a && dominance_year < 0 && static_cast<int>(realm_regions(w, *a)) * 10 >= regions * 6) dominance_year = w.year;
        }
        SeedStats st = collect(w);
        out.orc_share += st.orc_share;
        out.wealth += st.total_wealth;
        for (auto& rec : st.records) if (rec.id == "aldmere") out.protected_max_realm += rec.max_realm;
        if (dominance_year >= 0) {
            out.dominant += 1;
            if (collapse_year >= 0 && collapse_year - dominance_year <= 1000) out.collapsed_after_dominance += 1;
        }
        out.hunger_wakes += hunger;
    }
    out.orc_share /= seeds;
    out.wealth /= seeds;
    out.protected_max_realm /= seeds;
    return out;
}

int main(int argc, char** argv) {
    if (argc > 1) seed_offset = std::atoi(argv[1]);
    const int seeds = 100;
    PolicyRun idle = run(nullptr, seeds), raze = run("raze_all", seeds), protect = run("protect_one", seeds);
    std::printf("policies over %d seeds x 2000 years, sleep 100\n", seeds);
    std::printf("  %-12s %10s %12s %14s %9s %9s %12s\n", "policy", "orc_share", "wealth", "aldmere_realm", "dominant", "collapsed", "hunger_woke");
    auto row = [&](const char* name, const PolicyRun& p) {
        std::printf("  %-12s %10.3f %12.0f %14.2f %9d %9d %12d\n", name, p.orc_share, p.wealth, p.protected_max_realm, p.dominant, p.collapsed_after_dominance, p.hunger_wakes);
    };
    row("idle", idle);
    row("raze_all", raze);
    row("protect_one", protect);
    std::fflush(stdout);
    CHECK(raze.orc_share > idle.orc_share, "raze_all must end with a higher orc region share than idle");
    CHECK(raze.wealth < idle.wealth, "raze_all must end with lower total valley wealth than idle");
    CHECK(raze.hunger_wakes * 2 >= seeds, "raze_all (no food taken) must wake the dragon by hunger in >= 50% of seeds");
    CHECK(protect.protected_max_realm > idle.protected_max_realm, "protect_one must raise the protected faction's max realm (own plus vassal regions)");
    CHECK(protect.dominant > 0, "protect_one: the protected faction must reach >= 60% of regions in some seeds");
    CHECK(protect.collapsed_after_dominance > 0, "protect_one: the protected faction must collapse within 1000 years of dominance");
    return 0;
}
