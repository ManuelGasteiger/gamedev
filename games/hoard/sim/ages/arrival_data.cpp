#include "data.h"
#include "json.h"

// item 06: data/arrivals.json; a missing file loads as no arrivals

namespace ages {

using nlohmann::json;

namespace {

template <class T>
void get(const json& j, const char* key, T& out) {
    auto it = j.find(key);
    if (it != j.end() && !it->is_null()) out = it->get<T>();
}

void load_gates(const json& v, const char* key, std::vector<ArrivalGateDef>& out) {
    if (!v.contains(key)) return;
    for (auto& g : v[key]) {
        ArrivalGateDef d;
        get(g, "gate", d.gate);
        get(g, "unless", d.unless);
        get(g, "event", d.event);
        get(g, "faith_pool", d.faith_pool);
        d.has_gte = g.contains("gte"); if (d.has_gte) get(g, "gte", d.gte);
        d.has_lte = g.contains("lte"); if (d.has_lte) get(g, "lte", d.lte);
        d.has_gt = g.contains("gt"); if (d.has_gt) get(g, "gt", d.gt);
        d.has_lt = g.contains("lt"); if (d.has_lt) get(g, "lt", d.lt);
        out.push_back(d);
    }
}

}

bool arrivals_from_json(const DataJson& d, Params& p, std::string& err) {
    if (d.arrivals.empty()) return true;
    json j = json::parse(d.arrivals, nullptr, false);
    if (j.is_discarded()) { err = "invalid json in arrivals"; return false; }
    ArrivalBook& b = p.arrivals;
    get(j, "oral_floor", b.oral_floor);
    get(j, "lure_gold", b.lure_gold);
    get(j, "hunt_food", b.hunt_food);
    get(j, "slopes_hunt_mult", b.slopes_hunt_mult);
    get(j, "rumour_years", b.rumour_years);
    get(j, "thief_era", b.thief_era);
    get(j, "refugee_years", b.refugee_years);
    get(j, "envoy_kill_generations", b.envoy_kill_generations);
    get(j, "sight_regions", b.sight_regions);
    if (j.contains("kinds"))
        for (auto& v : j["kinds"]) {
            ArrivalDef a;
            get(v, "kind", a.kind);
            if (a.kind.empty()) { err = "arrivals: a row without a kind"; return false; }
            get(v, "person_role", a.person_role);
            get(v, "window", a.window);
            get(v, "text_key", a.text_key);
            get(v, "cause", a.cause);
            get(v, "specificity", a.specificity);
            get(v, "cooldown_years", a.cooldown_years);
            get(v, "ignores_decade_limit", a.ignores_decade_limit);
            get(v, "answers", a.answers);
            load_gates(v, "requires", a.requires_);
            load_gates(v, "blocked_when", a.blocked_when);
            b.kinds.push_back(a);
        }
    return true;
}

}
