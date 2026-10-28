#include "person_json.h"

// item 07: the person record beside the 17.2 fields; empty parts are left out so older saves hash as written

namespace ages {

using nlohmann::json;

static json mem_json(const std::vector<PersonMemory>& v) {
    json a = json::array();
    for (auto& m : v) {
        json e = {{"ev", m.ev}, {"year", m.year}, {"sev", m.sev}, {"src", m.src}, {"tier", m.tier}, {"s", m.s},
                  {"ax", {{"dread", m.dread}, {"awe", m.awe}, {"debt", m.debt}}}};
        if (m.act >= 0) e["act"] = m.act;
        if (m.hidden) e["hidden"] = true;
        a.push_back(e);
    }
    return a;
}

static std::vector<PersonMemory> mem_from(const json& a) {
    std::vector<PersonMemory> out;
    for (auto& e : a) {
        PersonMemory m;
        m.ev = e.value("ev", "");
        m.year = e.value("year", 0);
        m.sev = e.value("sev", 1);
        m.src = e.value("src", "witness");
        m.tier = e.value("tier", "oral");
        m.s = e.value("s", 1.0);
        m.act = e.value("act", -1);
        m.hidden = e.value("hidden", false);
        if (e.contains("ax")) {
            m.dread = e["ax"].value("dread", 0.0);
            m.awe = e["ax"].value("awe", 0.0);
            m.debt = e["ax"].value("debt", 0.0);
        }
        out.push_back(m);
    }
    return out;
}

void person_record_json(const Person& x, json& j) {
    if (!x.traits.list.empty()) j["traits"]["list"] = x.traits.list;
    if (!x.virtue.empty()) j["virtue"] = x.virtue;
    if (!x.vice.empty()) { j["vice"] = x.vice; j["vice_hidden"] = x.vice_hidden; }
    if (!x.mem.empty()) j["mem"] = mem_json(x.mem);
    if (!x.permanent.empty()) j["permanent"] = mem_json(x.permanent);
    if (!x.rel.empty()) {
        j["rel"] = json::array();
        for (auto& r : x.rel) j["rel"].push_back({{"to", r.to}, {"kind", r.kind}, {"w", r.w}});
    }
    if (x.flips) j["flips"] = x.flips;
}

void person_record_from(const json& j, Person& x) {
    if (j.contains("traits") && j["traits"].contains("list")) x.traits.list = j["traits"]["list"].get<std::vector<std::string>>();
    x.virtue = j.value("virtue", "");
    x.vice = j.value("vice", "");
    x.vice_hidden = j.value("vice_hidden", true);
    if (j.contains("mem")) x.mem = mem_from(j["mem"]);
    if (j.contains("permanent")) x.permanent = mem_from(j["permanent"]);
    if (j.contains("rel"))
        for (auto& r : j["rel"]) x.rel.push_back({r.value("to", ""), r.value("kind", ""), r.value("w", 0)});
    x.flips = j.value("flips", 0);
}

void secrets_json(const World& w, json& j) {
    if (w.secrets.empty()) return;
    j["secrets"] = json::array();
    for (auto& s : w.secrets)
        j["secrets"].push_back({{"holder", s.holder}, {"about", s.about}, {"kind", s.kind}, {"year", s.year}, {"discovery_per_decade", s.discovery_per_decade}});
}

void secrets_from(const json& j, World& w) {
    if (!j.contains("secrets")) return;
    for (auto& v : j["secrets"])
        w.secrets.push_back({v.value("holder", ""), v.value("about", ""), v.value("kind", ""), v.value("year", 0), v.value("discovery_per_decade", 0.0)});
}

}
