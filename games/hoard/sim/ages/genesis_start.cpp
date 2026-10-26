#include "genesis.h"
#include <cstdlib>
#include "names.h"
#include "pop.h"
#include "rules/card_ctx.h"
#include "rules/rules.h"
#include "rules/world_rules.h"
#include "world_util.h"

// the starting situation (genesis.json "templates"): a template drawn by weight deals the map, lays the ruins of a
// fallen kingdom when it has one, places its courts on their terrain (capital first, then neighbouring land), adds
// courts up to the minimum, and sets the old wars, fealties, alliances, marks and remnants it names

namespace ages {

namespace {

double roll(Rng& rng, const Span& s) { return s.lo + (s.hi - s.lo) * rng.uniform(); }
int roll_int(Rng& rng, const Span& s) {
    int lo = static_cast<int>(s.lo + 0.5), hi = static_cast<int>(s.hi + 0.5);
    return hi <= lo ? lo : rng.range(lo, hi + 1);
}

// a weighted pick in key order; "fallen" stands for the fallen kingdom's race
std::string pick(Rng& rng, const std::map<std::string, double>& weights, const std::string& fallen) {
    double total = 0;
    for (auto& kv : weights) if (kv.second > 0 && (kv.first != "fallen" || !fallen.empty())) total += kv.second;
    if (total <= 0) return "";
    double r = rng.uniform() * total;
    std::string last;
    for (auto& kv : weights) {
        if (kv.second <= 0 || (kv.first == "fallen" && fallen.empty())) continue;
        last = kv.first == "fallen" ? fallen : kv.first;
        if (r < kv.second) return last;
        r -= kv.second;
    }
    return last;
}

const StartTemplate& draw_template(Rng& rng, const GenesisDef& g) {
    double total = 0;
    for (auto& t : g.templates) total += std::max(0.0, t.weight);
    double r = rng.uniform() * total;
    for (auto& t : g.templates) {
        if (r < std::max(0.0, t.weight)) return t;
        r -= std::max(0.0, t.weight);
    }
    return g.templates.back();
}

struct Placer {
    World& w;
    const Params& p;
    const StartTemplate& t;
    std::string fallen_race, fallen_name;
    std::vector<std::string> ruins;
    std::map<std::string, std::vector<std::string>> labels;
    std::vector<std::pair<std::string, std::string>> marks;   // (faction, tag), written once the founding event exists
    std::vector<std::string> undying;

    Rng& rng() { return w.rng; }
    const Culture& culture(const std::string& race) const { return p.races.at(race).culture; }
    bool open(const Region& r) const { return !r.lair && r.owner.empty() && !contains(ruins, r.id); }
    bool borders_any(const Region& r, const std::vector<std::string>& ids) const {
        for (auto& n : r.neighbours) if (contains(ids, n)) return true;
        return false;
    }
    std::string any_of(const std::vector<std::string>& v) { return v.empty() ? "" : v[static_cast<size_t>(rng().range(0, static_cast<int>(v.size())))]; }

    std::vector<std::string> near_ids(const std::string& near) const {
        if (near == "ruins") return ruins;
        std::vector<std::string> out;
        auto it = labels.find(near);
        if (it == labels.end()) return out;
        for (auto& id : it->second) if (const Faction* f = find_faction(w, id)) out.insert(out.end(), f->regions.begin(), f->regions.end());
        return out;
    }

    // the capital: its terrain next to what it should border, else its terrain, its race's home, any open land, and
    // last the ruins themselves
    std::string capital(const CourtSpec& c, const std::string& race) {
        const Culture& cu = culture(race);
        std::vector<std::string> near = near_ids(c.near);
        for (int pass = 0; pass < 5; ++pass) {
            std::vector<std::string> out;
            for (auto& r : w.regions) {
                if (r.lair || !r.owner.empty() || (pass < 4 && contains(ruins, r.id))) continue;
                bool terrain = c.terrain.empty() ? home_terrain(cu, r) : contains(c.terrain, r.terrain);
                if (pass == 0 && !(terrain && !near.empty() && borders_any(r, near))) continue;
                if (pass == 1 && !terrain) continue;
                if (pass == 2 && !home_terrain(cu, r)) continue;
                out.push_back(r.id);
            }
            if (!out.empty()) return any_of(out);
        }
        return "";
    }

    void grow(Faction& f, const CourtSpec& c, int want) {
        while (static_cast<int>(f.regions.size()) < want) {
            std::vector<std::string> out;
            for (auto& r : w.regions) {
                if (!open(r) || !borders_any(r, f.regions)) continue;
                if (!c.any_terrain && !home_terrain(f.culture, r) && !contains(c.terrain, r.terrain)) continue;
                out.push_back(r.id);
            }
            std::string id = any_of(out);
            if (id.empty()) return;
            find_region(w, id)->owner = f.id;
            f.regions.push_back(id);
            std::sort(f.regions.begin(), f.regions.end());
        }
    }

    void mix(Faction& f, const CourtSpec& c) {
        std::map<std::string, double> pool = c.mix;
        pool.erase(f.race);
        std::string minor = pick(rng(), pool, fallen_race);
        if (minor.empty() || minor == f.race || !p.races.count(minor)) return;
        double share = roll(rng(), c.mix_share);
        f.pop.clear();
        f.pop[f.race] = 1.0 - share;
        f.pop[minor] = share;
        blend_culture(f, p);
        blend_traits(f.culture.traits, culture(minor).traits, share);
    }

    void adjust_traits(Faction& f, const CourtSpec& c) {
        Traits& tr = f.culture.traits;
        for (auto& kv : c.traits) {
            double* v = kv.first == "authority" ? &tr.authority : kv.first == "openness" ? &tr.openness : kv.first == "militarism" ? &tr.militarism : kv.first == "piety" ? &tr.piety : nullptr;
            if (v) *v = clamp01(*v + kv.second);
        }
    }

    void court(const CourtSpec& c) {
        if (!rng().chance(c.chance)) return;
        int n = roll_int(rng(), c.count);
        for (int i = 0; i < n && static_cast<int>(w.factions.size()) < p.genesis.max_courts; ++i) {
            std::string race = pick(rng(), c.race, fallen_race);
            if (race.empty() || !p.races.count(race)) continue;
            std::string cap = capital(c, race);
            if (cap.empty()) return;
            double wealth = roll(rng(), c.wealth);
            double cohesion = clamp01(roll(rng(), c.cohesion));
            int era = roll_int(rng(), c.era);
            Faction f = found_faction(w, p, race, cap, wealth, cohesion, era);
            find_region(w, cap)->owner = f.id;
            if (!c.mix.empty()) mix(f, c);
            adjust_traits(f, c);
            grow(f, c, roll_int(rng(), c.regions));
            f.max_regions = static_cast<int>(f.regions.size());
            for (auto tag : c.tags) {
                size_t at = tag.find("$fallen");
                if (at != std::string::npos) { if (fallen_name.empty()) continue; tag.replace(at, 7, fallen_name); }
                marks.push_back({f.id, tag});
            }
            if (c.undying) undying.push_back(f.id);
            labels[c.label].push_back(f.id);
            w.factions.push_back(f);
            sort_by_id(w);
        }
    }

    void lay_ruins() {
        fallen_race = pick(rng(), t.fallen, "");
        if (fallen_race.empty() || !p.races.count(fallen_race)) { fallen_race.clear(); return; }
        fallen_name = new_faction_name(w, p, fallen_race);
        int age = roll_int(rng(), t.ruin_age);
        FactionRecord rec;
        rec.id = fallen_name;
        rec.race = fallen_race;
        rec.died = w.year - age;
        rec.born = rec.died - roll_int(rng(), t.fallen_reign);
        rec.max_regions = roll_int(rng(), t.ruins);
        rec.max_realm = rec.max_regions;
        rec.era_years.assign(static_cast<size_t>(t.fallen_era) + 1, rec.born);
        w.graveyard.push_back(rec);
        const Culture& cu = culture(fallen_race);
        std::vector<std::string> starts;
        for (auto& r : w.regions) if (!r.lair && home_terrain(cu, r)) starts.push_back(r.id);
        if (starts.empty()) for (auto& r : w.regions) if (!r.lair) starts.push_back(r.id);
        ruins.push_back(any_of(starts));
        while (static_cast<int>(ruins.size()) < rec.max_regions) {
            std::vector<std::string> next;
            for (auto& r : w.regions) if (!r.lair && !contains(ruins, r.id) && borders_any(r, ruins)) next.push_back(r.id);
            std::string id = any_of(next);
            if (id.empty()) break;
            ruins.push_back(id);
        }
        for (auto& id : ruins) {
            Region* r = find_region(w, id);
            r->burn = clamp01(roll(rng(), t.ruin_burn));
            r->tags.insert("ruins");
            if (rng().chance(t.ruin_wasteland)) r->tags.insert("wasteland");
            r->empty_years = age;
        }
        w.race_max_era[fallen_race] = std::max(w.race_max_era[fallen_race], t.fallen_era);
        w.race_last_region[fallen_race] = ruins.front();
    }

    std::string label_id(const std::string& label) const {
        size_t dot = label.find('.');
        auto it = labels.find(label.substr(0, dot));
        size_t idx = dot == std::string::npos ? 0 : static_cast<size_t>(std::atoi(label.c_str() + dot + 1));
        return it == labels.end() || idx >= it->second.size() ? "" : it->second[idx];
    }

    Faction* neighbour_of(const Faction* a) {
        if (!a) return nullptr;
        std::vector<std::string> near;
        for (auto& g : w.factions) if (g.id != a->id && adjacent(w, *a, g) && !allied(*a, g)) near.push_back(g.id);
        return find_faction(w, any_of(near));
    }

    void relate(const RelationSpec& r, int began) {
        if (!rng().chance(r.chance)) return;
        Faction* a = find_faction(w, label_id(r.a));
        Faction* b = r.b == "neighbour" ? neighbour_of(a) : find_faction(w, label_id(r.b));
        if (!a || !b || a->id == b->id) return;
        std::string aid = a->id, bid = b->id;
        Cause why = because("genesis", {began});
        bool war = r.kind == "war" && adjacent(w, *a, *b);
        if (war && !at_war(*a, bid) && !is_vassal(*a) && !is_vassal(*b)) {
            start_war(w, *a, *b, "ancient_feud", why);
            w.chronicle.back().text_key = "ev.war_started_genesis";
        } else if (r.kind == "vassal" && !is_vassal(*a) && !is_vassal(*b) && overlord_of(*a) != bid) {
            make_vassal(w, *b, *a, why);
            w.chronicle.back().text_key = "ev.vassal_sworn_genesis";
        }
        else if (r.kind == "alliance" && !at_war(*a, bid)) { a->tags.insert("ally:" + bid); b->tags.insert("ally:" + aid); }
        else if (r.kind == "old_enemies" || (r.kind == "war" && !war)) {
            write_tag(w, aid, "old_enemies:" + bid, r.decades, true, began);
            write_tag(w, bid, "old_enemies:" + aid, r.decades, true, began);
        } else if (r.kind == "mark") {
            std::string tag = r.tag;
            size_t at = tag.find("$b");
            if (at != std::string::npos) tag.replace(at, 2, bid);
            write_tag(w, aid, tag, r.decades, true, began);
        }
    }

    void remnants(int began) {
        for (auto& rs : t.remnants) {
            if (!rng().chance(rs.chance)) continue;
            std::string race = pick(rng(), rs.race, fallen_race);
            if (race.empty() || !p.races.count(race)) continue;
            bool held = false;
            for (auto& r : w.remnants) if (r.race == race) held = true;
            if (held) continue;
            Remnant rem;
            rem.race = race;
            rem.strength = roll(rng(), rs.strength);
            if (race == fallen_race && !ruins.empty()) rem.origin = ruins.front();
            else for (auto& r : w.regions) if (!r.lair && home_terrain(culture(race), r)) { rem.origin = r.id; break; }
            rem.max_era = w.race_max_era[race];
            w.remnants.push_back(rem);
            std::sort(w.remnants.begin(), w.remnants.end(), [](const Remnant& a, const Remnant& b) { return a.race < b.race; });
            bool fallen = race == fallen_race;
            emit(w, "remnant_formed", race, rem.origin, rem.strength, {fallen ? "last_faction_gone" : "off_map"}, because(fallen ? "genesis_fall" : "off_map", {began}));
        }
    }
};

}

bool genesis_start(World& w, const Params& p, std::string& err) {
    const GenesisDef& g = p.genesis;
    if (!g.present) { err = "genesis requested but data/genesis.json is missing"; return false; }
    const StartTemplate& t = draw_template(w.rng, g);
    w.genesis.template_id = t.id;
    deal_map(w, p, t);
    Placer pl{w, p, t, "", "", {}, {}, {}, {}};
    if (!t.fallen.empty()) pl.lay_ruins();
    for (auto& c : t.courts) pl.court(c);
    for (int tries = 0; static_cast<int>(w.factions.size()) < g.min_courts && tries < g.min_courts * 2; ++tries) {
        CourtSpec c = g.fallback;
        if (!t.fallback_race.empty()) c.race = t.fallback_race;
        pl.court(c);
    }
    if (w.factions.empty()) { err = "genesis: no court could be placed"; return false; }
    int began = emit(w, "world_began", t.id, pl.fallen_name, static_cast<double>(w.factions.size()), {}, because("genesis"));
    w.chronicle.back().text_key = "ev.genesis_" + t.id;
    std::map<std::string, int> founded;
    for (auto& kv : pl.labels)
        for (auto& id : kv.second)
            if (const Faction* f = find_faction(w, id)) founded[id] = emit(w, "court_founded", id, f->regions.front(), static_cast<double>(f->regions.size()), {kv.first}, because("genesis", {began}));
    for (auto& m : pl.marks) write_tag(w, m.first, m.second, 0, true, founded[m.first]);
    note_race_eras(w);
    rule_modifiers(w, p);
    init_people(w, p);
    for (auto& id : pl.undying)
        if (Faction* f = find_faction(w, id)) make_undying(w, p, *f, because("genesis", {began}));
    for (auto& r : t.relations) pl.relate(r, began);
    pl.remnants(began);
    for (auto& f : w.factions) f.max_realm = static_cast<int>(realm_regions(w, f));
    return true;
}

}
