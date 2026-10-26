#include "ages.h"
#include "json.h"

namespace ages {

using nlohmann::json;

template <class T>
static void get(const json& j, const char* key, T& out) {
    auto it = j.find(key);
    if (it != j.end()) out = it->get<T>();
}

bool policy_from_json(const std::string& text, Policy& out, std::string& err) {
    json j = json::parse(text, nullptr, false);
    if (j.is_discarded() || !j.is_object()) { err = "invalid policy json"; return false; }
    get(j, "cycle", out.cycle);
    if (j.contains("acts")) {
        for (auto& v : j["acts"]) {
            PolicyAct a;
            get(v, "type", a.type);
            get(v, "target", a.target);
            get(v, "region", a.region);
            get(v, "against", a.against);
            get(v, "magnitude", a.magnitude);
            get(v, "payload_share", a.payload_share);
            out.acts.push_back(a);
        }
    }
    get(j, "talents", out.talents);       // v0.4 (S1)
    get(j, "sleep_cycle", out.sleep_cycle);
    get(j, "min_sleep", out.min_sleep);
    get(j, "sleep_open", out.sleep_open);
    get(j, "avoid_rings", out.avoid_rings);
    get(j, "act_wakings", out.act_wakings);
    if (j.contains("answer_hooks")) {   // v0.4 hooks (S3)
        const json& h = j["answer_hooks"];
        get(h, "acts", out.hook_acts);
        get(h, "choice", out.hook_choice);
        get(h, "magnitude", out.hook_magnitude);
        get(h, "payload_share", out.hook_payload);
        get(h, "wound_cap", out.hook_wound_cap);
        get(h, "reach", out.hook_reach);
        get(h, "max", out.hook_max);
    }
    return true;
}

}
