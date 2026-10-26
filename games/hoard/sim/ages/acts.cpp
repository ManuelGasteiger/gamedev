#include "ages.h"
#include "dragon_acts.h"
#include "hooks.h"
#include "importance.h"
#include "oaths.h"
#include "offers.h"
#include "reaction_lines.h"
#include "rules/coalitions.h"
#include "rules/rules.h"
#include "step.h"
#include "talents.h"
#include "world_util.h"
#include "wounds.h"

namespace ages {

static const Reaction* find_reaction(const Params& p, const std::string& act, const std::string& rel) {
    for (auto& r : p.reactions)
        if (contains(r.acts, act) && (r.relationship == rel || r.relationship == "any")) return &r;
    return nullptr;
}

static void react(World& w, const Params& p, Faction& f, const std::string& act, const std::string& rel, double m) {
    const Reaction* r = find_reaction(p, act, rel);
    if (!r) return;
    double fear = r->fear_per_m * m + r->fear_flat;
    double faith = r->faith_per_m * m + r->faith_flat;
    if (fear > 0) fear *= effect(w, p, "fear_mult");
    if (faith > 0) faith *= f.eff.faith_gain * effect(w, p, "faith_mult") * effect(w, p, "faith_gain_mult");
    f.anti_dragon += r->anti_dragon_per_m * m;
    if (r->has_faith_set) set_faith(f, p.c, w.year, act, m, r->faith_set, fear, r->tags);
    else remember(f, p.c, w.year, act, m, fear, faith, r->tags);
}

static bool is_harm(const std::string& t) { return t == "burn" || t == "kill" || t == "take"; }

static std::string harm_relationship(const World& w, const Params& p, const Faction& f) {
    const Constants& c = p.c;
    if (active_oath_with_pred(w, f.id, "no_harm") || active_oath_with_pred(w, f.id, "defend")) return "oath_active";
    if (f.faith > c.reaction_faith_high) return "faithful";
    return power(f, c) < c.reaction_weak_ratio * threat(w.dragon, c) ? "weak" : "strong";
}

static void immediate(World& w, const Params& p, Faction& f, const Act& a, double m) {
    const Constants& c = p.c;
    if (a.type == "burn") {
        f.wealth -= m * f.wealth;
        f.tags.insert("burned_by_dragon");
        if (Region* r = find_region(w, a.region)) r->burn = clamp01(r->burn + m);
        else for (auto& rid : f.regions) if (Region* rr = find_region(w, rid)) rr->burn = clamp01(rr->burn + m / static_cast<double>(f.regions.size()));
    } else if (a.type == "kill") {
        auto who = a.payload.find("soldiers");
        if (who != a.payload.end() && who->second > 0) f.wealth -= m * f.wealth * c.act_kill_soldier_wealth;
        else f.cohesion = clamp01(f.cohesion - c.act_kill_cohesion * m);
    } else if (a.type == "take") {
        double taken = m * f.wealth;
        double molten = molten_gold_mult(w, p, f, a);
        f.wealth -= taken;
        double shares = 0;
        for (auto& kv : a.payload) shares += kv.second;
        for (auto& kv : a.payload) {
            double amt = shares > 0 ? taken * kv.second / shares : 0;
            if (kv.first == "gold") w.dragon.hoard.gold += amt * molten;
            else if (kv.first == "books") w.dragon.hoard.books += amt;
            else if (kv.first == "artifacts") w.dragon.hoard.artifacts += amt;
            else if (kv.first == "art") w.dragon.hoard.art += amt;
            else if (kv.first == "royals") w.dragon.hoard.royals += kv.second;
            else if (kv.first == "food") w.dragon.hoard.food += amt;
        }
        if (a.payload.empty()) w.dragon.hoard.gold += taken * molten;
    } else if (a.type == "take_hostage") {   // 17.2: the heir (else the ruler) becomes a person in the lair
        f.tags.insert("hostage_taken");
        w.dragon.lineages.push_back(f.id);
        Person* x = a.item.empty() ? pick_hostage(w, f.id) : find_person(w, a.item);
        if (x && x->faction == f.id && !has_tag(x->tags, "hostage")) take_hostage_person(w, p, *x, false, because("seized", {}, {{"fear", f.fear}}));
    } else if (a.type == "release") {   // the heir goes home and the lair no longer holds that line
        f.tags.erase("hostage_taken");
        erase_value(w.dragon.lineages, f.id);
        release_hostage_person(w, p, f.id, because("released", {last_event_id(w, "hostage_taken", "", f.id)}));
    } else if (a.type == "defend") {
        if (Faction* att = find_faction(w, a.against)) {
            att->wealth *= (1.0 - c.act_defend_attacker_loss);
            if (overlord_of(f) == att->id) {
                f.tags.erase("vassal_of:" + att->id);
                emit(w, "vassal_freed", f.id, att->id, 0, {"dragon_defended"}, because("dragon_defended", {last_event_id(w, "vassal_sworn", f.id)}));
            }
            bool holds = false;
            for (auto& rid : att->regions)
                for (auto& e : w.chronicle)
                    if (e.kind == "region_taken" && e.subject == att->id && e.object == rid && e.tags.count(f.id)) holds = true;
            if (!holds && at_war(f, att->id)) {
                std::vector<War> ended;
                for (auto& x : w.wars)
                    if ((x.a == f.id && x.b == att->id) || (x.a == att->id && x.b == f.id)) ended.push_back(x);
                for (auto& x : ended) end_war(w, x, "defended_by_dragon", because("dragon_defended", {}, {{"magnitude", m}}));
                erase_value(att->at_war_with, f.id);
                erase_value(f.at_war_with, att->id);
            }
        }
    } else if (a.type == "give") {
        auto era = a.payload.find("era");
        auto gold = a.payload.find("gold");
        if (era != a.payload.end()) f.era_progress += m * c.era_threshold[static_cast<size_t>(std::min<int>(f.era, 3))] * c.act_give_era_share;
        else if (gold != a.payload.end()) {   // gold from the hoard, as much of the asked amount as the hoard holds
            double amt = std::min(std::max(0.0, gold->second), w.dragon.hoard.gold);
            w.dragon.hoard.gold -= amt;
            f.wealth += amt;
        } else f.wealth += m * std::max(f.wealth, c.act_give_wealth_floor);
    } else if (a.type == "accept_offering") {
        w.dragon.faith_pool += c.act_accept_offering_faith_pool;
    } else if (a.type == "miracle") {
        w.dragon.faith_pool = std::max(0.0, w.dragon.faith_pool - c.act_miracle_faith_cost);
    }
}

// the relationship row the act is read through (section 5); the narrated line is keyed by the same row
static std::string relationship(const World& w, const Params& p, const Faction& f, const Act& a) {
    if (is_harm(a.type)) return harm_relationship(w, p, f);
    if (a.type == "defend") return losing(w, p.c, f) ? "losing" : "not_losing";
    if (a.type == "miracle") {
        auto kind = a.payload.find("terror");
        return kind != a.payload.end() && kind->second > 0 ? "terror" : "protector";
    }
    return "any";
}

static std::string largest_loot(const Act& a) {
    std::string best;
    double top = 0;
    for (auto& kv : a.payload) if (kv.second > top) { top = kv.second; best = kv.first; }
    return best;
}

static ReactionNote react_to(World& w, const Params& p, Faction& f, const Act& a, double m) {
    ReactionNote n{f.id, a.type, relationship(w, p, f, a), a.target, a.region, a.item, m, largest_loot(a)};
    react(w, p, f, a.type, n.rel, m);
    return n;
}

static void witnesses(World& w, const Params& p, const Act& a, const Faction& target, double m, std::vector<ReactionNote>& notes) {
    if (a.type != "burn") return;
    for (auto& wid : a.witnesses) {
        if (wid == target.id) continue;
        Faction* f = find_faction(w, wid);
        if (!f) continue;
        std::string rel = "neutral";
        if (at_war(*f, target.id)) rel = "enemy";
        else if (f->race == target.race) rel = "friend";
        react(w, p, *f, "witness_burn", rel, m);
        notes.push_back({f->id, "witness_burn", rel, target.id, a.region, "", m, ""});
    }
}

// spell sponsor_remnant: the knowledge-gated verb resolves to a spawn; payload {race} or act.race names the remnant,
// act.region (or the remnant's origin, or any open home region) is where it rises
static void sponsor_remnant(World& w, const Params& p, const Act& a) {
    std::string race = a.race;
    if (race.empty()) for (auto& kv : a.payload) if (p.races.count(kv.first)) race = kv.first;
    if (w.dragon.knowledge < p.c.remnant_sponsor_knowledge) { emit(w, "spell_failed", "dragon", race, 0, {"knowledge"}); return; }
    for (auto& rem : w.remnants) {
        if (rem.race != race) continue;
        auto rt = p.races.find(race);
        if (rt == p.races.end()) return;
        std::string region;
        auto open = [&](const std::string& rid) {
            const Region* r = find_region(w, rid);
            return r && r->owner.empty() && !r->lair && home_terrain(rt->second.culture, *r);
        };
        if (open(a.region)) region = a.region;
        else if (open(rem.origin)) region = rem.origin;
        else for (auto& r : w.regions) if (open(r.id)) { region = r.id; break; }
        if (region.empty()) { emit(w, "spell_failed", "dragon", race, 0, {"no_region"}); return; }
        refound_remnant(w, p, rem, region, "sponsored");
        return;
    }
    emit(w, "spell_failed", "dragon", race, 0, {"no_remnant"});
}

// a take names what it carried off (loot:<kind>) so later cards can tell gold from an elven heirloom
static Tags act_tags(const Act& a) {
    Tags t;
    if (!a.item.empty()) t.insert(a.item);
    if (a.type == "take")
        for (auto& kv : a.payload) if (kv.second > 0) t.insert("loot:" + kv.first);
    return t;
}

std::vector<ReactionLine> apply_acts(World& w, const Params& p, const std::vector<Act>& acts) {
    if (dragon_dead(w)) return {};
    double burn_total = 0;
    bool rage = has_tag(w.dragon.tags, "rage");
    // a dragon nobody believes in any more (the forgotten-dragon card) is read like an enraged one: a show is a burn
    bool forgotten = has_tag(w.tags, "dragon_forgotten");
    size_t first_event = w.chronicle.size();
    std::vector<ReactionNote> notes;
    for (auto& a : acts) {
        if (dragon_dead(w)) break;
        Act act = a;
        act.year = w.year;
        if (act.type == "wound") { passive_wound(w, p, act); continue; }
        w.acts.push_back(act);
        if (act.type == "spell") {
            if (act.spell == "sponsor_remnant" || act.payload.count("sponsor_remnant")) sponsor_remnant(w, p, act);
            emit(w, "act_spell", act.spell.empty() ? "sponsor_remnant" : act.spell, act.race);
            continue;
        }
        Faction* f = find_faction(w, act.target);
        if (!f) continue;
        double m = clamp01(act.magnitude);
        if (rage && act.type == "burn") m = clamp01(m * p.c.rage_burn_mult);
        if (act.type == "burn") m = warded_burn(w, p, act, m);
        strike(w, p, act);
        if (dragon_dead(w)) break;
        if (act.type == "burn") burn_total += m;
        immediate(w, p, *f, act, m);
        if (act.type == "burn") after_burn(w, p, *f, act, m);
        feed(w, p, act, m);
        if ((rage || forgotten) && act.type == "show") {
            Act as_burn = act;
            as_burn.type = "burn";
            as_burn.magnitude = rage ? p.c.rage_show_as_burn : p.c.forgotten_show_as_burn;
            ReactionNote n = react_to(w, p, *f, as_burn, as_burn.magnitude);
            n.act = rage ? "show_in_rage" : "show_forgotten";
            notes.push_back(n);
        } else notes.push_back(react_to(w, p, *f, act, m));
        witnesses(w, p, act, *f, m, notes);
        emit(w, "act_" + act.type, act.target, act.region, m, act_tags(act));
        note_act(w, p, act, m);
        resolve_hooks(w, p, act);   // v0.4 hooks (S3)
    }
    if (burn_total >= p.shocks.volcanic_trigger_burn_total) w.tags.insert("mega_raze_pending");
    w.dragon.tags.erase("rage");
    score_events(w, p);
    return reaction_lines(w, p, notes, first_event);
}

}
