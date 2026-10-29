#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "ages.h"
#include "json.h"
#include "profile.h"
#include <cstdio>
#include "text/bundle_io.h"

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
static bool read_folder(const std::string& dir, const char* sub, std::string& out, std::string& err, const char* skip = "") {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path folder = fs::path(dir) / sub;
    if (!fs::is_directory(folder, ec)) { out.clear(); return true; }
    std::vector<std::string> files;
    for (auto& entry : fs::directory_iterator(folder, ec))
        if (entry.path().extension() == ".json" && entry.path().filename() != skip) files.push_back(entry.path().string());
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
           read_text(dir + "/text/en/narrator.json", d.text, err) && read_text(dir + "/importance.json", d.importance, err) &&
           text::bundles_dir_json(dir + "/text", d.text_bundles, err) && read_folder(dir, "events", d.events, err) &&
           read_dragon_files(dir, d) && read_optional(dir + "/hooks.json", d.hooks) && read_optional(dir + "/arrivals.json", d.arrivals) &&   // v0.4 hooks (S3), item 06
           read_optional(dir + "/genesis.json", d.genesis) && read_optional(dir + "/endings.json", d.endings) &&   // item 04
           read_optional(dir + "/moves.json", d.moves) &&   // item 07
           read_folder(dir, "patterns", d.patterns, err, "base_rates.json") && read_optional(dir + "/patterns/base_rates.json", d.base_rates);   // item 14
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
    return policy_from_json(j.dump(), out, err);
}

// item 04: the profile's five files
static const char* const kProfileFiles[][2] = {{"legends", "legends.json"}, {"ending_atoms", "ending_atoms.json"}, {"seen", "seen.json"},
                                                {"bestiary", "bestiary.json"}, {"glossary", "glossary.json"}};

bool load_profile(const std::string& dir, Profile& out, std::string& err) {
    json all = json::object();
    for (auto& f : kProfileFiles) {
        std::string text;
        if (!read_optional(dir + "/" + f[1], text) || text.empty()) continue;
        json v = json::parse(text, nullptr, false);
        if (v.is_discarded()) { err = "invalid json in " + dir + "/" + f[1]; return false; }
        all[f[0]] = v;
    }
    out = profile_from_json(all);
    return true;
}

bool save_profile(const std::string& dir, const Profile& pr, std::string& err) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    json all = profile_to_json(pr);
    for (auto& f : kProfileFiles) {
        std::string path = dir + "/" + f[1], tmp = path + ".tmp";
        {
            std::ofstream o(tmp);
            if (!o) { err = "cannot write " + path; return false; }
            o << all[f[0]].dump(1) << "\n";
        }
        std::rename(tmp.c_str(), path.c_str());
    }
    return true;
}


}
