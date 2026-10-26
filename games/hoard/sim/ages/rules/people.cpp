#include "people_life.h"
#include "../names.h"
#include "../world_util.h"

// 17.2: named mortals with race lifespans (races.json "person"). A ruler dies of age or when the reign counter of the
// cohesion rule runs out on an old ruler or a warlord; heirs are born a generation after the ruler and crowned. Who
// rises besides heirs is people_rise.cpp, the people in the lair hostages.cpp.

namespace ages {

namespace lives {

bool immortal(const Faction& f) { return f.ruler.cls == "immortal"; }

static const PersonLife& life_of(const Params& p, const std::string& race) {
    auto it = p.races.find(race);
    if (it == p.races.end()) it = p.races.find("human");
    static const PersonLife fallback;
    return it == p.races.end() ? fallback : it->second.person;
}

static int jittered(World& w, double base, double jitter) {
    return static_cast<int>(base * (1.0 + (w.rng.uniform() * 2 - 1) * jitter) + 0.5);
}

// the year the person dies of age (0 when the race is ageless); never before the next step
static int natural_death(World& w, const Params& p, const Person& x) {
    const PersonLife& l = life_of(p, x.race);
    if (l.lifespan <= 0) return 0;
    int span = std::min(jittered(w, l.lifespan, l.lifespan_jitter), static_cast<int>(l.max_age));
    return std::max(x.born + span, w.year + p.c.step_years);
}

// a mortal who enters the chronicle grown up: born accession_age (with jitter) ago
void grown(World& w, const Params& p, Person& x) {
    x.born = w.year - std::max(0, jittered(w, life_of(p, x.race).accession_age, p.c.accession_jitter));
    x.dies = natural_death(w, p, x);
}

// old enough that the reign counter's succession reads as a death of age
static bool old(const Params& p, const Person& x, int year) {
    const PersonLife& l = life_of(p, x.race);
    return l.lifespan > 0 && year - x.born >= l.lifespan * (1.0 - l.lifespan_jitter);
}

Person* ruler_of(World& w, const std::string& faction) { return find_person_role(w, faction, "ruler"); }
Person* heir_of(World& w, const std::string& faction) { return find_person_role(w, faction, "heir"); }

static void bury(World& w, const Person& person) {
    DeadPerson d;
    d.id = person.id;
    d.name = person.name;
    d.race = person.race;
    d.faction = person.faction;
    d.role = person.role;
    d.born = person.born;
    d.died = w.year;
    w.dead_people.push_back(d);
    if (Known* k = find_known(w, person.id)) k->tags.insert("dead");
    erase_value(w.dragons[kPlayer].hostages, person.id);
    std::string id = person.id;
    w.people.erase(std::remove_if(w.people.begin(), w.people.end(), [&](const Person& x) { return x.id == id; }), w.people.end());
}

int retire(World& w, Person person, const std::string& reason, const Cause& cause) {
    int ev = emit(w, "person_died", person.id, person.faction, 0, {reason, person.role}, cause);
    bury(w, person);
    return ev;
}

static void betroth(World& w, const Params& p, Faction& f, Person& heir) {
    if (!w.rng.chance(p.c.betrothal_chance)) return;
    std::vector<std::string> near;
    for (auto& g : w.factions) if (g.id != f.id && adjacent(w, f, g)) near.push_back(g.id);
    if (near.empty()) return;
    heir.tags.insert("betrothed:" + near[static_cast<size_t>(w.rng.range(0, static_cast<int>(near.size())))]);
}

// a child of the ruler, born when the ruler was about `generation` years old (never more than a generation ago): no
// heir while the ruler is too young to have one
static void make_heir(World& w, const Params& p, Faction& f, const Cause& cause) {
    if (immortal(f) || heir_of(w, f.id)) return;
    Person* ruler = ruler_of(w, f.id);
    if (!ruler) return;
    const PersonLife& l = life_of(p, ruler->race);
    if (ruler->born + static_cast<int>(l.generation) > w.year) return;
    Person heir = new_person(w, p, dominant_race(f), f.id, "heir");
    int born = ruler->born + jittered(w, l.generation, p.c.accession_jitter);
    heir.born = std::min(w.year, std::max(born, w.year - static_cast<int>(life_of(p, heir.race).generation)));
    heir.dies = natural_death(w, p, heir);
    betroth(w, p, f, heir);
    add_person(w, heir);
    emit(w, "person_rose", heir.id, f.id, 0, {"heir"}, cause);
}

void crown(World& w, const Params& p, Faction& f, Person& person, const std::string& how, const Cause& cause) {
    person.role = "ruler";
    for (auto it = person.tags.begin(); it != person.tags.end();) it = it->rfind("betrothed:", 0) == 0 ? person.tags.erase(it) : std::next(it);
    int crowned = emit(w, "person_crowned", person.id, f.id, 0, {how}, cause);
    if (has_tag(person.tags, "dragon_raised") && !has_tag(f.tags, "dragon_raised")) {
        f.tags.insert("dragon_raised");
        if (!contains(w.dragons[kPlayer].lineages, f.id)) w.dragons[kPlayer].lineages.push_back(f.id);
        emit(w, "dragon_raised_lineage", f.id, person.id, 0, {}, because("raised_in_lair", {crowned, last_event_id(w, "raised_in_lair", person.id)}));
    }
    make_heir(w, p, f, because("new_reign", {crowned}));
}

void crown_new(World& w, const Params& p, Faction& f, const std::string& how, const Cause& cause) {
    Person ruler = new_person(w, p, dominant_race(f), f.id, "ruler");
    grown(w, p, ruler);
    add_person(w, ruler);
    crown(w, p, f, *find_person(w, ruler.id), how, cause);
}

static void succeed(World& w, const Params& p, Faction& f, int death) {
    Person* heir = heir_of(w, f.id);
    if (heir && !has_tag(heir->tags, "hostage")) crown(w, p, f, *heir, "heir", because("succession", {death}));
    else crown_new(w, p, f, heir ? "regent" : "claimant", because(heir ? "heir_in_lair" : "no_heir", {death}));
}

// the faction has a ruler person at all times: a founding step creates one. The reign counter (rule_cohesion) takes a
// warlord, or a ruler old enough to die of age; a ruler crowned early after a natural death outlasts that reset
static void successions(World& w, const Params& p) {
    for (auto& f : w.factions) {
        Person* ruler = ruler_of(w, f.id);
        if (!ruler) {
            int founding = 0;
            for (const char* k : {"successor_founded", "orcs_spawned", "race_returned", "undead_raised"}) founding = std::max(founding, last_event_id(w, k, f.id));
            crown_new(w, p, f, "founded", because("founded", {founding}));
            continue;
        }
        if (immortal(f) || f.ruler.reigned != 0 || f.born == w.year) continue;
        bool warlord = f.ruler.cls == "warlord";
        if (!warlord && !old(p, *ruler, w.year)) continue;
        Person gone = *ruler;
        int death = retire(w, gone, warlord ? "warlord" : "old_age", because(warlord ? "warlord_death" : "old_age", {}, {{"age", static_cast<double>(w.year - gone.born)}}));
        succeed(w, p, f, death);
    }
}

// people whose faction vanished this step follow it: into the largest successor or the absorbing overlord, or they die
static void orphans(World& w, const Params& p) {
    std::vector<std::string> gone;
    for (auto& x : w.people) if (!x.faction.empty() && !find_faction(w, x.faction) && !contains(gone, x.faction)) gone.push_back(x.faction);
    for (auto& fid : gone) {
        std::string target;
        int cause_id = 0;
        const Faction* best = nullptr;
        for (auto& g : w.factions)
            if (has_tag(g.tags, "successor_of:" + fid) && g.born == w.year && (!best || g.regions.size() > best->regions.size() || (g.regions.size() == best->regions.size() && g.wealth > best->wealth))) best = &g;
        if (best) { target = best->id; cause_id = last_event_id(w, "collapse", fid); }
        else if (int ev = last_event_id(w, "vassal_absorbed", fid, "", "", w.year)) {
            if (const Event* e = find_event(w, ev)) { target = e->object; cause_id = ev; }
        } else cause_id = last_event_id(w, "faction_destroyed", fid);
        std::vector<std::string> ids;
        for (auto& x : w.people) if (x.faction == fid) ids.push_back(x.id);
        for (auto& id : ids) {
            Person* x = find_person(w, id);
            if (!x) continue;
            if (has_tag(x->tags, "hostage")) { x->faction = target; continue; }
            if (target.empty()) { retire(w, *x, "realm_fell", because("faction_destroyed", {cause_id})); continue; }
            if (x->role == "ruler") { retire(w, *x, "deposed", because("collapse", {cause_id})); continue; }
            x->faction = target;
            if (x->role == "heir") {
                Faction* t = find_faction(w, target);
                if (t && !ruler_of(w, target)) { crown(w, p, *t, *x, "heir", because("collapse", {cause_id})); continue; }
                x->role = "hero";
                x->tags.insert("claimant");
            }
        }
    }
}

// natural deaths, rulers included: a ruler who dies of age before the reign counter runs out is succeeded at once
static void lifespans(World& w, const Params& p) {
    std::vector<std::string> dying;
    for (auto& x : w.people) if (x.dies > 0 && w.year >= x.dies) dying.push_back(x.id);
    for (auto& id : dying) {
        Person* x = find_person(w, id);
        if (!x) continue;
        Person gone = *x;
        int death = retire(w, gone, "old_age", because("old_age", {}, {{"age", static_cast<double>(w.year - gone.born)}}));
        Faction* f = gone.role == "ruler" ? find_faction(w, gone.faction) : nullptr;
        if (f) succeed(w, p, *f, death);
    }
}

static void keep_heirs(World& w, const Params& p) {
    for (auto& f : w.factions)
        if (!immortal(f) && ruler_of(w, f.id) && !heir_of(w, f.id)) make_heir(w, p, f, because("line_continues"));
}

// sages and priests who were alive during one of the dragon's acts on their court carry its memory (17.2)
static void witness(World& w) {
    for (const Event& ev : events_this_step(w)) {
        if (ev.kind.rfind("act_", 0) != 0) continue;
        for (auto& x : w.people)
            if (x.faction == ev.subject && (x.role == "sage" || x.role == "priest") && x.memory_of_dragon < 1.0) x.memory_of_dragon = 1.0;
    }
}

}

using namespace lives;

void kill_person_now(World& w, const Params& p, const std::string& id, const std::string& reason, const Cause& cause) {
    (void)p;
    if (Person* person = find_person(w, id)) retire(w, *person, reason, cause);
}

void rule_people(World& w, const Params& p) {
    orphans(w, p);
    successions(w, p);
    lifespans(w, p);
    heroes(w, p);
    sages_and_priests(w, p);
    name_reigns(w, p);
    raise_hostages(w, p);
    witness(w);
    keep_heirs(w, p);
}

void init_people(World& w, const Params& p) {
    for (auto& f : w.factions) {
        if (ruler_of(w, f.id)) continue;
        Person ruler = new_person(w, p, dominant_race(f), f.id, "ruler");
        grown(w, p, ruler);
        add_person(w, ruler);
        make_heir(w, p, f, because("line_continues"));
    }
}

Person new_adult(World& w, const Params& p, const std::string& race, const std::string& faction, const std::string& role) {
    Person x = new_person(w, p, race, faction, role);
    grown(w, p, x);
    return x;
}

// a person outside the line of succession takes the throne: the sitting ruler is deposed
void crown_person(World& w, const Params& p, Faction& f, const std::string& id, const std::string& how, const Cause& cause) {
    Person* x = find_person(w, id);
    if (!x) return;
    if (Person* old = ruler_of(w, f.id)) if (old->id != id) retire(w, *old, "deposed", cause);
    x = find_person(w, id);
    if (!x) return;
    x->faction = f.id;
    x->tags.erase("hostage");
    crown(w, p, f, *x, how, cause);
}

}
