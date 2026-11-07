#include "bank.h"
#include <algorithm>
#include <functional>
#include "ages/data.h"
#include "predicate.h"

namespace ages { uint64_t json_hash(const nlohmann::json& j); }   // state_hash.cpp

namespace scn {

using nlohmann::json;

const Def* Bank::find(const std::string& id) const {
    auto it = std::lower_bound(defs.begin(), defs.end(), id, [](const Def& d, const std::string& k) { return d.id < k; });
    return it != defs.end() && it->id == id ? &*it : nullptr;
}

namespace {

const char* const kKinds[] = {"act", "faction", "region", "person", "item", "remnant"};

struct Vocab {
    std::set<std::string> themes, tones, endings, races, gates, choosers, nouns;
    std::map<std::string, std::set<std::string>> fields;
};

std::set<std::string> set_of(const json& j, const char* k) {
    std::set<std::string> s;
    if (j.contains(k)) for (auto& v : j[k]) s.insert(v.get<std::string>());
    return s;
}

void refs_in(const json& v, std::vector<std::string>& out) {
    if (v.is_string()) {
        const std::string& s = v.get_ref<const std::string&>();
        for (size_t at = s.find('$'); at != std::string::npos; at = s.find('$', at + 1)) {
            size_t e = at + 1;
            while (e < s.size() && ((s[e] >= 'a' && s[e] <= 'z') || (s[e] >= '0' && s[e] <= '9') || s[e] == '_')) ++e;
            if (e > at + 1) out.push_back(s.substr(at + 1, e - at - 1));
        }
    } else if (v.is_object() || v.is_array()) for (auto& x : v) refs_in(x, out);
}

struct Loader {
    const Vocab& vocab;
    Def& d;
    std::string& err;
    bool fail(const std::string& m) { err = "scenario " + d.id + ": " + m; return false; }

    bool known_role(const std::string& r) { return r == "self" || r == "slider" || d.role(r) || r == "lair" || r == "dragon" || r == "world"; }

    bool pred(const json& j, Pred& out) {
        std::string e;
        if (!j.is_string() || !parse_pred(j.get<std::string>(), out, e)) return fail(e.empty() ? "predicate is not a string" : e);
        for (auto& c : out.clauses) {
            if (c.role == "dragon" || c.role == "lair" || c.role == "world") {
                if (c.kind == Clause::cmp && !vocab.fields.at(c.role).count(c.field)) return fail("field " + c.role + "." + c.field + " not in vocab");
                continue;
            }
            const Role* r = d.role(c.role);
            if (!r) return fail("predicate names unknown role " + c.role);
            if (c.kind == Clause::cmp && !vocab.fields.at(r->kind).count(c.field)) return fail("field " + c.role + "." + c.field + " not in vocab for " + r->kind);
        }
        return true;
    }

    bool roles(const json& j) {
        std::vector<Role> all;
        for (auto it = j.begin(); it != j.end(); ++it) {
            Role r;
            r.name = it.key();
            const json& v = it.value();
            for (const char* k : kKinds) if (v.contains(k)) { r.kind = k; r.spec = v[k]; }
            if (r.kind.empty()) return fail("role " + r.name + " has no binder");
            r.optional = v.value("optional", false);
            r.noun = v.value("noun", std::string());
            r.sex = v.value("sex", std::string("any"));
            if (r.kind == "person" && !vocab.nouns.count(r.noun)) return fail("role " + r.name + ": noun " + r.noun + " not in vocab");
            if (r.spec.contains("choose") && !vocab.choosers.count(r.spec["choose"].get<std::string>())) return fail("role " + r.name + ": chooser not in vocab");
            refs_in(r.spec, r.deps);
            all.push_back(r);
        }
        std::set<std::string> names;
        for (auto& r : all) names.insert(r.name);
        for (auto& r : all)
            for (auto& dep : r.deps) if (!names.count(dep) && dep != "self") return fail("role " + r.name + " names unknown role $" + dep);
        std::set<std::string> placed;
        while (placed.size() < all.size()) {   // dependencies first; ties in name order (json objects are sorted)
            bool moved = false;
            for (auto& r : all) {
                if (placed.count(r.name)) continue;
                bool ready = true;
                for (auto& dep : r.deps) if (dep != r.name && !placed.count(dep)) ready = false;
                if (!ready) continue;
                d.roles.push_back(r);
                placed.insert(r.name);
                moved = true;
                break;
            }
            if (!moved) return fail("roles depend on each other in a cycle");
        }
        return true;
    }

    bool tag(const std::string& t) {
        size_t c = t.find(':');
        std::string ns = t.substr(0, c), v = c == std::string::npos ? "" : t.substr(c + 1);
        if (ns == "theme" && !vocab.themes.count(v)) return fail("tag " + t + " not in vocab");
        if (ns == "tone" && !vocab.tones.count(v)) return fail("tag " + t + " not in vocab");
        if (ns == "ending" && !vocab.endings.count(v)) return fail("tag " + t + " not in vocab");
        if (ns == "race" && !vocab.races.count(v)) return fail("tag " + t + " not in vocab");
        if (ns == "faction" && (v.size() < 2 || !d.role(v.substr(1)))) return fail("tag " + t + " names no role");
        return true;
    }

    bool load(const json& j) {
        d.id = j.value("id", std::string());
        d.kind = j.value("kind", std::string());
        d.stakes = j.value("stakes", std::string("minor"));
        d.once = j.value("once", std::string("run"));
        if (j.contains("eras") && j["eras"].is_array()) d.eras = j["eras"].get<std::vector<std::string>>();
        d.weight = j.value("weight", 100);
        if (j.contains("cooldown") && j["cooldown"].is_object()) { d.cd_wakings = j["cooldown"].value("wakings", 0); d.cd_years = j["cooldown"].value("years", 0); }
        if (j.contains("exclusive_group") && j["exclusive_group"].is_string()) d.group = j["exclusive_group"].get<std::string>();
        if (!roles(j.value("roles", json::object()))) return false;
        if (j.contains("arrival")) {
            const json& a = j["arrival"];
            d.has_arrival = true;
            d.arrival_who = a.value("who", std::string());
            d.arrival_from = a.value("from", std::string());
            if (d.arrival_from.size() < 2 || !d.role(d.arrival_from.substr(1))) return fail("arrival.from names no role");
            d.arrival_from = d.arrival_from.substr(1);
            for (auto& g : a.value("needs", json::array())) {
                if (!vocab.gates.count(g.get<std::string>())) return fail("arrival gate " + g.get<std::string>() + " not in vocab");
                d.needs.push_back(g.get<std::string>());
            }
        }
        for (auto& p : j.value("require", json::array())) { Pred x; if (!pred(p, x)) return false; d.require.push_back(x); }
        for (auto& f : j.value("forbid", json::array())) {
            Forbid x;
            if (f.is_string()) { if (!pred(f, x.pred)) return false; }
            else { x.is_act = true; x.act = f.value("act", json::object()); std::vector<std::string> r; refs_in(x.act, r); for (auto& n : r) if (!known_role(n)) return fail("forbid names unknown role $" + n); }
            d.forbid.push_back(x);
        }
        for (auto& s : j.value("salience", json::array())) { Salience x; if (!pred(s["if"], x.pred)) return false; x.w = s.value("w", 1); d.salience.push_back(x); }
        for (auto& t : j.value("tags", json::array())) { if (!tag(t.get<std::string>())) return false; d.tags.push_back(t.get<std::string>()); }
        const json& text = j.value("text", json::object());
        d.has_because = text.contains("because");
        for (auto& v : text.value("variants", json::array())) {
            Variant x;
            x.id = v.value("id", std::string());
            x.headline = v.contains("headline");
            if (!pred(v["if"], x.pred)) return false;
            d.variants.push_back(x);
        }
        for (auto& o : j.value("options", json::array())) {
            Option x;
            x.id = o.value("id", std::string());
            x.act = o.value("act", json::object());
            if (o.contains("gate") && !pred(o["gate"], x.gate)) return false;
            std::vector<std::string> r;
            refs_in(x.act, r);
            for (auto& n : r) if (!known_role(n)) return fail("option " + x.id + " names unknown role $" + n);
            d.options.push_back(x);
        }
        d.outcomes = j.value("outcomes", json::object());
        for (auto it = d.outcomes.begin(); it != d.outcomes.end(); ++it) {
            bool found = false;
            for (auto& o : d.options) found = found || o.id == it.key();
            if (!found) return fail("outcome for unknown option " + it.key());
            std::vector<std::string> r;
            refs_in(it.value(), r);
            for (auto& n : r) if (!known_role(n)) return fail("outcome names unknown role $" + n);
        }
        if (j.contains("followup")) {
            for (auto& t : j["followup"].value("seek", json::array())) d.seek.push_back(t.get<std::string>());
            d.seek_after = j["followup"].value("after_wakings", 1);
        }
        return true;
    }
};

void numbers(const json& c, Numbers& n) {
    if (!c.is_object()) return;
    auto g = [&](const char* k, auto& v) { if (c.contains(k)) v = c[k].get<std::decay_t<decltype(v)>>(); };
    g("specificity_w", n.spec_w); g("cite_w", n.cite_w); g("recent_w", n.recent_w); g("recent_half_wakings", n.recent_half);
    g("theme_w", n.theme_w); g("seen_w", n.seen_w); g("theme_share", n.theme_share); g("theme_share_w", n.theme_share_w);
    g("window", n.window); g("cite_half_wakings", n.cite_half); g("seek_w", n.seek_w); g("arrival_pity_w", n.arrival_pity_w);
    g("pity_magnitude", n.pity_magnitude); g("seen_cap", n.seen_cap); g("theme_wakings", n.theme_wakings); g("pity_wakings", n.pity_wakings);
    g("arrival_pity_after", n.arrival_pity_after); g("cites_per_run", n.cites_per_run); g("cites_per_waking", n.cites_per_waking);
    g("cites_monumental", n.cites_monumental); g("candidates_per_role", n.candidates_per_role); g("because_level", n.because_level);
    g("renown_events", n.renown_events); g("remember_stock", n.remember_stock); g("eras", n.eras);
    if (c.contains("sizes")) for (auto& s : c["sizes"]) n.sizes.push_back({s[0].get<double>(), s[1].get<std::string>()});
}

}

bool bank_from_json(const ages::DataJson& dj, Bank& out, std::string& err) {
    out = Bank();
    json c = json::parse(dj.constants, nullptr, false);
    if (!c.is_discarded() && c.contains("scenarios")) numbers(c["scenarios"], out.n);
    if (dj.scenarios.empty()) return true;
    json vj = json::parse(dj.scenario_vocab.empty() ? "{}" : dj.scenario_vocab, nullptr, false);
    json all = json::parse(dj.scenarios, nullptr, false);
    if (vj.is_discarded()) { err = "invalid json in data/scenarios/vocab.json"; return false; }
    if (all.is_discarded()) { err = "invalid json in data/scenarios"; return false; }
    Vocab v;
    v.themes = set_of(vj, "themes"); v.tones = set_of(vj, "tones"); v.endings = set_of(vj, "endings"); v.races = set_of(vj, "races");
    v.gates = set_of(vj, "gates"); v.choosers = set_of(vj, "choosers"); v.nouns = set_of(vj, "nouns");
    for (const char* k : {"faction", "region", "person", "item", "remnant", "act", "dragon", "lair", "world"})
        v.fields[k] = vj.contains("fields") && vj["fields"].contains(k) ? set_of(vj["fields"], k) : std::set<std::string>();
    for (auto& g : v.gates) {
        if (!vj.contains("gate_rows") || !vj["gate_rows"].contains(g)) { err = "vocab.json gate " + g + " has no gate_rows entry"; return false; }
        for (auto& row : vj["gate_rows"][g]) out.gates[g].push_back(row);
    }
    out.themes = v.themes;
    for (auto& file : all)
        for (auto& sj : file) {
            Def d;
            Loader l{v, d, err};
            if (!l.load(sj)) return false;
            if (out.find(d.id)) { err = "duplicate scenario id " + d.id; return false; }
            out.defs.push_back(d);
            std::sort(out.defs.begin(), out.defs.end(), [](const Def& a, const Def& b) { return a.id < b.id; });
        }
    out.hash = ages::json_hash(all);
    return true;
}

}
