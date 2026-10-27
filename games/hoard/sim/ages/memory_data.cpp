#include "memory_data.h"

namespace ages {

using nlohmann::json;

namespace {

template <class T>
void get(const json& j, const char* key, T& out) {
    auto it = j.find(key);
    if (it != j.end()) out = it->get<T>();
}

}

void memory_constants_from_json(const json& j, MemoryConstants& m) {
    get(j, "oral_halflife", m.oral_halflife);
    get(j, "written_halflife", m.written_halflife);
    get(j, "monumental_halflife", m.monumental_halflife);
    get(j, "oral_reference", m.oral_reference);
    get(j, "written_deposit", m.written_deposit);
    get(j, "monumental_deposit", m.monumental_deposit);
    get(j, "rep_years", m.rep_years);
    get(j, "rep_floor", m.rep_floor);
    get(j, "witness_owner", m.witness_owner);
    get(j, "witness_neighbour", m.witness_neighbour);
    get(j, "library_burn", m.library_burn);
    get(j, "temple_convert", m.temple_convert);
    get(j, "entry_drop", m.entry_drop);
    get(j, "tier_shown", m.tier_shown);
    get(j, "kept_oath_awe", m.kept_oath_awe);
    get(j, "pilgrims_awe", m.pilgrims_awe);
    get(j, "honest_awe", m.honest_awe);
    get(j, "cult_awe", m.cult_awe);
    get(j, "temple_awe", m.temple_awe);
    get(j, "faith_floor", m.faith_floor);
    get(j, "temple_years", m.temple_years);
    get(j, "demand_dread", m.demand_dread);
    get(j, "slayer_dread", m.slayer_dread);
    get(j, "flee_dread", m.flee_dread);
    get(j, "grievance_written", m.grievance_written);
    get(j, "stability_dread", m.stability_dread);
    get(j, "pressure_awe", m.pressure_awe);
    get(j, "research_dread", m.research_dread);
    get(j, "research_base", m.research_base);
    get(j, "research_spend", m.research_spend);
    get(j, "research_wealth_half", m.research_wealth_half);
}

bool reactions_from_json(const json& j, Params& p, std::string& err) {
    if (!j.is_object() || !j.contains("gains") || !j.contains("rows")) { err = "reactions: needs gains and rows"; return false; }
    for (auto it = j["gains"].begin(); it != j["gains"].end(); ++it) {
        Gain g;
        get(it.value(), "dread", g.dread);
        get(it.value(), "awe", g.awe);
        p.gains[it.key()] = g;
    }
    for (auto& v : j["rows"]) {
        Reaction r;
        get(v, "acts", r.acts);
        get(v, "relationship", r.relationship);
        get(v, "who", r.who);
        get(v, "dread_mult", r.dread_mult);
        get(v, "awe_mult", r.awe_mult);
        get(v, "faith_per_m", r.faith_per_m);
        get(v, "faith_flat", r.faith_flat);
        get(v, "anti_dragon_per_m", r.anti_dragon_per_m);
        if (v.contains("faith_set")) { r.has_faith_set = true; r.faith_set = v["faith_set"].get<double>(); }
        if (v.contains("tags")) for (auto& t : v["tags"]) r.tags.insert(t.get<std::string>());
        p.reactions.push_back(r);
    }
    return true;
}

}
