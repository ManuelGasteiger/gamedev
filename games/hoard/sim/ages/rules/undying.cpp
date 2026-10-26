#include "world_rules.h"
#include "card_ctx.h"
#include "rules.h"
#include "../world_util.h"

// the empire with an immortal ruler (core-loop.md section 6): an undying ruler never ages and the realm keeps a cohesion
// floor, so it does not fall apart while the ruler lives; each decade the ruler may be overthrown, and the realm, its
// floor gone, takes the shock

namespace ages {

static Person* undying_ruler(World& w, const Faction& f) {
    for (auto& x : w.people) if (x.faction == f.id && x.role == "ruler" && has_tag(x.tags, "undying")) return &x;
    return nullptr;
}

void make_undying(World& w, const Params& p, Faction& f, const Cause& cause, int event) {
    (void)p;
    Person* ruler = nullptr;
    for (auto& x : w.people) if (x.faction == f.id && x.role == "ruler") ruler = &x;
    if (!ruler || has_tag(f.tags, "undying")) return;
    ruler->dies = 0;
    ruler->tags.insert("undying");
    f.ruler.cls = "immortal";
    f.ruler.reigned = 0;
    int ev = event > 0 ? event : emit(w, "undying_rose", f.id, ruler->id, 0, {}, cause);
    write_tag(w, f.id, "undying", 0, false, ev);
}

static void fall(World& w, const Params& p, Faction& f, const std::string& person, const std::string& how) {
    const TimedTag* t = timed_tag(w, f.id, "undying");
    double years = t ? static_cast<double>(w.year - t->from) : 0;
    int since = t ? t->event : 0;
    remove_tags(w, f.id, "undying");
    f.ruler.cls = f.culture.ruler_class == "immortal" ? "mortal" : f.culture.ruler_class;
    f.ruler.reigned = 0;
    f.cohesion = clamp01(f.cohesion + p.genesis.undying.fall_cohesion);
    emit(w, "undying_fell", f.id, person, years, {how}, because(how, {since}, {{"years", years}}));
}

void rule_undying(World& w, const Params& p) {
    const UndyingDef& u = p.genesis.undying;
    for (auto& f : w.factions) {
        if (!has_tag(f.tags, "undying")) continue;
        Person* ruler = undying_ruler(w, f);
        if (!ruler) { fall(w, p, f, "", "ruler_gone"); continue; }
        if (w.rng.chance(u.fall_chance * dts(w, p.c))) {
            std::string id = ruler->id;
            kill_person_now(w, p, id, "overthrown", because("overthrown"));
            fall(w, p, f, id, "overthrown");
            continue;
        }
        f.cohesion = std::max(f.cohesion, u.cohesion_floor);
    }
}

}
