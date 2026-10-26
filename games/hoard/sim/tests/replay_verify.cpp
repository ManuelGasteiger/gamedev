#include "check.h"
#include "fixtures.h"
#include "ages/genesis.h"
#include "ages/record.h"
#include "ages/state_hash.h"

using namespace ages;

static std::vector<std::string> record(uint64_t seed, const Policy& pol, uint64_t& final_hash) {
    LoadResult r = load_or_die(seed);
    std::string err;
    CHECK(make_genesis(r.world, r.params, err), err.c_str());
    Recorder rec;
    rec.world = &r.world;
    r.world.recorder = &rec;
    Scenario s;
    s.policy = &pol;
    s.data_dir = AGES_DATA_DIR;
    s.years = 1500;
    run_scenario(r.world, r.params, s);
    final_hash = state_hash(r.world).root;
    return rec.lines;
}

static ReplayResult replay(uint64_t seed, const std::vector<std::string>& lines, uint64_t& final_hash) {
    LoadResult r = load_or_die(seed);
    std::string err;
    CHECK(make_genesis(r.world, r.params, err), err.c_str());
    ReplayResult out = replay_lines(r.world, r.params, lines);
    final_hash = state_hash(r.world).root;
    return out;
}

int main() {
    Policy pol;
    std::string err;
    CHECK(load_policy(AGES_DATA_DIR, "played", pol, err), err.c_str());
    for (uint64_t seed = 1; seed <= 3; ++seed) {
        uint64_t a = 0, b = 0;
        std::vector<std::string> lines = record(seed, pol, a);
        ReplayResult r = replay(seed, lines, b);
        CHECK(r.ok && r.checkpoints > 0, r.error.c_str());
        CHECK(a == b, "a replay must end where the recording ended");
        if (seed == 1) {
            for (auto& l : lines) {
                size_t m = l.find("\"magnitude\":0.3");
                if (m == std::string::npos) continue;
                l.replace(m, 15, "\"magnitude\":0.4");
                break;
            }
            ReplayResult bad = replay(seed, lines, b);
            CHECK(!bad.ok && bad.first_bad_year >= 0, "an edited input must fail at the next checkpoint");
        }
    }
    return 0;
}
