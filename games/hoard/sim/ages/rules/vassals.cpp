#include "rules.h"
#include "coalitions.h"
#include "../world_util.h"

namespace ages {

// the vassal leaves every war it is in; its own wars end with "submission"
void make_vassal(World& w, Faction& vassal, const Faction& overlord, const Cause& cause) {
    for (auto it = vassal.tags.begin(); it != vassal.tags.end();)
        it = it->rfind("vassal_of:", 0) == 0 || it->rfind("ally:", 0) == 0 ? vassal.tags.erase(it) : std::next(it);
    vassal.tags.insert("vassal_of:" + overlord.id);
    vassal.last_invaded = w.year;
    std::vector<War> gone;
    for (auto& x : w.wars) if (x.a == vassal.id || x.b == vassal.id) gone.push_back(x);
    for (auto& x : gone) end_war(w, x, "submission", because("submitted", {}, {{"years", static_cast<double>(w.year - x.started)}}));
    for (auto& x : w.wars) { erase_value(x.allies_a, vassal.id); erase_value(x.allies_b, vassal.id); }
    for (auto& f : w.factions) {
        erase_value(f.at_war_with, vassal.id);
        f.tags.erase("ally:" + vassal.id);
        if (overlord_of(f) == vassal.id) {
            f.tags.erase("vassal_of:" + vassal.id);
            const Event* sworn = find_event(w, last_event_id(w, "vassal_sworn", f.id));
            double years = static_cast<double>(w.year - (sworn ? sworn->year : f.born));
            emit(w, "vassal_freed", f.id, vassal.id, 0, {"overlord_submitted"}, because("overlord_submitted", {}, {{"years", years}}));
        }
    }
    vassal.at_war_with.clear();
    emit(w, "vassal_sworn", vassal.id, overlord.id, 0, {}, cause, "vassalage:" + vassal.id + ":" + std::to_string(w.year));
}

// a vassal that collapses is annexed by the overlord instead of splitting off again (vassal_absorb: "none",
// "same_race" or "all"); its people shift the overlord's pop like a conquest
static void absorb(World& w, const Params& p, const Faction vassal, Faction& lord) {
    const Constants& c = p.c;
    for (auto& rid : vassal.regions) transfer_region(w, rid, vassal.id, lord.id);
    lord.wealth += vassal.wealth;
    shift_pop(lord, p, vassal.pop, c.conquest_pop_shift);
    blend_traits(lord.culture.traits, vassal.culture.traits, c.conquest_pop_shift);
    emit(w, "vassal_absorbed", vassal.id, lord.id, static_cast<double>(vassal.regions.size()), {},
         because("vassal_collapsed", {last_event_id(w, "vassal_sworn", vassal.id)}, {{"cohesion", vassal.cohesion}}));
    remove_faction(w, vassal.id);
}

static void free_vassal(World& w, Faction& vassal, const std::string& lord, const char* reason, const Cause& cause) {
    vassal.tags.erase("vassal_of:" + lord);
    int sworn = last_event_id(w, "vassal_sworn", vassal.id);
    Cause c = cause;
    add_ref(c, sworn);
    int since = vassal.born;   // a successor that inherited its parent's fealty has no oath of its own
    std::string thread;
    if (const Event* e = find_event(w, sworn)) { since = e->year; thread = e->thread; }
    c.params["years"] = static_cast<double>(w.year - since);
    emit(w, "vassal_freed", vassal.id, lord, 0, {reason}, c, thread);
}

void rule_vassals(World& w, const Params& p) {
    const Constants& c = p.c;
    double k = dts(w, c);
    std::vector<std::string> absorbing;
    for (auto& f : w.factions) {
        std::string lord = overlord_of(f);
        if (lord.empty()) continue;
        Faction* o = find_faction(w, lord);
        if (!o) {
            if (w.rng.chance(c.vassal_independence_chance * k))
                free_vassal(w, f, lord, "overlord_gone", because("overlord_gone", {std::max(last_event_id(w, "collapse", lord), last_event_id(w, "faction_destroyed", lord))}));
            continue;
        }
        double due = f.last_growth * c.vassal_tribute;
        if (due > 0) {
            f.wealth = std::max(0.0, f.wealth - due);
            o->wealth += due;
        }
        if (o->cohesion < c.vassal_overlord_cohesion && w.rng.chance(c.vassal_independence_chance * k))
            free_vassal(w, f, lord, "overlord_weak", because("overlord_weak", {weakness_cause(w, c, *o)}, {{"cohesion", o->cohesion}}));
        else if (f.cohesion < c.collapse_cohesion &&
                 (c.vassal_absorb == "all" || (c.vassal_absorb == "same_race" && dominant_race(f) == dominant_race(*o))))
            absorbing.push_back(f.id);
    }
    for (auto& id : absorbing) {
        const Faction* f = find_faction(w, id);
        Faction* o = f ? find_faction(w, overlord_of(*f)) : nullptr;
        if (f && o) absorb(w, p, *f, *o);
    }
}

}
