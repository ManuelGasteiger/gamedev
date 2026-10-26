#pragma once
#include <map>
#include <string>
#include <vector>
#include "data.h"
#include "state.h"

// v0.4 (S1): talents (data/talents.json). Effects of taken talents and ranks merge by key: `_mult` multiplies
// (default 1), `_add` adds (default 0), any other key takes the maximum (default 0).

namespace ages {

struct Pick {
    std::string axis;
    int node = 0;
    double points = 0;
    int needs_phase = 1;
    bool open = false;
    std::vector<std::string> options;
};

double growth_of(const Dragon& d, const std::string& axis);
int last_authored_node(const TalentBook& b, const std::string& axis);   // -1 when the axis has no talents
int ranks_of(const Dragon& d, const TalentBook& b, const std::string& axis);
int milestones_of(const Dragon& d, const TalentBook& b, const std::string& axis);   // every milestone_every ranks
std::map<std::string, int> all_ranks(const Dragon& d, const TalentBook& b);
double next_node_points(const Dragon& d, const TalentBook& b, const std::string& axis);   // -1 when nothing is left
std::vector<Pick> pending_picks(const World& w, const Params& p);
bool pick_talent(World& w, const Params& p, const std::string& id, std::string& reason);

double effect(const World& w, const Params& p, const std::string& key);
std::map<std::string, double> merged_effects(const World& w, const Params& p);
TalentLook merged_look(const World& w, const Params& p);

}
