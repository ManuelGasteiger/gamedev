#include "genesis_json.h"

namespace ages {

using nlohmann::json;

void genesis_fields_to_json(const World& w, json& j) {
    for (size_t i = 0; i < w.regions.size() && i < j["regions"].size(); ++i) {
        j["regions"][i]["name"] = w.regions[i].name;
        j["regions"][i]["empty_years"] = w.regions[i].empty_years;
    }
    const Genesis& g = w.genesis;
    j["genesis"] = {{"template", g.template_id}, {"prehistory_years", g.prehistory_years}, {"hatched_year", g.hatched_year}, {"deal", g.deal}};
    j["last_major"] = w.last_major;
    j["story"] = {{"quiet_wakings", w.story_quiet_wakings}, {"breather", w.story_breather}};
}

void genesis_fields_from_json(const json& j, World& w) {
    if (j.contains("regions"))
        for (auto& v : j["regions"]) {
            std::string id = v.value("id", "");
            for (auto& r : w.regions) {
                if (r.id != id) continue;
                r.name = v.value("name", "");
                r.empty_years = v.value("empty_years", 0);
            }
        }
    if (j.contains("genesis")) {
        const json& g = j["genesis"];
        w.genesis.template_id = g.value("template", "");
        w.genesis.prehistory_years = g.value("prehistory_years", 0);
        w.genesis.hatched_year = g.value("hatched_year", 0);
        if (g.contains("deal")) w.genesis.deal = g["deal"].get<std::map<std::string, std::string>>();
    }
    w.last_major = j.value("last_major", 0);
    if (j.contains("story")) {
        w.story_quiet_wakings = j["story"].value("quiet_wakings", 0);
        w.story_breather = j["story"].value("breather", false);
    }
}

}
