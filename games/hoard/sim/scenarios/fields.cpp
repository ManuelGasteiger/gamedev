#include "fields.h"
#include <algorithm>
#include "ages/arrivals.h"
#include "ages/chron.h"
#include "ages/memory_tiers.h"
#include "ages/names.h"
#include "ages/offers.h"
#include "ages/person_record.h"
#include "ages/rules/slayers.h"
#include "ages/world_util.h"
#include "ages/wounds.h"

namespace scn {

using namespace ages;

namespace {

Value num(double v) { Value x; x.known = x.is_num = true; x.num = v; return x; }
Value word(const std::string& s) { Value x; x.known = true; x.str = s; return x; }
Value flag(bool b) { return num(b ? 1 : 0); }

const Region* lair_of(const World& w) {
    for (auto& r : w.regions) if (r.lair) return &r;
    return nullptr;
}

bool region_tag(const World& w, const Faction& f, const std::string& tag) {
    for (auto& rid : f.regions) if (const Region* r = find_region(w, rid)) if (has_tag(r->tags, tag)) return true;
    return false;
}

std::string stage_word(const World& w, const Faction& f) {
    if (has_tag(w.tags, "slayers_coming:" + f.id)) return "marching";
    std::string s = slayer_stage(w, f.id);
    return s == "slayers_ready" ? "ready" : s == "slayers_forming" ? "forming" : "none";
}

bool has_order(const World& w, const Faction& f) {
    if (has_tag(f.tags, "slayers")) return true;
    for (auto& o : w.orders) if (o.court == f.id) return true;
    return false;
}

}

Value faction_value(const Ctx& x, const Faction& f, const std::string& k) {
    const World& w = x.w;
    if (k == "dread") return num(dread_eff(f));
    if (k == "awe") return num(awe_eff(f));
    if (k == "memory_tier") return word(memory_tier_word(w, x.p, f));
    if (k == "era") return num(f.era);
    if (k == "cohesion") return num(f.cohesion);
    if (k == "faith_share") return num(f.faith);   // until item 12, as arrival_gates.cpp reads it
    if (k == "at_war") return flag(!f.at_war_with.empty());
    if (k == "piety") return num(f.culture.traits.piety);
    if (k == "openness") return num(f.culture.traits.openness);
    if (k == "militarism") return num(f.culture.traits.militarism);
    if (k == "has_slayers") return flag(has_order(w, f));
    if (k == "slayer_stage") return word(stage_word(w, f));
    if (k == "knows_prophecy") return flag(contains(w.prophecy.known_by, f.id));
    if (k == "portal_progress") return num(0);   // no portal rite in the sim yet
    if (k == "wealth_rank") { auto it = x.c.wealth_rank.find(f.id); return num(it == x.c.wealth_rank.end() ? 99 : it->second); }
    if (k == "race") return word(f.race);
    if (k == "is_vassal") return flag(is_vassal(f));
    if (k == "has_library") return flag(region_tag(w, f, "library"));
    if (k == "has_temple") return flag(region_tag(w, f, "temple") || awe_eff(f) >= x.p.c.mem.temple_awe);
    if (k == "cult_fervor") return num(0);   // item 12
    if (k == "name") return word(display_name(w, f.id));
    return Value();
}

namespace {

Value person_field(const Ctx& x, const Person& p, const std::string& k) {
    const World& w = x.w;
    if (k == "role") return word(p.role);
    if (k == "age") return num(w.year - p.born);
    if (k == "race") return word(p.race);
    if (k == "court") return word(person_court(w, p));
    if (k == "renown") {   // chronicle lines naming the person, renown_events of them for 1
        auto it = x.c.renown.find(p.id);
        return num(std::min(1.0, (it == x.c.renown.end() ? 0 : it->second) / std::max(1.0, x.p.scenarios.n.renown_events)));
    }
    if (k == "cunning") return num(p.traits.cunning);
    if (k == "is_lie") return flag(person_is_lie(w, p));
    Axes a = axes(x.p, p);
    if (k == "dread") return num(a.dread);
    if (k == "awe") return num(a.awe);
    if (k == "debt") return num(a.debt);
    if (k == "name") return word(p.name);
    if (k == "gender") return word(p.sex);
    return Value();
}

}

int memory_rank(const std::string& tier) {
    return tier == "monumental" ? 3 : tier == "written" ? 2 : tier == "oral" ? 1 : 0;
}

std::string memory_tier_word(const World&, const Params& p, const Faction& f) {
    std::string t = memory_tier(f, p.c.mem);
    return t.empty() ? "none" : t;
}

std::string size_word(const Numbers& n, double m) {
    for (auto& s : n.sizes) if (m <= s.first) return s.second;
    return n.sizes.empty() ? "" : n.sizes.back().second;
}

bool tag_match(const Tags& t, const std::string& tag) {
    if (t.count(tag)) return true;
    auto it = t.lower_bound(tag + ":");
    return it != t.end() && it->compare(0, tag.size() + 1, tag + ":") == 0;
}

std::string person_court(const World& w, const Person& x) {
    return contains(w.dragons[kPlayer].hostages, x.id) || has_tag(x.tags, "hostage") ? "lair" : x.faction;
}

bool person_is_lie(const World& w, const Person& x) {
    for (auto& o : w.offers) if (o.envoy == x.id && o.is_lie) return true;
    for (auto& a : w.arrivals) if (a.person == x.id && a.is_lie) return true;
    return false;
}

// the role words of spec section 3: the sim's own roles, the court in the lair, the court's envoy (the one at the
// ledge, else the one an offer would send: heir, ruler, sage, priest, hero), kept people by their keep role
bool person_role(const World& w, const Person& x, const std::string& role) {
    if (x.role == role) return true;
    if (role == "hostage") return person_court(w, x) == "lair";
    if (role == "envoy") {
        for (auto& a : w.arrivals) if (a.court == x.faction) return a.person == x.id;
        for (auto& o : w.offers) if (o.from_faction == x.faction) return o.envoy == x.id;
        static const char* const order[] = {"heir", "ruler", "sage", "priest", "hero"};
        for (const char* r : order) if (const Person* y = find_person_role(w, x.faction, r)) return y->id == x.id;
        return false;
    }
    if (role == "historian" || role == "consort" || role == "champion" || role == "ward") return has_tag(x.tags, "kept:" + role);
    return false;
}

bool faction_tag(const World& w, const Faction& f, const std::string& tag) {
    if (tag_match(f.tags, tag)) return true;
    for (auto& rid : f.regions) if (const Region* r = find_region(w, rid)) if (tag_match(r->tags, tag)) return true;
    return false;
}

// the person's own tags, or a court tag naming them (`false_dragon:<id>`, `inquisition:<id>`), or the roster and lair
bool person_tag(const World& w, const Person& x, const std::string& tag) {
    if (tag_match(x.tags, tag)) return true;
    if (const Faction* f = find_faction(w, x.faction)) if (has_tag(f->tags, tag + ":" + x.id)) return true;
    if (tag == "slayer") return !x.slayer.rank.empty();
    if (tag == "hostage") return person_court(w, x) == "lair";
    return x.role == tag;
}

Cache make_cache(const World& w, const Params& p) {
    Cache c;
    c.act_waking.resize(w.acts.size());
    for (size_t i = 0; i < w.acts.size(); ++i) {
        c.act_waking[i] = c.waking;
        c.acts_by_type[w.acts[i].type].push_back(static_cast<int>(i));
        if (w.acts[i].type == "sleep") c.waking += 1;
    }
    for (auto& e : w.chronicle) {
        if (is_person_id(w, e.subject)) c.renown[e.subject] += 1;
        if (!e.object.empty() && e.object != e.subject && is_person_id(w, e.object)) c.renown[e.object] += 1;
    }
    std::vector<const Faction*> fs;
    for (auto& f : w.factions) fs.push_back(&f);
    std::sort(fs.begin(), fs.end(), [](const Faction* a, const Faction* b) { return a->wealth != b->wealth ? a->wealth > b->wealth : a->id < b->id; });
    for (size_t i = 0; i < fs.size(); ++i) c.wealth_rank[fs[i]->id] = static_cast<int>(i) + 1;
    double best = -1;
    for (auto& f : w.factions) if (power(f, p.c) > best) { best = power(f, p.c); c.strongest = f.id; c.strongest_era = f.era; }
    // wakings in a row, newest first, in which the dragon ignored something (a hook or a scenario)
    std::vector<int> ignores(static_cast<size_t>(c.waking + 1), 0);
    for (size_t i = 0; i < w.acts.size(); ++i) if (w.acts[i].type == "ignore") ignores[static_cast<size_t>(c.act_waking[i])] += 1;
    for (int k = c.waking - 1; k >= 0 && ignores[static_cast<size_t>(k)] > 0; --k) c.ignored_streak += 1;
    return c;
}

int act_age_wakings(const Cache& c, int act) { return c.waking - 1 - c.act_waking[static_cast<size_t>(act)]; }

std::string act_court(const World& w, const Act& a) {
    if (!a.target.empty()) return a.target;
    if (const Region* r = find_region(w, a.region)) return r->owner;
    if (const Person* x = find_person(w, a.item)) return x->faction;
    return "";
}

std::string owner_of(const World& w, const std::string& id, const std::string& kind) {
    if (kind == "act") {
        int i = std::atoi(id.c_str());
        return i >= 0 && static_cast<size_t>(i) < w.acts.size() ? act_court(w, w.acts[static_cast<size_t>(i)]) : "";
    }
    if (const Person* x = find_person(w, id)) return x->faction;
    if (const Region* r = find_region(w, id)) return r->owner;
    if (find_faction(w, id)) return id;
    for (auto& r : w.dragons[kPlayer].chron.relics) if (r.id == id) return r.court;
    return "";
}

std::string Ctx::kind(const std::string& role) const {
    const Role* r = d.role(role);
    return r ? r->kind : "";
}

std::string Ctx::id(const std::string& role) const {
    auto it = bind.find(role);
    return it == bind.end() ? "" : it->second;
}

const Act* Ctx::act(const std::string& role) const {
    if (kind(role) != "act" || !bound(role)) return nullptr;
    int i = std::atoi(id(role).c_str());
    return i >= 0 && static_cast<size_t>(i) < w.acts.size() ? &w.acts[static_cast<size_t>(i)] : nullptr;
}

Value field(const Ctx& x, const std::string& role, const std::string& f) {
    const World& w = x.w;
    const Dragon& d = w.dragons[kPlayer];
    if (role == "dragon") {
        if (f == "might" || f == "flame" || f == "arcana" || f == "lore" || f == "dominion") { auto it = d.growth.find(f); return num(it == d.growth.end() ? 0 : it->second); }
        if (f == "phase") return num(d.phase);
        if (f == "wounds_share") return num(d.wounds / std::max(1.0, health_max(w, x.p)));
        if (f == "health") return num(health_max(w, x.p) - d.wounds);
        if (f == "talents") return num(static_cast<double>(d.talents.size()));
        if (f == "hostages") return num(static_cast<double>(d.hostages.size()));
        if (f == "sources") return num(static_cast<double>(d.chron.sources.size()));
        if (f == "stamina") return num(1);   // no stamina pool in the sim yet
        return Value();
    }
    if (role == "lair") {
        auto kept = [&](const char* r) { for (auto& p : w.people) if (has_tag(p.tags, std::string("kept:") + r)) return true; return false; };
        if (f == "has_historian") return flag(historian_now(w) != nullptr);
        if (f == "has_consort") return flag(kept("consort"));
        if (f == "has_library") return flag(has_tag(d.tags, "built:library"));
        if (f == "has_altar") return flag(has_tag(d.tags, "built:altar"));
        if (f == "has_vault") return flag(has_tag(d.tags, "built:vault"));
        if (f == "structures") { int n = 0; for (auto& t : d.tags) if (t.rfind("built:", 0) == 0) ++n; return num(n); }
        if (f == "region_name") { const Region* r = lair_of(w); return word(r ? display_name(w, r->id) : ""); }
        return Value();
    }
    if (role == "world") {
        if (f == "magic") return num(w.magic);
        if (f == "year") return num(w.year - w.genesis.hatched_year);
        if (f == "wakings") return num(x.c.waking);
        if (f == "ignored_streak") return num(x.c.ignored_streak);
        if (f == "strongest_era") return num(x.c.strongest_era);
        if (f == "chronicle_level") return num(historian_at(w, w.year) ? (lore_counterfactual(w, x.p) ? 3 : 2) : (d.chron.sources.empty() ? 0 : 1));
        if (f == "prophecy_ending") return word(w.prophecy.ending);
        return Value();
    }
    if (!x.bound(role)) return Value();
    std::string id = x.id(role), kind = x.kind(role);
    if (kind == "faction") { const Faction* fc = find_faction(w, id); return fc ? faction_value(x, *fc, f) : Value(); }
    if (kind == "person") { const Person* p = find_person(w, id); return p ? person_field(x, *p, f) : Value(); }
    if (kind == "region") {
        const Region* r = find_region(w, id);
        if (!r) return Value();
        if (f == "terrain") return word(r->terrain);
        if (f == "burn") return num(r->burn);
        if (f == "owner") return word(r->owner.empty() ? "none" : r->owner);
        if (f == "name") return word(display_name(w, r->id));
        return Value();
    }
    if (kind == "remnant") {
        for (auto& r : w.remnants) if (r.race == id) { if (f == "race") return word(r.race); if (f == "strength") return num(r.strength); }
        return Value();
    }
    if (kind == "item") {
        for (auto& r : d.chron.relics) if (r.id == id) {
            if (f == "agenda") return word(r.agenda);
            if (f == "kind") return word(r.kind);
            if (f == "origin_court") return word(r.court);
            if (f == "name") return word(r.kind);
        }
        return Value();
    }
    if (kind == "act") {
        const Act* a = x.act(role);
        if (!a) return Value();
        int i = std::atoi(id.c_str());
        if (f == "type" || f == "verb") return word(a->type);
        if (f == "magnitude") return num(a->magnitude);
        if (f == "years_ago") return num(w.year - a->year);
        if (f == "wakings_ago") return num(act_age_wakings(x.c, i));
        if (f == "size") return word(size_word(x.p.scenarios.n, a->magnitude));
        if (f == "target_name") return word(display_name(w, act_court(w, *a)));
        if (f == "place") {
            std::string r = a->region;
            if (r.empty()) if (const Faction* fc = find_faction(w, act_court(w, *a))) if (!fc->regions.empty()) r = fc->regions.front();
            return word(display_name(w, r));
        }
    }
    return Value();
}

bool has_tag_of(const Ctx& x, const std::string& role, const std::string& tag) {
    const World& w = x.w;
    if (role == "dragon") return tag_match(w.dragons[kPlayer].tags, tag);
    if (role == "world") return tag_match(w.tags, tag);
    if (!x.bound(role)) return false;
    std::string id = x.id(role), kind = x.kind(role);
    if (kind == "faction") { const Faction* f = find_faction(w, id); return f && faction_tag(w, *f, tag); }
    if (kind == "person") { const Person* p = find_person(w, id); return p && person_tag(w, *p, tag); }
    if (kind == "region") { const Region* r = find_region(w, id); return r && tag_match(r->tags, tag); }
    return false;
}

PredEnv env_of(const Ctx& x) {
    PredEnv e;
    e.bound = [&x](const std::string& r) { return r == "dragon" || r == "lair" || r == "world" || x.bound(r); };
    e.value = [&x](const std::string& r, const std::string& f) { return field(x, r, f); };
    e.has_tag = [&x](const std::string& r, const std::string& t) { return has_tag_of(x, r, t); };
    e.member = [&x](const std::string& r, const std::string& f, const std::string& v) -> int {
        if (r == "dragon" && f == "talents") return contains(x.w.dragons[kPlayer].talents, v) ? 1 : 0;
        if (f != "trait" || x.kind(r) != "person") return -1;
        const Person* p = find_person(x.w, x.id(r));
        return p && (has_trait(*p, v) || p->virtue == v || p->vice == v) ? 1 : 0;
    };
    return e;
}

bool gate_open(const World& w, const Params& p, const Bank& b, const std::string& gate, const Faction& f) {
    auto it = b.gates.find(gate);
    if (it == b.gates.end()) return false;
    for (auto& row : it->second) {
        std::string g = row.value("gate", std::string());
        bool ok = false;
        if (g == "knows_prophecy") ok = contains(w.prophecy.known_by, f.id);
        else if (g == "order_grievance") { for (auto& o : w.orders) if (o.court == f.id && o.grievance > 0) ok = true; }
        else {
            ArrivalGateDef d;
            d.gate = g;
            d.unless = row.value("unless", std::string());
            if (row.contains("gte")) { d.has_gte = true; d.gte = row["gte"].get<double>(); }
            if (row.contains("lte")) { d.has_lte = true; d.lte = row["lte"].get<double>(); }
            if (row.contains("gt")) { d.has_gt = true; d.gt = row["gt"].get<double>(); }
            if (row.contains("lt")) { d.has_lt = true; d.lt = row["lt"].get<double>(); }
            GateCtx gc{w, p, f};
            if (g == "motive") gc.motive = !envoy_drafts(w, p, f).empty();
            ok = gate_holds(gc, d);
        }
        if (ok) return true;
    }
    return false;
}

}
