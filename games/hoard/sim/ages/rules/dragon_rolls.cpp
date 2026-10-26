#include "rules.h"
#include "slayers.h"
#include "../growth.h"
#include "../oaths.h"
#include "../talents.h"
#include "../world_util.h"

namespace ages {

static bool has_tribute_oath(const World& w, const std::string& id) {
    for (auto& o : w.oaths) {
        if (o.status != "active") continue;
        if (o.parties[0] != id && o.parties[1] != id) continue;
        for (auto& cl : o.clauses) if (cl.pred == "tribute" && cl.who == id) return true;
    }
    return false;
}

// 17.7: tribute also feeds the sleeper (tribute_food per gold unit goes into the food stock). v0.4: a court under a
// tribute oath pays out of faith or out of fear; tribute feeds Dominion
static void tribute(World& w, const Params& p) {
    const Constants& c = p.c;
    double k = dts(w, c);
    auto points = c.growth_points.find("tribute");
    for (auto& f : w.factions) {
        bool sworn = has_tribute_oath(w, f.id) && (f.faith > c.tribute_faith || f.fear >= c.demand_fear);
        if (!sworn && f.faith <= c.tribute_faith_no_oath) continue;
        double amount = f.wealth * c.tribute_rate * k * effect(w, p, "tribute_mult");
        f.wealth -= amount;
        w.dragon.hoard.gold += amount;
        w.dragon.hoard.food += amount * c.tribute_food;
        w.dragon.faith_pool += f.faith * c.tribute_faith_pool * f.eff.faith_gain * k;
        if (points != c.growth_points.end()) add_growth(w, p, "dominion", amount * points->second);
        const Oath* o = active_oath_with_pred(w, f.id, "tribute");
        emit(w, "tribute_paid", f.id, "", amount, {}, because(o ? "oath" : "worship", {o ? last_event_id(w, "oath_sworn", o->id) : 0}, {{"faith", f.faith}}));
    }
}

void rule_dragon_rolls(World& w, const Params& p) {
    tribute(w, p);
    anti_dragon_research(w, p);
    slayer_clock(w, p);
}

}
