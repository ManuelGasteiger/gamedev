#include "hook_json.h"
#include "hooks.h"
#include "world_util.h"

namespace ages {

using nlohmann::json;

namespace {

template <class T>
void get(const json& j, const char* key, T& out) {
    auto it = j.find(key);
    if (it != j.end() && !it->is_null()) out = it->get<T>();
}

}

json hook_json(const World& w, const Params& p, const Hook& h) {
    json resolve = {{"acts", h.acts}, {"region", h.region}, {"target", h.faction}, {"person", h.person},
                    {"against", contains(h.acts, "defend") ? h.other : ""}, {"choices", h.choices}};
    return {{"id", h.id}, {"kind", h.kind}, {"source", h.source}, {"title", h.title}, {"text", h.text}, {"region", h.region}, {"faction", h.faction},
            {"person", h.person}, {"other", h.other}, {"resolve", resolve}, {"reward_text", h.reward_text}, {"ignore_text", h.ignore_text},
            {"fight", h.fight}, {"fight_damage", hook_fight_damage(w, p, h)}, {"offered", h.offered}, {"expires_year", h.expires_year}};
}

json hooks_json(const World& w, const Params& p, const std::vector<Hook>& hooks) {
    json arr = json::array();
    for (auto& h : hooks) arr.push_back(hook_json(w, p, h));
    return arr;
}

void hooks_state_to_json(const World& w, const Params& p, json& j) {
    j["hooks"] = hooks_json(w, p, w.hooks);
    j["hook_fired"] = w.hook_fired;
}

void hooks_state_from_json(const json& j, World& w) {
    get(j, "hook_fired", w.hook_fired);
    if (!j.contains("hooks") || !j["hooks"].is_array()) return;
    for (auto& v : j["hooks"]) {
        Hook h;
        get(v, "id", h.id);
        get(v, "kind", h.kind);
        get(v, "source", h.source);
        get(v, "title", h.title);
        get(v, "text", h.text);
        get(v, "region", h.region);
        get(v, "faction", h.faction);
        get(v, "person", h.person);
        get(v, "other", h.other);
        if (v.contains("resolve")) { get(v["resolve"], "acts", h.acts); get(v["resolve"], "choices", h.choices); }
        get(v, "reward_text", h.reward_text);
        get(v, "ignore_text", h.ignore_text);
        get(v, "fight", h.fight);
        get(v, "offered", h.offered);
        get(v, "expires_year", h.expires_year);
        if (!h.id.empty()) w.hooks.push_back(h);
    }
}

void forecast_hook_to_json(const Forecast& f, json& j) {
    if (f.hook_kind.empty()) return;
    j["hook_preview"] = {{"kind", f.hook_kind}, {"title", f.hook_title}, {"region", f.hook_region}};
}

}
