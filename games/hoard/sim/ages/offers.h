#pragma once
#include <string>
#include "data.h"
#include "state.h"

namespace ages {

// 17.3: offers are generated into w.offers at wake from faction state and data/offers.json; the envoy is a person
void generate_offers(World& w, const Params& p);
const OfferDef* offer_def(const Params& p, const std::string& kind);

// after an act is applied: fulfil pending asks (faith x honest_faith_mult when honest, deceived_dragon when a lie),
// and move the ledger of the people the dragon knows in that court
void note_act(World& w, const Params& p, const Act& a, double m);

}
