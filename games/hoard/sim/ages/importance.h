#pragma once
#include <string>
#include "data.h"
#include "state.h"

namespace ages {

// the en.json template an event is told with: the kind, or a variant (collapse_one, person_died_old_age, ...)
std::string template_key(const Event& e);

// data/text/en.json importance of the event's kind (0..1), plus the boosts of constants.json "briefing" when the
// event touches someone the dragon knows, a court under oath with it, or a hostage
double event_importance(const World& w, const Params& p, const Event& e);

// scores the events at the end of the chronicle that have no importance yet (as the world stands now)
void score_events(World& w, const Params& p);

}
