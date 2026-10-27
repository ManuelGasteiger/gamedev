#pragma once
#include <string>
#include <vector>
#include "ages/data.h"

namespace ages {

// a thread, a callback on a thread already shown, or raw events (one legacy arc or one event) up for a line
struct Candidate {
    const SiftThread* thread = nullptr;
    const SiftPattern* pattern = nullptr;
    std::vector<int> events;     // kernel (thread) or the raw events, oldest first
    std::vector<int> fresh;      // the ones that fall inside this sleep
    std::string kind = "line";   // line | callback
    std::string protagonist, court;
    int tier = 0, year = 0;
    double stakes = 0.5;         // T_thread
    double base = 0, score = 0;
    double dragon = 0;
    int reversals = 0;
};

struct Features {
    double dragon = 0, stakes = 0, causal = 0, continuity = 0, proximity = 0, surprise = 0.5, reversals = 0, irreversible = 0;
    int reversal_count = 0;
};

Features features(const World& w, const Params& p, const Candidate& c);
double weighted(const SiftConstants& k, const Features& f);
double reportable(const World& w, const Params& p, const Event& e);   // importance x magnitude percentile

}
