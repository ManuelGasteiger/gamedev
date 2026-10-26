#include "analyse.h"
#include "ages/memory_tiers.h"
#include <algorithm>
#include <memory>
#include "ages/defences.h"
#include "ages/dragon_acts.h"
#include "ages/growth.h"
#include "ages/json.h"
#include "ages/rng.h"
#include "ages/rules/slayers.h"
#include "ages/world_util.h"
#include "ages/wounds.h"

namespace ages {

using nlohmann::json;

namespace {

const int kBuckets[] = {1, 10, 30, 100, 300, 1000, 3000};

json snapshot(const World& w, const Params& p) {
    const Dragon& d = w.dragons[kPlayer];
    json j;
    j["year"] = w.year;
    j["since_hatch"] = w.year - w.genesis.hatched_year;
    j["hoard"] = {{"gold", d.hoard.gold}, {"books", d.hoard.books}, {"artifacts", d.hoard.artifacts}, {"art", d.hoard.art},
                  {"royals", d.hoard.royals}, {"food", d.hoard.food}, {"embers", d.hoard.embers}};
    j["growth"] = d.growth;
    j["phase"] = d.phase;
    j["wounds"] = d.wounds;
    j["health_max"] = health_max(w, p);
    j["slain"] = dragon_dead(w);
    json courts = json::array();
    for (auto& f : w.factions)
        courts.push_back({{"id", f.id}, {"race", f.race}, {"wealth", f.wealth}, {"era", f.era}, {"anti_dragon", f.anti_dragon},
                          {"dread", dread_eff(f)}, {"awe", awe_eff(f)}, {"faith", f.faith}, {"cohesion", f.cohesion}, {"regions", f.regions.size()}});
    j["courts"] = courts;
    return j;
}

int era_of(const World& w, const std::string& faction) {
    const Faction* f = find_faction(w, faction);
    return f ? f->era : -1;
}

json listed_wounds(const World& w, const Params& p, const std::vector<Act>& acts) {
    json by = json::array();
    double total = 0;
    for (auto& a : acts) {
        if (!harmful_act(a.type)) continue;
        double dmg = strike_damage(w, p, a);
        total += dmg;
        by.push_back({{"type", a.type}, {"target", a.target}, {"era", era_of(w, a.target)}, {"damage", dmg}, {"ring", inside_ring(w, p, a)}});
    }
    return {{"total", total}, {"acts", by}};
}

json buckets(const World& w, const Params& p, int index, int samples) {
    json out = json::array();
    for (int years : kBuckets) {
        double dead = 0, ready = 0, growth = 0, wound_share = 0;
        for (int k = 0; k < samples; ++k) {
            World c = w;
            c.recorder = nullptr;
            c.rng = rng_for(w.seed, "bucket", std::to_string(index) + ":" + std::to_string(years) + ":" + std::to_string(k));
            double before = total_growth(c.dragons[kPlayer]);
            sleep(c, p, years);
            bool slain = dragon_dead(c);
            dead += slain ? 1 : 0;
            bool any_ready = false;
            for (auto& f : c.factions) if (slayer_stage(c, f.id) == "slayers_ready") any_ready = true;
            ready += any_ready ? 1 : 0;
            growth += slain ? 0 : total_growth(c.dragons[kPlayer]) - before;
            wound_share += slain ? 1 : c.dragons[kPlayer].wounds / std::max(1.0, health_max(c, p));
        }
        out.push_back({{"years", years}, {"dead", dead / samples}, {"ready", ready / samples},
                       {"growth", growth / samples}, {"wounds", wound_share / samples}});
    }
    return out;
}

int towers(const World& w, const Params& p) {
    int n = 0;
    for (auto& r : w.regions)
        for (auto& v : region_defences(w, p, r))
            if (v.def && v.def->id != "levies" && (v.per_act > 0 || v.passive > 0)) { ++n; break; }
    return n;
}

int next_bucket(int years) {
    for (int b : kBuckets) if (b > years) return b;
    return years;
}

}

void attach_analysis(Scenario& s, const Params& p, const AnalyseOptions& o) {
    auto pending = std::make_shared<json>();
    auto chained = s.after_sleep;
    if (o.snapshots)
        s.after_sleep = [chained, &p, o](const World& w) {
            if (chained) chained(w);
            *o.snapshots << snapshot(w, p).dump() << "\n";
        };
    s.at_waking = [pending, &p, o](World& w, std::vector<Act>& acts, int index) {
        if (o.perturb == "burn" && index == 0)
            for (auto& a : acts) if (a.type == "burn") { a.magnitude = std::min(1.0, a.magnitude + 0.1); break; }
        if (o.snapshots && index == 0) *o.snapshots << snapshot(w, p).dump() << "\n";
        if (!o.wakings) return;
        json hooks = json::array();
        for (auto& h : w.hooks) hooks.push_back({{"kind", h.kind}, {"refs", h.refs}});
        *pending = {{"index", index}, {"year", w.year}, {"since_hatch", w.year - w.genesis.hatched_year},
                    {"phase", w.dragons[kPlayer].phase}, {"health_max", health_max(w, p)}, {"wounds", w.dragons[kPlayer].wounds},
                    {"listed", listed_wounds(w, p, acts)}, {"towers", towers(w, p)}, {"hooks", hooks}};
    };
    if (o.perturb == "sleep")
        s.adjust_sleep = [](int years, int index) { return index == 0 ? next_bucket(years) : years; };
    if (o.wakings)
        s.before_sleep = [pending, &p, o](const World& w, int index) {
            json line = pending->is_null() ? json{{"index", index}, {"year", w.year}} : *pending;
            if (o.buckets && !dragon_dead(w)) line["buckets"] = buckets(w, p, index, o.bucket_samples);
            *o.wakings << line.dump() << "\n";
            *pending = json();
        };
}

}
