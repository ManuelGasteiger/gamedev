#include "slayers.h"
#include "people_life.h"
#include "../slayer_roster.h"
#include "../world_util.h"

// item 08: what an encounter costs a court's order: each named slayer dies at `death` or comes home scarred, one
// power higher and humiliated; a march spares the grandmaster, a fight at the gate does not

namespace ages {

namespace {

void clash(World& w, const Params& p, const std::string& court, int event, double death, bool spare_grandmaster, double hunters_kept) {
    Order* o = find_order(w, court);
    if (!o) return;
    const SlayerBook& b = p.slayers;
    std::vector<std::string> ids;
    for (const Person* x : roster(w, court)) ids.push_back(x->id);
    for (auto& id : ids) {
        Person* x = find_person(w, id);
        if (!x) continue;
        Rng r = rng_for(w.seed, "clash", id + "|" + std::to_string(w.year));
        double roll = r.uniform();
        bool dies = roll < death && !(spare_grandmaster && x->slayer.rank == "grandmaster");
        Person copy = *x;
        rival_gain(w, p, copy);
        if (dies) {
            lives::retire(w, p, copy, "slain_by_dragon", because("slay_attempt_failed", {event}));
            continue;
        }
        if (add_scar(w, p, *x)) emit(w, "slayer_scarred", id, court, x->slayer.power, {x->slayer.scars.back()}, because("slay_attempt_failed", {event}));
        raise_power(p, *x, b.march_promote);
        remember(w, p, *x, b.humiliated, "witness");
        x->slayer.last_fight = w.year;
        if (x->slayer.rank == "grandmaster") meet(w, id, "fight");
    }
    o = find_order(w, court);
    if (o) o->hunters = static_cast<int>(o->hunters * hunters_kept);
}

}

void march_failed(World& w, const Params& p, const Faction& f, int attempt_event) {
    clash(w, p, f.id, attempt_event, p.slayers.march_death, true, 1.0 - p.slayers.march_hunters_lost);
}

void gate_fight(World& w, const Params& p, const Faction& f) {
    clash(w, p, f.id, last_event_id(w, "slayers_ready", f.id), p.slayers.gate_death, false, 0.0);
}

}
