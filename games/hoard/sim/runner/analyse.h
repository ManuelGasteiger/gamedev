#pragma once
#include <ostream>
#include <string>
#include "ages/ages.h"

// item 01: read-only measurements the runner writes beside a run (docs/prompts/01-interest-baseline.md); nothing here
// changes the run unless a perturbation is asked for
namespace ages {

struct AnalyseOptions {
    std::ostream* snapshots = nullptr;   // one JSON line per sleep: the core stocks
    std::ostream* wakings = nullptr;     // one JSON line per waking: listed wounds, hooks and their refs, buckets
    bool buckets = false;                // sleep copies of the world for every bucket at every waking
    int bucket_samples = 2;
    std::string perturb;                 // "burn": the first burn +0.1; "sleep": the first sleep one bucket longer
};

void attach_analysis(Scenario& s, const Params& p, const AnalyseOptions& o);

}
