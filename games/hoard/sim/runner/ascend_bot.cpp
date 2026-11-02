#include "ascend_bot.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <sstream>
#include "ages/arrivals.h"
#include "ages/endings.h"
#include "ages/memory_tiers.h"
#include "ages/rules/card_ctx.h"
#include "ages/world_util.h"
#include "ages/wounds.h"

using namespace ages;
using nlohmann::json;

namespace {

struct Knobs {
    int courts = 5, races = 3;
    bool stamina = true;
    std::map<std::string, double> cost, magnitude, take_payload;
    double give_gold = 1, show_dread_max = 0.4;
    std::vector<std::string> accept_offers;
    std::string sleep = "short";
};

double at(const std::map<std::string, double>& m, const std::string& k) {
    auto it = m.find(k);
    return it == m.end() ? 0 : it->second;
}

std::string capital(const World& w, const Faction& f) {
    std::string best;
    double top = -1;
    for (auto& rid : f.regions)
        if (const Region* r = find_region(w, rid)) {
            double y = r->eff_land * (1.0 - r->burn);
            if (y > top) { top = y; best = rid; }
        }
    return best;
}

// the page's witnesses: the owners of the regions next to the act's region
std::vector<std::string> witnesses(const World& w, const std::string& region, const std::string& target) {
    std::set<std::string> ids;
    if (const Region* r = find_region(w, region))
        for (auto& n : r->neighbours)
            if (const Region* nr = find_region(w, n)) if (!nr->owner.empty() && nr->owner != target) ids.insert(nr->owner);
    return {ids.begin(), ids.end()};
}

std::vector<std::string> attackers(const World& w, const Faction& f) {
    std::vector<std::string> out;
    for (auto& x : w.wars) {
        if (x.b != f.id) continue;
        out.push_back(x.a);
        out.insert(out.end(), x.allies_a.begin(), x.allies_a.end());
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

double score(const Faction& f) { return awe_eff(f) + f.faith - dread_eff(f); }

// a target that vanished passes to its largest successor
std::string follow(const World& w, const std::string& id) {
    if (find_faction(w, id)) return id;
    const Faction* best = nullptr;
    for (auto& f : w.factions)
        if (has_tag(f.tags, "successor_of:" + id) && (!best || f.regions.size() > best->regions.size())) best = &f;
    return best ? best->id : "";
}

void pick_targets(const World& w, const Knobs& k, std::vector<std::string>& targets) {
    std::vector<std::string> kept;
    for (auto& id : targets) {
        std::string now = follow(w, id);
        if (!now.empty() && !contains(kept, now)) kept.push_back(now);
    }
    targets = kept;
    std::vector<const Faction*> pool;
    for (auto& f : w.factions) if (!contains(targets, f.id)) pool.push_back(&f);
    std::stable_sort(pool.begin(), pool.end(), [](const Faction* a, const Faction* b) { return score(*a) > score(*b); });
    auto races_of = [&]() {
        std::set<std::string> r;
        for (auto& id : targets) if (const Faction* f = find_faction(w, id)) r.insert(dominant_race(*f));
        return r;
    };
    for (auto it = pool.begin(); it != pool.end() && static_cast<int>(races_of().size()) < k.races && static_cast<int>(targets.size()) < k.courts;)
        if (!races_of().count(dominant_race(**it))) { targets.push_back((*it)->id); it = pool.erase(it); }
        else ++it;
    for (auto* f : pool) {
        if (static_cast<int>(targets.size()) >= k.courts) break;
        targets.push_back(f->id);
    }
}

Act act_on(const World& w, const Knobs& k, const std::string& type, const Faction& f) {
    Act a;
    a.type = type;
    a.target = f.id;
    a.region = capital(w, f);
    a.magnitude = at(k.magnitude, type);
    a.witnesses = witnesses(w, a.region, f.id);
    return a;
}

// one waking's acts, most valuable first, cut at the stamina budget
std::vector<Act> plan(World& w, const Params& p, const Knobs& k, const std::vector<std::string>& targets) {
    std::vector<Act> out;
    double budget = 0;
    if (k.stamina) {
        auto base = [&](const char* key) { auto it = p.talents.flight.find(key); return it == p.talents.flight.end() ? 0.0 : it->second; };
        double total = 0;
        for (auto& kv : w.dragons[kPlayer].growth) total += kv.second;
        budget = base("stamina_base") + base("stamina_per_sqrt_points") * std::sqrt(total);
    }
    auto spend = [&](const std::string& type) {
        if (!k.stamina) return true;
        if (budget < at(k.cost, type)) return false;
        budget -= at(k.cost, type);
        return true;
    };
    double pool = w.dragons[kPlayer].faith_pool;
    for (int pass = 0; pass < 5; ++pass)
        for (auto& id : targets) {
            const Faction* f = find_faction(w, id);
            if (!f) continue;
            if (pass == 0) {
                for (auto& att : attackers(w, *f)) {
                    if (contains(targets, att) || !spend("defend")) continue;
                    Act a = act_on(w, k, "defend", *f);
                    a.against = att;
                    out.push_back(a);
                }
            } else if (pass == 1) {
                if (pool < p.c.act_miracle_faith_cost || !spend("miracle")) continue;
                pool -= p.c.act_miracle_faith_cost;
                out.push_back(act_on(w, k, "miracle", *f));
            } else if (pass == 2) {
                if (f->faith <= p.c.reaction_faith_high || !spend("accept_offering")) continue;
                out.push_back(act_on(w, k, "accept_offering", *f));
            } else if (pass == 3) {
                if (!spend("give")) continue;
                Act a = act_on(w, k, "give", *f);
                a.payload["gold"] = k.give_gold;
                out.push_back(a);
            } else if (dread_eff(*f) <= k.show_dread_max && spend("show")) {
                out.push_back(act_on(w, k, "show", *f));
            }
        }
    const Faction* rich = nullptr;
    for (auto& f : w.factions) if (!contains(targets, f.id) && (!rich || f.wealth > rich->wealth)) rich = &f;
    if (rich && spend("take")) {
        Act a = act_on(w, k, "take", *rich);
        a.payload = k.take_payload;
        out.push_back(a);
    }
    return out;
}

void answer(World& w, const Params& p, const Knobs& k, const std::vector<std::string>& targets) {
    for (size_t i = 0; i < w.offers.size();) {
        const Offer& of = w.offers[i];
        if (contains(targets, of.from_faction) && contains(k.accept_offers, of.kind) && accept_offer(w, p, i)) continue;
        ++i;
    }
    std::vector<std::string> waiting;
    for (auto& a : w.arrivals) if (contains(targets, a.court)) waiting.push_back(a.id);
    for (auto& id : waiting) { std::string reason; receive(w, p, id, "receive", reason); }
}

const EndingDef* ascension(const Params& p) {
    for (auto& e : p.endings.endings) if (e.id == "ascension" && e.enabled) return &e;
    return nullptr;
}

void best(json& j, const std::string& key, double v) {
    if (!j.contains(key) || j[key].get<double>() < v) j[key] = v;
}

// per waking: each count of each condition (courts and races), and the parts: awe, faith, both, dread, dominion
void sample(const World& w, const Params& p, json& probe) {
    const EndingDef* e = ascension(p);
    if (!e) return;
    json& counts = probe["counts"];
    for (auto& c : e->conds) {
        int i = 0;
        for (auto& q : c.when) {
            std::vector<const CardPred*> qs;
            if (q.type == "count") qs.push_back(&q);
            if (q.type == "any") for (auto& a : q.any) if (a.type == "count") qs.push_back(&a);
            for (auto* cq : qs) {
                int n = 0;
                std::set<std::string> races;
                double dread = 0;
                for (auto& f : w.factions) {
                    CardCtx ctx;
                    ctx.vars["self"] = "world";
                    ctx.vars["year"] = std::to_string(w.year);
                    ctx.vars[cq->bind.empty() ? "court" : cq->bind] = f.id;
                    if (!card_holds(w, p, cq->where, ctx)) continue;
                    ++n;
                    races.insert(dominant_race(f));
                    dread += dread_eff(f);
                }
                std::string key = c.id + "." + std::to_string(i++);
                best(counts, key + ".courts", n);
                best(counts, key + ".races", static_cast<double>(races.size()));
                if (n >= 4 && !cq->mean.empty()) {
                    double mean = dread / n;
                    if (!counts.contains(key + ".mean_dread_min") || counts[key + ".mean_dread_min"].get<double>() > mean) counts[key + ".mean_dread_min"] = mean;
                }
            }
        }
    }
    json& parts = probe["parts"];
    int awe = 0, faith = 0, both = 0;
    std::set<std::string> awe_races;
    double top_awe = 0, top_faith = 0;
    for (auto& f : w.factions) {
        top_awe = std::max(top_awe, awe_eff(f));
        top_faith = std::max(top_faith, f.faith);
        if (awe_eff(f) >= 0.6) { ++awe; awe_races.insert(dominant_race(f)); }
        if (f.faith >= 0.4) ++faith;
        if (awe_eff(f) >= 0.6 && f.faith >= 0.4) ++both;
    }
    best(parts, "courts_awe_0.6", awe);
    best(parts, "races_awe_0.6", static_cast<double>(awe_races.size()));
    best(parts, "courts_faith_0.4", faith);
    best(parts, "courts_both", both);
    best(parts, "max_awe", top_awe);
    best(parts, "max_faith", top_faith);
    best(parts, "courts", static_cast<double>(w.factions.size()));
    best(parts, "magic", w.magic);
    auto g = w.dragons[kPlayer].growth.find("dominion");
    best(parts, "dominion", g == w.dragons[kPlayer].growth.end() ? 0 : g->second);
    int acts = 0;
    for (auto& a : w.acts) if (a.type == "give" || a.type == "miracle" || a.type == "consecrate") ++acts;
    best(parts, "give_miracle_acts", acts);
    int wakings = probe.value("wakings", 0);
    probe["wakings"] = wakings + 1;
    probe["awe_both_sum"] = probe.value("awe_both_sum", 0) + both;
}

}

bool attach_ascend_bot(Scenario& s, const Params& p, bool free, AscendBot& out, std::string& err) {
    std::ifstream in(s.data_dir + "/bots/ascension.json");
    if (!in) { err = "cannot read bots/ascension.json"; return false; }
    std::stringstream ss;
    ss << in.rdbuf();
    json j = json::parse(ss.str(), nullptr, false);
    if (j.is_discarded()) { err = "bots/ascension.json is not json"; return false; }
    auto k = std::make_shared<Knobs>();
    k->courts = j.value("courts", 5);
    k->races = j.value("races", 3);
    k->stamina = j.value("stamina", true) && !free;
    k->cost = j.value("cost", std::map<std::string, double>());
    k->magnitude = j.value("magnitude", std::map<std::string, double>());
    k->take_payload = j.value("take_payload", std::map<std::string, double>());
    k->give_gold = j.value("give_gold", 1.0);
    k->show_dread_max = j.value("show_dread_max", 0.4);
    k->accept_offers = j.value("accept_offers", std::vector<std::string>());
    k->sleep = j.value("sleep", std::string("short"));
    out.base = std::make_shared<Policy>();
    if (!load_policy(s.data_dir, j.value("base_policy", std::string("steer_ascension")), *out.base, err)) return false;
    out.probe = std::make_shared<json>(json::object());
    s.policy = out.base.get();
    auto targets = std::make_shared<std::vector<std::string>>();
    auto probe = out.probe;
    s.at_waking = [k, targets, probe, &p](World& w, std::vector<Act>& acts, int) {
        sample(w, p, *probe);
        acts.clear();
        std::string reason;
        if (ending_ready(w, "ascension") && exit_run(w, p, "ascension", reason)) return;
        pick_targets(w, *k, *targets);
        (*probe)["targets"] = *targets;
        acts = plan(w, p, *k, *targets);
        answer(w, p, *k, *targets);
    };
    if (k->sleep == "short") { out.base->sleep_cycle = {"short"}; out.base->sleep_open.clear(); }
    return true;
}

void finish_ascend_probe(const World& w, const Params& p, json& probe) {
    const EndingDef* e = ascension(p);
    const EndingState* st = ending_state(w, "ascension");
    if (!e) return;
    json conds = json::object();
    for (size_t i = 0; i < e->conds.size(); ++i) {
        const CondHold* h = st && i < st->conds.size() ? &st->conds[i] : nullptr;
        conds[e->conds[i].id] = {{"ever_met", h && h->ever_met}, {"best_years", h ? h->best_years : 0}};
    }
    probe["conditions"] = conds;
    probe["ready"] = st && st->ready;
    probe["ended"] = w.ended;
    probe["slain"] = dragon_dead(w);
    probe["year"] = w.year;
}
