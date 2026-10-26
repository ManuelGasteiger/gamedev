#include <iterator>
#include <fstream>
#include "fixtures.h"
#include "ages/genesis.h"

using namespace ages;

static std::string hex(uint64_t h) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%016llx", static_cast<unsigned long long>(h));
    return buf;
}

// returns false when the stored hash differs; writes the file when it is missing
static bool check(const char* file, const std::string& hash) {
    std::string path = std::string(AGES_GOLDEN_DIR) + "/" + file;
    std::ifstream in(path);
    std::string stored;
    if (in && (in >> stored)) {
        if (stored != hash) {
            std::fprintf(stderr, "golden hash changed: stored %s, got %s (update %s deliberately)\n", stored.c_str(), hash.c_str(), path.c_str());
            return false;
        }
        std::printf("golden ok: %s %s\n", file, hash.c_str());
        return true;
    }
    std::ofstream out(path);
    CHECK(static_cast<bool>(out), "cannot write golden hash file");
    out << hash << "\n";
    std::printf("golden written: %s %s\n", file, hash.c_str());
    return true;
}

static bool check_text(const std::string& file, const std::string& text) {
    std::string path = std::string(AGES_GOLDEN_DIR) + "/" + file;
    std::ifstream in(path, std::ios::binary);
    if (in) {
        std::string stored((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (stored == text) { std::printf("golden ok: %s\n", file.c_str()); return true; }
        std::string got = path + ".new";
        std::ofstream(got, std::ios::binary) << text;
        std::fprintf(stderr, "golden text changed: %s (new version in %s; tools/chronicle_diff.py %s %s)\n", path.c_str(), got.c_str(), path.c_str(), got.c_str());
        return false;
    }
    std::ofstream out(path, std::ios::binary);
    CHECK(static_cast<bool>(out), "cannot write golden text file");
    out << text;
    std::printf("golden written: %s\n", file.c_str());
    return true;
}

int main() {
    // v0: seed 42, the sample act log once, idle, 2000 years in sleeps of 100
    LoadResult idle = load_or_die(42);
    Scenario s;
    s.first_acts = sample_log();
    s.years = 2000;
    s.sleep = 100;
    run_scenario(idle.world, idle.params, s);
    bool ok = check("ages_v0.hash", hex(chronicle_hash(idle.world)));

    // v0.1: seed 42, raze_all every sleep, 2000 years in sleeps of 100 (collapse, vassalage, remnants, hunger)
    LoadResult raze = load_or_die(42);
    Policy policy;
    std::string err;
    CHECK(load_policy(AGES_DATA_DIR, "raze_all", policy, err), err.c_str());
    Scenario r;
    r.policy = &policy;
    r.data_dir = AGES_DATA_DIR;
    r.years = 2000;
    r.sleep = 100;
    run_scenario(raze.world, raze.params, r);
    ok = check("ages_v0_1_raze.hash", hex(chronicle_hash(raze.world))) && ok;

    // text goldens: seeds 1 to 3, generated worlds, played, 3000 years; tools/chronicle_diff.py reads a mismatch
    Policy played;
    CHECK(load_policy(AGES_DATA_DIR, "played", played, err), err.c_str());
    for (int seed = 1; seed <= 3; ++seed) {
        LoadResult g = load_or_die(static_cast<uint64_t>(seed));
        CHECK(make_genesis(g.world, g.params, err), err.c_str());
        Scenario sc;
        sc.policy = &played;
        sc.data_dir = AGES_DATA_DIR;
        run_scenario(g.world, g.params, sc);
        ok = check_text("chronicle/seed-" + std::to_string(seed) + ".txt", chronicle_text(g.world.chronicle)) && ok;
    }
    return ok ? 0 : 1;
}
