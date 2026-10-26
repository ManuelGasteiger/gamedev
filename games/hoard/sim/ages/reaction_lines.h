#pragma once
#include <string>
#include <vector>
#include "ages.h"

namespace ages {

// one court's reaction to one act: the reaction row (act x relationship) that fired, recorded by apply_acts
struct ReactionNote {
    std::string faction;
    std::string act;      // burn | kill | take | defend | ... | witness_burn | show_as_burn
    std::string rel;      // the relationship row: oath_active | faithful | weak | strong | losing | enemy | any ...
    std::string target;   // the court the act was aimed at (the victim, for a witness)
    std::string region;
    std::string item;     // the person acted on, if any
    double magnitude = 0;
    std::string loot;     // what a take carried off (the largest payload share)
};

// the lines of apply_acts: per court the weightiest note, told by narrator.json "reactions" plus at most one sentence on an
// offer fulfilled or a lie acted on, a person the dragon knows, or a mark a card left (events from first_event on)
std::vector<ReactionLine> reaction_lines(const World& w, const Params& p, const std::vector<ReactionNote>& notes, size_t first_event);

}
