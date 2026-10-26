#include "talents.h"
#include "unrecorded.h"
#include "core/detmath.h"
#include <algorithm>
#include <cmath>
#include "world_util.h"

namespace ages {

namespace {

enum class Merge { mult, add, max };

Merge merge_of(const std::string& key) {
    auto ends = [&](const char* s) { size_t n = std::char_traits<char>::length(s); return key.size() >= n && key.compare(key.size() - n, n, s) == 0; };
    if (ends("_mult")) return Merge::mult;
    if (ends("_add")) return Merge::add;
    return Merge::max;
}

void combine(double& v, double x, Merge m, int times = 1) {
    if (m == Merge::mult) v *= det::pow(x, times);
    else if (m == Merge::add) v += x * times;
    else v = std::max(v, x);
}

const TalentDef* def_of(const TalentBook& b, const std::string& id) {
    auto it = b.index.find(id);
    return it == b.index.end() ? nullptr : &b.talents[it->second];
}

bool taken_at(const Dragon& d, const TalentBook& b, const std::string& axis, int node) {
    for (auto& id : d.talents)
        if (const TalentDef* t = def_of(b, id)) if (t->axis == axis && t->node == node) return true;
    return false;
}

const TalentAxis* axis_of(const TalentBook& b, const std::string& id) {
    for (auto& a : b.axes) if (a.id == id) return &a;
    return nullptr;
}

// ranks past the axis' last authored node: the first costs rank_every, each next one rank_growth times the last
struct RankState {
    int ranks = 0;
    double next = -1;
};

RankState rank_state(const Dragon& d, const TalentBook& b, const TalentAxis& a) {
    RankState s;
    int last = last_authored_node(b, a.id);
    if (last < 0 || a.rank_every <= 0) return s;
    double points = growth_of(d, a.id), at = b.nodes[static_cast<size_t>(last)], cost = a.rank_every;
    while (at + cost <= points) {
        at += cost;
        cost *= std::max(1.0, a.rank_growth);
        s.ranks += 1;
    }
    s.next = at + cost;
    return s;
}

int milestones(const TalentAxis& a, int ranks) { return a.milestone_every > 0 ? ranks / a.milestone_every : 0; }

}

double growth_of(const Dragon& d, const std::string& axis) {
    auto it = d.growth.find(axis);
    return it == d.growth.end() ? 0 : it->second;
}

int last_authored_node(const TalentBook& b, const std::string& axis) {
    int last = -1;
    for (auto& t : b.talents) if (t.axis == axis) last = std::max(last, t.node);
    return last;
}

int ranks_of(const Dragon& d, const TalentBook& b, const std::string& axis) {
    const TalentAxis* a = axis_of(b, axis);
    return a ? rank_state(d, b, *a).ranks : 0;
}

int milestones_of(const Dragon& d, const TalentBook& b, const std::string& axis) {
    const TalentAxis* a = axis_of(b, axis);
    return a ? milestones(*a, rank_state(d, b, *a).ranks) : 0;
}

std::map<std::string, int> all_ranks(const Dragon& d, const TalentBook& b) {
    std::map<std::string, int> out;
    for (auto& a : b.axes) out[a.id] = rank_state(d, b, a).ranks;
    return out;
}

double next_node_points(const Dragon& d, const TalentBook& b, const std::string& axis) {
    double pts = growth_of(d, axis);
    int last = last_authored_node(b, axis);
    for (int n = 0; n <= last; ++n) {
        bool authored = false;
        for (auto& t : b.talents) if (t.axis == axis && t.node == n) authored = true;
        if (authored && b.nodes[static_cast<size_t>(n)] > pts) return b.nodes[static_cast<size_t>(n)];
    }
    const TalentAxis* a = axis_of(b, axis);
    return a ? rank_state(d, b, *a).next : -1;
}

std::vector<Pick> pending_picks(const World& w, const Params& p) {
    const TalentBook& b = p.talents;
    std::vector<Pick> out;
    for (auto& a : b.axes) {
        for (size_t n = 0; n < b.nodes.size(); ++n) {
            int node = static_cast<int>(n);
            if (growth_of(w.dragons[kPlayer], a.id) < b.nodes[n] || taken_at(w.dragons[kPlayer], b, a.id, node)) continue;
            Pick pk;
            for (auto& t : b.talents) if (t.axis == a.id && t.node == node) pk.options.push_back(t.id);
            if (pk.options.empty()) continue;
            pk.axis = a.id;
            pk.node = node;
            pk.points = b.nodes[n];
            pk.needs_phase = b.node_phase[n];
            pk.open = w.dragons[kPlayer].phase >= pk.needs_phase;
            out.push_back(pk);
        }
    }
    return out;
}

bool pick_talent_unrecorded(World& w, const Params& p, const std::string& id, std::string& reason) {
    const TalentDef* t = def_of(p.talents, id);
    if (!t) { reason = "unknown_talent|id=" + id; return false; }
    for (auto& pk : pending_picks(w, p)) {
        if (pk.axis != t->axis || pk.node != t->node) continue;
        if (!pk.open) { reason = "needs_phase|n=" + std::to_string(pk.needs_phase); return false; }
        w.dragons[kPlayer].talents.push_back(id);
        emit(w, "talent_taken", "dragon", id, pk.points, {t->axis}, because("growth", {}, {{"points", growth_of(w.dragons[kPlayer], t->axis)}}));
        return true;
    }
    reason = taken_at(w.dragons[kPlayer], p.talents, t->axis, t->node) ? "node_taken" : "node_not_reached";
    return false;
}

double effect(const World& w, const Params& p, const std::string& key) {
    Merge m = merge_of(key);
    double v = m == Merge::mult ? 1 : 0;
    for (auto& id : w.dragons[kPlayer].talents)
        if (const TalentDef* t = def_of(p.talents, id)) {
            auto it = t->effects.find(key);
            if (it != t->effects.end()) combine(v, it->second, m);
        }
    for (auto& a : p.talents.axes) {
        auto rank = a.rank_effects.find(key);
        auto mile = a.milestone_effects.find(key);
        if (rank == a.rank_effects.end() && mile == a.milestone_effects.end()) continue;
        int r = rank_state(w.dragons[kPlayer], p.talents, a).ranks;
        if (r > 0 && rank != a.rank_effects.end()) combine(v, rank->second, m, r);
        if (milestones(a, r) > 0 && mile != a.milestone_effects.end()) combine(v, mile->second, m, milestones(a, r));
    }
    return v;
}

std::map<std::string, double> merged_effects(const World& w, const Params& p) {
    std::map<std::string, double> out;
    for (auto& id : w.dragons[kPlayer].talents)
        if (const TalentDef* t = def_of(p.talents, id)) for (auto& kv : t->effects) out[kv.first] = 0;
    for (auto& a : p.talents.axes) {
        int r = rank_state(w.dragons[kPlayer], p.talents, a).ranks;
        if (r > 0) for (auto& kv : a.rank_effects) out[kv.first] = 0;
        if (milestones(a, r) > 0) for (auto& kv : a.milestone_effects) out[kv.first] = 0;
    }
    for (auto& kv : out) kv.second = effect(w, p, kv.first);
    return out;
}

// strings: the talent at the highest node wins (ties: the later pick); numbers: the largest, plus each rank's and each
// milestone's step
TalentLook merged_look(const World& w, const Params& p) {
    TalentLook out;
    std::vector<const TalentDef*> taken;
    for (auto& id : w.dragons[kPlayer].talents) if (const TalentDef* t = def_of(p.talents, id)) taken.push_back(t);
    std::stable_sort(taken.begin(), taken.end(), [](const TalentDef* a, const TalentDef* b) { return a->node < b->node; });
    for (const TalentDef* t : taken) {
        for (auto& kv : t->look.str) out.str[kv.first] = kv.second;
        for (auto& kv : t->look.num) out.num[kv.first] = std::max(out.num[kv.first], kv.second);
    }
    for (auto& a : p.talents.axes) {
        int r = rank_state(w.dragons[kPlayer], p.talents, a).ranks;
        if (r > 0) for (auto& kv : a.rank_look.num) out.num[kv.first] += kv.second * r;
        int steps = milestones(a, r);
        if (steps > 0) for (auto& kv : a.milestone_look.num) out.num[kv.first] += kv.second * steps;
    }
    return out;
}

}
