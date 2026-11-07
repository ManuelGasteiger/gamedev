#pragma once
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "ages/json.h"

// item 10: the scenario bank (data/scenarios/*.json, scenario-spec.md) as the runtime reads it. Namespace scn keeps the
// scenario runtime apart from ages::run_scenario, the policy runner.

namespace scn {

// spec section 4: clause (" and " clause)*; no `or`
struct Clause {
    enum Kind { cmp, bound, has_tag } kind = cmp;
    bool neg = false;
    std::string role, field, op, value, tag;
};

struct Pred {
    std::vector<Clause> clauses;
    std::string text;
    bool empty() const { return clauses.empty(); }
};

struct Role {
    std::string name, kind;   // act | faction | region | person | item | remnant
    nlohmann::json spec;      // the binder's fields as written
    bool optional = false;
    std::string noun, sex = "any";
    std::vector<std::string> deps;   // roles named by $refs in the spec
};

struct Forbid {
    bool is_act = false;
    nlohmann::json act;
    Pred pred;
};

struct Salience {
    Pred pred;
    int w = 1;
};

struct Option {
    std::string id;
    nlohmann::json act;
    Pred gate;
};

struct Variant {
    std::string id;
    Pred pred;
    bool headline = false;
};

struct Def {
    std::string id, kind, stakes = "minor", once = "run", group;
    std::vector<std::string> eras;
    int weight = 100, cd_wakings = 0, cd_years = 0;
    bool has_arrival = false;
    std::string arrival_who, arrival_from;
    std::vector<std::string> needs;
    std::vector<Role> roles;   // in binding order (dependencies first, then file order by name)
    std::vector<Pred> require;
    std::vector<Forbid> forbid;
    std::vector<Salience> salience;
    std::vector<std::string> tags;
    bool has_because = false;
    std::vector<Variant> variants;
    std::vector<Option> options;
    nlohmann::json outcomes = nlohmann::json::object();
    std::vector<std::string> seek;
    int seek_after = 1;
    const Role* role(const std::string& n) const {
        for (auto& r : roles) if (r.name == n) return &r;
        return nullptr;
    }
};

// constants.json "scenarios": the selection's numbers (spec section 6)
struct Numbers {
    double spec_w = 2, cite_w = 9, recent_w = 9, recent_half = 4, theme_w = 3, seen_w = 3, theme_share = 0.15, theme_share_w = 9;
    double window = 3, cite_half = 2, seek_w = 9, arrival_pity_w = 3, pity_magnitude = 0.2;
    int seen_cap = 9, theme_wakings = 3, pity_wakings = 2, arrival_pity_after = 4;
    int cites_per_run = 2, cites_per_waking = 1, cites_monumental = 4, candidates_per_role = 12, because_level = 1;
    double renown_events = 10, remember_stock = 0.1;
    std::vector<std::string> eras = {"bronze", "iron", "castles", "gunpowder", "arcane"};
    std::vector<std::pair<double, std::string>> sizes;
};

struct Bank {
    std::vector<Def> defs;   // sorted by id
    uint64_t hash = 0;
    Numbers n;
    // vocab.json: gate name -> alternatives, each an arrival gate row (data/arrivals.json shape); any alternative holds
    std::map<std::string, std::vector<nlohmann::json>> gates;
    std::set<std::string> themes;
    const Def* find(const std::string& id) const;
};

}
