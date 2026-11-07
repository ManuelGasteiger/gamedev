#include "aging_json.h"

namespace ages {

using nlohmann::json;

void aging_fields_to_json(const World& w, json& j) {
    for (size_t i = 0; i < w.regions.size() && i < j["regions"].size(); ++i) {
        const Region& r = w.regions[i];
        json& o = j["regions"][i];
        if (r.last_burn_year >= 0) { o["last_burn_year"] = r.last_burn_year; o["destroyed_share_at_last_burn"] = r.destroyed_share_at_last_burn; }
        if (r.ruin_year >= 0) o["ruin_year"] = r.ruin_year;
        if (r.abandoned_year >= 0) o["abandoned_year"] = r.abandoned_year;
        if (r.resettled_year >= 0) o["resettled_year"] = r.resettled_year;
    }
}

void aging_fields_from_json(const json& j, World& w) {
    if (!j.contains("regions")) return;
    for (auto& v : j["regions"]) {
        std::string id = v.value("id", "");
        for (auto& r : w.regions) {
            if (r.id != id) continue;
            r.last_burn_year = v.value("last_burn_year", -1);
            r.destroyed_share_at_last_burn = v.value("destroyed_share_at_last_burn", 0.0);
            r.ruin_year = v.value("ruin_year", -1);
            r.abandoned_year = v.value("abandoned_year", -1);
            r.resettled_year = v.value("resettled_year", -1);
        }
    }
}

}
