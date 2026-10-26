#include "hooks.h"
#include "unrecorded.h"
#include "defences.h"
#include "growth.h"
#include "rules/slayers.h"
#include "talents.h"
#include "world_util.h"
#include "wounds.h"

// v0.4 hooks (S3): an act of the raid (or a choice) answers a hook: the fight's wound first, then the reward
// (data/hooks.json reward keys: gold, gold_share of the court's wealth, books, artifacts, art, royals, food, faith_pool,
// hunger_add, growth_<axis>, fear, fear_all, faith, faith_neighbours, other_fear, other_faith) and hook_resolved;
// an expired hook applies its ignore keys (wealth_mult, cohesion_add, anti_dragon_add, faith_add, fear_add,
// other_cohesion_add, lair_burn_add, hoard_gold_mult, gold_to_court)

namespace ages {

namespace {

double of(const std::map<std::string, double>& m, const char* k) { auto it = m.find(k); return it == m.end() ? 0 : it->second; }

void mark(World& w, const Params& p, Faction& f, const char* type, double fear, double faith) {
    if (fear == 0 && faith == 0) return;
    if (fear > 0) fear *= effect(w, p, "fear_mult");
    if (faith > 0) faith *= f.eff.faith_gain * effect(w, p, "faith_mult") * effect(w, p, "faith_gain_mult");
    remember(f, p.c, w.year, type, 1.0, fear, faith, {"hook"});
}

std::map<std::string, double> reward(World& w, const Params& p, const Hook& h, const std::map<std::string, double>& r) {
    Hoard& hoard = w.dragon.hoard;
    Faction* f = find_faction(w, h.faction);
    Faction* o = find_faction(w, h.other);
    double gold = of(r, "gold");
    if (f && of(r, "gold_share") > 0) { double amt = f->wealth * of(r, "gold_share"); f->wealth -= amt; gold += amt; }
    hoard.gold += gold;
    hoard.books += of(r, "books");
    hoard.artifacts += of(r, "artifacts");
    hoard.art += of(r, "art");
    hoard.royals += of(r, "royals");
    hoard.food += of(r, "food");
    w.dragon.faith_pool += of(r, "faith_pool");
    w.dragon.hunger = clamp01(w.dragon.hunger + of(r, "hunger_add"));
    std::map<std::string, double> grown;
    for (const char* axis : kGrowthAxes) {
        double pts = of(r, (std::string("growth_") + axis).c_str());
        if (pts <= 0) continue;
        add_growth(w, p, axis, pts);
        grown[axis] = pts;
    }
    if (f) mark(w, p, *f, "hook", of(r, "fear"), of(r, "faith"));
    if (o) mark(w, p, *o, "hook", of(r, "other_fear"), of(r, "other_faith"));
    if (of(r, "fear_all") > 0) for (auto& g : w.factions) if (!f || g.id != f->id) mark(w, p, g, "hook", of(r, "fear_all"), 0);
    if (f && of(r, "faith_neighbours") != 0)
        for (auto& g : w.factions) if (g.id != f->id && adjacent(w, *f, g)) mark(w, p, g, "hook", 0, of(r, "faith_neighbours"));
    return grown;
}

void fight(World& w, const Params& p, const Hook& h) {
    double damage = hook_fight_damage(w, p, h);
    if (damage <= 0) return;
    const HookDef* t = hook_def(p, h);
    wound_dragon(w, p, h.faction, h.region, damage, {t && !t->fight_defence.empty() ? t->fight_defence : std::string("fight")});
}

void resolve(World& w, const Params& p, const Hook& h, const std::string& choice) {
    const HookDef* t = hook_def(p, h);
    if (!t) return;
    fight(w, p, h);
    if (dragon_dead(w)) return;
    auto cr = t->choice_reward.find(choice);
    std::map<std::string, double> grown = reward(w, p, h, cr == t->choice_reward.end() ? t->reward : cr->second);
    if (h.kind == "slayers") if (Faction* f = find_faction(w, h.faction)) disperse_slayers(w, *f);
    std::map<std::string, double> params = grown;
    params["fight"] = hook_fight_damage(w, p, h);
    emit(w, "hook_resolved", h.faction, h.region, 0, {h.kind}, because("hook", {}, params));
    w.chronicle.back().text_key = "ev.hook_" + h.kind + (choice.empty() ? "" : "_" + choice);
}

void drop(World& w, const std::string& id) {
    w.hooks.erase(std::remove_if(w.hooks.begin(), w.hooks.end(), [&](const Hook& x) { return x.id == id; }), w.hooks.end());
}

}

double hook_fight_damage(const World& w, const Params& p, const Hook& h) {
    const HookDef* t = hook_def(p, h);
    if (!t || (t->fight_damage <= 0 && t->fight_strength_mult <= 0 && t->fight_health_share <= 0)) return 0;
    const Faction* f = find_faction(w, h.faction);
    double armor = effect(w, p, "armor_mult");
    for (auto& d : p.defences.defences) if (d.id == t->fight_defence) armor = armor_against(w, p, d);
    double damage = t->fight_damage * (1.0 + t->fight_era_scale * (f ? f->era : 0));
    if (f && t->fight_strength_mult > 0) damage += t->fight_strength_mult * slayer_strength(w, p, *f);
    damage += health_max(w, p) * t->fight_health_share;
    return damage * armor;
}

// take, burn, kill and take_hostage must hit the hook's court at its region (the court's capital when the act names
// none); kill_person is a kill that names the person; defend protects the other side against the hook's court
bool hook_matches(const World& w, const Hook& h, const Act& a) {
    if (!h.choices.empty()) return false;
    for (auto& want : h.acts) {
        if (want == "defend") {
            if (a.type == "defend" && a.against == h.faction && (h.other.empty() || a.target == h.other)) return true;
            continue;
        }
        if (want == "kill_person") {
            if (a.type == "kill" && !h.person.empty() && a.item == h.person) return true;
            continue;
        }
        if (a.type != want || a.target != h.faction) continue;
        const Region* r = act_region(w, a);
        if (h.region.empty() || (r && r->id == h.region)) return true;
    }
    return false;
}

void resolve_hooks(World& w, const Params& p, const Act& a) {
    std::vector<Hook> hit;
    for (auto& h : w.hooks) if (hook_matches(w, h, a)) hit.push_back(h);
    for (auto& h : hit) {
        if (dragon_dead(w)) return;
        drop(w, h.id);
        resolve(w, p, h, "");
    }
}

bool answer_hook_unrecorded(World& w, const Params& p, const std::string& id, const std::string& choice, std::string& reason) {
    if (dragon_dead(w)) { reason = "dragon_slain"; return false; }
    const Hook* found = nullptr;
    for (auto& h : w.hooks) if (h.id == id) found = &h;
    if (!found) { reason = "no such hook"; return false; }
    if (found->choices.empty()) { reason = "this hook is answered by an act"; return false; }
    if (!contains(found->choices, choice)) { reason = "no such choice"; return false; }
    Hook h = *found;
    drop(w, id);
    resolve(w, p, h, choice);
    return true;
}

void apply_hook_ignore(World& w, const Params& p, const Hook& h) {
    const HookDef* t = hook_def(p, h);
    if (!t) return;
    const std::map<std::string, double>& g = t->ignore;
    if (Faction* f = find_faction(w, h.faction)) {
        if (of(g, "wealth_mult") > 0) f->wealth *= of(g, "wealth_mult");
        f->cohesion = clamp01(f->cohesion + of(g, "cohesion_add"));
        f->anti_dragon = std::max(0.0, f->anti_dragon + of(g, "anti_dragon_add"));
        mark(w, p, *f, "hook_ignored", of(g, "fear_add"), of(g, "faith_add"));
    }
    if (Faction* o = find_faction(w, h.other)) o->cohesion = clamp01(o->cohesion + of(g, "other_cohesion_add"));
    if (of(g, "lair_burn_add") > 0) if (Region* r = find_region(w, lair_region(w))) r->burn = clamp01(r->burn + of(g, "lair_burn_add"));
    if (of(g, "hoard_gold_mult") > 0) w.dragon.hoard.gold *= of(g, "hoard_gold_mult");
    if (of(g, "gold_to_court") > 0) {
        double amt = w.dragon.hoard.gold * of(g, "gold_to_court");
        w.dragon.hoard.gold -= amt;
        if (Faction* f = find_faction(w, h.faction)) f->wealth += amt;
    }
}

}
