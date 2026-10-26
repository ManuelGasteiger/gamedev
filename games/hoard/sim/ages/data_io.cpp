#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "ages.h"
#include "json.h"

namespace ages {

using nlohmann::json;

template <class T>
static void get(const json& j, const char* key, T& out) {
    auto it = j.find(key);
    if (it != j.end()) out = it->get<T>();
}

static bool read_text(const std::string& path, std::string& out, std::string& err) {
    std::ifstream in(path);
    if (!in) { err = "cannot open " + path; return false; }
    std::ostringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}

static bool read_json(const std::string& path, json& out, std::string& err) {
    std::string text;
    if (!read_text(path, text, err)) return false;
    out = json::parse(text, nullptr, false);
    if (out.is_discarded()) { err = "invalid json in " + path; return false; }
    return true;
}

// data/events/*.json in file-name order, joined into one JSON array; a missing folder means no cards
static bool read_cards(const std::string& dir, std::string& out, std::string& err) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path folder = fs::path(dir) / "events";
    if (!fs::is_directory(folder, ec)) { out.clear(); return true; }
    std::vector<std::string> files;
    for (auto& entry : fs::directory_iterator(folder, ec))
        if (entry.path().extension() == ".json") files.push_back(entry.path().string());
    std::sort(files.begin(), files.end());
    out = "[";
    for (size_t i = 0; i < files.size(); ++i) {
        std::string text;
        if (!read_text(files[i], text, err)) return false;
        out += (i ? ",\n" : "\n") + text;
    }
    out += "]";
    return true;
}

// v0.4: a missing optional file loads as empty text
static bool read_optional(const std::string& path, std::string& out) {
    std::string err;
    if (!read_text(path, out, err)) out.clear();
    return true;
}

static bool read_dragon_files(const std::string& dir, DataJson& d) {
    return read_optional(dir + "/talents.json", d.talents) && read_optional(dir + "/defences.json", d.defences);
}

bool load_data_json(const std::string& dir, DataJson& d, std::string& err) {
    return read_text(dir + "/constants.json", d.constants, err) && read_text(dir + "/races.json", d.races, err) &&
           read_text(dir + "/reactions.json", d.reactions, err) && read_text(dir + "/regions.json", d.regions, err) &&
           read_text(dir + "/factions.json", d.factions, err) && read_text(dir + "/shocks.json", d.shocks, err) &&
           read_text(dir + "/names.json", d.names, err) && read_text(dir + "/offers.json", d.offers, err) &&
           read_text(dir + "/text/en.json", d.text, err) && read_cards(dir, d.events, err) &&
           read_dragon_files(dir, d) && read_optional(dir + "/hooks.json", d.hooks) &&   // v0.4 hooks (S3)
           read_optional(dir + "/genesis.json", d.genesis);
}

bool load_params(const std::string& dir, Params& out, std::string& err) {
    DataJson d;
    return load_data_json(dir, d, err) && params_from_json(d, out, err);
}

bool load_world(const std::string& dir, const Params& p, World& out, std::string& err) {
    DataJson d;
    return load_data_json(dir, d, err) && world_from_data_json(d, p, out, err);
}

LoadResult load_data(const std::string& dir, uint64_t seed) {
    DataJson d;
    LoadResult r;
    if (!load_data_json(dir, d, r.error)) return r;
    return load_data_from_json(d, seed);
}

bool load_policy(const std::string& dir, const std::string& name, Policy& out, std::string& err) {
    json j;
    if (!read_json(dir + "/policies/" + name + ".json", j, err)) return false;
    get(j, "cycle", out.cycle);
    if (j.contains("acts")) {
        for (auto& v : j["acts"]) {
            PolicyAct a;
            get(v, "type", a.type);
            get(v, "target", a.target);
            get(v, "region", a.region);
            get(v, "against", a.against);
            get(v, "magnitude", a.magnitude);
            get(v, "payload_share", a.payload_share);
            out.acts.push_back(a);
        }
    }
    get(j, "talents", out.talents);       // v0.4 (S1)
    get(j, "sleep_cycle", out.sleep_cycle);
    get(j, "min_sleep", out.min_sleep);
    if (j.contains("answer_hooks")) {   // v0.4 hooks (S3)
        const json& h = j["answer_hooks"];
        get(h, "acts", out.hook_acts);
        get(h, "choice", out.hook_choice);
        get(h, "magnitude", out.hook_magnitude);
        get(h, "payload_share", out.hook_payload);
        get(h, "wound_cap", out.hook_wound_cap);
        get(h, "reach", out.hook_reach);
        get(h, "max", out.hook_max);
    }
    return true;
}


}
