#include "act_script.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include "ages/ages.h"
#include "ages/json.h"

using namespace ages;
using nlohmann::json;

// item 09 (tools/act_scenarios.py): {"world": <world json>, "acts": [...]} applied one act at a time; one JSON line per act
int act_script_cmd(const std::string& data, const std::string& path) {
    std::ifstream in(path);
    if (!in) { std::fprintf(stderr, "cannot open %s\n", path.c_str()); return 1; }
    std::stringstream ss;
    ss << in.rdbuf();
    json j = json::parse(ss.str(), nullptr, false);
    if (j.is_discarded() || !j.is_object()) { std::fprintf(stderr, "bad script\n"); return 1; }
    World w;
    std::string err;
    if (!world_from_json(j.value("world", json::object()).dump(), w, err)) { std::fprintf(stderr, "bad world: %s\n", err.c_str()); return 1; }
    LoadResult loaded = load_data(data, w.seed);
    if (!loaded.ok) { std::fprintf(stderr, "load failed: %s\n", loaded.error.c_str()); return 1; }
    const json& acts = j.value("acts", json::array());
    for (size_t i = 0; i < acts.size(); ++i) {
        std::vector<Act> one;
        if (!acts_from_json(json::array({acts[i]}).dump(), one, err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
        std::vector<ActRefusal> refused;
        size_t logged = w.acts.size();
        apply_acts(w, loaded.params, one, &refused);
        json line = {{"i", i}, {"type", one.empty() ? "" : one[0].type}, {"accepted", refused.empty()}};
        if (!refused.empty()) line["reason"] = refused.front().reason;
        if (w.acts.size() > logged) line["logged"] = w.acts.back().args;
        std::printf("%s\n", line.dump().c_str());
    }
    return 0;
}
