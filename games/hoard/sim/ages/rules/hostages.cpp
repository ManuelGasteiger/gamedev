#include "people_life.h"
#include "../moves.h"
#include "../person_record.h"
#include "../world_util.h"

// 17.2: hostages are people in the lair. The heir (else the ruler) is taken; a ruler taken leaves the throne to the
// heir or a regent; one raised there long enough carries `dragon_raised`; release sends them home.

namespace ages {

namespace lives {

// a hostage held hostage_raise_years grows up in the lair: tag dragon_raised, and the faction's lineage follows
void raise_hostages(World& w, const Params& p) {
    std::vector<std::string> ids = w.dragons[kPlayer].hostages;
    for (auto& id : ids) {
        Person* x = find_person(w, id);
        if (!x || has_tag(x->tags, "dragon_raised")) continue;
        int held = w.year - x->hostage_since;
        if (held < p.c.hostage_raise_years) continue;
        x->tags.insert("dragon_raised");
        x->memory_of_dragon = 1.0;
        auto ep = p.names.epithets.find("dragon_raised");
        if (ep != p.names.epithets.end()) x->name += " " + ep->second;
        emit(w, "raised_in_lair", x->id, x->faction, static_cast<double>(held), {}, because("hostage_years", {last_event_id(w, "act_take_hostage", x->faction)}, {{"years", static_cast<double>(held)}}));
        meet(w, x->id, "hostage", p.c.known_help);
    }
}

}

using namespace lives;

Person* pick_hostage(World& w, const std::string& faction) {
    for (auto& x : w.people) if (x.faction == faction && x.role == "heir" && !has_tag(x.tags, "hostage")) return &x;
    for (auto& x : w.people) if (x.faction == faction && x.role == "ruler" && !has_tag(x.tags, "hostage")) return &x;
    for (auto& x : w.people) if (x.faction == faction && !has_tag(x.tags, "hostage")) return &x;
    return nullptr;
}

void take_hostage_person(World& w, const Params& p, Person& person, bool consent, const Cause& cause) {
    person.tags.insert("hostage");
    person.hostage_since = w.year;
    if (p.moves.rel.count("hostage_of")) add_rel(person, "dragon", "hostage_of", p.moves.rel.at("hostage_of"));   // item 07
    if (!contains(w.dragons[kPlayer].hostages, person.id)) {
        w.dragons[kPlayer].hostages.push_back(person.id);
        std::sort(w.dragons[kPlayer].hostages.begin(), w.dragons[kPlayer].hostages.end());
    }
    meet(w, person.id, "hostage", consent ? p.c.known_help : p.c.known_harm);
    emit(w, "hostage_taken", person.id, person.faction, 0, {consent ? "consent" : "seized", person.role}, cause);
    if (person.role == "ruler") {
        Faction* f = find_faction(w, person.faction);
        if (f) {
            person.role = "hostage_ruler";
            f->ruler.reigned = 0;
            Person* heir = heir_of(w, f->id);
            if (heir && !has_tag(heir->tags, "hostage")) crown(w, p, *f, *heir, "heir", because("ruler_in_lair", {last_event_id(w, "hostage_taken", person.id)}));
            else crown_new(w, p, *f, "regent", because("ruler_in_lair", {last_event_id(w, "hostage_taken", person.id)}));
        }
    }
}

void release_hostage_person(World& w, const Params& p, const std::string& faction, const Cause& cause) {
    std::vector<std::string> ids = w.dragons[kPlayer].hostages;
    for (auto& id : ids) {
        Person* x = find_person(w, id);
        if (!x || x->faction != faction) continue;
        x->tags.erase("hostage");
        erase_value(w.dragons[kPlayer].hostages, id);
        if (x->role == "hostage_ruler") x->role = "hero";
        if (x->role == "heir" && heir_of(w, faction) && heir_of(w, faction)->id != id) { x->role = "hero"; x->tags.insert("claimant"); }
        meet(w, id, "released", p.c.known_release);
        emit(w, "hostage_released", id, faction, static_cast<double>(w.year - x->hostage_since), {x->role}, cause);
        read_release(w, p, id);   // item 07
        return;
    }
}

}
