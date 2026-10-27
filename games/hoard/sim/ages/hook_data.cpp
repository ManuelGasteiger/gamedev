#include "card_preds.h"
#include "data.h"
#include "json.h"

// v0.4 hooks (S3): data/hooks.json; a missing file loads as no hooks

namespace ages {

using nlohmann::json;

namespace {

template <class T>
void get(const json& j, const char* key, T& out) {
    auto it = j.find(key);
    if (it != j.end() && !it->is_null()) out = it->get<T>();
}

bool load_template(const json& v, HookDef& h, std::string& err) {
    get(v, "id", h.id);
    get(v, "kind", h.kind);
    if (h.kind.empty()) h.kind = h.id;
    if (h.id.empty()) { err = "hook without id"; return false; }
    get(v, "scope", h.scope);
    if (v.contains("when") && !card_preds_from_json(v["when"], h.when, err)) return false;
    if (v.contains("force_if") && !card_preds_from_json(v["force_if"], h.force_if, err)) return false;
    get(v, "where", h.where);
    get(v, "pick", h.pick);
    get(v, "arrival", h.arrival);
    get(v, "acts", h.acts);
    get(v, "choices", h.choices);
    get(v, "priority", h.priority);
    get(v, "fallback", h.fallback);
    get(v, "cooldown_years", h.cooldown_years);
    get(v, "reward", h.reward);
    get(v, "ignore", h.ignore);
    get(v, "choice_reward", h.choice_reward);
    get(v, "text", h.text);
    if (v.contains("fight")) {
        const json& f = v["fight"];
        get(f, "damage", h.fight_damage);
        get(f, "era_scale", h.fight_era_scale);
        get(f, "strength_mult", h.fight_strength_mult);
        get(f, "health_share", h.fight_health_share);
        get(f, "defence", h.fight_defence);
    }
    for (auto& c : h.choices)
        if (!h.choice_reward.count(c)) { err = "hook " + h.id + " has no reward for choice " + c; return false; }
    return true;
}

}

bool hooks_from_json(const DataJson& d, Params& p, std::string& err) {
    if (d.hooks.empty()) return true;
    json j = json::parse(d.hooks, nullptr, false);
    if (j.is_discarded()) { err = "invalid json in hooks"; return false; }
    HookBook& b = p.hooks;
    get(j, "max_hooks", b.max_hooks);
    get(j, "second", b.second);
    get(j, "third", b.third);
    get(j, "repeat_mult", b.repeat_mult);
    if (j.contains("templates"))
        for (auto& v : j["templates"]) {
            HookDef h;
            if (!load_template(v, h, err)) { err = "hooks: " + err; return false; }
            b.templates.push_back(h);
        }
    std::sort(b.templates.begin(), b.templates.end(), [](const HookDef& a, const HookDef& c) { return a.id < c.id; });
    return true;
}

}
