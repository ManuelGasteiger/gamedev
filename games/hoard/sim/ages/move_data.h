#pragma once
#include <map>
#include <string>
#include <vector>

// item 07: data/moves.json (people traits, person memory, the six moves, release readings, secrets, inquisition)

namespace ages {

struct MemoryRow {
    std::string ev;
    int sev = 1;
    double dread = 0, awe = 0, debt = 0;
};

struct MoveDef {
    std::string id, intent;
    bool access = true, secret = false, witnessed = false, guise_opens = false;
    std::map<std::string, double> lore, arcana;   // teach: per subject
    double lore_min = 0, arcana_min = 0;
    std::map<std::string, double> weights, mods;  // mods: axis -> cap
    double threshold = 0, ledger = 0;
    MemoryRow accept;
    std::map<std::string, std::string> acquire;   // subject (or "*") -> acquired tag with $subject / $toward
    std::string rel;
    bool has_hostage = false, has_humiliated = false;
    MemoryRow hostage_giver, hostage_taken, humiliated;
    std::vector<std::string> humiliated_traits;
    double court_dread = 0, court_awe = 0;
};

struct MoveBook {
    std::vector<std::string> base, virtues, vices;
    int traits_max = 3;
    double trait_on = 0.75, extra_trait = 0.5;
    std::vector<std::string> acquired;
    int acquired_max = 2;
    std::map<std::string, double> src, decay;
    double monumental_floor = 0.3, drop_below = 0.05, permanent_at = 2, flip_chance = 0.3;
    double inherit_negative = 0.5, inherit_positive = 0.25, axis_cap = 5, sev2_at = 0.3, sev3_at = 0.6;
    int touched_sev = 3, recent_max = 12;
    std::map<std::string, MemoryRow> acts;   // act type -> per-person row (ev act_<type>, sev from magnitude)
    std::map<std::string, std::vector<std::string>> flips;   // axis -> traits a permanent entry may take away
    std::map<std::string, int> cap, rel;
    double terms_accept = 2;
    double term_mercy = 2, term_betrayal = -3, term_same = 1, term_clamp = 6;
    std::map<std::string, MoveDef> moves;
    std::string release_default = "mercy";
    std::map<std::string, std::string> release_by_trait;
    std::map<std::string, MemoryRow> readings;
    double secret_base = 0.05, secret_per_sage = 0.02, secret_intrigue_mult = 0.5;
    std::vector<std::string> inq_joiners, inq_bonds;
    double inq_per_decade = 0.25, inq_strength_min = 0.8;
};

}
