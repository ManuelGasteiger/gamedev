#include "coalitions.h"
#include "../world_util.h"

namespace ages {

std::vector<std::string> side_members(const War& war, bool side_a) {
    std::vector<std::string> m = side_a ? war.allies_a : war.allies_b;
    m.insert(m.begin(), side_a ? war.a : war.b);
    return m;
}

double side_power(const World& w, const War& war, bool side_a, const Constants& c) {
    double sum = 0;
    for (auto& id : side_members(war, side_a))
        if (const Faction* f = find_faction(w, id)) sum += power(*f, c);
    return sum;
}

static bool in_war(const War& war, const std::string& id) {
    return war.a == id || war.b == id || contains(war.allies_a, id) || contains(war.allies_b, id);
}

void join_side(World& w, War& war, bool side_a, Faction& member, const std::string& reason, const Cause& cause) {
    (side_a ? war.allies_a : war.allies_b).push_back(member.id);
    for (auto& other : side_members(war, !side_a)) {
        if (Faction* o = find_faction(w, other)) {
            if (!at_war(*o, member.id)) o->at_war_with.push_back(member.id);
            if (!at_war(member, other)) member.at_war_with.push_back(other);
        }
    }
    emit(w, "joined_war", member.id, side_a ? war.a : war.b, 0, {reason}, cause, war_thread(war));
}

void clear_side_wars(World& w, const War& war) {
    for (auto& ma : side_members(war, true))
        for (auto& mb : side_members(war, false)) {
            bool elsewhere = false;
            for (auto& x : w.wars)
                if (!(x.a == war.a && x.b == war.b) && in_war(x, ma) && in_war(x, mb)) elsewhere = true;
            if (elsewhere) continue;
            if (Faction* a = find_faction(w, ma)) erase_value(a->at_war_with, mb);
            if (Faction* b = find_faction(w, mb)) erase_value(b->at_war_with, ma);
        }
}

static War* coalition_against(World& w, const std::string& hegemon) {
    for (auto& x : w.wars) if (x.coalition && x.b == hegemon) return &x;
    return nullptr;
}

// a realm (own plus vassal regions) of coalition_region_share whose power is more than coalition_power_ratio
// times that of every faction bordering it
static bool hegemon(const World& w, const Faction& h, const Constants& c) {
    if (static_cast<double>(realm_regions(w, h)) < c.coalition_region_share * static_cast<double>(w.regions.size())) return false;
    double ph = power(h, c);
    std::vector<std::string> around = realm_neighbours(w, h);
    for (auto& id : around)
        if (const Faction* g = find_faction(w, id))
            if (ph <= c.coalition_power_ratio * power(*g, c)) return false;
    return !around.empty();
}

// a vassal that joins the coalition against its own overlord revolts and is free
static void revolt(World& w, Faction& g, const std::string& hid, const Cause& cause) {
    if (overlord_of(g) != hid) return;
    g.tags.erase("vassal_of:" + hid);
    emit(w, "vassal_revolt", g.id, hid, 0, {"coalition"}, cause, "hegemony:" + hid);
}

void form_coalitions(World& w, const Params& p) {
    const Constants& c = p.c;
    std::vector<std::string> hegemons;
    for (auto& h : w.factions) if (hegemon(w, h, c)) hegemons.push_back(h.id);
    for (auto& hid : hegemons) {
        Faction* h = find_faction(w, hid);
        if (!h) continue;
        std::vector<std::string> around = realm_neighbours(w, *h);
        for (auto& gid : around) {
            Faction* g = find_faction(w, gid);
            if (!g || at_war(*g, hid) || (is_vassal(*g) && overlord_of(*g) != hid)) continue;
            if (affinity(p, *g, *h) >= c.coalition_affinity || !w.rng.chance(c.coalition_join_chance)) continue;
            int heg = last_event_id(w, "hegemony", hid);
            Cause why = because("against_hegemon", {heg}, {{"realm", static_cast<double>(realm_regions(w, *h))}, {"affinity", affinity(p, *g, *h)}});
            revolt(w, *g, hid, why);
            if (War* war = coalition_against(w, hid)) {
                join_side(w, *war, true, *g, "coalition", why);
                continue;
            }
            War war;
            war.a = g->id;
            war.b = hid;
            war.started = w.year;
            war.coalition = true;
            war.thread = "coalition:" + hid + ":" + std::to_string(w.year);
            g->at_war_with.push_back(hid);
            h->at_war_with.push_back(g->id);
            war.declared = emit(w, "coalition_formed", g->id, hid, 0, {}, why, war.thread);
            open_thread(w, war.thread);
            w.wars.push_back(war);
        }
    }
}

static void prune_allies(World& w, const Params& p) {
    for (auto& f : w.factions) {
        std::vector<std::string> drop;
        for (auto& t : f.tags) {
            if (t.rfind("ally:", 0) != 0) continue;
            const Faction* o = find_faction(w, t.substr(5));
            if (!o || affinity(p, f, *o) <= p.c.affinity_alliance) drop.push_back(t);
        }
        for (auto& t : drop) f.tags.erase(t);
    }
}

void join_alliances(World& w, const Params& p) {
    const Constants& c = p.c;
    prune_allies(w, p);
    for (size_t wi = 0; wi < w.wars.size(); ++wi) {
        if (w.wars[wi].coalition) continue;
        Faction* defender = find_faction(w, w.wars[wi].b);
        Faction* attacker = find_faction(w, w.wars[wi].a);
        if (!defender || !attacker) continue;
        for (size_t i = 0; i < w.factions.size(); ++i) {
            Faction& g = w.factions[i];
            if (g.id == defender->id || g.id == attacker->id || at_war(g, attacker->id) || at_war(g, defender->id) || is_vassal(g)) continue;
            if (!adjacent(w, g, *attacker) || !adjacent(w, g, *defender)) continue;
            if (affinity(p, g, *defender) <= c.affinity_alliance || !w.rng.chance(c.alliance_join_chance * dts(w, c))) continue;
            join_side(w, w.wars[wi], false, g, "alliance", because("friend_attacked", {w.wars[wi].declared}, {{"affinity", affinity(p, g, *defender)}}));
            g.tags.insert("ally:" + defender->id);
            defender->tags.insert("ally:" + g.id);
        }
    }
}

}
