#include "record.h"
#include "ages.h"
#include "arrivals.h"
#include "endings.h"
#include "json.h"
#include "state_hash.h"
#include "talents.h"

#ifndef HOARD_BUILD
#define HOARD_BUILD "dev"
#endif

namespace ages {

using nlohmann::json;
uint64_t json_hash(const json& j);   // state_hash.cpp

RecordScope::RecordScope(World& w) : w_(w), year_(w.year) {
    if (!w.recorder || w.recorder->world != &w) return;
    counted_ = true;
    outer_ = w.recorder->depth == 0;
    w.recorder->depth += 1;
}

RecordScope::~RecordScope() {
    if (counted_) w_.recorder->depth -= 1;
}

void RecordScope::done(const char* verb, const std::string& args_json) {
    if (!outer_) return;
    Recorder& r = *w_.recorder;
    json line = {{"i", r.next++}, {"y", year_}, {"t", verb}, {"a", json::parse(args_json, nullptr, false)}};
    r.lines.push_back(line.dump());
    if (std::string(verb) == "sleep")
        r.lines.push_back(json({{"chk", w_.year}, {"h", hash_hex(state_hash(w_).root)}}).dump());
}

std::string args_of(std::initializer_list<std::pair<const char*, std::string>> strings) {
    json j = json::object();
    for (auto& kv : strings) j[kv.first] = kv.second;
    return j.dump();
}

std::string args_int(const char* key, long long v) { return json({{key, v}}).dump(); }

std::string args_acts(const std::vector<Act>& acts) {
    json j = json::object();
    j["acts"] = json::parse(acts_to_json(acts), nullptr, false);
    return j.dump();
}

uint64_t data_hash(const DataJson& d) {
    json all = json::object();
    auto add = [&](const char* k, const std::string& text) {
        json v = json::parse(text.empty() ? "null" : text, nullptr, false);
        all[k] = v.is_discarded() ? json(text) : v;
    };
    add("constants", d.constants); add("races", d.races); add("reactions", d.reactions); add("regions", d.regions);
    add("factions", d.factions); add("shocks", d.shocks); add("names", d.names); add("offers", d.offers); add("text", d.text);
    add("events", d.events); add("talents", d.talents); add("defences", d.defences); add("hooks", d.hooks); add("arrivals", d.arrivals); add("moves", d.moves); add("genesis", d.genesis);
    add("importance", d.importance);
    if (!d.patterns.empty()) { add("patterns", d.patterns); add("base_rates", d.base_rates); }   // item 14
    if (!d.endings.empty()) add("endings", d.endings);   // item 04
    for (auto& b : d.text_bundles) { json v = json::parse(b.second, nullptr, false); all["text_bundles"][b.first] = v; }
    return json_hash(all);
}

std::string build_id() { return HOARD_BUILD; }

std::string replay_header(const World& w, const Params& p, bool genesis) {
    json h = {{"hoard_replay", kReplayFormat}, {"rules", kRulesVersion}, {"build", build_id()}, {"data", hash_hex(p.data_hash)},
              {"seed", w.seed}, {"genesis", genesis}, {"year", w.year}, {"start", hash_hex(state_hash(w).root)}};
    return h.dump();
}

namespace {

std::string str(const json& a, const char* k) { return a.contains(k) && a[k].is_string() ? a[k].get<std::string>() : ""; }

}

bool apply_input(World& w, const Params& p, const std::string& verb, const std::string& args_json, std::string& err) {
    json a = json::parse(args_json, nullptr, false);
    if (a.is_discarded()) { err = "bad args"; return false; }
    if (verb == "apply_acts") {
        std::vector<Act> acts;
        if (!acts_from_json(a.value("acts", json::array()).dump(), acts, err)) return false;
        apply_acts(w, p, acts);
        return true;
    }
    if (verb == "sleep") { sleep(w, p, a.value("years", 1)); return true; }
    if (verb == "pick_talent") return pick_talent(w, p, str(a, "id"), err);
    if (verb == "accept_offer") return accept_offer(w, p, a.value("index", size_t{0}));
    if (verb == "decline_offer") return decline_offer(w, p, a.value("index", size_t{0}));
    if (verb == "answer_hook") return answer_hook(w, p, str(a, "id"), str(a, "choice"), err);
    if (verb == "kill_person") return kill_person(w, p, str(a, "id"), err);
    if (verb == "receive") return receive(w, p, str(a, "id"), str(a, "outcome"), err);
    if (verb == "hunt") return hunt(w, p, str(a, "region"), err);
    if (verb == "exit") return exit_run(w, p, str(a, "kind"), err);   // item 04
    if (verb == "cast") return cast_spell(w, p, str(a, "spell"), str(a, "target"), str(a, "region"), str(a, "against")).cast;
    if (verb == "terraform") return terraform(w, p, str(a, "region"), str(a, "kind")).done;
    if (verb == "demand") { demand_oath(w, p, str(a, "target"), str(a, "pred")); return true; }
    if (verb == "load_world") {
        World n;
        if (!world_from_json(a.value("world", json::object()).dump(), n, err)) return false;
        n.recorder = w.recorder;
        w = n;
        return true;
    }
    err = "unknown verb " + verb;
    return false;
}

ReplayResult replay_lines(World& w, const Params& p, const std::vector<std::string>& lines) {
    ReplayResult r;
    for (auto& text : lines) {
        if (text.empty()) continue;
        json line = json::parse(text, nullptr, false);
        if (line.is_discarded()) { r.error = "bad line"; return r; }
        if (line.contains("chk")) {
            r.checkpoints += 1;
            std::string want = line.value("h", ""), got = hash_hex(state_hash(w).root);
            if (want != got || line.value("chk", -1) != w.year) {
                r.first_bad_year = line.value("chk", -1);
                r.error = "checkpoint " + std::to_string(r.checkpoints) + " at year " + std::to_string(r.first_bad_year) + ": recorded " + want + ", replayed " + got;
                return r;
            }
            continue;
        }
        if (!line.contains("t")) continue;   // header or end line
        std::string err;
        if (!apply_input(w, p, line.value("t", ""), line.value("a", json::object()).dump(), err)) {
            r.error = "input " + std::to_string(line.value("i", -1)) + " (" + line.value("t", "") + ") refused on replay: " + err;
            return r;
        }
    }
    r.ok = true;
    return r;
}

}
