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

const char* const kAxes[] = {"might", "flame", "arcana", "lore", "dominion"};

std::vector<GateAlt> alts(const json& arr) {
    std::vector<GateAlt> out;
    if (!arr.is_array()) return out;
    for (auto& v : arr) {
        GateAlt g;
        for (const char* a : kAxes) if (v.contains(a)) g.axes[a] = v[a].get<double>();
        get(v, "talents", g.talents);
        get(v, "phase", g.phase);
        out.push_back(g);
    }
    return out;
}

ActRule rule(const json& v) {
    ActRule r;
    get(v, "target", r.target);
    get(v, "secondary", r.secondary);
    get(v, "numbers", r.numbers);
    get(v, "flags", r.flags);
    get(v, "args", r.args);
    get(v, "pool", r.pool);
    get(v, "magnitude", r.magnitude);
    get(v, "secret", r.secret);
    get(v, "once", r.once);
    get(v, "via", r.via);
    if (v.contains("gates")) r.gates = alts(v["gates"]);
    return r;
}

void numbers(const json& j, ActNumbers& n) {
    get(j, "library_era_rate", n.library_era_rate); get(j, "library_decades", n.library_decades);
    get(j, "temple_faith", n.temple_faith);
    get(j, "foundry_era_progress", n.foundry_era_progress); get(j, "foundry_anti_dragon", n.foundry_anti_dragon);
    get(j, "granary_wealth", n.granary_wealth);
    get(j, "ruin_burn", n.ruin_burn);
    get(j, "take_item_artifacts", n.take_item_artifacts);
    get(j, "craft_era_progress", n.craft_era_progress); get(j, "war_militarism", n.war_militarism);
    get(j, "war_anti_dragon", n.war_anti_dragon); get(j, "magic_world", n.magic_world); get(j, "intrigue_cunning", n.intrigue_cunning);
    get(j, "trade_growth", n.trade_growth); get(j, "trade_decades", n.trade_decades);
    get(j, "haunt_abdicate", n.haunt_abdicate); get(j, "haunt_desert", n.haunt_desert); get(j, "haunt_cult", n.haunt_cult);
    get(j, "guise_discovery", n.guise_discovery); get(j, "guise_per_sage", n.guise_per_sage);
    get(j, "guise_forewarned", n.guise_forewarned); get(j, "guise_show", n.guise_show);
    get(j, "guise_preach_faith", n.guise_preach_faith); get(j, "guise_learn_knowledge", n.guise_learn_knowledge);
    get(j, "curse_research_mult", n.curse_research_mult); get(j, "curse_research_stacks", n.curse_research_stacks);
    get(j, "curse_blight_land", n.curse_blight_land);
    get(j, "consort_line_years", n.consort_line_years);
    if (j.contains("haunt"))
        for (auto it = j["haunt"].begin(); it != j["haunt"].end(); ++it) {
            MemoryRow r;
            r.ev = "haunt_" + it.key();
            get(it.value(), "sev", r.sev);
            get(it.value(), "dread", r.dread);
            get(it.value(), "awe", r.awe);
            get(it.value(), "debt", r.debt);
            n.haunt[it.key()] = r;
        }
}

}

// a missing gates.json loads as no rules: every act is then refused as unknown except the ones acts.cpp knew before
bool act_book_from_json(const DataJson& d, Params& p, std::string& err) {
    ActBook& b = p.act_book;
    if (!d.gates.empty()) {
        json j = json::parse(d.gates, nullptr, false);
        if (j.is_discarded() || !j.is_object()) { err = "gates.json: invalid json"; return false; }
        if (j.contains("acts")) for (auto it = j["acts"].begin(); it != j["acts"].end(); ++it) b.acts[it.key()] = rule(it.value());
        if (j.contains("keep_roles")) for (auto it = j["keep_roles"].begin(); it != j["keep_roles"].end(); ++it) b.keep_roles[it.key()] = alts(it.value());
        if (j.contains("build_what")) for (auto it = j["build_what"].begin(); it != j["build_what"].end(); ++it) b.build_what[it.key()] = alts(it.value());
        if (j.contains("reveal_lie")) b.reveal_lie = alts(j["reveal_lie"]);
        if (j.contains("sponsor_person")) b.sponsor_person = alts(j["sponsor_person"]);
    }
    json c = json::parse(d.constants, nullptr, false);
    if (!c.is_discarded() && c.is_object() && c.contains("acts")) numbers(c["acts"], b.n);
    return true;
}

}
