#include "growth.h"
#include "talents.h"

namespace ages {

const char* const kGrowthAxes[5] = {"might", "flame", "arcana", "lore", "dominion"};

void hatch(World& w, const Params& p) {
    Hoard& h = w.dragons[kPlayer].hoard;
    for (auto& kv : p.c.start_hoard) {
        if (kv.first == "gold") h.gold += kv.second;
        else if (kv.first == "books") h.books += kv.second;
        else if (kv.first == "artifacts") h.artifacts += kv.second;
        else if (kv.first == "art") h.art += kv.second;
        else if (kv.first == "food") h.food += kv.second;
    }
}

void add_growth(World& w, const Params& p, const std::string& axis, double points) {
    if (points <= 0) return;
    w.dragons[kPlayer].growth[axis] += points;
    if (axis == "might") w.dragons[kPlayer].size += points * p.c.size_per_might * effect(w, p, "size_mult");
}

double total_growth(const Dragon& d) {
    double t = 0;
    for (auto& kv : d.growth) t += kv.second;
    return t;
}

double next_phase_points(const Dragon& d, const Constants& c) {
    size_t i = static_cast<size_t>(d.phase < 1 ? 0 : d.phase - 1);
    return i < c.phase_points.size() ? c.phase_points[i] : -1;
}

}
