#include <cmath>
#include "core/detmath.h"
#include "ages.h"
#include "growth.h"
#include "step.h"
#include "talents.h"
#include "world_util.h"

namespace ages {

static double digest(double& stock, double halflife, double years) {
    if (stock <= 0 || halflife <= 0) return 0;
    double remaining = stock * det::half_life(years, halflife);
    double digested = stock - remaining;
    stock = remaining;
    return digested;
}

static double of(const std::map<std::string, double>& m, const char* k) { auto it = m.find(k); return it == m.end() ? 0 : it->second; }
static double pu(const Constants& c, const char* k) { return of(c.hoard_per_unit, k); }

// game taken from the lair and its neighbours: hoard_per_unit.hunt x their productive land (eff_land x (1 - burn)),
// so a razed or wasted valley around the lair starves the sleeper and a green one feeds it a little
static double hunt(const World& w, const Constants& c, double years) {
    double rate = pu(c, "hunt");
    if (rate <= 0) return 0;
    double land = 0;
    for (auto& r : w.regions) {
        if (!r.lair) continue;
        land += r.eff_land * (1.0 - r.burn);
        for (auto& n : r.neighbours)
            if (const Region* nr = find_region(w, n)) land += nr->eff_land * (1.0 - nr->burn);
    }
    return rate * land * years / c.step_years;
}

// 17.2: a digested royal is a hostage who grows up in the lair when one is held; otherwise the old lineage tag
static void raise_royals(World& w, const Params& p) {
    Dragon& d = w.dragons[kPlayer];
    while (d.royals_digested >= 1.0) {
        d.royals_digested -= 1.0;
        d.knowledge += pu(p.c, "royals_knowledge");
        Person* held = nullptr;
        for (auto& id : d.hostages) if (Person* x = find_person(w, id)) if (!has_tag(x->tags, "dragon_raised")) { held = x; break; }
        if (held) {
            held->tags.insert("dragon_raised");
            held->memory_of_dragon = 1.0;
            emit(w, "raised_in_lair", held->id, held->faction, static_cast<double>(w.year - held->hostage_since), {}, because("royals_digested", {last_event_id(w, "hostage_taken", held->id)}));
            continue;
        }
        for (auto& f : w.factions) {
            if (f.race != "human" || has_tag(f.tags, "dragon_raised")) continue;
            f.tags.insert("dragon_raised");
            d.lineages.push_back(f.race);
            emit(w, "dragon_raised_lineage", f.id, "", 0, {}, because("royals_digested", {last_event_id(w, "act_take", f.id)}));
            break;
        }
    }
}

// the step's years on the bed: full within the sleep's first long window, bed_overtime past it (the step just ended)
static double bed_years(const World& w, const Constants& c, double years, double window) {
    double end = w.year - w.sleep_started, start = end - years;
    double full = std::max(0.0, std::min(end, window) - std::max(start, 0.0));
    return full + c.bed_overtime * (years - full);
}

// the gold above the phase's floor is eaten at gold_burn per long window slept; the floor is what the dragon keeps
static double eat_gold(Dragon& d, const Constants& c, double windows) {
    double floor = c.gold_floor_base * c.floor_phase_mult[static_cast<size_t>(phase_index(d))];
    double excess = std::max(0.0, d.hoard.gold - floor);
    double left = excess * det::pow(1.0 - c.gold_burn, windows);
    d.hoard.gold -= excess - left;
    return excess - left;
}

// v0.4: digestion runs on the phase clock (half-life = growth_halflife[kind] x the phase's long window) and feeds the
// growth axes and the pools. Gold, bed_mode consumed: the gold above the floor is eaten and gives gold_pts Might each;
// bed_mode window: gold is slept on, Might = bed_rate x sqrt(gold) per long window slept and nothing is eaten. Past the
// sleep's first long window the bed runs at bed_overtime; a moult doubles it all. Returns the nourishment (everything
// digested but embers and gold, plus the hunt).
double digest_hoard(World& w, const Params& p, double years) {
    const Constants& c = p.c;
    Dragon& d = w.dragons[kPlayer];
    double hunted = hunt(w, c, years) * effect(w, p, "hunt_mult");
    double window = std::max(1, long_sleep(d, c));
    double slept = bed_years(w, c, years, window);
    if (w.moulting) { years *= c.moult_digest_mult; slept *= c.moult_digest_mult; }
    double bed = c.bed_mode == "consumed" ? eat_gold(d, c, slept / window) * c.gold_pts : c.bed_rate * std::sqrt(std::max(0.0, d.hoard.gold)) * slept / window;
    bed *= effect(w, p, "bed_mult");
    auto eat = [&](double& stock, const char* kind) { return digest(stock, of(c.growth_halflife, kind) * window, years); };
    double books = eat(d.hoard.books, "books");
    double art = eat(d.hoard.art, "art");
    double artifacts = eat(d.hoard.artifacts, "artifacts");
    double food = eat(d.hoard.food, "food");
    double embers = eat(d.hoard.embers, "embers");
    double royals = eat(d.hoard.royals, "royals");
    d.knowledge += books * pu(c, "books");
    d.faith_pool += art * pu(c, "art");
    d.magic += artifacts * pu(c, "artifacts");

    const std::map<std::string, double>& pts = c.growth_points;
    double digested = effect(w, p, "digest_mult");
    add_growth(w, p, "lore", (books * of(pts, "books") * effect(w, p, "lore_mult") + royals * of(pts, "royals") * effect(w, p, "hostage_lore_mult")) * digested);
    add_growth(w, p, "arcana", artifacts * of(pts, "artifacts") * digested);
    add_growth(w, p, "dominion", art * of(pts, "art") * digested);
    add_growth(w, p, "flame", embers * of(pts, "embers") * digested);
    add_growth(w, p, "might", food * of(pts, "food") * digested + bed);

    d.royals_digested += royals;
    raise_royals(w, p);
    return books + art + artifacts + food + royals + hunted;
}

}
