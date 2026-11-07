#include "scn_cmd.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include "ages/genesis.h"
#include "ages/json.h"
#include "ages/state_hash.h"
#include "ages/world_util.h"
#include "scenarios/bind.h"
#include "scenarios/scn.h"
#include "scenarios/text_ctx.h"
#include "text/bundle.h"
#include "text/render.h"

namespace ages {

using nlohmann::json;

namespace {

bool read_json(const std::string& path, json& out) {
    std::ifstream in(path);
    if (!in) return false;
    std::stringstream ss;
    ss << in.rdbuf();
    out = json::parse(ss.str(), nullptr, false);
    return !out.is_discarded();
}

// the bot's answers to one shown scenario, best first: the open options whose act type the bot prefers (in its order),
// then ignore, then the other open options; "random" shuffles the open options on a keyed stream
std::vector<std::string> bot_choices(const World& w, const json& sc, const json& prefs, const std::string& bot) {
    std::vector<std::string> open, out;
    std::map<std::string, std::string> type;
    for (auto& o : sc["options"]) {   // a bot never sleeps or leaves from inside a waking: the run loop owns both
        std::string t = o["act"].value("type", std::string());
        if (o.value("open", false) && t != "sleep" && t != "exit") { open.push_back(o["id"].get<std::string>()); type[open.back()] = t; }
    }
    if (prefs.is_string() && prefs.get<std::string>() == "random") {
        Rng g = rng_for(w.seed, "scn.bot." + bot, sc.value("id", std::string()) + ":" + std::to_string(w.year));
        for (size_t i = open.size(); i > 1; --i) std::swap(open[i - 1], open[static_cast<size_t>(g.range(0, static_cast<int>(i)))]);
        return open;
    }
    auto add = [&](const std::string& id) { if (std::find(out.begin(), out.end(), id) == out.end()) out.push_back(id); };
    if (prefs.is_array()) for (auto& t : prefs) for (auto& id : open) if (type[id] == t.get<std::string>()) add(id);
    for (auto& id : open) if (type[id] == "ignore") add(id);
    for (auto& id : open) add(id);
    return out;
}

}

void log_waking(const World& w, ScnRun& run) {
    if (!run.log || !w.scn.on) return;
    int W = scn::current_waking(w);
    if (W == run.logged) return;
    run.logged = W;
    json shown = json::array();
    for (auto& s : w.scn.shown) {
        if (s.waking != W) continue;
        json cites = json::array();
        for (int a : s.cites) {
            const Act& act = w.acts[static_cast<size_t>(a)];
            std::string src = act.source.empty() ? "raid" : act.source;
            cites.push_back({{"act", a}, {"type", act.type}, {"source", src}});
        }
        shown.push_back({{"id", s.scenario}, {"slot", s.slot}, {"cites", cites}, {"answered", s.answered}});
    }
    json line = {{"waking", W}, {"year", w.year}, {"era", w.scn.diag_era}, {"eligible", w.scn.diag_eligible}, {"depth", w.scn.diag_depth}, {"acted", w.scn.diag_acted}, {"shown", shown}};
    if (!run.refused.empty()) { line["refused"] = run.refused; run.refused.clear(); }
    (*run.log) << line.dump() << "\n";
}

bool attach_scenarios(Scenario& s, const Params& p, const std::string& data, ScnRun& run, std::string& err) {
    json prefs;
    if (!run.bot.empty()) {
        json all;
        if (!read_json(data + "/bots/scenario_answers.json", all) || !all.contains("bots") || !all["bots"].contains(run.bot)) { err = "no scenario bot " + run.bot + " in data/bots/scenario_answers.json"; return false; }
        prefs = all["bots"][run.bot];
    }
    auto previous = s.at_waking;
    std::string bot = run.bot;
    std::vector<std::string>* refused = &run.refused;
    s.at_waking = [previous, prefs, bot, refused, &p](World& w, std::vector<Act>& acts, int index) {
        if (previous) previous(w, acts, index);
        if (bot.empty() || !w.scn.on) return;
        json block = scn::briefing_block(w, p);
        for (auto& sc : block) {
            if (!sc["answered"].is_null()) continue;
            for (auto& choice : bot_choices(w, sc, prefs, bot)) {
                scn::Answer a = scn::answer_scenario(w, p, sc["id"].get<std::string>(), choice);
                if (a.ok) break;
                if (refused) refused->push_back(sc["id"].get<std::string>() + "." + choice + ": " + a.reason);
            }
        }
    };
    auto before = s.before_sleep;
    s.before_sleep = [before, &run, &p](const World& w, int index) {
        if (before) before(w, index);
        if (!run.why.empty() && run.log) {   // why the named scenarios bind or not, before this waking's sleep
            scn::Cache cache = scn::make_cache(w, p);
            json why = json::object();
            for (auto& id : run.why) {
                const scn::Def* d = p.scenarios.find(id);
                if (!d) continue;
                scn::Ctx x{w, p, *d, cache, {}};
                why[id] = scn::bind_roles(x, [](int) { return true; }, cache.waking) ? "binds" : scn::bind_why(x);
            }
            (*run.log) << json({{"why", why}, {"year", w.year}}).dump() << "\n";
        }
        log_waking(w, run);
    };
    return true;
}

// tools/scenario_check.py step 4: {"base": {seed, years, policy, genesis}, "cases": [{"scenario", ...lab spec}]}; one
// JSON line per case
int scn_lab_cmd(const std::string& data, const std::string& spec_path) {
    json spec;
    if (!read_json(spec_path, spec)) { std::fprintf(stderr, "cannot read %s\n", spec_path.c_str()); return 1; }
    const json& base = spec.value("base", json::object());
    LoadResult loaded = load_data(data, base.value("seed", uint64_t{7}));
    if (!loaded.ok) { std::fprintf(stderr, "load failed: %s\n", loaded.error.c_str()); return 1; }
    const Params& p = loaded.params;
    std::string err;
    if (base.value("genesis", true) && !make_genesis(loaded.world, p, err)) { std::fprintf(stderr, "genesis failed: %s\n", err.c_str()); return 1; }
    Policy policy;
    if (!load_policy(data, base.value("policy", std::string("played")), policy, err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
    Scenario run;
    run.policy = &policy;
    run.data_dir = data;
    run.years = base.value("years", 1200);
    run_scenario(loaded.world, p, run);
    const World base_world = loaded.world;
    for (auto& c : spec.value("cases", json::array())) {
        std::string id = c.value("scenario", std::string());
        json out = {{"scenario", id}};
        const scn::Def* d = p.scenarios.find(id);
        if (!d) { out["error"] = "unknown scenario"; std::printf("%s\n", out.dump().c_str()); continue; }
        World w = base_world;
        w.recorder = nullptr;
        if (!scn::lab_world(w, p, c, err)) { out["error"] = err; std::printf("%s\n", out.dump().c_str()); continue; }
        scn::enable(w);
        scn::Cache cache = scn::make_cache(w, p);
        std::string era = p.scenarios.n.eras[static_cast<size_t>(std::clamp(cache.strongest_era, 0, static_cast<int>(p.scenarios.n.eras.size()) - 1))];
        out["era"] = era;
        out["era_ok"] = d->eras.empty() || contains(d->eras, era);
        scn::Ctx x{w, p, *d, cache, {}};
        bool bound = scn::bind_roles(x, [](int) { return true; }, cache.waking);
        out["bound"] = bound;
        if (!bound) { out["why"] = scn::bind_why(x); std::printf("%s\n", out.dump().c_str()); continue; }
        out["roles"] = x.bind;
        json gates = json::array();
        if (d->has_arrival) {
            const Faction* f = find_faction(w, x.id(d->arrival_from));
            for (auto& g : d->needs) if (!f || !scn::gate_open(w, p, p.scenarios, g, *f)) gates.push_back(g);
        }
        out["closed_gates"] = gates;
        ScnShown s;
        s.scenario = id;
        s.slot = "lab";
        s.waking = cache.waking;
        s.year = w.year;
        s.roles = x.bind;
        w.scn.shown.push_back(s);
        // every string of the scenario whose roles are bound, in every language
        json problems = json::array();
        int renders = 0;
        if (p.sift.texts) {
            std::string pre = "scn." + id + ".";
            for (auto& lang : {"en", "de"}) {
                const text::Bundle* b = p.sift.texts->get(lang);
                if (!b) continue;
                text::Ctx tc = scn::text_ctx(x, lang);
                const json& table = text::part(*b, "scenarios");
                for (auto it = table.begin(); it != table.end(); ++it) {
                    if (it.key().rfind(pre, 0) != 0) continue;
                    std::string s = it.value().get<std::string>();
                    bool skip = false;
                    for (auto& r : d->roles) if (r.optional && !x.bound(r.name) && s.find("{" + r.name + ".") != std::string::npos) skip = true;
                    if (skip) continue;
                    std::vector<std::string> pr;
                    std::string t = text::render(*b, s, tc, &pr);
                    renders += 1;
                    if (t.find('{') != std::string::npos) pr.push_back("unbound token");
                    for (auto& m : pr) problems.push_back(std::string(lang) + " " + it.key() + ": " + m);
                }
            }
        }
        out["renders"] = renders;
        out["problems"] = problems;
        std::string before = hash_hex(state_hash(w).root);
        json options = json::array();
        for (auto& o : d->options) {
            World wc = w;
            scn::Answer a = scn::answer_unrecorded(wc, p, id, o.id, o.act.contains("magnitude") && o.act["magnitude"].is_string() ? 0.5 : -1);
            json oj = {{"id", o.id}, {"type", o.act.value("type", std::string())}, {"ok", a.ok}, {"changed", hash_hex(state_hash(wc).root) != before}};
            if (!a.ok) oj["reason"] = a.reason;
            options.push_back(oj);
        }
        out["options"] = options;
        std::printf("%s\n", out.dump().c_str());
    }
    return 0;
}

}
