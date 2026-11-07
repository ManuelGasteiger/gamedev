#include "resolve_fight.h"
#include <algorithm>
#include "rules/people_life.h"
#include "rules/slayers.h"
#include "slayer_roster.h"
#include "world_util.h"
#include "wounds.h"

namespace ages {

std::string act_family(const Params& p, const std::string& act_type) {
    auto it = p.slayers.act_family.find(act_type);
    return it == p.slayers.act_family.end() ? "" : it->second;
}

void reveal_order(World& w, const std::string& court) {
    for (Person* x : roster(w, court)) reveal_all(*x);
}

FightResult resolve_fight(World& w, const Params& p, const std::string& slayer, const std::string& family) {
    FightResult out;
    Person* x = find_person(w, slayer);
    if (!x || x->slayer.rank.empty() || dragon_dead(w)) return out;
    const SlayerBook& b = p.slayers;
    auto fam = b.families.find(family);
    bool feared = fam != b.families.end() && fam->second.fear == x->slayer.fear;
    bool hated = fam != b.families.end() && contains(fam->second.hates, x->slayer.hate);
    double power = x->slayer.power + (hated ? b.hate_bonus : 0);
    double ratio = dragon_points(w, p) / std::max(1.0, power);
    double counter = counters_dragon(w, p, *x) ? b.counter_mult : 1.0;
    out.dragon_wounds = feared ? 0 : health_max(w, p) * b.fight_base_share * power * counter;
    for (const char* k : kTraitKinds) if (!knows(*x, k)) out.traits_revealed.push_back(k);
    reveal_all(*x);
    std::string court = x->faction;
    bool dies = ratio > b.fight_death_ratio;
    out.slayer_outcome = dies ? "dead" : "fled_scarred";
    meet(w, x->id, "fight");
    emit(w, "slayer_fought", x->id, court, out.dragon_wounds, {out.slayer_outcome, feared ? "feared" : "open"}, because("fight", {}, {{"ratio", ratio}, {"power", power}}));
    Person copy = *x;
    rival_gain(w, p, copy);
    if (dies) {
        if (x->slayer.armed) { w.dragons[kPlayer].hoard.artifacts += b.fight_artifact; out.artifact = true; }
        lives::retire(w, p, copy, "slain_by_dragon", because("fight", {}, {{"ratio", ratio}}));
    } else {
        add_scar(w, p, *x);
        raise_power(p, *x, b.march_promote);
        remember(w, p, *x, b.humiliated, "witness");
        x->slayer.last_fight = w.year;
    }
    wound_dragon(w, p, court, "", out.dragon_wounds, {"slayer"});
    if (dragon_dead(w)) {
        w.chronicle.back().object = copy.id;
        w.chronicle.back().text_key = "ev.dragon_slain_duel";
    }
    return out;
}

}
