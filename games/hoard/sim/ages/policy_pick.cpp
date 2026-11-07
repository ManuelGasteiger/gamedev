#include "policy_pick.h"
#include "ages.h"
#include "defences.h"
#include "endings.h"
#include "memory_tiers.h"
#include "world_util.h"

namespace ages {

std::vector<std::string> extra_targets(const World& w, const Params& p, const std::string& key) {
    (void)p;
    std::vector<std::string> ids;
    const Faction* best = nullptr;
    if (key == "faithful" || key == "awed") {
        for (auto& f : w.factions) {
            double v = key == "faithful" ? f.faith : awe_eff(f);
            if (!best || v > (key == "faithful" ? best->faith : awe_eff(*best))) best = &f;
        }
        if (best) ids.push_back(best->id);
    } else if (key == "orc") {
        for (auto& f : w.factions) {
            if (dominant_race(f) != "orc") continue;
            for (auto& g : w.factions) if (g.id != f.id && dominant_race(g) != "orc" && adjacent(w, f, g)) { ids.push_back(f.id); break; }
            if (!ids.empty()) break;
        }
    } else if (key == "hostages") {
        for (auto& id : w.dragons[kPlayer].hostages)
            if (const Person* x = find_person(w, id)) if (!x->faction.empty() && !contains(ids, x->faction)) ids.push_back(x->faction);
    } else if (key == "pretender") {
        for (auto& f : w.factions)
            for (auto& t : f.tags) if (t.rfind("pretender:", 0) == 0) { ids.push_back(f.id); break; }
    } else if (key == "liar") {
        for (auto& o : w.offers) if (o.is_lie && !contains(ids, o.from_faction)) ids.push_back(o.from_faction);
    } else if (key == "slayer_neighbour") {
        for (auto& f : w.factions) {
            if (!has_tag(f.tags, "slayers")) continue;
            for (auto& g : w.factions) if (g.id != f.id && adjacent(w, f, g)) { ids.push_back(g.id); break; }
            if (!ids.empty()) break;
        }
    }
    return ids;
}

std::string person_of(const World& w, const Faction& f, const std::string& key) {
    if (key == "hostage" || key == "willing") {
        for (auto& x : w.people) {
            if (x.faction != f.id) continue;
            if (key == "hostage" && contains(w.dragons[kPlayer].hostages, x.id)) return x.id;
            if (key == "willing" && has_tag(x.tags, "willing") && !contains(w.dragons[kPlayer].hostages, x.id)) return x.id;
        }
        return "";
    }
    if (key == "slayer") {   // item 08's roster
        for (auto& x : w.people) if (x.faction == f.id && !x.slayer.rank.empty()) return x.id;
        return "";
    }
    if (key == "visitor") {
        for (auto& a : w.arrivals) if (a.court == f.id && !a.person.empty()) return a.person;
        return "";
    }
    if (key == "judged") {
        for (auto& x : w.people) {
            if (x.faction != f.id || contains(w.dragons[kPlayer].hostages, x.id)) continue;
            for (const char* t : {"false_dragon", "usurper", "necromancer", "tyrant"}) if (has_tag(x.tags, t)) return x.id;
        }
        const Person* r = find_person_role(w, f.id, "ruler");
        return r && !has_tag(r->tags, "hostage") ? r->id : "";
    }
    const Person* x = find_person_role(w, f.id, key);
    return x ? x->id : "";
}

std::string region_of(const World& w, const Faction* f, const std::string& key) {
    if (key == "lair") { for (auto& r : w.regions) if (r.lair) return r.id; return ""; }
    if (key == "wild" || key == "wild_burned")
        for (auto& r : w.regions) {
            if (r.lair || !r.owner.empty()) continue;
            if (key == "wild" || r.burn >= 0.5 || has_tag(r.tags, "wasteland")) return r.id;
        }
    if (key == "capital" && f) return capital_region(w, *f);
    return "";
}

std::string second_court(const World& w, const Params& p, const Faction& f, const std::string& key) {
    const Faction* best = nullptr;
    for (auto& g : w.factions) {
        if (g.id == f.id || !adjacent(w, f, g)) continue;
        if (key == "enemy" && !at_war(f, g.id)) continue;
        if (key == "non_orc_neighbour" && dominant_race(g) == "orc") continue;
        if (!best || power(g, p.c) > power(*best, p.c)) best = &g;
    }
    return best ? best->id : "";
}

void policy_leftovers(World& w, const Params& p, const Policy& policy) {
    std::vector<Act> acts;
    if (policy.ignore_hooks)
        for (auto& h : w.hooks) {
            Act a;
            a.type = "ignore";
            a.target_kind = "hook";
            a.target = h.id;
            a.source = "hook";
            a.answers = h.id;
            acts.push_back(a);
        }
    if (policy.swear_offers)
        for (auto& o : w.offers) {
            if (o.kind == "exit") continue;
            Act a;
            a.type = "swear";
            a.target_kind = "offer";
            a.target = o.id;
            a.source = "lair";
            a.answers = o.id;
            acts.push_back(a);
        }
    if (policy.take_exit)
        for (auto& e : p.endings.endings) {
            if (!e.enabled || e.exit_kind.empty() || !ending_ready(w, e.id)) continue;
            Act a;
            a.type = "exit";
            a.args["kind"] = e.exit_kind;
            a.source = "lair";
            acts.push_back(a);
            break;
        }
    if (!acts.empty()) apply_acts(w, p, acts);
}

}
