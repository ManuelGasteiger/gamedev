#include <algorithm>
#include "ages/act_gates.h"
#include "ages/act_log.h"
#include "ages/names.h"
#include "ages/world_util.h"
#include "bind.h"
#include "scn.h"
#include "text/render.h"
#include "text_ctx.h"

namespace scn {

using namespace ages;
using nlohmann::json;

namespace {

json ref_of(const World& w, const Def& d, const std::map<std::string, std::string>& roles, const std::string& v, bool& ok) {
    if (v == "$self") return {{"kind", "scenario"}, {"id", d.id}};
    std::string name = v.substr(1, v.find('.') == std::string::npos ? std::string::npos : v.find('.') - 1);
    auto it = roles.find(name);
    const Role* r = d.role(name);
    if (it == roles.end() || !r) { ok = false; return nullptr; }
    std::string kind = r->kind, id = it->second;
    if (kind == "act") { kind = "faction"; id = owner_of(w, id, "act"); }
    return {{"kind", kind}, {"id", id}};
}

json subst(const World& w, const Def& d, const std::map<std::string, std::string>& roles, const json& v, const std::string& key, bool& ok) {
    if (v.is_string()) {
        const std::string& s = v.get_ref<const std::string&>();
        if (s.empty() || s[0] != '$') return v;
        json r = ref_of(w, d, roles, s, ok);
        if (!ok) return nullptr;
        if (key == "who" || key == "about" || key == "item" || key == "region") return r;
        return r["id"];
    }
    if (v.is_object()) { json o = json::object(); for (auto it = v.begin(); it != v.end(); ++it) o[it.key()] = subst(w, d, roles, it.value(), it.key(), ok); return o; }
    if (v.is_array()) { json a = json::array(); for (auto& x : v) a.push_back(subst(w, d, roles, x, key, ok)); return a; }
    return v;
}

}

bool resolve_option(const World& w, const Def& d, const Option& o, const std::map<std::string, std::string>& roles, double magnitude, json& act) {
    bool ok = true;
    const json& t = o.act;
    act = json::object();
    act["type"] = t["type"];
    if (t.contains("target")) act["target"] = ref_of(w, d, roles, t["target"].get<std::string>(), ok);
    else if (t["type"] == "receive") act["target"] = {{"kind", "lair"}, {"id", "lair"}};
    if (t.contains("secondary")) act["secondary"] = ref_of(w, d, roles, t["secondary"].get<std::string>(), ok);
    if (t.contains("structure")) act["secondary"] = {{"kind", "structure"}, {"id", t["structure"]}};
    if (t.contains("magnitude")) act["magnitude"] = t["magnitude"].is_string() ? (magnitude >= 0 ? magnitude : 0.5) : t["magnitude"].get<double>();
    else if (magnitude >= 0) act["magnitude"] = magnitude;
    if (t.contains("payload")) act["payload"] = subst(w, d, roles, t["payload"], "", ok);
    const std::string type = t["type"].get<std::string>();
    if (ok && act.contains("target") && act["target"].is_object() && act["target"].value("kind", std::string()) == "faction") {
        std::string court = act["target"].value("id", std::string());
        if (type == "swear")
            for (auto& o : w.offers) if (o.from_faction == court) { act["target"] = {{"kind", "offer"}, {"id", o.id}}; break; }
        if (type == "bless" || type == "ward")
            if (const Faction* f = find_faction(w, court)) if (!f->regions.empty()) act["region"] = f->regions.front();
    }
    if (ok && type == "receive" && act.contains("payload") && act["payload"].contains("who") && act["payload"]["who"].is_object()) {
        std::string person = act["payload"]["who"].value("id", std::string());
        act["payload"].erase("who");
        for (auto& a : w.arrivals) if (a.person == person) act["payload"]["who"] = a.id;
    }
    act["source"] = "scenario";
    act["answers"] = d.id;
    return ok;
}

json briefing_block(const World& w, const Params& p, const std::string& lang) {
    json out = json::array();
    if (!w.scn.on || p.scenarios.defs.empty()) return out;
    int W = current_waking(w);
    Cache cache = make_cache(w, p);
    for (auto& s : w.scn.shown) {
        if (s.waking != W) continue;
        const Def* d = p.scenarios.find(s.scenario);
        if (!d) continue;
        Ctx x{w, p, *d, cache, s.roles};
        PredEnv env = env_of(x);
        text::Ctx tc = text_ctx(x, lang);
        std::string pre = "scn." + d->id + ".";
        auto render = [&](const std::string& key) {
            std::vector<std::string> problems;
            return p.sift.texts ? text::render_key(*p.sift.texts, lang, key, tc, &problems) : key;
        };
        std::string headline = pre + "headline", body = pre + "body";
        for (auto& v : d->variants) {
            if (!eval(v.pred, env)) continue;
            body = pre + "variant." + v.id + ".body";
            if (v.headline) headline = pre + "variant." + v.id + ".headline";
            break;
        }
        json j = {{"id", d->id}, {"slot", s.slot}, {"kind", d->kind}, {"stakes", d->stakes}, {"headline", render(headline)}, {"body", render(body)},
                  {"roles", s.roles}, {"cites", s.cites}, {"answered", s.answered.empty() ? json(nullptr) : json(s.answered)}};
        double level = field(x, "world", "chronicle_level").num;
        if (d->has_because && level >= p.scenarios.n.because_level) j["because"] = render(pre + "because");
        json options = json::array(), dropped = json::array();
        for (auto& o : d->options) {
            json act;
            if (!resolve_option(w, *d, o, s.roles, -1, act)) { dropped.push_back({{"id", o.id}, {"reason", "scn.unbound"}}); continue; }
            std::vector<Act> acts;
            std::string err;
            acts_from_json(json::array({act}).dump(), acts, err);
            Act a = acts.empty() ? Act() : acts.front();
            normalise_act(w, a);
            std::string shape = act_fields_refusal(w, p, a);
            if (!shape.empty()) { dropped.push_back({{"id", o.id}, {"reason", shape}}); continue; }
            json oj = {{"id", o.id}, {"label", render(pre + "option." + o.id)}, {"act", act}};
            std::string gate;
            if (!o.gate.empty() && !eval(o.gate, env)) gate = "scn.gate|gate=" + o.gate.text;
            if (gate.empty()) gate = act_gate_refusal(w, p, a);
            oj["open"] = gate.empty();
            if (!gate.empty()) oj["gate"] = gate;
            oj["defences_strike"] = inside_ring(w, p, a);
            options.push_back(oj);
        }
        j["options"] = options;
        if (!dropped.empty()) j["dropped"] = dropped;
        out.push_back(j);
    }
    return out;
}

}
