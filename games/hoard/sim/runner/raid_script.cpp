#include "raid_script.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include "ages/act_log.h"
#include "ages/ages.h"
#include "ages/json.h"
#include "ages/memory_tiers.h"
#include "raid/report.h"

using namespace ages;
using nlohmann::json;

// item 15 (tools/raid_report_check.py): {"world": <world json>, "report": <RaidReport>}; the sortie opens on the world as
// it is, the report ends it, and one JSON object comes back: the sortie snapshot, the acts that were logged (schema form),
// refusals, warnings and the courts' dread and awe before and after
int raid_script_cmd(const std::string& data, const std::string& path) {
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
    RaidReport report;
    if (!report_from_json(j.value("report", json::object()).dump(), report, err)) { std::fprintf(stderr, "bad report: %s\n", err.c_str()); return 1; }
    if (!w.sortie.open && !begin_sortie(w, loaded.params, report.sortie ? report.sortie : 1, err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
    json out = {{"snapshot", json::parse(sortie_view_json(loaded.params, w.sortie), nullptr, false)}};
    json before = json::object();
    for (auto& f : w.factions) before[f.id] = {{"dread", dread_eff(f)}, {"awe", awe_eff(f)}};
    SortieResult r = end_sortie(w, loaded.params, report);
    out["ok"] = r.ok;
    if (!r.ok) out["reason"] = r.reason;
    out["acts"] = json::array();
    for (size_t i = r.first_act; i < r.last_act; ++i)
        if (w.acts[i].type != "wound" && w.acts[i].type != "spell") out["acts"].push_back(act_schema_json(w, w.acts[i], static_cast<int>(i)));
    out["refused"] = json::array();
    for (auto& x : r.refused) out["refused"].push_back({{"index", x.index}, {"type", x.type}, {"reason", x.reason}});
    out["warnings"] = r.warnings;
    json after = json::object();
    for (auto& f : w.factions) after[f.id] = {{"dread", dread_eff(f)}, {"awe", awe_eff(f)}};
    out["courts_before"] = before;
    out["courts_after"] = after;
    out["hoard"] = {{"gold", w.dragons[kPlayer].hoard.gold}, {"books", w.dragons[kPlayer].hoard.books}, {"artifacts", w.dragons[kPlayer].hoard.artifacts},
                    {"art", w.dragons[kPlayer].hoard.art}, {"food", w.dragons[kPlayer].hoard.food}};
    std::printf("%s\n", out.dump().c_str());
    return 0;
}
