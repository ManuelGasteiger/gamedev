#pragma once
#include <string>
#include <vector>
#include "data.h"
#include "state.h"

namespace ages {

// 17.3: what a court would ask of the dragon now, one draft per offer row whose trigger holds; the envoy is a person.
// An envoy arrival (item 06) carries one draft to the lair; generate_offers turns the waiting ones into w.offers at wake
struct EnvoyDraft {
    std::string kind, target, motive, region, person;
    bool slay_true = false;   // the pretext "plan_to_slay" is true
};
std::vector<EnvoyDraft> envoy_drafts(const World& w, const Params& p, const Faction& f);
void roll_envoy_lie(const World& w, const Params& p, const Faction& f, const EnvoyDraft& d, Rng& rng, std::string& stated, bool& is_lie);
void generate_offers(World& w, const Params& p);
const OfferDef* offer_def(const Params& p, const std::string& kind);

// after an act is applied: fulfil pending asks (faith x honest_faith_mult when honest, deceived_dragon when a lie),
// and move the ledger of the people the dragon knows in that court
void note_act(World& w, const Params& p, const Act& a, double m);

}
