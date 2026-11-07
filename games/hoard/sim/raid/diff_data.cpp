#include "ages/data.h"
#include "ages/json.h"

namespace ages {

using nlohmann::json;

namespace {

template <class T>
void get(const json& j, const char* key, T& out) {
    auto it = j.find(key);
    if (it != j.end() && !it->is_null()) out = it->get<T>();
}

}

bool raid_diff_from_json(const DataJson& d, Params& out, std::string& err) {
    RaidDiffBook& b = out.raid_diff;
    b = RaidDiffBook();
    if (d.raid_diff.empty()) return true;
    json j = json::parse(d.raid_diff, nullptr, false);
    if (j.is_discarded() || !j.is_object()) { err = "invalid json in raid_diff.json"; return false; }
    get(j, "civilian_toll_factor", b.civilian_toll_factor);
    get(j, "structure_threshold", b.structure_threshold);
    get(j, "structures", b.structures);
    get(j, "plain", b.plain);
    get(j, "structure_share", b.structure_share);
    get(j, "oath_defends", b.oath_defends);
    get(j, "kill_person", b.kill_person);
    get(j, "hostage", b.hostage);
    get(j, "repelled", b.repelled);
    get(j, "item", b.item);
    get(j, "holdings", b.holdings);
    if (j.contains("show")) {
        const json& s = j["show"];
        get(s, "land", b.show_land); get(s, "roar", b.show_roar); get(s, "flyover", b.show_flyover); get(s, "flyover_seconds", b.flyover_seconds);
    }
    if (j.contains("snapshot")) {
        const json& s = j["snapshot"];
        get(s, "value_per_wealth", b.value_per_wealth); get(s, "population_per_wealth", b.population_per_wealth);
        get(s, "garrison_per_power", b.garrison_per_power); get(s, "soldiers_per_power", b.soldiers_per_power);
        get(s, "army_per_power", b.army_per_power);
    }
    return true;
}

}
