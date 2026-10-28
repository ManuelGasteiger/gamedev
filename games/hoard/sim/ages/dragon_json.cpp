#include "dragon_json.h"
#include <cmath>
#include "defences.h"
#include "growth.h"
#include "rules/slayers.h"
#include "talents.h"
#include "world_util.h"
#include "wounds.h"

namespace ages {

using nlohmann::json;

namespace {

json look_json(const TalentLook& l) {
    json j = json::object();
    for (auto& kv : l.num) j[kv.first] = kv.second;
    for (auto& kv : l.str) j[kv.first] = kv.second;
    return j;
}

json maybe(double v) { return v < 0 ? json(nullptr) : json(v); }

json picks_json(const World& w, const Params& p) {
    json arr = json::array();
    for (auto& pk : pending_picks(w, p))
        arr.push_back({{"axis", pk.axis}, {"node", pk.node}, {"points", pk.points}, {"needs_phase", pk.needs_phase},
                       {"open", pk.open}, {"options", pk.options}});
    return arr;
}

json defences_json(const World& w, const Params& p, const Region& r) {
    json arr = json::array();
    for (auto& v : region_defences(w, p, r))
        arr.push_back({{"id", v.def->id}, {"label", v.def->label}, {"per_act", v.per_act}, {"passive", v.passive},
                       {"radius", v.def->radius}, {"burn_mult", v.burn_mult}, {"wrecked", v.wrecked}});
    return arr;
}

}

static json chron_json(const ChronicleState& c) {
    json h = json::array(), s = json::array(), r = json::array();
    for (auto& t : c.historians) h.push_back({{"person", t.person}, {"from", t.from}, {"to", t.to}});
    for (auto& x : c.sources) s.push_back({{"id", x.id}, {"year", x.year}, {"how", x.how}});
    for (auto& x : c.relics)
        r.push_back({{"id", x.id}, {"kind", x.kind}, {"agenda", x.agenda}, {"court", x.court}, {"year", x.year}, {"quiet_until", x.quiet_until},
                     {"asked", x.asked}, {"answer", x.answer}});
    return {{"historians", h}, {"sources", s}, {"relics", r}};
}

static void chron_from_json(const json& j, ChronicleState& c) {
    c = ChronicleState();
    for (auto& t : j.value("historians", json::array())) c.historians.push_back({t.value("person", ""), t.value("from", 0), t.value("to", -1)});
    for (auto& x : j.value("sources", json::array())) c.sources.push_back({x.value("id", ""), x.value("year", 0), x.value("how", "")});
    for (auto& x : j.value("relics", json::array()))
        c.relics.push_back({x.value("id", ""), x.value("kind", ""), x.value("agenda", ""), x.value("court", ""), x.value("year", 0),
                            x.value("quiet_until", -1), x.value("asked", -1), x.value("answer", "")});
}

void dragon_state_to_json(const Dragon& d, json& dragon) {
    dragon["growth"] = d.growth;
    dragon["talents"] = d.talents;
    dragon["wounds"] = d.wounds;
    dragon["adaptation"] = d.adaptation;
    dragon["hoard"]["embers"] = d.hoard.embers;
    if (!d.chron.empty()) dragon["chronicle"] = chron_json(d.chron);   // item 05: absent until used, so older hashes hold
}

void dragon_state_from_json(const json& dragon, Dragon& d) {
    if (dragon.contains("growth") && dragon["growth"].is_object()) d.growth = dragon["growth"].get<std::map<std::string, double>>();
    if (dragon.contains("talents") && dragon["talents"].is_array()) d.talents = dragon["talents"].get<std::vector<std::string>>();
    if (dragon.contains("wounds") && dragon["wounds"].is_number()) d.wounds = dragon["wounds"].get<double>();
    if (dragon.contains("adaptation") && dragon["adaptation"].is_number()) d.adaptation = dragon["adaptation"].get<double>();
    if (dragon.contains("hoard") && dragon["hoard"].contains("embers")) d.hoard.embers = dragon["hoard"]["embers"].get<double>();
    if (dragon.contains("chronicle")) chron_from_json(dragon["chronicle"], d.chron);
}

// talents-v2 section 4: the flight numbers every shell reads, from talents.json "flight" and the taken talents
static json flight_json(const World& w, const Params& p) {
    auto base = [&](const char* k) { auto it = p.talents.flight.find(k); return it == p.talents.flight.end() ? 0.0 : it->second; };
    double speed = effect(w, p, "speed_mult");
    double hostage = effect(w, p, "hostage_weight");
    return {{"cruise", base("cruise") * speed}, {"surge", base("surge") * speed}, {"stall", base("stall")}, {"turn", base("turn")},
            {"travel_speed", base("travel_speed")}, {"travel_altitude", base("travel_altitude")},
            {"carry_cap", base("carry_cap") + effect(w, p, "carry_add")},
            {"stamina_budget", base("stamina_base") + base("stamina_per_sqrt_points") * std::sqrt(total_growth(w.dragons[kPlayer])) + effect(w, p, "stamina_add")},
            {"surge_drain", base("surge_drain") * effect(w, p, "surge_drain_mult")}, {"breath_cost", base("breath_cost") * effect(w, p, "burn_cost_mult")},
            {"hostage_weight", hostage > 0 ? hostage : base("hostage_weight")}, {"gold_weight", base("gold_weight") * effect(w, p, "gold_weight_mult")}};
}

void dragon_view_to_json(const World& w, const Params& p, json& view) {
    const Dragon& d = w.dragons[kPlayer];
    json& dj = view["dragon"];
    json growth = json::object(), next = json::object(), ranks = json::object(), steps = json::object();
    for (const char* axis : kGrowthAxes) {
        growth[axis] = growth_of(d, axis);
        next[axis] = maybe(next_node_points(d, p.talents, axis));
        ranks[axis] = ranks_of(d, p.talents, axis);
        steps[axis] = milestones_of(d, p.talents, axis);
    }
    dj["growth"] = growth;
    dj["next_node"] = next;
    dj["ranks"] = ranks;
    dj["picks"] = picks_json(w, p);
    dj["effects"] = merged_effects(w, p);
    dj["look"] = look_json(merged_look(w, p));
    dj["look"]["rank_step"] = steps;
    dj["health_max"] = health_max(w, p);
    dj["flight"] = flight_json(w, p);
    dj["slain"] = dragon_dead(w);
    dj["slayer_adaptation"] = adaptation_factor(w, p);
    bool lair_ward = effect(w, p, "lair_ward") > 0;
    for (size_t i = 0; i < w.regions.size(); ++i) {
        view["regions"][i]["defences"] = defences_json(w, p, w.regions[i]);
        if (w.regions[i].lair) view["regions"][i]["warded"] = lair_ward;   // no sieges (S3 reads it)
    }
    for (size_t i = 0; i < w.factions.size(); ++i) {
        const Faction& f = w.factions[i];
        Unlock u = next_unlock(w, p, f);
        view["factions"][i]["anti_dragon_next"] = u.at < 0 ? json(nullptr) : json{{"at", u.at}, {"defence", u.defence}};
        std::string stage = slayer_stage(w, f.id);
        view["factions"][i]["slayers"] = stage.empty() ? json(nullptr)
            : json{{"stage", stage == "slayers_ready" ? "ready" : "forming"}, {"strength", slayer_strength(w, p, f)}, {"odds", slaying_odds(w, p, f)}};
    }
    view["phase"]["points"] = total_growth(d);
    view["phase"]["next_phase_points"] = maybe(next_phase_points(d, p.c));
    if (effect(w, p, "lie_sight") > 0)
        for (size_t i = 0; i < w.offers.size() && i < view["offers"].size(); ++i) view["offers"][i]["revealed_lie"] = w.offers[i].is_lie;
}

void sleep_growth_to_json(const SleepResult& r, json& j) {
    j["growth_gained"] = r.growth_gained;
    j["healed"] = r.healed;
}

json talents_catalogue_json(const Params& p) {
    const TalentBook& b = p.talents;
    json axes = json::array(), talents = json::array();
    for (auto& a : b.axes)
        axes.push_back({{"id", a.id}, {"name", a.name}, {"fed_by", a.fed_by}, {"rank_every", a.rank_every}, {"rank_growth", a.rank_growth},
                        {"rank", {{"effects", a.rank_effects}, {"look", look_json(a.rank_look)}, {"text", a.rank_text}}},
                        {"milestone", {{"every", a.milestone_every}, {"effects", a.milestone_effects}, {"look", look_json(a.milestone_look)}}}});
    for (auto& t : b.talents)
        talents.push_back({{"id", t.id}, {"axis", t.axis}, {"node", t.node}, {"name", t.name}, {"text", t.text},
                           {"effects", t.effects}, {"look", look_json(t.look)}});
    return {{"ok", true}, {"axes", axes}, {"nodes", b.nodes}, {"node_phase", b.node_phase}, {"talents", talents}};
}

}
