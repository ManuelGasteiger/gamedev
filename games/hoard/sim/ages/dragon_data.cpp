#include "data.h"
#include "json.h"

// v0.4 (S1): constants.json "dragon", data/talents.json and data/defences.json; a missing file loads as empty

namespace ages {

using nlohmann::json;

namespace {

template <class T>
void get(const json& j, const char* key, T& out) {
    auto it = j.find(key);
    if (it != j.end() && !it->is_null()) out = it->get<T>();
}

bool parse(const std::string& text, const char* name, json& out, std::string& err) {
    out = json::parse(text, nullptr, false);
    if (out.is_discarded()) { err = std::string("invalid json in ") + name; return false; }
    return true;
}

void load_constants(const json& j, Constants& c) {
    get(j, "halflife", c.growth_halflife);
    get(j, "points", c.growth_points);
    get(j, "bed_rate", c.bed_rate);
    get(j, "bed_overtime", c.bed_overtime);
    get(j, "bed_mode", c.bed_mode);
    get(j, "gold_burn", c.gold_burn);
    get(j, "gold_floor_base", c.gold_floor_base);
    get(j, "floor_phase_mult", c.floor_phase_mult);
    get(j, "gold_pts", c.gold_pts);
    get(j, "size_per_might", c.size_per_might);
    get(j, "phase_points", c.phase_points);
    get(j, "health_base", c.health_base);
    get(j, "health_per_might", c.health_per_might);
    get(j, "premonition_decades", c.premonition_decades);
    get(j, "molten_gold_burn", c.molten_gold_burn);
    get(j, "clinging_recovery_mult", c.clinging_recovery_mult);
    get(j, "hex_anti_dragon_mult", c.hex_anti_dragon_mult);
    get(j, "cult_faith", c.cult_faith);
    get(j, "brood_faith", c.brood_faith);
    get(j, "anti_dragon_marks", c.anti_dragon_marks);
    get(j, "dread_research", c.dread_research);
    get(j, "start_hoard", c.start_hoard);
    if (j.contains("slayers")) {
        const json& s = j["slayers"];
        get(s, "lapse_dread", c.slay_lapse_dread);
        get(s, "ready_rate", c.slay_ready_rate);
        get(s, "health_div", c.slay_health_div);
        get(s, "points_base", c.slay_points_base);
        get(s, "points_hoard", c.slay_points_hoard);
        get(s, "points_growth", c.slay_points_growth);
        get(s, "adapt_gain", c.adapt_gain);
        get(s, "adapt_gain_negative", c.adapt_gain_negative);
        get(s, "adapt_loss", c.adapt_loss);
        get(s, "adapt_base", c.adapt_base);
        get(s, "adapt_step", c.adapt_step);
        get(s, "adapt_min", c.adapt_min);
        get(s, "adapt_max", c.adapt_max);
        get(s, "near_death_wounds", c.near_death_wounds);
    }
}

TalentLook look_from(const json& j) {
    TalentLook l;
    for (auto it = j.begin(); it != j.end(); ++it) {
        if (it.value().is_number()) l.num[it.key()] = it.value().get<double>();
        else if (it.value().is_string()) l.str[it.key()] = it.value().get<std::string>();
    }
    return l;
}

bool load_talents(const json& j, TalentBook& b, std::string& err) {
    get(j, "flight", b.flight);
    get(j, "nodes", b.nodes);
    get(j, "node_phase", b.node_phase);
    b.node_phase.resize(b.nodes.size(), 1);
    if (j.contains("axes"))
        for (auto& v : j["axes"]) {
            TalentAxis a;
            get(v, "id", a.id);
            get(v, "name", a.name);
            get(v, "fed_by", a.fed_by);
            get(v, "rank_every", a.rank_every);
            get(v, "rank_growth", a.rank_growth);
            if (v.contains("rank")) {
                get(v["rank"], "effects", a.rank_effects);
                get(v["rank"], "text", a.rank_text);
                if (v["rank"].contains("look")) a.rank_look = look_from(v["rank"]["look"]);
            }
            if (v.contains("milestone")) {
                get(v["milestone"], "every", a.milestone_every);
                get(v["milestone"], "effects", a.milestone_effects);
                if (v["milestone"].contains("look")) a.milestone_look = look_from(v["milestone"]["look"]);
            }
            b.axes.push_back(a);
        }
    if (j.contains("talents"))
        for (auto& v : j["talents"]) {
            TalentDef t;
            get(v, "id", t.id);
            get(v, "axis", t.axis);
            get(v, "node", t.node);
            get(v, "name", t.name);
            get(v, "text", t.text);
            get(v, "effects", t.effects);
            if (v.contains("look")) t.look = look_from(v["look"]);
            if (t.node < 0 || t.node >= static_cast<int>(b.nodes.size())) { err = "talent " + t.id + " has no node " + std::to_string(t.node); return false; }
            if (b.index.count(t.id)) { err = "talent " + t.id + " defined twice"; return false; }
            b.index[t.id] = b.talents.size();
            b.talents.push_back(t);
        }
    return true;
}

void load_defences(const json& j, DefenceBook& b) {
    get(j, "hero_bonus", b.hero_bonus);
    if (!j.contains("defences")) return;
    for (auto& v : j["defences"]) {
        DefenceDef d;
        get(v, "id", d.id);
        get(v, "label", d.label);
        get(v, "min_era", d.min_era);
        get(v, "min_anti_dragon", d.min_anti_dragon);
        get(v, "tags_any", d.tags_any);
        get(v, "races_any", d.races_any);
        get(v, "ward_modifier", d.ward_modifier);
        get(v, "per_act", d.per_act);
        get(v, "passive", d.passive);
        get(v, "radius", d.radius);
        get(v, "burn_mult", d.burn_mult);
        get(v, "era_scale", d.era_scale);
        get(v, "fearless", d.fearless);
        b.defences.push_back(d);
    }
}

}

bool dragon_data_from_json(const DataJson& d, Params& p, std::string& err) {
    json j;
    if (!parse(d.constants, "constants", j, err)) return false;
    if (j.contains("dragon")) load_constants(j["dragon"], p.c);
    if (p.c.floor_phase_mult.size() < 5 || (p.c.bed_mode != "consumed" && p.c.bed_mode != "window")) { err = "constants: dragon.floor_phase_mult needs 5 entries and bed_mode is consumed or window"; return false; }
    if (!d.talents.empty()) {
        if (!parse(d.talents, "talents", j, err) || !load_talents(j, p.talents, err)) return false;
    }
    if (!d.defences.empty()) {
        if (!parse(d.defences, "defences", j, err)) return false;
        load_defences(j, p.defences);
    }
    return true;
}

}
