#include "endings_def.h"
#include "card_preds.h"
#include "json.h"

namespace ages {

using nlohmann::json;

namespace {

template <class T>
void get(const json& j, const char* key, T& out) {
    auto it = j.find(key);
    if (it != j.end() && !it->is_null()) out = it->get<T>();
}

void pair_of(const json& j, const char* key, int& lo, int& hi) {
    if (!j.contains(key) || !j[key].is_array() || j[key].size() < 2) return;
    lo = j[key][0].get<int>();
    hi = j[key][1].get<int>();
}

bool cond_from(const json& j, EndingCond& c, std::string& err) {
    get(j, "id", c.id);
    get(j, "hold_years", c.hold_years);
    get(j, "lapse_years", c.lapse_years);
    get(j, "with_previous", c.with_previous);
    if (j.contains("when") && !card_preds_from_json(j["when"], c.when, err)) { err = "endings: " + c.id + ": " + err; return false; }
    for (auto& r : j.value("rumors", json::array())) {
        RumorRoute route;
        get(r, "via", route.via);
        get(r, "kinds", route.kinds);
        c.rumors.push_back(route);
    }
    return true;
}

}

const EndingDef* ending_def(const EndingsDef& d, const std::string& id) {
    for (auto& e : d.endings) if (e.id == id) return &e;
    return nullptr;
}

bool endings_from_json(const std::string& text, EndingsDef& out, std::string& err) {
    out = EndingsDef();
    if (text.empty()) return true;
    json j = json::parse(text, nullptr, false);
    if (j.is_discarded()) { err = "invalid json in endings.json"; return false; }
    out.present = true;
    get(j, "epilogue_decades", out.epilogue_decades);
    if (j.contains("myth")) get(j["myth"], "deeds", out.myth_deeds);
    if (j.contains("legends")) {
        const json& l = j["legends"];
        LegendWeave& w = out.legends;
        get(l, "weave_chance", w.chance);
        get(l, "max_active", w.max_active);
        pair_of(l, "years", w.year_lo, w.year_hi);
        pair_of(l, "events", w.events_lo, w.events_hi);
        get(l, "tier_decades", w.tier_decades);
        for (auto& v : l.value("weaves", json::array())) {
            WeaveDef d;
            get(v, "kind", d.kind);
            get(v, "verdict", d.verdict);
            get(v, "on", d.on);
            get(v, "tag", d.tag);
            w.weaves.push_back(d);
        }
    }
    if (j.contains("prophecy")) {
        const json& pj = j["prophecy"];
        ProphecyDef& p = out.prophecy;
        get(pj, "ending", p.ending);
        get(pj, "known_tag", p.known_tag);
        get(pj, "book_tag", p.book_tag);
        pair_of(pj, "years_before_hatching", p.years_lo, p.years_hi);
        for (auto& v : pj.value("lines", json::array())) {
            ProphecyLineDef l;
            get(v, "key", l.key);
            get(v, "cond", l.cond);
            get(v, "variants", l.variants);
            get(v, "slots", l.slots);
            p.lines.push_back(l);
        }
    }
    for (auto& v : j.value("endings", json::array())) {
        EndingDef e;
        get(v, "id", e.id);
        get(v, "enabled", e.enabled);
        get(v, "exit_kind", e.exit_kind);
        get(v, "offer_text_key", e.offer_text_key);
        get(v, "epilogue_text_key", e.epilogue_text_key);
        get(v, "sketch", e.sketch);
        for (auto& c : v.value("conditions", json::array())) {
            EndingCond cond;
            if (!cond_from(c, cond, err)) return false;
            e.conds.push_back(cond);
        }
        out.endings.push_back(e);
    }
    return true;
}

}
