#include "rules.h"
#include "../person_record.h"
#include "../world_util.h"

// item 07: after the inquisition card marked a court (tag inquisition:<person>), its devout people form a purge
// faction; those with a bond to the person cannot join. While the faction holds strength_min of the court's tracked
// people its discontent rises per decade; at 1 it sends the ultimatum (purge_ultimatum; the purge card decides civil
// war or the person's fall). The record ends with the ultimatum, the tag or the person.

namespace ages {

static bool bonded(const Params& p, const Person& a, const Person& b) {
    for (auto& k : p.moves.inq_bonds) {
        for (auto& r : a.rel) if (r.to == b.id && r.kind == k && r.w > 0) return true;
        for (auto& r : b.rel) if (r.to == a.id && r.kind == k && r.w > 0) return true;
    }
    return false;
}

double purge_strength(const World& w, const Params& p, const Secret& s) {
    const Person* x = find_person(w, s.about);
    if (!x) return 0;
    int members = 0, court = 0;
    for (auto& y : w.people) {
        if (y.faction != s.holder || y.id == x->id || has_tag(y.tags, "hostage") || !tracked(w, p, y)) continue;
        ++court;
        bool devout = false;
        for (auto& t : p.moves.inq_joiners) if (has_trait(y, t)) devout = true;
        if (devout && !bonded(p, y, *x)) ++members;
    }
    return court ? static_cast<double>(members) / court : 0;
}

void rule_inquisition(World& w, const Params& p) {
    std::vector<Secret> kept;
    for (auto& s : w.secrets) {
        if (!s.exposed) { kept.push_back(s); continue; }
        Faction* f = find_faction(w, s.holder);
        if (!f) continue;
        if (!has_tag(f->tags, "inquisition:" + s.about)) {
            if (w.year - s.exposed <= p.c.step_years) kept.push_back(s);   // the card fires in the step after
            continue;
        }
        if (purge_strength(w, p, s) >= p.moves.inq_strength_min) s.discontent += p.moves.inq_per_decade * dts(w, p.c);
        if (s.discontent < 1) { kept.push_back(s); continue; }
        int card = last_event_id(w, "inquisition", f->id, s.about);
        emit(w, "purge_ultimatum", f->id, s.about, s.discontent, {s.kind}, because("purge_discontent", {card}));
    }
    w.secrets = kept;
}

}
