#include <cstdlib>
#include <string>
#include "ages/ages.h"
#include "ages/dragon_json.h"
#include "ages/hook_json.h"
#include "ages/genesis.h"
#include "ages/importance.h"
#include "ages/json.h"
#include "ages/state_hash.h"
#include "ages/record.h"
#include "ages/save.h"
#include "ages/talents.h"
#include "ages/world_util.h"
#include "ages/wounds.h"
#include "text_api.h"

using namespace ages;
using nlohmann::json;

// the wasm build has no files: policies arrive as text (`golden`), so a policy that cycles through others cannot load them
bool ages::load_policy(const std::string&, const std::string& name, Policy&, std::string& err) {
    err = "no policy files in the wasm build: " + name;
    return false;
}

namespace {

struct Session {
    bool loaded = false;
    DataJson data;
    Params params;
    World world;
    Recorder recorder;     // every input since the last load or reset, for "Report a problem"
    std::string header;
};

Session g;
std::string g_out;

json fail(const std::string& msg) { return {{"ok", false}, {"error", msg}}; }
json fail_reason(const std::string& reason) { return {{"ok", false}, {"reason", reason}, {"error", text_reason(reason)}}; }

void start_recording(bool genesis) {
    g.recorder = Recorder();
    g.recorder.world = &g.world;
    g.world.recorder = &g.recorder;
    g.header = replay_header(g.world, g.params, genesis);
}

json fear_faith_snapshot(const World& w) {
    json j = json::object();
    for (auto& f : w.factions) j[f.id] = {{"fear", f.fear}, {"faith", f.faith}, {"wealth", f.wealth}};
    return j;
}

std::string text_of(const json& j, const char* key) {
    auto it = j.find(key);
    if (it == j.end()) return "";
    return it->is_string() ? it->get<std::string>() : it->dump();
}

json cmd_load(const json& in) {
    const json& d = in.value("data", json::object());
    g.data.constants = text_of(d, "constants");
    g.data.races = text_of(d, "races");
    g.data.reactions = text_of(d, "reactions");
    g.data.regions = text_of(d, "regions");
    g.data.factions = text_of(d, "factions");
    g.data.shocks = text_of(d, "shocks");
    g.data.names = text_of(d, "names");
    g.data.offers = text_of(d, "offers");
    g.data.text = text_of(d, "text");
    g.data.events = text_of(d, "events");
    g.data.talents = text_of(d, "talents");     // v0.4 (S1)
    g.data.defences = text_of(d, "defences");
    g.data.hooks = text_of(d, "hooks");         // v0.4 hooks (S3)
    g.data.genesis = text_of(d, "genesis");   // v0.4 world (S2)
    g.data.importance = text_of(d, "importance");
    g.data.text_bundles.clear();
    if (d.contains("text_bundles") && d["text_bundles"].is_object())
        for (auto it = d["text_bundles"].begin(); it != d["text_bundles"].end(); ++it)
            g.data.text_bundles[it.key()] = it->is_string() ? it->get<std::string>() : it->dump();
    LoadResult r = load_data_from_json(g.data, in.value("seed", uint64_t{1}));
    if (!r.ok) return fail(r.error);
    if (!g.data.text_bundles.empty() && !text_load(g.data.text_bundles, r.error)) return fail(r.error);
    text_set_lang(in.value("lang", std::string("en")));
    if (in.value("genesis", false) && !make_genesis(r.world, r.params, r.error)) return fail(r.error);   // v0.4 world (S2)
    g.params = r.params;
    g.world = r.world;
    g.loaded = true;
    start_recording(in.value("genesis", false));
    return {{"ok", true}};
}

json cmd_reset(const json& in) {
    if (!g.loaded) return fail("not loaded");
    LoadResult r = load_data_from_json(g.data, in.value("seed", g.world.seed));
    if (!r.ok) return fail(r.error);
    if (in.value("genesis", false) && !make_genesis(r.world, r.params, r.error)) return fail(r.error);   // v0.4 world (S2)
    g.world = r.world;
    start_recording(in.value("genesis", false));
    return {{"ok", true}};
}

// scores the returned copies only: the world changes through the API verbs alone, so a replay sees the same state
json events_since(size_t first) {
    std::vector<Event> events(g.world.chronicle.begin() + static_cast<std::ptrdiff_t>(first), g.world.chronicle.end());
    for (auto& e : events) if (e.importance < 0) e.importance = event_importance(g.world, g.params, e);
    return json::parse(chronicle_to_json(events), nullptr, false);
}

json cmd_apply_acts(const json& in) {
    std::vector<Act> acts;
    std::string err;
    if (!acts_from_json(in.value("acts", json::array()).dump(), acts, err)) return fail(err);
    json before = fear_faith_snapshot(g.world);
    size_t first_event = g.world.chronicle.size();
    json lines = json::parse(reaction_lines_to_json(apply_acts(g.world, g.params, acts)), nullptr, false);
    json after = fear_faith_snapshot(g.world);
    json changed = json::array();
    for (auto& a : acts) {
        std::vector<std::string> touched = a.witnesses;
        touched.push_back(a.target);
        for (auto& id : touched) {
            if (!before.contains(id)) continue;
            bool seen = false;
            for (auto& c : changed) if (c["id"] == id) seen = true;
            if (!seen) changed.push_back({{"id", id}, {"before", before[id]}, {"after", after[id]}});
        }
    }
    return {{"ok", true}, {"changed", changed}, {"events", events_since(first_event)}, {"lines", lines}};
}

json cmd_offer(const json& in, bool accept) {
    size_t index = in.value("index", size_t{0});
    size_t first_event = g.world.chronicle.size();
    bool ok = accept ? accept_offer(g.world, g.params, index) : decline_offer(g.world, g.params, index);
    return ok ? json{{"ok", true}, {"events", events_since(first_event)}} : fail("no such offer");
}

json cmd_sleep(const json& in) {
    int years = in.value("years", 100);
    if (years < 1) return fail("years must be at least 1");
    int since = g.world.year;
    SleepResult r = sleep(g.world, g.params, years);
    json j = json::parse(sleep_result_to_json(r, g.world, g.params), nullptr, false);
    j["briefing"] = json::parse(briefing_json(g.world, g.params, since), nullptr, false);
    return j;
}

json cmd_briefing(const json& in) {
    int since = in.value("since", g.world.sleep_started);
    return json::parse(briefing_json(g.world, g.params, since), nullptr, false);
}

json cmd_kill_person(const json& in) {
    std::string id = in.value("id", "");
    if (id.empty()) return fail("id required");
    size_t first_event = g.world.chronicle.size();
    std::string reason;
    std::vector<ReactionLine> lines;
    if (!kill_person(g.world, g.params, id, reason, &lines)) return fail_reason(reason);
    return {{"ok", true}, {"events", events_since(first_event)}, {"lines", json::parse(reaction_lines_to_json(lines), nullptr, false)}};
}

json cmd_forecast(const json& in) {
    int years = in.value("years", 100);
    if (years < 1) return fail("years must be at least 1");
    json j = json::parse(forecast_to_json(forecast(g.world, g.params, years)), nullptr, false);
    j["ok"] = true;
    return j;
}

json cmd_sponsor_remnant(const json& in) {
    Act a;
    a.type = "spell";
    a.spell = "sponsor_remnant";
    a.race = in.value("race", "");
    a.region = in.value("region", "");
    if (a.race.empty()) return fail("race required");
    size_t first_event = g.world.chronicle.size();
    apply_acts(g.world, g.params, {a});
    return {{"ok", true}, {"events", events_since(first_event)}};
}

json cmd_cast(const json& in) {
    std::string spell = in.value("spell", "");
    if (spell.empty()) return fail("spell required");
    size_t first_event = g.world.chronicle.size();
    CastResult r = cast_spell(g.world, g.params, spell, in.value("target", ""), in.value("region", ""), in.value("against", ""));
    json j = json::parse(cast_result_to_json(r), nullptr, false);
    j["events"] = events_since(first_event);
    return j;
}

json cmd_terraform(const json& in) {
    std::string region = in.value("region", ""), kind = in.value("kind", "");
    if (region.empty() || kind.empty()) return fail("region and kind required");
    size_t first_event = g.world.chronicle.size();
    TerraformResult r = terraform(g.world, g.params, region, kind);
    json j = json::parse(terraform_result_to_json(r), nullptr, false);
    j["events"] = events_since(first_event);
    return j;
}

json cmd_demand(const json& in) {
    std::string target = in.value("target", ""), pred = in.value("pred", "tribute");
    if (target.empty()) return fail("target required");
    size_t first_event = g.world.chronicle.size();
    DemandResult r = demand_oath(g.world, g.params, target, pred);
    json j = json::parse(demand_result_to_json(r), nullptr, false);
    j["events"] = events_since(first_event);
    return j;
}

json cmd_load_world(const json& in) {
    if (!g.loaded) return fail("not loaded");
    World w;
    std::string err;
    if (!world_from_json(in.value("world", json::object()).dump(), w, err)) return fail(err);
    RecordScope rec(g.world);
    g.world = w;
    g.world.recorder = &g.recorder;
    rec.done("load_world", json({{"world", in.value("world", json::object())}}).dump());
    return {{"ok", true}};
}

// ---- v0.4 (S1): wounds in apply_acts, a slain dragon refuses its verbs, talents -----------------------------------

const char* const kVerbs[] = {"apply_acts", "sleep", "kill_person", "sponsor_remnant", "cast", "terraform", "demand",
                              "accept_offer", "decline_offer", "pick_talent"};

json cmd_apply_acts_wounded(const json& in) {
    double before = g.world.dragons[kPlayer].wounds;
    json out = cmd_apply_acts(in);
    if (!out.value("ok", false)) return out;
    out["wounds"] = {{"taken", g.world.dragons[kPlayer].wounds - before}, {"total", g.world.dragons[kPlayer].wounds}, {"max", health_max(g.world, g.params)}};
    out["slain"] = dragon_dead(g.world);
    return out;
}

json cmd_pick_talent(const json& in) {
    std::string id = in.value("id", ""), reason;
    size_t first_event = g.world.chronicle.size();
    if (!pick_talent(g.world, g.params, id, reason)) return fail_reason(reason);
    return {{"ok", true}, {"events", events_since(first_event)}};
}

bool dragon_dispatch(const std::string& cmd, const json& in, json& out) {
    if (dragon_dead(g.world))
        for (const char* v : kVerbs) if (cmd == v) { out = fail("dragon_slain"); return true; }
    if (cmd == "apply_acts") out = cmd_apply_acts_wounded(in);
    else if (cmd == "talents") out = talents_catalogue_json(g.params);
    else if (cmd == "pick_talent") out = cmd_pick_talent(in);
    else return false;
    return true;
}

// ---- v0.4 hooks (S3): hooks_resolved on apply_acts and kill_person, answer_hook ---------------------------------------

json hooks_resolved_json(const std::vector<Hook>& before) {
    json arr = json::array();
    for (auto& h : before) {
        bool open = false;
        for (auto& x : g.world.hooks) if (x.id == h.id) open = true;
        if (!open) arr.push_back({{"id", h.id}, {"kind", h.kind}, {"reward_text", h.reward_text}});
    }
    return arr;
}

json cmd_apply_acts_hooked(const json& in) {
    if (dragon_dead(g.world)) return fail("dragon_slain");
    std::vector<Hook> before = g.world.hooks;
    json out = cmd_apply_acts_wounded(in);
    if (out.value("ok", false)) out["hooks_resolved"] = hooks_resolved_json(before);
    return out;
}

json cmd_answer_hook(const json& in) {
    if (dragon_dead(g.world)) return fail("dragon_slain");
    std::string id = in.value("id", ""), choice = in.value("choice", ""), reason;
    std::vector<Hook> before = g.world.hooks;
    size_t first_event = g.world.chronicle.size();
    double wounds = g.world.dragons[kPlayer].wounds;
    if (!answer_hook(g.world, g.params, id, choice, reason)) return fail_reason(reason);
    return {{"ok", true}, {"events", events_since(first_event)}, {"hooks_resolved", hooks_resolved_json(before)},
            {"wounds", {{"taken", g.world.dragons[kPlayer].wounds - wounds}, {"total", g.world.dragons[kPlayer].wounds}, {"max", health_max(g.world, g.params)}}},
            {"slain", dragon_dead(g.world)}};
}

json cmd_kill_person_hooked(const json& in) {
    if (dragon_dead(g.world)) return fail("dragon_slain");
    std::vector<Hook> before = g.world.hooks;
    json out = cmd_kill_person(in);
    if (out.value("ok", false)) out["hooks_resolved"] = hooks_resolved_json(before);
    return out;
}

bool hooks_dispatch(const std::string& cmd, const json& in, json& out) {
    if (cmd == "apply_acts") out = cmd_apply_acts_hooked(in);
    else if (cmd == "kill_person") out = cmd_kill_person_hooked(in);
    else if (cmd == "answer_hook") out = cmd_answer_hook(in);
    else return false;
    return true;
}

// the state-hash golden of the native runner (`hoard-sim --golden`): a generated world, a policy, 3000 years
json cmd_golden(const json& in) {
    LoadResult r = load_data_from_json(g.data, in.value("seed", uint64_t{1}));
    if (!r.ok) return fail(r.error);
    if (!make_genesis(r.world, r.params, r.error)) return fail(r.error);
    Policy policy;
    std::string err;
    if (!policy_from_json(text_of(in, "policy"), policy, err)) return fail(err);
    Scenario s;
    s.policy = &policy;
    s.years = 3000;
    s.sleep = 100;
    run_scenario(r.world, r.params, s);
    return {{"ok", true}, {"hash", hash_hex(state_hash(r.world).root)}};
}

json dispatch(const json& in) {
    std::string cmd = in.value("cmd", "");
    if (cmd == "load") return cmd_load(in);
    if (!g.loaded) return fail("not loaded");
    if (json out; hooks_dispatch(cmd, in, out)) return out;    // v0.4 hooks (S3)
    if (json out; dragon_dispatch(cmd, in, out)) return out;   // v0.4 (S1)
    if (json out; text_dispatch(cmd, in, out)) return out;     // item 19
    if (cmd == "reset") return cmd_reset(in);
    if (cmd == "world") return json::parse(world_view_json(g.world, g.params), nullptr, false);
    if (cmd == "apply_acts") return cmd_apply_acts(in);
    if (cmd == "accept_offer") return cmd_offer(in, true);
    if (cmd == "decline_offer") return cmd_offer(in, false);
    if (cmd == "sleep") return cmd_sleep(in);
    if (cmd == "briefing") return cmd_briefing(in);
    if (cmd == "people") return json::parse(people_json(g.world, g.params), nullptr, false);
    if (cmd == "kill_person") return cmd_kill_person(in);
    if (cmd == "forecast") return cmd_forecast(in);
    if (cmd == "sponsor_remnant") return cmd_sponsor_remnant(in);
    if (cmd == "cast") return cmd_cast(in);
    if (cmd == "terraform") return cmd_terraform(in);
    if (cmd == "demand") return cmd_demand(in);
    if (cmd == "catalogue") { json j = json::parse(spells_catalogue_json(g.params), nullptr, false); j["ok"] = true; return j; }
    if (cmd == "hash") return {{"ok", true}, {"hash", chronicle_hash_hex(g.world)}};
    if (cmd == "state_hash") return json::parse(state_hash_json(g.world), nullptr, false);
    if (cmd == "golden") return cmd_golden(in);
    if (cmd == "report") {   // "Report a problem": the replay of this session, the current save, the build
        std::string hrp = g.header + "\n";
        for (auto& l : g.recorder.lines) hrp += l + "\n";
        hrp += json({{"end", 1}, {"year", g.world.year}, {"h", hash_hex(state_hash(g.world).root)}}).dump() + "\n";
        return {{"ok", true}, {"hrp", hrp}, {"save", save_json(g.world, g.params)}, {"build", build_id()}};
    }
    if (cmd == "load_world") return cmd_load_world(in);
    // v0.4 world (S2)
    if (cmd == "legends") return json::parse(legends_json(g.world, g.params), nullptr, false);
    return fail("unknown cmd " + cmd);
}

}

extern "C" {

const char* ages_cmd(const char* json_in) {
    json in = json::parse(json_in ? json_in : "", nullptr, false);
    json out = in.is_object() ? dispatch(in) : fail("input must be a json object");
    text_word_reason(out);
    g_out = out.dump();
    return g_out.c_str();
}

void* ages_alloc(size_t n) { return std::malloc(n); }
void ages_free(void* p) { std::free(p); }

}
