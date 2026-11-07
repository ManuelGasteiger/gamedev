#include "data.h"
#include "json.h"

namespace ages {

using nlohmann::json;

namespace {

template <class T>
void get(const json& j, const char* key, T& out) {
    auto it = j.find(key);
    if (it != j.end() && !it->is_null()) out = it->get<T>();
}

MemoryRow row(const json& j) {
    MemoryRow r;
    get(j, "ev", r.ev);
    get(j, "sev", r.sev);
    if (j.contains("ax")) {
        get(j["ax"], "dread", r.dread);
        get(j["ax"], "awe", r.awe);
        get(j["ax"], "debt", r.debt);
    }
    return r;
}

}

bool slayers_from_json(const DataJson& d, Params& out, std::string& err) {
    if (d.slayers.empty()) return true;
    json j = json::parse(d.slayers, nullptr, false);
    if (j.is_discarded()) { err = "slayers.json: not valid JSON"; return false; }
    SlayerBook& b = out.slayers;
    b.present = true;
    if (j.contains("found")) {
        get(j["found"], "court_magnitude", b.court_magnitude);
        get(j["found"], "court_weight", b.court_weight);
        get(j["found"], "person_sev", b.person_sev);
        get(j["found"], "person_s", b.person_s);
        get(j["found"], "first_slayers", b.first_slayers);
    }
    if (j.contains("ranks")) {
        get(j["ranks"], "masters", b.masters);
        get(j["ranks"], "slayers", b.slayers);
    }
    if (j.contains("power")) {
        const json& v = j["power"];
        get(v, "start_min", b.power_start_min);
        get(v, "start_max", b.power_start_max);
        get(v, "cap", b.power_cap);
        get(v, "hunter", b.hunter_power);
        get(v, "ready_ratio", b.ready_ratio);
        get(v, "grandmaster_odds", b.grandmaster_odds);
        get(v, "counter_mult", b.counter_mult);
        get(v, "hate_bonus", b.hate_bonus);
    }
    get(j, "counters", b.counters);
    get(j, "hates", b.hates);
    get(j, "fears", b.fears);
    get(j, "zealots", b.zealots);
    get(j, "opportunists", b.opportunists);
    if (j.contains("families"))
        for (auto it = j["families"].begin(); it != j["families"].end(); ++it) {
            FamilyDef f;
            get(it.value(), "fear", f.fear);
            get(it.value(), "hates", f.hates);
            b.families[it.key()] = f;
        }
    get(j, "act_family", b.act_family);
    get(j, "scars", b.scars);
    get(j, "scar_max", b.scar_max);
    if (j.contains("missions")) {
        const json& m = j["missions"];
        if (m.contains("train")) { get(m["train"], "weight", b.w_train); get(m["train"], "hunters", b.train_hunters); }
        if (m.contains("hunt")) { get(m["hunt"], "weight", b.w_hunt); get(m["hunt"], "power", b.hunt_power); }
        if (m.contains("seek")) { get(m["seek"], "weight", b.w_seek); get(m["seek"], "artifact", b.seek_artifact); get(m["seek"], "magic_min", b.seek_magic_min); }
        if (m.contains("march")) get(m["march"], "weight", b.w_march);
    }
    get(j, "relative_div", b.relative_div);
    get(j, "hunters_per_slayer", b.hunters_per_slayer);
    get(j, "hunters_max", b.hunters_max);
    get(j, "recruit_chance", b.recruit_chance);
    get(j, "hunters_start", b.hunters_start);
    get(j, "peace_years", b.peace_years);
    get(j, "peace_loss", b.peace_loss);
    get(j, "rel", b.rel);
    get(j, "kin_chance", b.kin_chance);
    if (j.contains("fight")) {
        get(j["fight"], "base_share", b.fight_base_share);
        get(j["fight"], "death_ratio", b.fight_death_ratio);
        get(j["fight"], "artifact", b.fight_artifact);
    }
    if (j.contains("march")) {
        get(j["march"], "death", b.march_death);
        get(j["march"], "hunters_lost", b.march_hunters_lost);
        get(j["march"], "promote", b.march_promote);
        get(j["march"], "rival_gain", b.rival_gain);
    }
    if (j.contains("gate")) get(j["gate"], "death", b.gate_death);
    get(j, "ally_promote", b.ally_promote);
    if (j.contains("reveal")) { get(j["reveal"], "hostage", b.reveal_hostage); get(j["reveal"], "source", b.reveal_source); }
    get(j, "inherit_negative", b.inherit_negative);
    if (j.contains("memory")) {
        const json& m = j["memory"];
        if (m.contains("grief")) b.grief = row(m["grief"]);
        if (m.contains("mentor_grief")) b.mentor_grief = row(m["mentor_grief"]);
        if (m.contains("humiliated")) b.humiliated = row(m["humiliated"]);
        if (m.contains("scarred")) b.scarred = row(m["scarred"]);
    }
    if (j.contains("estimate")) {
        get(j["estimate"], "min_years", b.estimate_min);
        get(j["estimate"], "max_years", b.estimate_max);
        get(j["estimate"], "round", b.estimate_round);
    }
    return true;
}

}
