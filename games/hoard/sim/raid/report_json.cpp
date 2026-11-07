#include "report.h"
#include "ages/json.h"

namespace ages {

using nlohmann::json;

namespace {

std::map<std::string, double> numbers(const json& j) {
    std::map<std::string, double> out;
    if (j.is_object())
        for (auto it = j.begin(); it != j.end(); ++it) if (it->is_number()) out[it.key()] = it->get<double>();
    return out;
}

std::vector<std::string> ids(const json& j) {
    std::vector<std::string> out;
    if (!j.is_array()) return out;
    for (auto& v : j) {
        if (v.is_string()) out.push_back(v.get<std::string>());
        else if (v.is_object() && v.contains("id") && v["id"].is_string()) out.push_back(v["id"].get<std::string>());
    }
    return out;
}

double num(const json& j, const char* k) { return j.contains(k) && j[k].is_number() ? j[k].get<double>() : 0; }
std::string str(const json& j, const char* k) { return j.contains(k) && j[k].is_string() ? j[k].get<std::string>() : ""; }
bool flag(const json& j, const char* k) { return j.contains(k) && j[k].is_boolean() && j[k].get<bool>(); }

json strings(const std::vector<std::string>& v) { json a = json::array(); for (auto& s : v) a.push_back(s); return a; }

}

bool report_from_json(const std::string& text, RaidReport& out, std::string& err) {
    json j = json::parse(text, nullptr, false);
    if (j.is_discarded() || !j.is_object()) { err = "report must be a json object"; return false; }
    out = RaidReport();
    out.sortie = j.contains("sortie") && j["sortie"].is_number() ? j["sortie"].get<int>() : 0;
    out.shell = str(j, "shell");
    if (j.contains("settlements")) {
        if (!j["settlements"].is_array()) { err = "settlements must be an array"; return false; }
        for (auto& s : j["settlements"]) {
            RaidSettlement x;
            x.region = str(s, "region");
            x.owner = str(s, "owner");
            if (x.region.empty()) { err = "settlement without region"; return false; }
            x.destroyed = numbers(s.value("destroyed", json::object()));
            const json c = s.value("collected", json::object());
            x.collected = numbers(c);
            if (c.is_object() && c.contains("items")) x.items = ids(c["items"]);
            const json k = s.value("killed", json::object());
            x.soldiers = num(k, "soldiers"); x.garrison = num(k, "garrison"); x.civilians = num(k, "civilians");
            if (k.is_object() && k.contains("persons")) x.persons = ids(k["persons"]);
            if (s.contains("taken") && s["taken"].is_array())
                for (auto& t : s["taken"]) x.taken.push_back(t.is_string() ? t.get<std::string>() : t.is_object() ? str(t, "id") : "");
            const json pr = s.value("presence", json::object());
            x.lowest_altitude = num(pr, "lowest_altitude"); x.seconds_below_150 = num(pr, "seconds_below_150");
            x.roared = flag(pr, "roared"); x.landed_in_square = flag(pr, "landed_in_square");
            out.settlements.push_back(x);
        }
    }
    if (j.contains("armies") && j["armies"].is_array())
        for (auto& a : j["armies"]) {
            RaidArmy x;
            x.attacker = str(a, "attacker"); x.defender = str(a, "defender"); x.region = str(a, "region");
            x.killed_share = num(a, "killed_share"); x.repelled = flag(a, "repelled");
            out.armies.push_back(x);
        }
    if (j.contains("lair") && j["lair"].is_object()) {
        const json& l = j["lair"];
        if (l.contains("dropped") && l["dropped"].is_object()) {
            out.lair.has_dropped = true;
            out.lair.dropped = numbers(l["dropped"]);
            if (l["dropped"].contains("items")) out.lair.dropped_items = ids(l["dropped"]["items"]);
        }
        if (l.contains("received") && l["received"].is_array())
            for (auto& r : l["received"]) out.lair.received.push_back({str(r, "id"), str(r, "outcome")});
        if (l.contains("fought")) out.lair.fought = ids(l["fought"]);
    }
    if (j.contains("wounds")) out.wounds = numbers(j["wounds"]);
    for (const char* k : {"spells", "speech"}) {
        if (!j.contains(k)) continue;
        std::vector<Act>& dst = std::string(k) == "spells" ? out.spells : out.speech;
        if (!j[k].is_array() || !acts_from_json(j[k].dump(), dst, err)) { if (err.empty()) err = std::string(k) + " must be an array of acts"; return false; }
    }
    return true;
}

std::string report_to_json(const RaidReport& r) {
    json j = {{"sortie", r.sortie}, {"settlements", json::array()}, {"armies", json::array()}};
    if (!r.shell.empty()) j["shell"] = r.shell;
    for (auto& x : r.settlements) {
        json c = json(x.collected);
        c["items"] = strings(x.items);
        json s = {{"region", x.region}, {"owner", x.owner}, {"destroyed", x.destroyed}, {"collected", c},
                  {"killed", {{"soldiers", x.soldiers}, {"garrison", x.garrison}, {"civilians", x.civilians}, {"persons", strings(x.persons)}}},
                  {"taken", strings(x.taken)},
                  {"presence", {{"lowest_altitude", x.lowest_altitude}, {"seconds_below_150", x.seconds_below_150}, {"roared", x.roared}, {"landed_in_square", x.landed_in_square}}}};
        j["settlements"].push_back(s);
    }
    for (auto& a : r.armies)
        j["armies"].push_back({{"attacker", a.attacker}, {"defender", a.defender}, {"region", a.region}, {"killed_share", a.killed_share}, {"repelled", a.repelled}});
    json lair = {{"received", json::array()}, {"fought", strings(r.lair.fought)}};
    if (r.lair.has_dropped) { lair["dropped"] = json(r.lair.dropped); lair["dropped"]["items"] = strings(r.lair.dropped_items); }
    for (auto& x : r.lair.received) lair["received"].push_back({{"id", x.id}, {"outcome", x.outcome}});
    j["lair"] = lair;
    j["wounds"] = r.wounds;
    j["spells"] = json::parse(acts_to_json(r.spells), nullptr, false);
    j["speech"] = json::parse(acts_to_json(r.speech), nullptr, false);
    return j.dump();
}

std::string sortie_to_json(const SortieState& s) {
    json j = {{"number", s.number}, {"year", s.year}, {"settlements", json::array()}, {"courts", json::array()}, {"armies", json::array()}};
    for (auto& x : s.settlements)
        j["settlements"].push_back({{"region", x.region}, {"owner", x.owner}, {"value", x.value}, {"population", x.population},
                                    {"garrison", x.garrison}, {"soldiers", x.soldiers}, {"structures", x.structures}});
    for (auto& c : s.courts) j["courts"].push_back({{"id", c.id}, {"wealth", c.wealth}, {"holds", c.holds}});
    for (auto& a : s.armies) j["armies"].push_back({{"attacker", a.attacker}, {"defender", a.defender}, {"region", a.region}, {"size", a.size}});
    return j.dump();
}

bool sortie_from_json(const std::string& text, SortieState& out) {
    json j = json::parse(text, nullptr, false);
    if (j.is_discarded() || !j.is_object()) return false;
    out = SortieState();
    out.open = true;
    out.number = j.value("number", 0);
    out.year = j.value("year", 0);
    for (auto& x : j.value("settlements", json::array())) {
        SortieSettlement s;
        s.region = str(x, "region"); s.owner = str(x, "owner");
        s.value = num(x, "value"); s.population = num(x, "population"); s.garrison = num(x, "garrison"); s.soldiers = num(x, "soldiers");
        s.structures = numbers(x.value("structures", json::object()));
        out.settlements.push_back(s);
    }
    for (auto& x : j.value("courts", json::array())) {
        SortieCourt c;
        c.id = str(x, "id"); c.wealth = num(x, "wealth"); c.holds = numbers(x.value("holds", json::object()));
        out.courts.push_back(c);
    }
    for (auto& x : j.value("armies", json::array())) {
        SortieArmy a;
        a.attacker = str(x, "attacker"); a.defender = str(x, "defender"); a.region = str(x, "region"); a.size = num(x, "size");
        out.armies.push_back(a);
    }
    return true;
}

std::string sortie_view_json(const Params& p, const SortieState& s) {
    json j = json::parse(sortie_to_json(s), nullptr, false);
    j["civilian_toll_factor"] = p.raid_diff.civilian_toll_factor;
    j["structure_threshold"] = p.raid_diff.structure_threshold;
    return j.dump();
}

}
