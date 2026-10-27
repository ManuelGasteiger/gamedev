#include "save.h"
#include "ages.h"
#include "json.h"
#include "record.h"
#include "state_hash.h"

namespace ages {

using nlohmann::json;

namespace {

// format_version migrations, each a pure JSON-to-JSON step from version i + 1 to i + 2; empty until a save leaves the
// machine with an older layout
using Migration = json (*)(const json&);
const Migration kMigrate[] = {nullptr};

}

std::string save_json(const World& w, const Params& p) {
    json j;
    j["format"] = "hoard-save";
    j["format_version"] = kSaveFormat;
    j["rules_version"] = kRulesVersion;
    j["build"] = build_id();
    j["data"] = {{"root", hash_hex(p.data_hash)}, {"scenario_bank", nullptr}, {"pattern_bank", nullptr}};
    j["seed"] = w.seed;
    j["year"] = w.year;
    j["acts"] = w.acts.size();
    j["state_hash"] = hash_hex(state_hash(w).root);
    j["world"] = json::parse(world_to_json(w), nullptr, false);
    return j.dump();
}

LoadSave load_save(const std::string& text, World& out, std::string& err, bool* replay_verifiable) {
    json j = json::parse(text, nullptr, false);
    if (j.is_discarded() || !j.is_object()) { err = "not json"; return LoadSave::bad_json; }
    if (j.value("format", "") != "hoard-save") { err = "not a hoard save"; return LoadSave::not_a_save; }
    int version = j.value("format_version", 0);
    if (version > kSaveFormat) { err = "written by a newer build (" + j.value("build", std::string("?")) + ")"; return LoadSave::newer_format; }
    for (int v = version; v < kSaveFormat; ++v)
        if (v >= 1 && kMigrate[v - 1]) j = kMigrate[v - 1](j);
    // the hash is checked on the world as saved, so a save from older rules verifies before its fields are read into
    // this build's state (rules 1: no sex; rules 2: one fear number)
    if (!j.contains("world") || !j["world"].is_object()) { err = "no world"; return LoadSave::bad_world; }
    if (hash_hex(state_hash_of_json(j["world"]).root) != j.value("state_hash", "")) { err = "state hash mismatch: the save is damaged"; return LoadSave::hash_mismatch; }
    World w;
    if (!world_from_json(j["world"].dump(), w, err)) { if (err.empty()) err = "no world"; return LoadSave::bad_world; }
    if (replay_verifiable) *replay_verifiable = j.value("rules_version", 0) == kRulesVersion;
    out = w;
    return LoadSave::ok;
}

}
