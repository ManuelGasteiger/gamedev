#include "genesis_data.h"
#include "json.h"

namespace ages {

using nlohmann::json;

namespace {

template <class T>
void get(const json& j, const char* key, T& out) {
    auto it = j.find(key);
    if (it != j.end() && !it->is_null()) out = it->get<T>();
}

// a number or [lo, hi]
void span(const json& j, const char* key, Span& out) {
    auto it = j.find(key);
    if (it == j.end()) return;
    if (it->is_number()) out.lo = out.hi = it->get<double>();
    else if (it->is_array() && it->size() == 2) { out.lo = (*it)[0].get<double>(); out.hi = (*it)[1].get<double>(); }
}

void court(const json& j, CourtSpec& c) {
    get(j, "id", c.label);
    get(j, "race", c.race);
    get(j, "mix", c.mix);
    span(j, "mix_share", c.mix_share);
    span(j, "count", c.count);
    span(j, "regions", c.regions);
    span(j, "wealth", c.wealth);
    span(j, "cohesion", c.cohesion);
    span(j, "era", c.era);
    get(j, "chance", c.chance);
    get(j, "terrain", c.terrain);
    get(j, "any_terrain", c.any_terrain);
    get(j, "near", c.near);
    get(j, "traits", c.traits);
    get(j, "tags", c.tags);
    get(j, "undying", c.undying);
}

void start_template(const json& j, StartTemplate& t) {
    get(j, "id", t.id);
    get(j, "weight", t.weight);
    get(j, "deal", t.deal);
    get(j, "fallen", t.fallen);
    span(j, "ruins", t.ruins);
    span(j, "ruin_burn", t.ruin_burn);
    span(j, "ruin_age", t.ruin_age);
    span(j, "fallen_reign", t.fallen_reign);
    get(j, "fallen_era", t.fallen_era);
    get(j, "ruin_wasteland", t.ruin_wasteland);
    get(j, "fallback_race", t.fallback_race);
    if (j.contains("courts"))
        for (auto& v : j["courts"]) { CourtSpec c; court(v, c); t.courts.push_back(c); }
    if (j.contains("relations"))
        for (auto& v : j["relations"]) {
            RelationSpec r;
            get(v, "kind", r.kind);
            get(v, "a", r.a);
            get(v, "b", r.b);
            get(v, "tag", r.tag);
            get(v, "chance", r.chance);
            get(v, "decades", r.decades);
            t.relations.push_back(r);
        }
    if (j.contains("remnants"))
        for (auto& v : j["remnants"]) {
            RemnantSpec r;
            get(v, "race", r.race);
            span(v, "strength", r.strength);
            get(v, "chance", r.chance);
            t.remnants.push_back(r);
        }
}

void deck(const json& j, const char* key, std::vector<DeckEntry>& out) {
    if (!j.contains(key)) return;
    for (auto& v : j[key]) {
        DeckEntry d;
        get(v, "card", d.card);
        get(v, "weight", d.weight);
        out.push_back(d);
    }
}

void storyteller(const json& j, StorytellerDef& s) {
    get(j, "major_importance", s.major_importance);
    get(j, "chance_base", s.chance_base);
    get(j, "chance_per_window", s.chance_per_window);
    get(j, "chance_max", s.chance_max);
    get(j, "forced_after", s.forced_after);
    get(j, "forced_years", s.forced_years);
    get(j, "second_chance", s.second_chance);
    get(j, "near_end_share", s.near_end_share);
    get(j, "prehistory_waking_years", s.prehistory_waking_years);
    get(j, "grace_years", s.grace_years);
    get(j, "major_per_year", s.major_per_year);
    get(j, "major_max", s.major_max);
    get(j, "breather_mult", s.breather_mult);
    deck(j, "deck", s.deck);
    deck(j, "wake_deck", s.wake_deck);
}

void deal(const json& j, DealDef& d) {
    get(j, "base", d.base);
    get(j, "wild", d.wild);
    get(j, "max", d.max);
    get(j, "cluster", d.cluster);
    get(j, "lair_max", d.lair_max);
    get(j, "lair_min", d.lair_min);
    get(j, "tries", d.tries);
}

}

bool genesis_from_json(const std::string& text, GenesisDef& g, std::string& err) {
    if (text.empty()) return true;
    json j = json::parse(text, nullptr, false);
    if (j.is_discarded() || !j.is_object()) { err = "invalid json in genesis"; return false; }
    g.present = true;
    if (j.contains("terrain"))
        for (auto it = j["terrain"].begin(); it != j["terrain"].end(); ++it) {
            TerrainDef t;
            get(it.value(), "land", t.land);
            get(it.value(), "jitter", t.jitter);
            get(it.value(), "name_a", t.name_a);
            get(it.value(), "name_b", t.name_b);
            g.terrain[it.key()] = t;
        }
    if (j.contains("deal")) deal(j["deal"], g.deal);
    get(j, "rim", g.rim);
    if (j.contains("courts")) { Span s; span(j, "courts", s); g.min_courts = static_cast<int>(s.lo); g.max_courts = static_cast<int>(s.hi); }
    if (j.contains("fallback_court")) court(j["fallback_court"], g.fallback);
    span(j, "prehistory", g.prehistory);
    if (j.contains("legends")) {
        get(j["legends"], "max", g.legends.max);
        get(j["legends"], "per_kind", g.legends.per_kind);
        get(j["legends"], "per_century", g.legends.per_century);
        get(j["legends"], "skip", g.legends.skip);
    }
    if (j.contains("templates"))
        for (auto& v : j["templates"]) { StartTemplate t; start_template(v, t); g.templates.push_back(t); }
    if (j.contains("extinction")) {
        const json& e = j["extinction"];
        get(e, "races", g.extinction.races);
        get(e, "strength", g.extinction.strength);
        get(e, "pop", g.extinction.pop);
        get(e, "gone_years", g.extinction.gone_years);
        get(e, "assimilate", g.extinction.assimilate);
        get(e, "chance", g.extinction.chance);
    }
    if (j.contains("undying")) {
        const json& u = j["undying"];
        get(u, "cohesion_floor", g.undying.cohesion_floor);
        get(u, "fall_chance", g.undying.fall_chance);
        get(u, "fall_cohesion", g.undying.fall_cohesion);
        get(u, "empire_years", g.undying.empire_years);
    }
    if (j.contains("magic")) {
        const json& m = j["magic"];
        get(m, "start", g.magic.start);
        get(m, "rise", g.magic.rise);
        get(m, "cap", g.magic.cap);
        get(m, "ebb", g.magic.ebb);
    }
    if (j.contains("empty_land")) get(j["empty_land"], "razed_burn", g.empty_land.razed_burn);
    if (j.contains("storyteller")) storyteller(j["storyteller"], g.story);
    if (g.templates.empty()) { err = "genesis: no start templates"; return false; }
    return true;
}

}
