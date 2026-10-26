#include "world_rules.h"
#include "../world_util.h"

namespace ages {

// a region counts the years it has had no owner; land left this burned, or ruined, is `razed`
void rule_empty_land(World& w, const Params& p) {
    for (auto& r : w.regions) {
        if (!r.owner.empty() || r.lair) { r.empty_years = 0; r.tags.erase("razed"); continue; }
        if (r.empty_years == 0 && (r.burn >= p.genesis.empty_land.razed_burn || has_tag(r.tags, "ruins"))) r.tags.insert("razed");
        r.empty_years += w.dt;
    }
}

// the S2 world rules run in generated worlds only: the fixed start keeps the v0.3 behaviour the tests and goldens read
void rule_world(World& w, const Params& p) {
    if (!has_tag(w.tags, "generated")) return;
    rule_empty_land(w, p);
    rule_magic(w, p);
    rule_undying(w, p);
    rule_extinction(w, p);
    rule_storyteller(w, p);
}

}
