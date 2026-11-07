#include "ages/act_log.h"
#include "ages/move_data.h"
#include "ages/person_record.h"
#include "ages/unrecorded.h"
#include "ages/world_util.h"
#include "bind.h"
#include "scn.h"
#include "text/render.h"
#include "text_ctx.h"

namespace scn {

using namespace ages;
using nlohmann::json;

namespace {

Answer refuse(const std::string& reason) { Answer a; a.reason = reason; return a; }

std::string role_id(const ScnShown& s, const std::string& ref) {
    if (ref.size() < 2 || ref[0] != '$') return "";
    auto it = s.roles.find(ref.substr(1));
    return it == s.roles.end() ? "" : it->second;
}

}

// spec section 2 `outcomes` and the answer: the option's act with `answers` and `source: scenario` through apply_acts,
// then suppress, history (a memory line on the person, item 07), seek and reveal_to_player
Answer answer_unrecorded(World& w, const Params& p, const std::string& id, const std::string& option, double magnitude) {
    int W = current_waking(w);
    ScnShown* s = nullptr;
    for (auto& x : w.scn.shown) if (x.scenario == id && x.waking == W) s = &x;
    if (!s) return refuse("scn.not_shown|id=" + id);
    if (!s->answered.empty()) return refuse("scn.answered|id=" + id);
    const Def* d = p.scenarios.find(id);
    if (!d) return refuse("scn.unknown|id=" + id);
    const Option* o = nullptr;
    for (auto& x : d->options) if (x.id == option) o = &x;
    if (!o) return refuse("scn.no_option|option=" + option);
    Cache cache = make_cache(w, p);
    Ctx x{w, p, *d, cache, s->roles};
    if (!o->gate.empty() && !eval(o->gate, env_of(x))) return refuse("scn.gate|gate=" + o->gate.text);
    json act;
    if (!resolve_option(w, *d, *o, s->roles, magnitude, act)) return refuse("scn.unbound|option=" + option);
    std::vector<Act> acts;
    std::string err;
    if (!acts_from_json(json::array({act}).dump(), acts, err) || acts.empty()) return refuse("act.malformed|field=act");
    std::vector<ActRefusal> refused;
    size_t before = w.acts.size();
    Answer a;
    a.lines = apply_acts_unrecorded(w, p, acts, &refused);
    if (!refused.empty()) { a.reason = refused.front().reason; return a; }
    a.ok = true;
    a.act = w.acts.size() > before ? static_cast<int>(w.acts.size()) - 1 : -1;
    for (auto& y : w.scn.shown) if (y.scenario == id && y.waking == W) { y.answered = option; s = &y; }
    if (!d->group.empty()) w.scn.closed.insert(d->group);
    text::Ctx tc = text_ctx(x, "en");
    auto render = [&](const std::string& key) { return p.sift.texts ? text::render_key(*p.sift.texts, "en", key, tc) : key; };
    std::string pre = "scn." + d->id + ".";
    if (p.sift.texts && p.sift.texts->get("en") && text::find_string(*p.sift.texts->get("en"), pre + "result." + option)) a.texts.push_back(render(pre + "result." + option));
    if (d->outcomes.contains(option))
        for (auto& out : d->outcomes[option]) {
            if (out.contains("suppress")) w.scn.suppress[out["suppress"].value("tag", std::string())] = w.year + out["suppress"].value("years", 0);
            if (out.contains("seek")) for (auto& t : out["seek"]) w.scn.seek[t.get<std::string>()] = W + 1;
            if (out.contains("reveal_to_player")) a.texts.push_back(render(pre + "reveal." + option));
            if (out.contains("history")) {
                Person* who = find_person(w, role_id(*s, out["history"].value("person", std::string())));
                if (!who) continue;
                MemoryRow row;
                row.ev = out["history"].value("line", std::string());
                row.sev = out["history"].value("severity", 1);
                remember_dragon(p, *who, memory_entry(w, p, *who, row, "witness", a.act));
            }
        }
    return a;
}

json answer_json(const Answer& a) {
    json lines = json::array();
    for (auto& l : a.lines) lines.push_back({{"faction", l.faction}, {"text", l.text}});
    json j = {{"ok", a.ok}, {"act", a.act}, {"lines", lines}, {"texts", a.texts}};
    if (!a.reason.empty()) j["reason"] = a.reason;
    return j;
}

}
