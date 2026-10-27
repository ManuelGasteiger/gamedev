#include "offers.h"
#include "talents.h"
#include "memory_tiers.h"
#include "unrecorded.h"
#include "ages.h"
#include "oaths.h"
#include "rules/rules.h"
#include "world_util.h"

// 17.3: one row of data/offers.json per kind; the trigger predicates live here, the motives, pretexts and prices in the data

namespace ages {

const OfferDef* offer_def(const Params& p, const std::string& kind) {
    for (auto& d : p.offers) if (d.kind == kind && d.enabled) return &d;
    return nullptr;
}

static int max_era(const World& w) {
    int m = 0;
    for (auto& f : w.factions) m = std::max(m, f.era);
    return m;
}

static std::string capital_of(const World& w, const Faction& f) {
    std::string best;
    double top = -1;
    for (auto& rid : f.regions)
        if (const Region* r = find_region(w, rid)) {
            double y = r->eff_land * (1.0 - r->burn);
            if (y > top) { top = y; best = rid; }
        }
    return best;
}

static const Faction* strongest_enemy(const World& w, const Constants& c, const Faction& f) {
    const Faction* best = nullptr;
    for (auto& e : f.at_war_with)
        if (const Faction* o = find_faction(w, e)) if (!best || power(*o, c) > power(*best, c)) best = o;
    return best;
}

static const Faction* hegemon_neighbour(const World& w, const Faction& f) {
    for (auto& g : w.factions) if (g.id != f.id && has_tag(g.tags, "hegemon") && adjacent(w, f, g)) return &g;
    return nullptr;
}

static const Person* envoy_for(const World& w, const Faction& f, const std::string& role) {
    static const char* const fallback[] = {"heir", "ruler", "sage", "priest", "hero"};
    if (const Person* x = find_person_role(w, f.id, role)) return x;
    for (const char* r : fallback) if (const Person* x = find_person_role(w, f.id, r)) return x;
    for (auto& x : w.people) if (x.faction == f.id) return &x;
    return nullptr;
}

static bool was_deceived(const Faction& f) {
    for (auto& t : f.tags) if (t.rfind("deceived_dragon:", 0) == 0) return true;
    return false;
}

struct Draft {
    const OfferDef* def = nullptr;
    std::string target, motive, region;
    const Person* envoy = nullptr;
    bool plan_to_slay_true = false;
};

// the envoys' lies roll on an rng derived from the world stream (as the forecast does), so the history itself does not
// depend on how many courts came to the lair
static const uint64_t kOfferSalt = 0x0FFE25A17ull;

static void add(World& w, Rng& rng, const Params& p, const Faction& f, const Draft& d) {
    const Constants& c = p.c;
    Offer of;
    of.kind = d.def->kind;
    of.id = "offer_" + f.id + "_" + of.kind + "_" + std::to_string(w.year);
    of.from_faction = f.id;
    of.envoy = d.envoy ? d.envoy->id : "";
    of.target = d.target;
    of.motive = d.motive;
    of.text_key = "offer." + of.kind;
    of.expires = w.year + d.def->expires_years;
    of.price = f.wealth * d.def->price_rate;
    of.marked = was_deceived(f);
    double cunning = d.envoy ? d.envoy->traits.cunning : 0.5;
    bool lie = d.def->can_lie && !d.def->pretexts.empty() && awe_eff(f) < c.mem.honest_awe &&
               rng.chance(c.offer_lie_base + c.offer_lie_authority * f.culture.traits.authority + c.offer_lie_cunning * cunning);
    if (lie) {
        of.stated_reason = d.def->pretexts[static_cast<size_t>(rng.range(0, static_cast<int>(d.def->pretexts.size())))];
        of.is_lie = !(of.stated_reason == "plan_to_slay" && d.plan_to_slay_true);
    } else of.stated_reason = of.motive;
    Oath& o = of.draft;
    o.id = of.id;
    o.parties[0] = "dragon";
    o.parties[1] = f.id;
    o.sworn = w.year;
    o.expires = of.expires;
    for (auto& pred : d.def->clauses) {
        Clause cl;
        cl.who = pred == "tribute" ? f.id : "dragon";
        cl.pred = pred;
        if (pred == "tribute") cl.params["amount"] = of.price;
        o.clauses.push_back(cl);
    }
    o.breach_by_dragon = contains(d.def->clauses, "defend") || contains(d.def->clauses, "no_harm") ? "betrayal" : "none";
    o.ask = d.def->pred;
    o.target = d.target;
    o.motive = of.motive;
    o.kind = of.kind;
    o.envoy = of.envoy;
    o.is_lie = of.is_lie;
    o.price = of.price;
    w.offers.push_back(of);
}

static void offers_of(World& w, Rng& rng, const Params& p, const Faction& f) {
    const Constants& c = p.c;
    auto propose = [&](const char* kind, const std::string& target, const std::string& motive, const std::string& region = "", bool slay_true = false) {
        const OfferDef* def = offer_def(p, kind);
        if (!def) return;
        Draft d;
        d.def = def;
        d.target = target;
        d.motive = motive.empty() ? def->motive : motive;
        d.region = region;
        d.plan_to_slay_true = slay_true;
        d.envoy = def->envoy == "self" ? find_person(w, target) : envoy_for(w, f, def->envoy);
        add(w, rng, p, f, d);
    };
    if (losing(w, c, f)) {
        const Faction* e = strongest_enemy(w, c, f);
        propose("protect", e ? e->id : "", "survival");
    } else if (const Faction* h = hegemon_neighbour(w, f)) propose("protect", h->id, "survival");

    for (auto& g : w.factions) {
        if (g.id == f.id || !adjacent(w, f, g)) continue;
        if (power(g, c) <= power(f, c) || affinity(p, f, g) >= c.offer_affinity_rival) continue;
        bool revenge = last_event_id(w, "region_taken", g.id, "", f.id, w.year - c.offer_act_years) > 0;
        propose("burn_rival", g.id, revenge ? "revenge" : "fear_of_conquest", "", has_tag(g.tags, "slayers"));
        break;
    }
    for (auto& id : w.dragons[kPlayer].hostages)
        if (const Person* x = find_person(w, id)) if (x->faction == f.id) { propose("return_hostage", id, "succession"); break; }

    bool threatened = has_tag(f.tags, "civil_war");
    for (auto& x : w.people) if (x.faction == f.id && (has_tag(x.tags, "usurper") || has_tag(x.tags, "claimant")) && x.role != "ruler") threatened = true;
    if (threatened)
        if (const Person* heir = find_person_role(w, f.id, "heir")) if (!has_tag(heir->tags, "hostage")) propose("hide_heir", heir->id, "protect_the_line");

    if (f.culture.traits.openness > c.offer_teach_openness && f.era < max_era(w) && p.spells.count("teach")) propose("teach", f.id, "catch_up");

    int blow = std::max(last_event_id(w, "plague", f.id, "", "", w.year - c.offer_act_years), 0);
    std::string hurt;
    for (auto& rid : f.regions) if (last_event_id(w, "bad_century", rid, "", "", w.year - c.offer_act_years)) hurt = rid;
    if ((blow || !hurt.empty()) && f.faith > c.offer_bless_faith && p.spells.count("bless")) propose("bless", hurt.empty() ? capital_of(w, f) : hurt, "hunger", hurt);

    for (auto& x : w.people)
        if (x.faction == f.id && x.role == "hero" && (has_tag(x.tags, "claimant") || has_tag(x.tags, "usurper"))) { propose("judge", x.id, "legitimacy"); break; }

    for (auto& g : w.factions)
        if (g.id != f.id && has_tag(g.tags, "undead") && adjacent(w, f, g)) { propose("purge", g.id, "survival"); break; }

    if (f.faith > c.reaction_faith_high && f.culture.traits.piety > c.offer_ward_piety && p.spells.count("ward")) propose("ward", capital_of(w, f), "protect_the_altar");

    if (f.faith > c.offer_faith) propose("tribute", "", "awe");

    for (auto& x : w.people) {
        if (x.faction != f.id || x.traits.bold < c.offer_elope_bold) continue;
        std::string match;
        for (auto& t : x.tags) if (t.rfind("betrothed:", 0) == 0) match = t.substr(10);
        const Faction* g = match.empty() ? nullptr : find_faction(w, match);
        if (g && affinity(p, f, *g) < c.offer_elope_affinity) { propose("elope", x.id, "escape_marriage"); break; }
    }
    if (f.era >= c.offer_exit_era && f.faith > c.offer_exit_faith) propose("rite", "", "exit");
}

void generate_offers(World& w, const Params& p) {
    w.offers.clear();
    Rng rng;
    rng.seed(w.rng.state ^ kOfferSalt);
    for (auto& f : w.factions) offers_of(w, rng, p, f);
}

// ---- fulfilment and the ledger -------------------------------------------------------------------------------------

static bool contains_pred(const Oath& o, const std::string& pred) {
    for (auto& cl : o.clauses) if (cl.pred == pred) return true;
    return false;
}

static bool matches(const Oath& o, const Act& a) {
    if (o.ask.empty() || o.fulfilled || o.status != "active") return false;
    bool same = a.type == o.ask || (a.type == "spell" && a.spell == o.ask);
    if (!same) return false;
    // a defence is asked for the court itself, against the enemy the offer names
    if (a.type == "defend") return a.target == o.parties[1] && (o.target.empty() || a.against == o.target);
    if (o.target.empty()) return a.target == o.parties[1];
    return a.target == o.target || a.region == o.target || a.item == o.target;
}

static void fulfil(World& w, const Params& p, Oath& o, const Act& a) {
    const Constants& c = p.c;
    o.fulfilled = true;
    Faction* f = find_faction(w, o.parties[1]);
    if (!f) return;
    int sworn = last_event_id(w, "oath_sworn", o.id);
    int act = last_event_id(w, "act_" + a.type, a.target, "", "", w.year);
    if (o.is_lie) {
        f->tags.insert("deceived_dragon:" + o.id);
        if (Known* k = find_known(w, o.envoy)) { k->relationship = std::max(-1.0, k->relationship + c.known_deceived); k->tags.insert("liar"); }
        emit(w, "deceived_dragon", f->id, o.envoy, 0, {o.kind, o.motive}, because("acted_on_lie", {sworn, act}), "oath:" + o.id);
        return;
    }
    double target = std::min(1.0, f->faith * c.offer_honest_faith_mult);
    if (target > f->faith) set_faith(*f, c, w.year, "offer_fulfilled", 1.0, target, {"gratitude"});
    if (o.price > 0 && !contains_pred(o, "tribute")) w.dragons[kPlayer].hoard.gold += o.price;
    if (Known* k = find_known(w, o.envoy)) k->relationship = std::min(1.0, k->relationship + c.known_help);
    emit(w, "offer_fulfilled", f->id, o.envoy, f->faith, {o.kind, o.motive}, because("honest_offer", {sworn, act}, {{"faith", f->faith}, {"gold", o.price}}), "oath:" + o.id);
}

static void ledger(World& w, const Params& p, const Act& a, double m) {
    const Constants& c = p.c;
    double delta = 0;
    std::string how = a.type;
    if (a.type == "burn" || a.type == "kill" || a.type == "take") delta = c.known_harm * m;
    else if (a.type == "defend" || a.type == "give" || a.type == "miracle") delta = c.known_help * std::max(m, 0.5);
    else if (a.type == "show") delta = c.known_show;
    else return;
    // a show, a defence or a gift is the dragon at the court itself: the ruler is met; harm only moves existing ledgers
    bool introduces = a.type == "show" || a.type == "defend" || a.type == "give";
    for (auto& x : w.people) {
        if (x.faction != a.target || (x.role != "ruler" && x.role != "heir")) continue;
        if ((introduces && x.role == "ruler") || find_known(w, x.id)) meet(w, x.id, how, delta);
    }
}

void note_act(World& w, const Params& p, const Act& a, double m) {
    for (auto& o : w.oaths) if (matches(o, a)) fulfil(w, p, o, a);
    ledger(w, p, a, m);
}

bool kill_person_unrecorded(World& w, const Params& p, const std::string& id, std::string& reason, std::vector<ReactionLine>* lines) {
    const Constants& c = p.c;
    Person* x = find_person(w, id);
    if (!x) { reason = "no_person"; return false; }
    Faction* f = find_faction(w, x->faction);
    if (!f) { reason = "no_court"; return false; }
    Act a;
    a.type = "kill";
    a.target = f->id;
    a.magnitude = c.envoy_kill_magnitude;
    a.item = id;
    for (auto& g : w.factions) a.witnesses.push_back(g.id);
    bool envoy = false;
    for (auto& of : w.offers) if (of.envoy == id) envoy = true;
    if (envoy) for (auto& r : w.regions) if (r.lair) a.region = r.id;   // v0.4: an envoy dies in the lair, out of reach of its court's defences
    std::vector<ReactionLine> told = apply_acts(w, p, {a});
    if (lines) *lines = told;
    Deposit d;
    d.type = "kill_person";
    d.magnitude = 1.0;
    d.dread = c.envoy_kill_dread * effect(w, p, "dread_mult");
    d.act = static_cast<int>(w.acts.size()) - 1;
    d.tags = {envoy ? "envoy_killed" : "murder"};
    deposit(w, p, *f, d);
    meet(w, id, "kill", c.known_kill_envoy);
    int act = last_event_id(w, "act_kill", f->id, "", "", w.year);
    kill_person_now(w, p, id, "killed_by_dragon", because(envoy ? "envoy_killed" : "dragon_will", {act}));
    w.offers.erase(std::remove_if(w.offers.begin(), w.offers.end(), [&](const Offer& o) { return o.envoy == id || o.target == id; }), w.offers.end());
    return true;
}

}
