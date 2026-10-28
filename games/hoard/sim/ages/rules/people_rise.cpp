#include "people_life.h"
#include "../names.h"
#include "../world_util.h"

// 17.2: who rises at court besides the heir. Great rulers name their reign; heroes rise from the step's events and by
// renown, may usurp a weak throne and die leading a failed slaying; sages and priests appear with era and traits. A
// sage, priest or hero of renown takes up the oldest memory the court still carries (catalogue insight 5).

namespace ages {

namespace lives {

static void name_reign(World& w, const Params& p, Person& ruler, Faction& f, const char* tag, double& trait, double gain, int ev) {
    if (has_tag(ruler.tags, tag)) return;
    ruler.tags.insert(tag);
    trait = clamp01(trait + gain);
    auto ep = p.names.epithets.find(tag);
    if (ep != p.names.epithets.end()) {
        size_t cut = ruler.name.find(" the ");
        ruler.name = (cut == std::string::npos ? ruler.name : ruler.name.substr(0, cut)) + " " + ep->second;
    }
    emit(w, "reign_named", ruler.id, f.id, 0, {tag}, because(tag, {ev}));
}

void name_reigns(World& w, const Params& p) {
    for (auto& f : w.factions) {
        Person* ruler = ruler_of(w, f.id);
        if (!ruler) continue;
        if (int ev = last_event_id(w, "tyrant_rises", f.id, "", "", w.year)) name_reign(w, p, *ruler, f, "tyrant", ruler->traits.greedy, p.c.tyrant_greedy, ev);
        else if (int ev2 = last_event_id(w, "golden_age", f.id, "", "", w.year)) name_reign(w, p, *ruler, f, "golden", ruler->traits.pious, p.c.golden_pious, ev2);
    }
}

static int heroes_of(const World& w, const std::string& faction) {
    int n = 0;
    for (auto& x : w.people) if (x.faction == faction && x.role == "hero") n += 1;
    return n;
}

// the act behind a memory entry: on the faction itself or on a realm it descends from (successors inherit memory)
static int act_event_at(const World& w, const std::string& kind, const Faction& f, int year) {
    for (size_t i = w.chronicle.size(); i > 0; --i) {
        const Event& e = w.chronicle[i - 1];
        if (e.year < year) break;
        if (e.year == year && e.kind == kind && (e.subject == f.id || has_tag(f.tags, "successor_of:" + e.subject))) return e.id;
    }
    return 0;
}

// the oldest memory the court still carries: a mark an event card left on it (data/events) or a dragon act it has not
// yet forgotten; `tag` names the card mark, empty for a dragon memory
static int oldest_memory(const World& w, const Params& p, const Faction& f, std::string& tag) {
    int best = 0, year = w.year + 1;
    for (auto& t : w.timed_tags)
        if (t.on == "faction" && t.target == f.id && t.event > 0 && t.from < year) { year = t.from; best = t.event; tag = t.tag; }
    for (auto& m : f.memory) {
        if (m.year >= year || m.weight < p.c.keeper_min_weight) continue;
        if (int ev = act_event_at(w, "act_" + m.type, f, m.year)) { year = m.year; best = ev; tag.clear(); }
    }
    return best;
}

// a person of this role at this court has kept that memory before: the chronicle names a memory once per line of
// keepers, and only once it is old enough to be news (keeper_named_years)
static bool named_before(const World& w, const Params& p, const std::string& faction, const std::string& role, int kept) {
    const Event* k = find_event(w, kept);
    if (!k || w.year - k->year < p.c.keeper_named_years) return true;
    for (size_t i = w.chronicle.size(); i > 0; --i) {
        const Event& e = w.chronicle[i - 1];
        if (e.kind == "person_rose" && e.object == faction && e.tags.count(role) && contains_ref(e.cause, kept)) return true;
        if (e.id > 0 && e.id < kept) return false;
    }
    return false;
}

// a hero of renown takes up the court's oldest cause (sworn_to:<mark>, or sworn_to:dragon for a dragon memory)
static void rise_hero(World& w, const Params& p, Faction& f, const std::string& why, Cause cause) {
    if (heroes_of(w, f.id) >= p.c.hero_max) return;
    Person hero = new_person(w, p, dominant_race(f), f.id, "hero");
    grown(w, p, hero);
    hero.tags.insert(why);
    std::string tag;
    if (why == "renown")
        if (int kept = oldest_memory(w, p, f, tag)) {
            hero.tags.insert("sworn_to:" + (tag.empty() ? std::string("dragon") : tag));
            cause = because(named_before(w, p, f.id, "hero", kept) ? "renown" : "sworn_hero", {kept});
        }
    add_person(w, hero);
    emit(w, "person_rose", hero.id, f.id, hero.traits.bold, {"hero", why}, cause);
}

void heroes(World& w, const Params& p) {
    const Constants& c = p.c;
    for (auto& f : w.factions) {
        if (int ev = last_event_id(w, "slayers_founded", f.id, "", "", w.year)) rise_hero(w, p, f, "slayer", because("slayers_founded", {ev}));
        if (int ev = last_event_id(w, "coalition_formed", f.id, "", "", w.year)) rise_hero(w, p, f, "captain", because("coalition", {ev}));
        for (const Event& ev : events_this_step(w)) {
            if (ev.kind == "war_ended" && ev.object == f.id && (ev.tags.count("conquest") || ev.tags.count("annihilation"))) {
                rise_hero(w, p, f, "general", because("lost_war", {ev.id}));
                break;
            }
            if (ev.kind == "war_started" && ev.tags.count("civil_war") && (ev.subject == f.id || ev.object == f.id)) {
                rise_hero(w, p, f, "claimant", because("civil_war", {ev.id}));
                break;
            }
        }
        if (w.rng.chance(c.hero_chance * dts(w, c))) rise_hero(w, p, f, "renown", because("renown"));
    }
    // a failed slay attempt this step kills the hero who led it
    for (const Event& ev : events_this_step(w)) {
        if (ev.kind != "slay_attempt_failed" || ev.object.empty()) continue;
        Person* lead = find_person(w, ev.object);
        if (lead) retire(w, p, *lead, "slain_by_dragon", because("slay_attempt_failed", {ev.id}));
    }
    // usurpation: a bold hero takes a weak throne and the realm rallies to the new line
    for (auto& f : w.factions) {
        if (immortal(f) || f.cohesion >= c.hero_usurp_cohesion) continue;
        Person* hero = nullptr;
        for (auto& x : w.people) if (x.faction == f.id && x.role == "hero" && x.traits.bold >= c.hero_usurp_bold && (!hero || x.traits.bold > hero->traits.bold)) hero = &x;
        if (!hero || !w.rng.chance(c.hero_usurp_chance * dts(w, c))) continue;
        std::string hid = hero->id;
        int deposed = 0;
        if (Person* old = ruler_of(w, f.id)) deposed = retire(w, p, *old, "deposed", because("usurped", {}, {{"cohesion", f.cohesion}}));
        Person* h = find_person(w, hid);
        h->tags.insert("usurper");
        f.ruler.reigned = 0;
        f.ruler.reign_length = std::max(1, h->dies - w.year);
        f.cohesion = clamp01(f.cohesion + c.hero_unite_cohesion);
        crown(w, p, f, *h, "usurped", because("usurpation", {deposed}, {{"cohesion", f.cohesion}}));
    }
}

// 17.2: a new sage or priest takes up the oldest memory of the court; keeping a dragon memory slows its decay
// (memory_of_dragon), keeping a card's mark names the keeper for the cards that read it (keeper:<mark>)
static void rise_keeper(World& w, const Params& p, Faction& f, const std::string& role, Cause cause) {
    Person s = new_person(w, p, dominant_race(f), f.id, role);
    grown(w, p, s);
    std::string tag;
    if (int kept = oldest_memory(w, p, f, tag)) {
        s.tags.insert(tag.empty() ? "keeper" : "keeper:" + tag);
        if (tag.empty()) s.memory_of_dragon = 1.0;
        cause = because(named_before(w, p, f.id, role, kept) ? "keeper_again" : "keeper", {kept});
    }
    add_person(w, s);
    emit(w, "person_rose", s.id, f.id, 0, {role}, cause);
}

void sages_and_priests(World& w, const Params& p) {
    const Constants& c = p.c;
    for (auto& f : w.factions) {
        if (f.era < c.sage_min_era) continue;
        if (f.culture.traits.openness > c.sage_openness && !find_person_role(w, f.id, "sage"))
            rise_keeper(w, p, f, "sage", because("learning", {last_event_id(w, "era_advance", f.id)}, {{"openness", f.culture.traits.openness}}));
        if (f.culture.traits.piety > c.priest_piety && !find_person_role(w, f.id, "priest"))
            rise_keeper(w, p, f, "priest", because("piety", {}, {{"piety", f.culture.traits.piety}}));
    }
}

}

}
