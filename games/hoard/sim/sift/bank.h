#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

// item 14: the pattern bank (data/patterns), the template tiers (data/importance.json "tiers") and the sifter's weights
// (constants.json "sift"); research-story-sifting.md sections 2, 4 and 5

namespace text { struct Bundles; }

namespace ages {

struct SiftMatch {
    std::vector<std::string> kinds;
    std::string subject, object;   // "" any, "?x" a variable, else a literal id ("dragon")
    std::vector<std::string> tags_all, tags_any, tags_none;
    bool has_magnitude = false;
    double magnitude_gte = 0;
};

struct SiftRef {
    int clause = -1;
    int max_hops = 3;
};

struct SiftClause {
    std::string as;
    std::vector<SiftMatch> alts;   // the clause itself, or its any_of alternatives
    SiftRef caused_by, not_caused_by;
    int same_thread_as = -1;
    int within_years = 0;          // from the previous clause; 0 on the first
    std::vector<std::pair<std::string, std::string>> bind;    // ?var <- path
    std::vector<std::pair<std::string, std::string>> where;   // path == ?var | literal
};

// active once clause `from` has matched and until `to` matches; -1 -1 for the whole match
struct SiftGuard {
    SiftMatch m;
    int from = -1, to = -1;
};

struct SiftPattern {
    std::string id;
    int level = 1;
    int window_years = 3000;
    int tier = 3;
    double stakes = 0.5;
    bool survives_death = false;
    std::vector<SiftClause> clauses;
    std::vector<SiftGuard> unless, unless_forever;
    std::vector<std::string> once_per;
    int outcome_clause = -1;
    std::string outcome_field;   // kind | tag
    std::map<std::string, std::string> outcomes;
    std::string emit_kind, emit_subject, emit_object, emit_magnitude;
    std::set<std::string> emit_tags, tags;   // tags: "prophecy", "ending" (the run-phase bonus)
};

struct SiftWeights {
    double dragon = 0.20, stakes = 0.15, causal = 0.15, continuity = 0.15, proximity = 0.10, surprise = 0.10,
           reversals = 0.10, irreversible = 0.05;
};

struct SiftConstants {
    SiftWeights w;
    double shown_end_bonus = 0.20, tier5_bonus = 0.30, recent_headline_penalty = 0.25;
    int recent_headline_wakings = 3;
    double floor = 0.30, same_pattern_penalty = 0.20;
    double diversity_start = 0.20, diversity_end = 0.05, run_bonus_max = 0.15;
    int run_phase_wakings = 30;
    double t_major = 0.9, t_breather = 0.5, arc_k = 2.0;
    int headline_pool = 3, max_callbacks = 2, min_lines = 2, max_lines = 9, max_ages = 3;
    int partial_cap = 64, dragon_hops = 4, because_hops = 3;
    double because_stop_importance = 0.2;   // importance <= 1 on the research's 0..5 scale
    double oral_reroll = 0.15, blame_dread = 0.6;
    int generation_years = 30;
    std::vector<double> reversal_score = {0.2, 0.7, 1.0, 1.0, 0.6, 0.3};
    std::map<std::string, int> fortune;      // kind -> sign for its subject (the object gets the opposite)
    std::set<std::string> irreversible;      // kinds that end a court, dynasty, race or era
    std::set<std::string> motivating;        // kinds that motivate a person (IPOCL gate)
    std::set<std::string> harm;              // kinds whose subject harms its object (a source omits its own)
};

struct SiftBank {
    std::vector<SiftPattern> patterns;
    uint64_t hash = 0;
    std::map<std::string, int> tiers;   // template key -> 0..5
    // dry-run base rates: "<pattern>|<era>" -> {"n", "<property>"...} counts; magnitude deciles per kind
    std::map<std::string, std::map<std::string, double>> rates;
    std::map<std::string, std::vector<double>> magnitudes;
    SiftConstants c;
    std::shared_ptr<const text::Bundles> texts;   // the line templates, data/text/<lang>/patterns.json
};

}
