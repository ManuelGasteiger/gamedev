#include "raid_api.h"
#include "ages/act_log.h"
#include "ages/chron.h"
#include "ages/importance.h"
#include "ages/memory_tiers.h"
#include "ages/world_util.h"
#include "ages/wounds.h"
#include "raid/report.h"
#include "text_api.h"

using namespace ages;
using nlohmann::json;

namespace {

json fail(const std::string& msg) { return {{"ok", false}, {"error", msg}}; }
json fail_reason(const std::string& reason) { return {{"ok", false}, {"reason", reason}, {"error", text_reason(reason)}}; }

json court_snapshot(const World& w) {
    json j = json::object();
    for (auto& f : w.factions) j[f.id] = {{"dread", dread_eff(f)}, {"awe", awe_eff(f)}, {"faith", f.faith}, {"wealth", f.wealth}};
    return j;
}

json events_since(const World& w, const Params& p, size_t first) {
    std::vector<Event> events(w.chronicle.begin() + static_cast<std::ptrdiff_t>(first), w.chronicle.end());
    for (auto& e : events) if (e.importance < 0) e.importance = event_importance(w, p, e);
    return json::parse(chronicle_to_json(events), nullptr, false);
}

json coded(const std::vector<std::string>& codes) {
    json out = json::array();
    for (auto& c : codes) out.push_back({{"reason", c}, {"reason_text", text_reason(c)}});
    return out;
}

}

json raid_begin_sortie(World& w, const Params& p, const json& in) {
    std::string reason;
    if (!begin_sortie(w, p, in.value("sortie", 1), reason)) return fail_reason(reason);
    json out = json::parse(sortie_view_json(p, w.sortie), nullptr, false);
    out["ok"] = true;
    return out;
}

json raid_end_sortie(World& w, const Params& p, const json& in) {
    RaidReport report;
    std::string err;
    if (!report_from_json(in.value("report", json::object()).dump(), report, err)) return fail(err);
    json before = court_snapshot(w);
    std::vector<Hook> hooks = w.hooks;
    size_t first_event = w.chronicle.size();
    double wounds = w.dragons[kPlayer].wounds;
    SortieResult r = end_sortie(w, p, report);
    if (!r.ok) return fail_reason(r.reason);
    json acts = json::array();
    for (size_t i = r.first_act; i < r.last_act; ++i)
        if (w.acts[i].type != "wound" && w.acts[i].type != "spell") acts.push_back(act_schema_json(w, w.acts[i], static_cast<int>(i)));
    json changed = json::array();
    json after = court_snapshot(w);
    std::vector<std::string> order;
    for (auto& a : r.derived) order.push_back(a.target);
    for (auto& f : w.factions) order.push_back(f.id);
    for (auto& id : order) {
        if (!before.contains(id) || !after.contains(id) || before[id] == after[id]) continue;
        bool seen = false;
        for (auto& c : changed) if (c["id"] == id) seen = true;
        if (!seen) changed.push_back({{"id", id}, {"before", before[id]}, {"after", after[id]}});
    }
    json refused = json::array();
    for (auto& x : r.refused) refused.push_back({{"index", x.index}, {"type", x.type}, {"reason", x.reason}, {"reason_text", text_reason(x.reason)}});
    json resolved = json::array();
    for (auto& h : hooks) {
        bool open = false;
        for (auto& x : w.hooks) if (x.id == h.id) open = true;
        if (!open) resolved.push_back({{"id", h.id}, {"kind", h.kind}, {"reward_text", h.reward_text}});
    }
    json out = {{"ok", true}, {"shell", r.shell}, {"acts", acts}, {"refused", refused}, {"changed", changed},
                {"events", events_since(w, p, first_event)}, {"lines", json::parse(reaction_lines_to_json(r.lines), nullptr, false)},
                {"hooks_resolved", resolved}, {"warnings", coded(r.warnings)},
                {"wounds", {{"taken", w.dragons[kPlayer].wounds - wounds}, {"total", w.dragons[kPlayer].wounds}, {"max", health_max(w, p)}}},
                {"slain", dragon_dead(w)}};
    return out;
}
