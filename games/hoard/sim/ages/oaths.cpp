#include "oaths.h"
#include "talents.h"
#include "world_util.h"

namespace ages {

const Oath* active_oath_with_pred(const World& w, const std::string& faction, const std::string& pred) {
    for (auto& o : w.oaths) {
        if (o.status != "active" || (o.parties[0] != faction && o.parties[1] != faction)) continue;
        for (auto& cl : o.clauses) if (cl.pred == pred) return &o;
    }
    return nullptr;
}

static bool dragon_act_against(const World& w, const std::string& faction, int from, const char* const* types, int n) {
    for (auto& a : w.acts) {
        if (a.year < from || a.target != faction) continue;
        for (int i = 0; i < n; ++i) if (a.type == types[i]) return true;
    }
    return false;
}

static bool dragon_defended(const World& w, const std::string& faction, const std::string& attacker, int from) {
    for (auto& a : w.acts)
        if (a.year >= from && a.type == "defend" && a.target == faction && (attacker.empty() || a.against == attacker))
            return true;
    return false;
}

static std::string mortal_party(const Oath& o) { return o.parties[0] == "dragon" ? o.parties[1] : o.parties[0]; }

bool clause_holds(const World& w, const Params& p, const Oath& o, const Clause& cl, int window_from, bool& by_dragon) {
    const Constants& c = p.c;
    by_dragon = false;
    std::string who = cl.who.empty() ? mortal_party(o) : cl.who;
    const Faction* f = find_faction(w, who == "dragon" ? mortal_party(o) : who);
    if (!f) return true;
    // v0.4: a clause sworn out of fear holds while the fear does; Dragon-king (oath_steadfast) keeps it past forgetting
    bool remembered = f->fear >= c.oath_fade_fear || effect(w, p, "oath_steadfast") > 0;

    if (cl.pred == "tribute") {
        auto amt = cl.params.find("amount");
        double amount = amt == cl.params.end() ? 0 : amt->second;
        return f->wealth >= amount && (f->faith > c.oath_tribute_min_faith || remembered);
    }
    if (cl.pred == "no_walls" || cl.pred == "no_settle") {
        if (!remembered) return false;
        if (f->era >= c.oath_walls_era) return false;
        for (auto& rid : f->regions) {
            const Region* r = find_region(w, rid);
            if (r && has_tag(r->tags, "walls")) return false;
        }
        return true;
    }
    if (cl.pred == "no_harm") {
        static const char* const harm[] = {"burn", "kill", "take"};
        by_dragon = true;
        return !dragon_act_against(w, f->id, window_from, harm, 3);
    }
    if (cl.pred == "defend") {
        by_dragon = true;
        if (f->last_invaded < window_from) return true;
        std::string attacker;
        for (auto& e : w.chronicle)
            if (e.year >= window_from && e.kind == "region_taken" && e.tags.count(f->id)) attacker = e.subject;
        return dragon_defended(w, f->id, attacker, window_from);
    }
    if (cl.pred == "hostage") {
        by_dragon = true;
        for (auto& l : w.dragon.lineages) if (l == f->race || l == f->id) return true;
        return false;
    }
    return true;
}

void apply_oath_effect(World& w, const Params& p, const std::string& effect, const std::string& faction) {
    auto it = p.oath_effects.find(effect);
    if (it == p.oath_effects.end()) return;
    const OathEffect& e = it->second;
    Faction* f = find_faction(w, faction);
    if (!f) return;
    if (e.has_faith_set || e.fear_add != 0 || !e.tags.empty()) {
        if (e.has_faith_set) set_faith(*f, p.c, w.year, "oath_" + effect, 1.0, e.faith_set, e.fear_add, e.tags);
        else remember(*f, p.c, w.year, "oath_" + effect, 1.0, e.fear_add, 0, e.tags);
    }
    if (e.has_modifier) add_modifier(w, "oath." + effect, faction, e.mod_var, e.mod_op, e.mod_value, e.mod_decades);
}

}
