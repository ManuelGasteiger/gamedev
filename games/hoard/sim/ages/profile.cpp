#include "profile.h"

namespace ages {

using nlohmann::json;

json legends_to_json(const std::vector<LegendEntry>& legends) {
    json arr = json::array();
    for (auto& l : legends) {
        json deeds = json::array();
        for (auto& d : l.deeds) deeds.push_back({{"type", d.type}, {"target", d.target}, {"year", d.year}, {"weight", d.weight}});
        arr.push_back({{"id", l.id}, {"world_seed", l.world_seed}, {"name", l.name}, {"reign", l.reign}, {"span", {l.hatched, l.died}},
                       {"killer", l.killer}, {"slain_by_order", l.slain_by_order}, {"verdict", {{"dread", l.dread}, {"awe", l.awe}}},
                       {"deeds", deeds}, {"tier", l.tier}, {"ending", l.ending}});
    }
    return arr;
}

std::vector<LegendEntry> legends_from_json(const json& j) {
    std::vector<LegendEntry> out;
    if (!j.is_array()) return out;
    for (auto& v : j) {
        LegendEntry l;
        l.id = v.value("id", "");
        l.world_seed = v.value("world_seed", uint64_t{0});
        l.name = v.value("name", "");
        l.reign = v.value("reign", 0);
        if (v.contains("span") && v["span"].size() == 2) { l.hatched = v["span"][0].get<int>(); l.died = v["span"][1].get<int>(); }
        l.killer = v.value("killer", "");
        l.slain_by_order = v.value("slain_by_order", false);
        if (v.contains("verdict")) { l.dread = v["verdict"].value("dread", 0.0); l.awe = v["verdict"].value("awe", 0.0); }
        for (auto& d : v.value("deeds", json::array()))
            l.deeds.push_back({d.value("type", ""), d.value("target", ""), d.value("year", 0), d.value("weight", 0.0)});
        l.tier = v.value("tier", "oral");
        l.ending = v.value("ending", "myth");
        if (!l.id.empty()) out.push_back(l);
    }
    return out;
}

json profile_to_json(const Profile& pr) {
    json atoms = json::object();
    for (auto& e : pr.atoms)
        for (auto& c : e.second) atoms[e.first][c.first] = {{"state", c.second.state}, {"how", c.second.how}};
    json bestiary = json::object();
    for (auto& b : pr.bestiary)
        bestiary[b.first] = {{"kind", b.second.kind}, {"waking", b.second.waking}, {"years_after_hatching", b.second.years_after_hatching}, {"era", b.second.era}};
    json glossary = json::object();
    for (auto& g : pr.glossary) glossary[g.first] = {{"ending", g.second.ending}, {"cond", g.second.cond}, {"confirmed", g.second.confirmed}};
    return {{"legends", legends_to_json(pr.legends)}, {"ending_atoms", atoms}, {"seen", pr.seen}, {"bestiary", bestiary}, {"glossary", glossary}};
}

Profile profile_from_json(const json& j) {
    Profile pr;
    if (!j.is_object()) return pr;
    pr.legends = legends_from_json(j.value("legends", json::array()));
    const json atoms = j.value("ending_atoms", json::object()), bestiary = j.value("bestiary", json::object()), glossary = j.value("glossary", json::object());
    for (auto& e : atoms.items())
        for (auto& c : e.value().items()) pr.atoms[e.key()][c.key()] = {c.value().value("state", ""), c.value().value("how", "")};
    if (j.contains("seen") && j["seen"].is_object()) pr.seen = j["seen"];
    for (auto& b : bestiary.items())
        pr.bestiary[b.key()] = {b.value().value("kind", ""), b.value().value("waking", 0), b.value().value("years_after_hatching", 0), b.value().value("era", 0)};
    for (auto& g : glossary.items())
        pr.glossary[g.key()] = {g.value().value("ending", ""), g.value().value("cond", ""), g.value().value("confirmed", false)};
    return pr;
}

}
