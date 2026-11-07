#include <algorithm>
#include "ages/memory_tiers.h"
#include "ages/world_util.h"
#include "scn.h"

// item 10, the forced-state test (spec section 8.4): overlays a spec on a world so a scenario's roles can bind. Courts
// take over existing factions (a `near` court an adjacent one), persons and relics are added, acts are written into the
// log with sleep acts between their wakings and a memory entry on the court that remembers them.

namespace scn {

using namespace ages;
using nlohmann::json;

namespace {

void stock(Tiers& t, const std::string& tier, double v) {
    if (tier == "monumental") t.monumental = std::max(t.monumental, v);
    else if (tier == "written") t.written = std::max(t.written, v);
    else t.oral = std::max(t.oral, v);
}

void set_value(Tiers& t, double want, const std::string& tier) {
    t = Tiers();
    if (tier == "monumental") t.monumental = want;
    else if (tier == "written") t.written = want;
    else t.oral = want;
}

}

bool lab_world(World& w, const Params& p, const json& spec, std::string& err) {
    std::map<std::string, std::string> as;   // spec name -> world id
    std::vector<std::string> used;
    for (auto& c : spec.value("courts", json::array())) {
        std::string name = c.value("as", std::string()), race = c.value("race", std::string()), near = c.value("near", std::string());
        Faction* pick = nullptr;
        for (int pass = 0; pass < 3 && !pick; ++pass)
            for (auto& f : w.factions) {
                if (contains(used, f.id) || f.regions.empty()) continue;
                if (pass == 0 && !race.empty() && f.race != race) continue;
                if (pass <= 1 && !near.empty()) { const Faction* n = find_faction(w, as[near]); if (!n || !adjacent(w, *n, f)) continue; }
                pick = &f;
                break;
            }
        if (!pick) { err = "lab: no faction left for court " + name; return false; }
        used.push_back(pick->id);
        as[name] = pick->id;
        Faction& f = *pick;
        if (!race.empty()) f.race = race;
        if (c.contains("era")) f.era = c["era"].get<int>();
        if (c.contains("cohesion")) f.cohesion = c["cohesion"].get<double>();
        if (c.contains("piety")) f.culture.traits.piety = c["piety"].get<double>();
        if (c.contains("openness")) f.culture.traits.openness = c["openness"].get<double>();
        if (c.contains("militarism")) f.culture.traits.militarism = c["militarism"].get<double>();
        if (c.contains("faith")) f.faith = c["faith"].get<double>();
        if (c.contains("wealth")) f.wealth = c["wealth"].get<double>();
        std::string tier = c.value("memory_tier", std::string("oral"));
        if (tier == "none") tier = "oral";
        if (c.contains("dread")) set_value(f.dread, c["dread"].get<double>(), tier);
        if (c.contains("awe")) set_value(f.awe, c["awe"].get<double>(), tier);
        if (c.value("memory_tier", std::string()) != "" && c.value("memory_tier", std::string()) != "none") stock(f.dread, tier, p.c.mem.tier_shown);
        for (auto& t : c.value("tags", json::array())) f.tags.insert(t.get<std::string>());
        for (auto& t : c.value("region_tags", json::array())) if (Region* r = find_region(w, f.regions.front())) r->tags.insert(t.get<std::string>());
        if (c.value("slayers", false)) {
            f.tags.insert("slayers");
            Order o;
            o.id = "order:" + f.id;
            o.court = f.id;
            o.founded = w.year - 50;
            o.grievance = 1;
            w.orders.push_back(o);
            std::sort(w.orders.begin(), w.orders.end(), [](const Order& a, const Order& b) { return a.id < b.id; });
        }
        if (c.contains("slayer_stage") && c["slayer_stage"] != "none") {
            TimedTag t;
            t.on = "faction";
            t.target = f.id;
            t.tag = c["slayer_stage"] == "ready" ? "slayers_ready" : "slayers_forming";
            t.from = w.year - 10;
            t.until = w.year + 1000;
            w.timed_tags.push_back(t);
        }
        if (c.value("knows_prophecy", false) && !contains(w.prophecy.known_by, f.id)) { w.prophecy.known_by.push_back(f.id); std::sort(w.prophecy.known_by.begin(), w.prophecy.known_by.end()); }
        if (c.contains("lost_region_years")) f.last_region_lost = w.year - c["lost_region_years"].get<int>();
        if (c.contains("lair_known")) f.lair_written = f.lair_oral = c["lair_known"].get<double>();
    }
    for (auto& c : spec.value("courts", json::array())) {
        Faction* f = find_faction(w, as[c.value("as", std::string())]);
        f->at_war_with.clear();
        for (auto& e : c.value("at_war_with", json::array())) {
            std::string other = as.count(e.get<std::string>()) ? as[e.get<std::string>()] : "";
            if (other.empty()) for (auto& g : w.factions) if (g.id != f->id && adjacent(w, *f, g)) { other = g.id; break; }
            if (other.empty()) continue;
            if (!contains(f->at_war_with, other)) f->at_war_with.push_back(other);
            if (Faction* g = find_faction(w, other)) if (!contains(g->at_war_with, f->id)) g->at_war_with.push_back(f->id);
        }
        if (c.contains("peace") && c["peace"].get<bool>()) {
            for (auto& o : f->at_war_with) if (Faction* g = find_faction(w, o)) erase_value(g->at_war_with, f->id);
            f->at_war_with.clear();
        }
    }
    Dragon& d = w.dragons[kPlayer];
    const json& dj = spec.value("dragon", json::object());
    if (dj.contains("growth")) for (auto it = dj["growth"].begin(); it != dj["growth"].end(); ++it) d.growth[it.key()] = it.value().get<double>();
    if (dj.contains("phase")) d.phase = dj["phase"].get<int>();
    if (dj.contains("wounds")) d.wounds = dj["wounds"].get<double>();
    for (auto& t : dj.value("talents", json::array())) if (!contains(d.talents, t.get<std::string>())) d.talents.push_back(t.get<std::string>());
    for (auto& t : dj.value("tags", json::array())) d.tags.insert(t.get<std::string>());
    if (dj.contains("magic_pool")) d.magic = dj["magic_pool"].get<double>();
    if (dj.contains("knowledge")) d.knowledge = dj["knowledge"].get<double>();
    if (dj.contains("faith_pool")) d.faith_pool = dj["faith_pool"].get<double>();
    const json& wj = spec.value("world", json::object());
    if (wj.contains("magic")) w.magic = wj["magic"].get<double>();
    for (auto& t : wj.value("tags", json::array())) w.tags.insert(t.get<std::string>());
    if (wj.contains("prophecy_ending")) w.prophecy.ending = wj["prophecy_ending"].get<std::string>();
    int serial = 0;
    for (auto& x : spec.value("persons", json::array())) {
        Person n;
        n.id = "lab" + std::to_string(++serial);
        static const char* const names[] = {"Aldric", "Marta", "Voss", "Ilsa", "Kerz", "Hadda", "Orm", "Senna"};
        n.name = names[(serial - 1) % 8];
        std::string court = x.value("court", std::string());
        bool lair = court == "lair";
        std::string from = x.value("from", court);
        n.faction = as.count(from) ? as[from] : (w.factions.empty() ? "" : w.factions.front().id);
        n.race = find_faction(w, n.faction) ? find_faction(w, n.faction)->race : "human";
        n.role = x.value("role", std::string("sage"));
        n.sex = x.value("sex", std::string("m")) == "f" ? "f" : "m";
        n.born = w.year - x.value("age", 40);
        n.dies = w.year + 200;
        for (auto& t : x.value("tags", json::array())) n.tags.insert(t.get<std::string>());
        for (auto& t : x.value("traits", json::array())) n.traits.list.push_back(t.get<std::string>());
        if (x.contains("cunning")) n.traits.cunning = x["cunning"].get<double>();
        for (auto& m : x.value("memory", json::array())) { PersonMemory pm; pm.ev = m.get<std::string>(); pm.year = w.year - 20; pm.sev = 2; n.mem.push_back(pm); }
        if (x.contains("debt")) { PersonMemory pm; pm.ev = "give"; pm.year = w.year - 20; pm.sev = 3; pm.debt = x["debt"].get<double>(); pm.s = 1; n.mem.push_back(pm); }
        if (lair) { n.tags.insert("hostage"); n.hostage_since = w.year - x.value("held_years", 10); d.hostages.push_back(n.id); std::sort(d.hostages.begin(), d.hostages.end()); }
        if (x.contains("kept")) n.tags.insert("kept:" + x["kept"].get<std::string>());
        if (x.contains("slayer_rank")) { n.slayer.rank = x["slayer_rank"].get<std::string>(); n.slayer.power = 5; }
        w.people.push_back(n);
        std::sort(w.people.begin(), w.people.end(), [](const Person& a, const Person& b) { return a.id < b.id; });
        as[x.value("as", std::string())] = n.id;
        if (x.value("known", false)) meet(w, n.id, "lab");
        if (x.value("envoy", false) || x.value("is_lie", false)) {
            Arrival a;
            a.id = "arr:" + n.id;
            a.kind = "envoy";
            a.court = n.faction;
            a.person = n.id;
            a.year = w.year;
            a.is_lie = x.value("is_lie", false);
            w.arrivals.push_back(a);
        }
        if (x.contains("arrival")) {
            Arrival a;
            a.id = "arr:" + n.id;
            a.kind = x["arrival"].get<std::string>();
            a.court = n.faction;
            a.person = n.id;
            a.year = w.year;
            w.arrivals.push_back(a);
        }
    }
    for (auto& r : spec.value("relics", json::array())) {
        Relic rl;
        rl.kind = r.value("kind", std::string("crown"));
        rl.agenda = r.value("agenda", std::string());
        rl.court = as.count(r.value("court", std::string())) ? as[r.value("court", std::string())] : "";
        rl.id = rl.kind + ":" + rl.court;
        rl.year = w.year - 20;
        d.chron.relics.push_back(rl);
        as[r.value("as", std::string())] = rl.id;
    }
    for (auto& r : spec.value("remnants", json::array())) {
        Remnant m;
        m.race = r.value("race", std::string("orc"));
        m.strength = r.value("strength", 0.5);
        w.remnants.push_back(m);
    }
    const json& acts = spec.value("acts", json::array());
    int oldest = 0;
    for (auto& a : acts) oldest = std::max(oldest, a.value("wakings_ago", 0));
    for (int k = oldest; k >= 0; --k) {
        for (auto& aj : acts) {
            if (aj.value("wakings_ago", 0) != k) continue;
            Act a;
            a.type = aj.value("type", std::string());
            a.year = w.year - aj.value("years_ago", 10 * k + 5);
            std::string t = aj.value("target", std::string());
            std::string id = as.count(t) ? as[t] : t;
            if (const Person* x = find_person(w, id)) { a.target_kind = "person"; a.item = id; a.target = x->faction; }
            else { a.target = id; if (const Faction* f = find_faction(w, id)) if (!f->regions.empty()) a.region = f->regions.front(); }
            if (aj.contains("structure")) a.item = aj["structure"].get<std::string>();
            a.magnitude = aj.value("magnitude", 0.5);
            a.source = aj.value("source", std::string());
            a.answers = aj.value("answers", std::string());
            if (aj.contains("payload")) for (auto it = aj["payload"].begin(); it != aj["payload"].end(); ++it) {
                if (it.value().is_number()) a.payload[it.key()] = it.value().get<double>();
                else if (it.value().is_string()) a.args[it.key()] = it.value().get<std::string>();
            }
            if (!a.target.empty()) a.witnesses.push_back(a.target);
            int idx = static_cast<int>(w.acts.size());
            w.acts.push_back(a);
            as[aj.value("as", std::string())] = std::to_string(idx);
            if (Faction* f = find_faction(w, a.target)) {
                MemoryEntry m;
                m.year = a.year;
                m.type = a.type;
                m.magnitude = a.magnitude;
                m.axis = "dread";
                m.tier = memory_tier(*f, p.c.mem).empty() ? "oral" : memory_tier(*f, p.c.mem);
                m.strength = p.scenarios.n.remember_stock;
                m.act = idx;
                f->memory.push_back(m);
                stock(f->dread, m.tier, p.scenarios.n.remember_stock);
            }
        }
        Act s;
        s.type = "sleep";
        s.year = w.year - 10 * k;
        s.payload["years"] = 10;
        s.source = "sleep";
        w.acts.push_back(s);
    }
    return true;
}

}
