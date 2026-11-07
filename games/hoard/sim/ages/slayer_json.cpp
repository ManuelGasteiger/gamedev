#include "slayer_json.h"
#include "slayer_roster.h"

namespace ages {

using nlohmann::json;

void slayer_person_json(const Person& x, json& j) {
    const Slayer& s = x.slayer;
    if (s.rank.empty()) return;
    json v = {{"rank", s.rank}, {"power", s.power}, {"counter", s.counter}, {"hate", s.hate}, {"fear", s.fear}, {"last_fight", s.last_fight}};
    if (!s.scars.empty()) v["scars"] = s.scars;
    if (!s.known.empty()) v["known"] = s.known;
    if (s.armed) v["armed"] = true;
    j["slayer"] = v;
}

void slayer_person_from(const json& j, Person& x) {
    if (!j.contains("slayer")) return;
    const json& v = j["slayer"];
    Slayer& s = x.slayer;
    s.rank = v.value("rank", "");
    s.power = v.value("power", 0);
    s.counter = v.value("counter", "");
    s.hate = v.value("hate", "");
    s.fear = v.value("fear", "");
    s.last_fight = v.value("last_fight", 0);
    if (v.contains("scars")) s.scars = v["scars"].get<std::vector<std::string>>();
    if (v.contains("known")) s.known = v["known"].get<std::vector<std::string>>();
    s.armed = v.value("armed", false);
}

void orders_json(const World& w, json& j) {
    if (w.orders.empty()) return;
    j["orders"] = json::array();
    for (auto& o : w.orders)
        j["orders"].push_back({{"id", o.id}, {"court", o.court}, {"founded", o.founded}, {"grievance", o.grievance}, {"hunters", o.hunters}, {"power_total", o.power_total}});
}

void orders_from(const json& j, World& w) {
    if (!j.contains("orders")) return;
    for (auto& v : j["orders"]) {
        Order o;
        o.id = v.value("id", "");
        o.court = v.value("court", "");
        o.founded = v.value("founded", 0);
        o.grievance = v.value("grievance", 0);
        o.hunters = v.value("hunters", 0);
        o.power_total = v.value("power_total", 0.0);
        w.orders.push_back(o);
    }
}

void scrub_unknown(const Person& x, json& j) {
    if (x.slayer.rank.empty() || !j.contains("slayer")) return;
    json& v = j["slayer"];
    for (const char* k : kTraitKinds) if (!knows(x, k)) v[k] = "?";
}

}
