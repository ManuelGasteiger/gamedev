#include "check.h"
#include "fixtures.h"
#include "ages/genesis.h"
#include "ages/save.h"
#include "ages/state_hash.h"

using namespace ages;

static void play(World& w, const Params& p, const Policy& pol, int from, int to) {
    for (int i = from; i < to; ++i) {
        apply_acts(w, p, resolve_policy(w, p, pol, i, AGES_DATA_DIR));
        sleep(w, p, 100);
    }
}

int main() {
    LoadResult r = load_or_die(6);
    std::string err;
    CHECK(make_genesis(r.world, r.params, err), err.c_str());
    Policy pol;
    CHECK(load_policy(AGES_DATA_DIR, "played", pol, err), err.c_str());
    play(r.world, r.params, pol, 0, 8);

    std::string text = save_json(r.world, r.params);
    World loaded;
    CHECK(load_save(text, loaded, err) == LoadSave::ok, err.c_str());
    CHECK(state_hash(loaded).root == state_hash(r.world).root, "a loaded save must hash like the world it came from");
    play(r.world, r.params, pol, 8, 16);
    play(loaded, r.params, pol, 8, 16);
    CHECK(state_hash(loaded).root == state_hash(r.world).root, "a loaded save must continue exactly as the original");

    std::string damaged = text;
    size_t at = damaged.find("\"wealth\":");
    CHECK(at != std::string::npos, "test save has no wealth field");
    at = damaged.find_first_of("123456789", at);
    damaged[at] = damaged[at] == '9' ? '8' : static_cast<char>(damaged[at] + 1);
    World refused;
    CHECK(load_save(damaged, refused, err) == LoadSave::hash_mismatch, "a damaged save must be refused");
    std::string newer = text;
    newer.replace(newer.find("\"format_version\":1"), 18, "\"format_version\":9");
    CHECK(load_save(newer, refused, err) == LoadSave::newer_format, "a save from a newer build must be refused");
    return 0;
}
