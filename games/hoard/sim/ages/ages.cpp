#include <cstdio>
#include "record.h"
#include "unrecorded.h"
#include "ages.h"
#include "growth.h"
#include "memory_tiers.h"
#include "oaths.h"
#include "offers.h"
#include "rules/rules.h"
#include "rules/slayers.h"
#include "slayer_roster.h"
#include <algorithm>
#include "arrivals.h"
#include "endings.h"
#include "talents.h"
#include "world_util.h"

namespace ages {

LoadResult load_data_from_json(const DataJson& d, uint64_t seed) {
    LoadResult r;
    if (!params_from_json(d, r.params, r.error)) return r;
    r.params.data_hash = data_hash(d);
    if (!world_from_data_json(d, r.params, r.world, r.error)) return r;
    r.world.seed = seed;
    r.world.rng.seed(seed);
    for (auto& f : r.world.factions) {
        jitter_traits(f, r.params.c, r.world.rng);
        init_faction_ruler(f, r.params.c, r.world.rng);
    }
    note_race_eras(r.world);
    rule_modifiers(r.world, r.params);
    init_people(r.world, r.params);
    hatch(r.world, r.params);   // v0.4 (S1)
    r.ok = true;
    return r;
}

// accepting swears the draft; an offer that asks nothing but an act (teach, bless, judge, ...) still becomes an oath
// record so that the act can be matched to it later (fulfilment, lies). An elope or hide_heir takes the hostage now.
bool accept_offer_unrecorded(World& w, const Params& p, size_t index) {
    if (index >= w.offers.size()) return false;
    Offer of = w.offers[index];
    if (of.kind == "exit") { std::string reason; return exit_unrecorded(w, p, of.motive, reason); }   // item 04
    Oath o = of.draft;
    o.status = "active";
    w.oaths.push_back(o);
    Act sw;   // item 09: swear as a logged act
    sw.year = w.year;
    sw.type = "swear";
    sw.target = of.from_faction;
    sw.source = "lair";
    sw.answers = of.id;
    sw.args["answers_offer"] = of.id;
    w.acts.push_back(sw);
    const OfferDef* def = offer_def(p, of.kind);
    Tags tags = {of.text_key};
    if (def) for (auto& t : def->tags) tags.insert(t);
    int sworn = emit(w, "oath_sworn", o.id, of.from_faction, 0, tags, because("offer_accepted", {}, {{"lie", of.is_lie ? 1.0 : 0.0}}), "oath:" + o.id);
    if (!of.envoy.empty()) meet(w, of.envoy, "offer", p.c.known_accept);
    envoy_returned(w, p, of);
    if (of.kind == "hide_heir" || of.kind == "elope") {
        if (Person* x = find_person(w, of.target)) {
            Faction* f = find_faction(w, x->faction);
            take_hostage_person(w, p, *x, true, because("offer_accepted", {sworn}));
            if (f) {   // a hidden heir is a consent hostage (awe), an elopement a theft (dread)
                auto g = p.gains.find(of.kind == "hide_heir" ? "take_hostage.consent" : "take_hostage");
                Deposit d;
                d.type = of.kind;
                d.magnitude = 1.0;
                d.tags = {of.kind == "hide_heir" ? "gift" : "harm"};
                if (g != p.gains.end()) { d.dread = g->second.dread * effect(w, p, "dread_mult"); d.awe = g->second.awe; }
                deposit(w, p, *f, d);
                if (of.kind == "hide_heir") remember(*f, p.c, w.year, "hide_heir", 1.0, p.c.offer_consent_faith * f->eff.faith_gain, {"gift"});
                else remember(*f, p.c, w.year, "elope", 1.0, -0.1, {"harm"});
            }
            for (auto& oa : w.oaths) if (oa.id == o.id) oa.fulfilled = true;
        }
    }
    w.offers.erase(w.offers.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

static std::string num(double v) { char b[32]; std::snprintf(b, sizeof b, "%g", v); return b; }

// item 09: what a sworn clause does at once (hostage, disband, tear_down); pilgrimage and no_harm_to are kept over time
static void demand_now(World& w, const Params& p, Faction& f, const std::string& pred, const std::string& what, int sworn) {
    if (pred == "hostage") {
        Person* x = pick_hostage(w, f.id);
        if (!x || has_tag(x->tags, "hostage")) return;
        f.tags.insert("hostage_taken");
        if (!contains(w.dragons[kPlayer].lineages, f.id)) w.dragons[kPlayer].lineages.push_back(f.id);
        take_hostage_person(w, p, *x, false, because("demanded", {sworn}));
    } else if (pred == "disband") {
        for (const char* t : {"slayers", "slayers_forming", "slayers_ready"}) f.tags.erase(t);
        w.tags.erase("slayers_coming:" + f.id);
        emit(w, "order_disbanded", f.id, "", 0, {}, because("demanded", {sworn}));
        Person* gm = grandmaster(w, f.id);
        std::string gm_id = gm ? gm->id : "";
        for (Person* x : roster(w, f.id)) if (x->id != gm_id) x->slayer = Slayer();   // item 08's roster
        w.orders.erase(std::remove_if(w.orders.begin(), w.orders.end(), [&](const Order& o) { return o.court == f.id; }), w.orders.end());
        gm = find_person(w, gm_id);
        const Faction* to = nullptr;
        for (auto& g : w.factions) if (g.id != f.id && adjacent(w, f, g) && (!to || power(g, p.c) > power(*to, p.c))) to = &g;
        if (gm && to) {
            std::string id = gm->id, dest = to->id;
            gm->faction = dest;
            gm->tags.insert("grudge");
            emit(w, "grandmaster_fled", id, dest, 0, {}, because("disbanded", {last_event_id(w, "order_disbanded", f.id)}));
        }
    } else if (pred == "tear_down") {
        for (auto& rid : f.regions)
            if (Region* r = find_region(w, rid)) r->tags.erase(what);
        if (what == "temple" || what == "altar") raze_monuments(f, "", "");
        emit(w, "structure_torn_down", f.id, what, 0, {}, because("demanded", {sworn}));
    }
}

DemandResult demand_oath_unrecorded(World& w, const Params& p, const std::string& target, const std::string& pred,
                                    const std::map<std::string, std::string>& args) {
    const Constants& c = p.c;
    DemandResult r;
    r.target = target;
    r.pred = pred;
    Faction* f = find_faction(w, target);
    if (!f) { r.reason = "no_faction"; return r; }
    static const char* const kClauses[] = {"tribute", "no_walls", "no_settle", "no_harm_to", "hostage", "disband", "tear_down", "pilgrimage"};
    bool known = false;
    for (const char* k : kClauses) if (pred == k) known = true;
    if (!known) { r.reason = "unknown_demand|pred=" + pred; return r; }
    auto argv = [&](const char* k) { auto it = args.find(k); return it == args.end() ? std::string() : it->second; };
    std::string protect = argv("faction"), what = argv("structure");
    if (pred == "no_harm_to" && (protect.empty() || protect == target || !find_faction(w, protect))) { r.reason = "demand.no_ward|pred=" + pred; return r; }
    if (pred == "disband" && !has_tag(f->tags, "slayers")) { r.reason = "demand.no_order"; return r; }
    if (pred == "tear_down" && what.empty()) { r.reason = "demand.no_structure"; return r; }
    double knowledge = c.demand_knowledge * effect(w, p, "demand_mult");   // v0.4: Old Tongues
    if (w.dragons[kPlayer].knowledge < knowledge) { r.reason = "needs_knowledge|n=" + num(knowledge); return r; }
    if (active_oath_with_pred(w, f->id, pred)) { r.reason = "already_sworn"; return r; }
    Act a;
    a.year = w.year;
    a.type = "demand";
    a.target = f->id;
    a.item = pred;
    a.source = "lair";
    a.args["clause"] = pred;
    if (!protect.empty()) a.args["faction"] = protect;
    if (!what.empty()) a.args["structure"] = what;
    w.acts.push_back(a);
    double dread = dread_eff(*f);
    bool cowed = dread >= c.mem.demand_dread || power(*f, c) < c.reaction_weak_ratio * threat(w.dragons[kPlayer], c);
    if (const Person* ruler = find_person_role(w, f->id, "ruler")) meet(w, ruler->id, "demand", cowed ? 0 : c.known_show);
    auto it = p.gains.find(cowed ? "demand.sworn" : "demand.refused");
    if (it != p.gains.end()) {
        Deposit d;
        d.type = "demand";
        d.magnitude = 1;
        d.dread = it->second.dread * effect(w, p, "dread_mult");
        d.awe = it->second.awe;
        d.act = static_cast<int>(w.acts.size()) - 1;
        deposit(w, p, *f, d);
    }
    if (!cowed) {
        f->anti_dragon += c.demand_refused_resolve;
        emit(w, "demand_refused", f->id, pred, dread, {}, because("not_cowed", {}, {{"dread", dread}}));
        r.reason = "not_cowed|dread=" + num(dread) + "|need=" + num(c.mem.demand_dread);
        return r;
    }
    Oath o;
    o.id = "demand_" + f->id + "_" + std::to_string(w.year);
    o.parties[0] = "dragon";
    o.parties[1] = f->id;
    o.sworn = w.year;
    o.expires = w.year + c.offer_expiry_years;
    Clause cl;
    cl.who = f->id;
    cl.pred = pred;
    if (pred == "tribute") cl.params["amount"] = f->wealth * c.demand_tribute_rate;
    o.clauses.push_back(cl);
    o.breach_by_them = "curse";
    o.breach_by_dragon = "none";
    o.status = "active";
    o.kind = "demand";
    o.target = protect;
    o.ask = what;
    w.oaths.push_back(o);
    int sworn = emit(w, "oath_sworn", o.id, f->id, 0, {"demand." + pred}, because("cowed", {}, {{"dread", dread}}), "oath:" + o.id);
    demand_now(w, p, *f, pred, what, sworn);
    r.sworn = true;
    r.oath = o;
    return r;
}

bool decline_offer_unrecorded(World& w, const Params& p, size_t index) {
    if (index >= w.offers.size()) return false;
    Offer of = w.offers[index];
    if (!of.envoy.empty()) meet(w, of.envoy, "offer", p.c.known_decline);
    envoy_returned(w, p, of);
    Act a;
    a.type = "show";
    a.target = of.from_faction;
    a.magnitude = 0.1;
    a.source = "lair";
    a.answers = of.id;
    apply_acts(w, p, {a});
    emit(w, "offer_declined", of.from_faction, of.envoy, 0, {of.kind}, because("declined", {last_event_id(w, "act_show", of.from_faction, "", "", w.year)}));
    w.offers.erase(w.offers.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

}
