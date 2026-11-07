#include "scn.h"

namespace scn {

using nlohmann::json;

json state_json(const ages::ScnState& s) {
    json shown = json::array();
    for (auto& x : s.shown) {
        json j = {{"scenario", x.scenario}, {"slot", x.slot}, {"waking", x.waking}, {"year", x.year}, {"roles", x.roles}, {"cites", x.cites}};
        if (!x.answered.empty()) j["answered"] = x.answered;
        shown.push_back(j);
    }
    return {{"on", s.on}, {"shown", shown}, {"seen_before", s.seen_before}, {"suppress", s.suppress}, {"arrival_open", s.arrival_open},
            {"seek", s.seek}, {"closed", s.closed}};
}

void state_from_json(const json& j, ages::ScnState& s) {
    s = ages::ScnState();
    s.on = j.value("on", false);
    for (auto& x : j.value("shown", json::array())) {
        ages::ScnShown v;
        v.scenario = x.value("scenario", std::string());
        v.slot = x.value("slot", std::string());
        v.answered = x.value("answered", std::string());
        v.waking = x.value("waking", 0);
        v.year = x.value("year", 0);
        if (x.contains("roles")) v.roles = x["roles"].get<std::map<std::string, std::string>>();
        if (x.contains("cites")) v.cites = x["cites"].get<std::vector<int>>();
        s.shown.push_back(v);
    }
    auto ints = [&](const char* k, std::map<std::string, int>& m) { if (j.contains(k)) m = j[k].get<std::map<std::string, int>>(); };
    ints("seen_before", s.seen_before);
    ints("suppress", s.suppress);
    ints("arrival_open", s.arrival_open);
    ints("seek", s.seek);
    if (j.contains("closed")) s.closed = j["closed"].get<std::set<std::string>>();
}

}
