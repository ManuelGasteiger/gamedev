#include "slayer_roster.h"
#include <algorithm>
#include "growth.h"
#include "names.h"
#include "person_record.h"
#include "talents.h"
#include "world_util.h"

namespace ages {

Order* find_order(World& w, const std::string& court) {
    for (auto& o : w.orders) if (o.court == court) return &o;
    return nullptr;
}

const Order* find_order(const World& w, const std::string& court) {
    for (auto& o : w.orders) if (o.court == court) return &o;
    return nullptr;
}

std::vector<Person*> roster(World& w, const std::string& court) {
    std::vector<Person*> out;
    for (auto& x : w.people) if (x.faction == court && !x.slayer.rank.empty()) out.push_back(&x);
    return out;
}

std::vector<const Person*> roster(const World& w, const std::string& court) {
    std::vector<const Person*> out;
    for (auto& x : w.people) if (x.faction == court && !x.slayer.rank.empty()) out.push_back(&x);
    return out;
}

const Person* grandmaster(const World& w, const std::string& court) {
    for (auto& x : w.people) if (x.faction == court && x.slayer.rank == "grandmaster") return &x;
    return nullptr;
}

Person* grandmaster(World& w, const std::string& court) {
    for (auto& x : w.people) if (x.faction == court && x.slayer.rank == "grandmaster") return &x;
    return nullptr;
}

double power_total(const World& w, const Params& p, const Order& o) {
    double sum = o.hunters * p.slayers.hunter_power;
    for (const Person* x : roster(w, o.court)) sum += x->slayer.power;
    return sum;
}

std::string dragon_axis(const World& w) {
    std::string best;
    double top = 0;
    for (auto& kv : w.dragons[kPlayer].growth)
        if (kv.second > top) { top = kv.second; best = kv.first; }
    return best;
}

bool counters_dragon(const World& w, const Params& p, const Person& x) {
    auto it = p.slayers.counters.find(x.slayer.counter);
    return it != p.slayers.counters.end() && it->second == dragon_axis(w);
}

static bool any_trait(const std::vector<std::string>& wanted, const Person& x) {
    for (auto& t : wanted) if (has_trait(x, t)) return true;
    return false;
}

bool is_zealot(const Params& p, const Person& x) { return any_trait(p.slayers.zealots, x); }
bool is_opportunist(const Params& p, const Person& x) { return any_trait(p.slayers.opportunists, x); }

std::vector<std::string> weaknesses(const Params& p, const Person& x) {
    std::vector<std::string> out;
    if (!x.slayer.fear.empty()) out.push_back("fear:" + x.slayer.fear);
    auto own = p.slayers.counters.find(x.slayer.counter);
    for (auto& kv : p.slayers.counters)
        if (own == p.slayers.counters.end() || kv.second != own->second) out.push_back("axis:" + kv.second);
    return out;
}

bool knows(const Person& x, const std::string& kind) { return contains(x.slayer.known, kind); }

void reveal(Person& x, int n) {
    for (const char* k : kTraitKinds) {
        if (n <= 0) return;
        if (knows(x, k)) continue;
        x.slayer.known.push_back(k);
        --n;
    }
}

void reveal_all(Person& x) { reveal(x, 3); }

void raise_power(const Params& p, Person& x, int by) {
    x.slayer.power = std::max(1, std::min(p.slayers.power_cap, x.slayer.power + by));
}

void remember(World& w, const Params& p, Person& x, const MemoryRow& row, const std::string& src) {
    remember_dragon(p, x, memory_entry(w, p, x, row, src));
}

bool add_scar(World& w, const Params& p, Person& x) {
    auto it = p.slayers.scars.find(dragon_axis(w));
    if (it == p.slayers.scars.end()) return false;
    x.slayer.scars.push_back(it->second);
    if (static_cast<int>(x.slayer.scars.size()) > p.slayers.scar_max) x.slayer.scars.erase(x.slayer.scars.begin());
    acquire(p, x, "scarred");
    remember(w, p, x, p.slayers.scarred, "witness");
    return true;
}

static std::string pick(Rng& r, const std::vector<std::string>& from) {
    if (from.empty()) return "";
    int i = r.range(0, static_cast<int>(from.size()));
    return from[static_cast<size_t>(i)];
}

void enrol(World& w, const Params& p, Person& x, const std::string& rank) {
    const SlayerBook& b = p.slayers;
    Rng r = rng_for(w.seed, "slayer", x.id);
    std::vector<std::string> counters;
    for (auto& kv : b.counters) counters.push_back(kv.first);
    x.slayer.counter = pick(r, counters);
    x.slayer.hate = pick(r, b.hates);
    x.slayer.fear = pick(r, b.fears);
    x.slayer.power = b.power_start_min + r.range(0, b.power_start_max - b.power_start_min + 1);
    x.slayer.rank = rank;
    x.slayer.last_fight = w.year;
    remember(w, p, x, b.grief, "told");
}

static const PersonLife& life_of(const Params& p, const std::string& race) {
    auto it = p.races.find(race);
    if (it == p.races.end()) it = p.races.find("human");
    static const PersonLife fallback;
    return it == p.races.end() ? fallback : it->second.person;
}

static int jittered(Rng& r, double base, double jitter) {
    double u = r.uniform();
    return static_cast<int>(base * (1.0 + (u * 2 - 1) * jitter) + 0.5);
}

Person* recruit(World& w, const Params& p, Order& o, const std::string& rank) {
    Faction* f = find_faction(w, o.court);
    if (!f) return nullptr;
    std::string race = dominant_race(*f);
    Rng r = rng_for(w.seed, "recruit", o.id + "|" + std::to_string(w.year) + "|" + std::to_string(w.person_counters[race] + 1));
    Person x = new_person(w, p, race, o.court, "hero", r);
    const PersonLife& l = life_of(p, race);
    x.born = w.year - std::max(0, jittered(r, l.accession_age, p.c.accession_jitter));
    if (l.lifespan > 0) {
        int span = std::min(jittered(r, l.lifespan, l.lifespan_jitter), static_cast<int>(l.max_age));
        x.dies = std::max(x.born + span, w.year + p.c.step_years);
    }
    x.tags.insert("slayer");
    add_person(w, x);
    Person* made = find_person(w, x.id);
    enrol(w, p, *made, rank);
    return made;
}

static int rank_level(const std::string& rank) { return rank == "grandmaster" ? 3 : rank == "master" ? 2 : rank == "slayer" ? 1 : 0; }

// the rank below first; among equals the opportunist (for a master), then the higher power, then the lower id
bool promote(World& w, const Params& p, const std::string& court, const std::string& rank, const std::string& exclude) {
    int want = rank_level(rank);
    Person* best = nullptr;
    for (int level = want - 1; level >= 1 && !best; --level) {
        for (Person* x : roster(w, court)) {
            if (x->id == exclude || rank_level(x->slayer.rank) != level) continue;
            bool better = !best || x->slayer.power > best->slayer.power ||
                          (x->slayer.power == best->slayer.power && rank == "master" && is_opportunist(p, *x) && !is_opportunist(p, *best));
            if (better) best = x;
        }
    }
    if (!best) return false;
    best->slayer.rank = rank;
    emit(w, rank == "grandmaster" ? "grandmaster_named" : "slayer_promoted", best->id, court, best->slayer.power, {rank}, because("slayer_promoted"));
    return true;
}

void rival_gain(World& w, const Params& p, const Person& harmed) {
    for (auto& r : harmed.rel) {
        if (r.kind != "rival") continue;
        if (Person* x = find_person(w, r.to)) if (!x->slayer.rank.empty()) raise_power(p, *x, p.slayers.rival_gain);
    }
}

void slayer_died(World& w, const Params& p, const Person& dead) {
    if (dead.slayer.rank.empty()) return;
    for (Person* x : roster(w, dead.faction)) {
        if (x->id == dead.id) continue;
        for (auto& r : x->rel) if (r.kind == "mentor" && r.to == dead.id) remember(w, p, *x, p.slayers.mentor_grief, "witness");
    }
    if (dead.slayer.rank != "grandmaster" || !promote(w, p, dead.faction, "grandmaster", dead.id)) return;
    Person* heir = nullptr;
    for (Person* x : roster(w, dead.faction)) if (x->id != dead.id && x->slayer.rank == "grandmaster") heir = x;
    if (!heir) return;
    for (auto* v : {&dead.mem, &dead.permanent})
        for (auto m : *v) {
            if (m.awe + m.debt - m.dread >= 0) continue;
            m.s *= p.slayers.inherit_negative;
            if (m.s < p.moves.drop_below) continue;
            m.src = "told";
            remember_dragon(p, *heir, m);
        }
}

}
