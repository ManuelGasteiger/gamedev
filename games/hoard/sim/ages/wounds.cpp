#include "wounds.h"
#include "talents.h"
#include "world_util.h"

namespace ages {

double health_max(const World& w, const Params& p) {
    return p.c.health_base + p.c.health_per_might * growth_of(w.dragons[kPlayer], "might") + effect(w, p, "health_add");
}

bool dragon_dead(const World& w) { return has_tag(w.tags, "dragon_slain"); }

void wound_dragon(World& w, const Params& p, const std::string& court, const std::string& region, double damage, const Tags& defences) {
    if (damage <= 0 || dragon_dead(w)) return;
    w.dragons[kPlayer].wounds += damage;
    double max = health_max(w, p);
    int hit = emit(w, "dragon_wounded", court, region, damage, defences, because("defences", {}, {{"wounds", w.dragons[kPlayer].wounds}, {"health", max}}));
    if (w.dragons[kPlayer].wounds < max) return;
    w.tags.insert("dragon_slain");
    w.wake_reason = "dragon_slain";
    emit(w, "dragon_slain", court, region, w.dragons[kPlayer].wounds, defences, because("defences", {hit}, {{"wounds", w.dragons[kPlayer].wounds}, {"health", max}}));
    w.chronicle.back().text_key = "ev.dragon_slain_defences";
}

void heal(World& w, const Params& p, int years) {
    Dragon& d = w.dragons[kPlayer];
    if (d.wounds <= 0) return;
    double share = std::min(1.0, static_cast<double>(years) / std::max(1, short_sleep(d, p.c)));
    d.wounds = std::max(0.0, d.wounds - health_max(w, p) * share * effect(w, p, "heal_mult"));
}

}
