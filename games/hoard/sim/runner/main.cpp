#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "ages/ages.h"
#include "ages/genesis.h"
#include "ages/state_hash.h"
#include "ages/record.h"
#include "ages/save.h"
#include "analyse.h"
#include "ages/json.h"
#include "ages/world_util.h"
#include "ages/wounds.h"
#include "ages/endings.h"
#include "ages/profile.h"
#include "bot.h"
#include "text_cmd.h"

using namespace ages;
using nlohmann::json;

namespace {

struct Options {
    std::string data = "data";
    std::string out = "out";
    std::string acts;
    std::string policy;
    uint64_t seed = 1;
    int years = 3000;
    int sleep = 100;
    bool briefing = false;
    bool narrate = false;
    bool genesis = false;   // v0.4 world (S2)
    bool no_bank = false;   // item 14: run without the pattern bank (the goldens run so)
    std::string trace;      // one state-hash line per sleep
    std::string record;     // write every input and a checkpoint per sleep as a .hrp
    std::string replay;     // replay a .hrp and verify its checkpoints (exit 3 on the first mismatch)
    std::string save;       // write the final world as a save (temp file, rename, one .bak)
    std::string load_save;  // start from a save instead of a fresh world
    std::string snapshots;  // item 01: core stocks per sleep (JSON lines)
    std::string wakings;    // item 01: per waking: listed wounds, hooks, buckets (JSON lines)
    bool buckets = false;
    int bucket_samples = 2;
    std::string perturb;    // burn | sleep
    bool state_hash = false;
    uint64_t golden_from = 0, golden_to = 0;   // --golden A-B: played, generated world, print `seed hash` per seed
    std::string render_batch, text_bundle;     // item 19, text_cmd.h
    std::string profile;    // item 04: the profile folder (read before genesis, written at the end)
    std::string bot;        // item 04: amnesiac | carry | oracle:<ending>
};

bool parse(int argc, char** argv, Options& o) {
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto need = [&](std::string& dst) { if (i + 1 >= argc) return false; dst = argv[++i]; return true; };
        std::string v;
        if (k == "--data") { if (!need(o.data)) return false; }
        else if (k == "--out") { if (!need(o.out)) return false; }
        else if (k == "--acts") { if (!need(o.acts)) return false; }
        else if (k == "--policy") { if (!need(o.policy)) return false; }
        else if (k == "--seed") { if (!need(v)) return false; o.seed = std::strtoull(v.c_str(), nullptr, 10); }
        else if (k == "--years") { if (!need(v)) return false; o.years = std::atoi(v.c_str()); }
        else if (k == "--sleep") { if (!need(v)) return false; o.sleep = std::atoi(v.c_str()); }
        else if (k == "--briefing") o.briefing = true;
        else if (k == "--narrate") o.narrate = true;
        else if (k == "--genesis") o.genesis = true;
        else if (k == "--no-bank") o.no_bank = true;
        else if (k == "--trace") { if (!need(o.trace)) return false; }
        else if (k == "--state-hash") o.state_hash = true;
        else if (k == "--render-batch") { if (!need(o.render_batch)) return false; }
        else if (k == "--text-bundle") { if (!need(o.text_bundle)) return false; }
        else if (k == "--record") { if (!need(o.record)) return false; }
        else if (k == "--replay") { if (!need(o.replay)) return false; }
        else if (k == "--save") { if (!need(o.save)) return false; }
        else if (k == "--load-save") { if (!need(o.load_save)) return false; }
        else if (k == "--snapshots") { if (!need(o.snapshots)) return false; }
        else if (k == "--wakings") { if (!need(o.wakings)) return false; }
        else if (k == "--buckets") o.buckets = true;
        else if (k == "--bucket-samples") { if (!need(v)) return false; o.bucket_samples = std::max(1, std::atoi(v.c_str())); }
        else if (k == "--perturb") { if (!need(o.perturb)) return false; }
        else if (k == "--profile") { if (!need(o.profile)) return false; }
        else if (k == "--bot") { if (!need(o.bot)) return false; }
        else if (k == "--golden") {
            if (!need(v)) return false;
            size_t dash = v.find('-');
            o.golden_from = std::strtoull(v.c_str(), nullptr, 10);
            o.golden_to = dash == std::string::npos ? o.golden_from : std::strtoull(v.c_str() + dash + 1, nullptr, 10);
        }
        else { std::fprintf(stderr, "unknown argument %s\n", k.c_str()); return false; }
    }
    return o.years > 0 && o.sleep > 0;
}

bool write_file(const std::string& path, const std::string& text) {
    std::ofstream out(path);
    if (!out) return false;
    out << text;
    return true;
}

bool write_save(const std::string& path, const std::string& text) {
    std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary);
        if (!out) return false;
        out << text;
        out.flush();
        if (!out) return false;
    }
    std::remove((path + ".bak").c_str());
    std::rename(path.c_str(), (path + ".bak").c_str());
    return std::rename(tmp.c_str(), path.c_str()) == 0;
}

json record_json(const FactionRecord& r, bool alive) {
    json j;
    j["id"] = r.id;
    j["race"] = r.race;
    j["born"] = r.born;
    if (alive) j["died"] = nullptr; else j["died"] = r.died;
    j["lifetime"] = r.died - r.born;
    j["max_regions"] = r.max_regions;
    j["max_realm"] = r.max_realm;
    j["eras"] = r.era_years;
    return j;
}

// v0.4 (S1): how the dragon died (wounds from the defences while raiding, or a slaying while asleep) and whether the
// briefing before the fatal sleep named the slayers
json death_json(const World& w) {
    size_t begin = 0, previous = 0;
    for (size_t i = 0; i < w.chronicle.size(); ++i) {
        const Event& e = w.chronicle[i];
        if (e.kind == "sleep_begin") { previous = begin; begin = i; }
        if (e.kind != "dragon_slain") continue;
        bool wounds = e.cause.text_key == "defences";
        bool warned = wounds;
        for (size_t k = previous; k < begin && !warned; ++k) {
            const Event& x = w.chronicle[k];
            warned = x.subject == e.subject && (x.kind == "slayers_ready" || x.kind == "slayers_coming" || x.kind == "slayers_forming");
        }
        return {{"cause", wounds ? "wounds" : "slaying"}, {"year", e.year}, {"court", e.subject}, {"warned", warned}};
    }
    return nullptr;
}

// v0.4 hooks (S3): hooks per waking (resolved and ignored events share the waking's year; the last waking's still open),
// answered share, fights, growth points the rewards paid per axis
json hooks_json(const World& w, const Params& p) {
    std::map<int, int> per_wake, answered_per_wake;
    std::map<std::string, int> by_kind, answered_by_kind;
    std::map<std::string, double> reward_growth;
    int wakings = 0, fights = 0;
    double fight_wounds = 0;
    for (auto& e : w.chronicle) {
        if (e.kind == "wake" && e.object != "dragon_slain") { wakings += 1; per_wake[e.year] += 0; }
        if (e.kind != "hook_resolved" && e.kind != "hook_ignored") continue;
        std::string kind = e.tags.empty() ? "" : *e.tags.begin();
        per_wake[e.year] += 1;
        by_kind[kind] += 1;
        if (e.kind != "hook_resolved") continue;
        answered_per_wake[e.year] += 1;
        answered_by_kind[kind] += 1;
        for (auto& kv : e.cause.params) {
            if (kv.first == "fight") { if (kv.second > 0) { fights += 1; fight_wounds += kv.second; } }
            else reward_growth[kv.first] += kv.second;
        }
    }
    for (auto& h : w.hooks) { per_wake[h.offered] += 1; by_kind[h.kind] += 1; }
    int with_hook = 0, offered = 0, answered = 0;
    for (auto& kv : per_wake) { if (kv.second > 0) with_hook += 1; offered += kv.second; }
    for (auto& kv : answered_per_wake) answered += kv.second;
    return {{"wakings", wakings}, {"with_hook", with_hook}, {"offered", offered}, {"answered", answered}, {"by_kind", by_kind},
            {"answered_by_kind", answered_by_kind}, {"fights", fights}, {"fight_wounds", fight_wounds}, {"reward_growth", reward_growth},
            {"health_max", health_max(w, p)}};
}

// item 04: per ending its conditions (ever met, longest hold, best count) and whether it was ever ready; the prophecy
// cards that fired (first year after the hatching); the Myth at a death; the exit taken
json endings_stats_json(const World& w, const Params& p) {
    json endings = json::object();
    for (auto& e : p.endings.endings) {
        if (!e.enabled) continue;
        const EndingState* s = ending_state(w, e.id);
        json conds = json::object();
        for (size_t i = 0; i < e.conds.size(); ++i) {
            CondHold h = s && i < s->conds.size() ? s->conds[i] : CondHold();
            conds[e.conds[i].id] = {{"ever_met", h.ever_met}, {"best_years", h.best_years}, {"best_value", h.best_value}};
        }
        int first_ready = -1;
        for (auto& ev : w.chronicle) if (ev.kind == "ending_ready" && ev.subject == e.id) { first_ready = ev.year; break; }
        endings[e.id] = {{"ever_ready", first_ready >= 0}, {"first_ready", first_ready}, {"conditions", conds}};
    }
    json cards = json::object();
    for (auto& ev : w.chronicle)
        if (ev.kind.rfind("prophecy_", 0) == 0 && ev.kind != "prophecy_fulfilled" && !cards.contains(ev.kind)) cards[ev.kind] = ev.year - w.genesis.hatched_year;
    return {{"endings", endings}, {"prophecy_cards", cards}, {"prophecy_known_by", w.prophecy.known_by.size()}, {"myth", myth_qualifies(w, p)},
            {"ended", w.ended}, {"closest", dragon_dead(w) ? json(closest_conditions(w, p)) : json::array()}};
}

json stats_json(const World& w, const Params& p) {
    json factions = json::array();
    std::vector<FactionRecord> all = w.graveyard;
    for (auto& g : w.graveyard) factions.push_back(record_json(g, false));
    for (auto& f : w.factions) {
        FactionRecord r = record_of(f, w.year);
        all.push_back(r);
        factions.push_back(record_json(r, true));
    }

    std::map<std::string, std::vector<int>> era_timeline;
    for (auto& r : all) {
        auto& best = era_timeline[r.race];
        for (size_t e = 1; e < r.era_years.size(); ++e) {
            if (best.size() < e) best.resize(e, -1);
            if (best[e - 1] < 0 || r.era_years[e] < best[e - 1]) best[e - 1] = r.era_years[e];
        }
    }
    json timeline;
    for (auto& kv : era_timeline) timeline[kv.first] = kv.second;

    int orc_regions = 0, owned = 0;
    double wealth = 0;
    std::map<std::string, int> race_regions;
    for (auto& r : w.regions) {
        if (r.owner.empty()) continue;
        owned += 1;
        const Faction* f = find_faction(w, r.owner);
        if (!f) continue;
        race_regions[dominant_race(*f)] += 1;
        if (f->race == "orc") orc_regions += 1;
    }
    for (auto& f : w.factions) wealth += f.wealth;
    json alive = json::object(), remnants = json::object();
    for (auto& kv : p.races) alive[kv.first] = false;
    for (auto& f : w.factions) alive[dominant_race(f)] = true;
    for (auto& r : w.remnants) remnants[r.race] = r.strength;
    int hegemonies = 0, hunger_wakes = 0;
    for (auto& e : w.chronicle) {
        if (e.kind == "hegemony_ended") hegemonies += 1;
        if (e.kind == "wake" && e.object == "hunger") hunger_wakes += 1;
    }

    json j;
    j["seed"] = w.seed;
    j["year"] = w.year;
    j["factions"] = factions;
    j["era_timeline"] = timeline;
    j["orc_region_share"] = w.regions.empty() ? 0.0 : static_cast<double>(orc_regions) / static_cast<double>(w.regions.size());
    j["empty_valley"] = owned == 0;
    j["total_wealth"] = wealth;
    j["regions_owned"] = owned;
    j["dragon"] = {{"size", w.dragons[kPlayer].size}, {"knowledge", w.dragons[kPlayer].knowledge}, {"magic", w.dragons[kPlayer].magic},
                   {"threat", threat(w.dragons[kPlayer], p.c)}};
    j["dragon_slain"] = has_tag(w.tags, "dragon_slain");
    j["race_regions"] = race_regions;
    j["alive"] = alive;
    j["remnants"] = remnants;
    j["hegemonies_fallen"] = hegemonies;
    j["hunger_wakes"] = hunger_wakes;
    j["dragon"]["hunger"] = w.dragons[kPlayer].hunger;
    j["dragon_death"] = death_json(w);   // v0.4 (S1)
    j["dragon_growth"] = {{"phase", w.dragons[kPlayer].phase}, {"points", w.dragons[kPlayer].growth}, {"talents", w.dragons[kPlayer].talents}, {"wounds", w.dragons[kPlayer].wounds}};
    j["hooks"] = hooks_json(w, p);   // v0.4 hooks (S3)
    j["genesis"] = {{"template", w.genesis.template_id}, {"prehistory_years", w.genesis.prehistory_years}, {"deal", w.genesis.deal}};   // v0.4 world (S2)
    j["endings"] = endings_stats_json(w, p);   // item 04
    return j;
}

}

// the state-hash golden: every seed on a generated world, the played policy, 3000 years in its own sleep lengths
int golden(const Options& o) {
    for (uint64_t seed = o.golden_from; seed <= o.golden_to; ++seed) {
        LoadResult loaded = load_data(o.data, seed);
        if (!loaded.ok) { std::fprintf(stderr, "load failed: %s\n", loaded.error.c_str()); return 1; }
        std::string err;
        loaded.params.sift.patterns.clear();   // item 14: the golden suite runs with the bank off
        if (!make_genesis(loaded.world, loaded.params, err)) { std::fprintf(stderr, "genesis failed: %s\n", err.c_str()); return 1; }
        Policy policy;
        if (!load_policy(o.data, "played", policy, err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
        Scenario s;
        s.policy = &policy;
        s.data_dir = o.data;
        s.years = 3000;
        s.sleep = 100;
        run_scenario(loaded.world, loaded.params, s);
        std::printf("%llu %s\n", static_cast<unsigned long long>(seed), hash_hex(state_hash(loaded.world).root).c_str());
        std::fflush(stdout);
    }
    return 0;
}

std::string out_dir(const std::string& out) {
    std::string mkdir = "mkdir -p \"" + out + "\"";
#ifdef _WIN32
    mkdir = "mkdir \"" + out + "\" 2>nul";
#endif
    if (std::system(mkdir.c_str()) != 0) std::fprintf(stderr, "warning: mkdir failed for %s\n", out.c_str());
    return out.empty() || out.back() == '/' ? out : out + "/";
}

bool write_core(const std::string& base, const World& w, const Params& p) {
    return write_file(base + "chronicle.json", chronicle_to_json(w.chronicle)) &&
           write_file(base + "world_final.json", world_to_json(w)) &&
           write_file(base + "stats.json", stats_json(w, p).dump(1));
}

int replay(const Options& o) {
    std::ifstream in(o.replay);
    if (!in) { std::fprintf(stderr, "cannot open %s\n", o.replay.c_str()); return 1; }
    std::vector<std::string> lines;
    for (std::string l; std::getline(in, l);) if (!l.empty()) lines.push_back(l);
    if (lines.empty()) { std::fprintf(stderr, "empty replay\n"); return 1; }
    json h = json::parse(lines.front(), nullptr, false);
    if (h.is_discarded() || h.value("hoard_replay", 0) != kReplayFormat) { std::fprintf(stderr, "not a hoard replay (format %d)\n", kReplayFormat); return 1; }
    if (h.value("rules", 0) != kRulesVersion) std::fprintf(stderr, "warning: recorded under rules %d, this build has %d\n", h.value("rules", 0), kRulesVersion);
    LoadResult loaded = load_data(o.data, h.value("seed", uint64_t{1}));
    if (!loaded.ok) { std::fprintf(stderr, "load failed: %s\n", loaded.error.c_str()); return 1; }
    if (hash_hex(loaded.params.data_hash) != h.value("data", "")) std::fprintf(stderr, "warning: data differs from the recording (%s, recorded %s)\n", hash_hex(loaded.params.data_hash).c_str(), h.value("data", "").c_str());
    std::string err;
    Profile woven;   // item 04: the legends genesis wove when it was recorded
    if (h.contains("legends")) woven.legends = legends_from_json(h["legends"]);
    if (h.value("genesis", false) && !make_genesis(loaded.world, loaded.params, woven, err)) { std::fprintf(stderr, "genesis failed: %s\n", err.c_str()); return 1; }
    if (hash_hex(state_hash(loaded.world).root) != h.value("start", "")) { std::printf("start state differs from the recording\n"); return 3; }
    std::vector<std::string> body(lines.begin() + 1, lines.end());
    ReplayResult r = replay_lines(loaded.world, loaded.params, body);
    if (!r.ok) { std::printf("replay failed: %s\n", r.error.c_str()); return r.first_bad_year >= 0 ? 3 : 1; }
    json end = json::parse(lines.back(), nullptr, false);
    if (end.contains("end") && end.value("h", "") != hash_hex(state_hash(loaded.world).root)) { std::printf("final state differs from the recording\n"); return 3; }
    std::printf("replay ok: %d checkpoints, year %d\n", r.checkpoints, loaded.world.year);
    if (!o.out.empty() && !write_core(out_dir(o.out), loaded.world, loaded.params)) { std::fprintf(stderr, "cannot write to %s\n", o.out.c_str()); return 1; }
    return 0;
}

int main(int argc, char** argv) {
    Options o;
    if (argc >= 3 && std::string(argv[1]) == "--golden") o.years = 3000;
    if (!parse(argc, argv, o)) {
        std::fprintf(stderr, "usage: hoard-sim --data data/ --seed N --years Y [--acts log.json] [--policy name] [--sleep 100] [--briefing] [--narrate] [--genesis] [--no-bank] [--trace t.jsonl] [--state-hash] --out out/\n       hoard-sim --data data/ --golden 1-300\n");
        return 2;
    }
    if (o.golden_to) return golden(o);
    if (!o.render_batch.empty()) return render_batch_cmd(o.data, o.render_batch);
    if (!o.text_bundle.empty()) return text_bundle_cmd(o.data, o.text_bundle);
    if (!o.replay.empty()) return replay(o);
    LoadResult loaded = load_data(o.data, o.seed);
    if (!loaded.ok) { std::fprintf(stderr, "load failed: %s\n", loaded.error.c_str()); return 1; }
    if (o.no_bank) loaded.params.sift.patterns.clear();
    World& w = loaded.world;
    const Params& p = loaded.params;
    std::string gerr, legends;
    Profile profile;   // item 04
    if (!o.profile.empty() && !load_profile(o.profile, profile, gerr)) { std::fprintf(stderr, "%s\n", gerr.c_str()); return 1; }
    if (o.genesis && !make_genesis(w, p, profile, gerr)) { std::fprintf(stderr, "genesis failed: %s\n", gerr.c_str()); return 1; }   // v0.4 world (S2)
    if (o.genesis) legends = legends_json(w, p);
    note_hatching(profile, w, p);
    if (!o.load_save.empty()) {
        std::ifstream in(o.load_save, std::ios::binary);
        std::stringstream ss;
        ss << in.rdbuf();
        std::string err;
        if (!in || load_save(ss.str(), w, err) != LoadSave::ok) { std::fprintf(stderr, "cannot load %s: %s\n", o.load_save.c_str(), err.c_str()); return 1; }
    }

    std::vector<Act> log;
    if (!o.acts.empty()) {
        std::ifstream in(o.acts);
        if (!in) { std::fprintf(stderr, "cannot open %s\n", o.acts.c_str()); return 1; }
        std::stringstream ss;
        ss << in.rdbuf();
        std::string err;
        if (!acts_from_json(ss.str(), log, err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
    }
    Policy policy;
    bool has_policy = false;
    if (!o.policy.empty()) {
        std::string err;
        if (!load_policy(o.data, o.policy, policy, err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
        has_policy = true;
    }

    Scenario s;
    s.first_acts = log;
    s.policy = has_policy ? &policy : nullptr;
    s.data_dir = o.data;
    s.years = o.years;
    s.sleep = o.sleep;
    std::ofstream trace;
    if (!o.trace.empty()) {
        trace.open(o.trace);
        if (!trace) { std::fprintf(stderr, "cannot write %s\n", o.trace.c_str()); return 1; }
        trace << state_hash_json(w) << "\n";
        s.after_sleep = [&](const World& x) { trace << state_hash_json(x) << "\n"; };
    }
    std::ofstream snaps, wakes;
    AnalyseOptions ao;
    if (!o.snapshots.empty()) { snaps.open(o.snapshots); ao.snapshots = &snaps; }
    if (!o.wakings.empty()) { wakes.open(o.wakings); ao.wakings = &wakes; }
    ao.buckets = o.buckets;
    ao.bucket_samples = o.bucket_samples;
    ao.perturb = o.perturb;
    if (ao.snapshots || ao.wakings || !ao.perturb.empty()) attach_analysis(s, p, ao);
    if (!o.bot.empty()) {   // item 04
        Bot bot;
        if (!parse_bot(o.bot, bot)) { std::fprintf(stderr, "unknown bot %s\n", o.bot.c_str()); return 2; }
        attach_bot(s, p, bot_target(bot, profile, p));
    }
    auto traced = s.after_sleep;
    s.after_sleep = [&](const World& x) { if (traced) traced(x); note_waking(profile, x, p); };
    Recorder recorder;
    std::string header;
    if (!o.record.empty()) {
        header = replay_header(w, p, o.genesis);
        if (!profile.legends.empty()) { json hj = json::parse(header); hj["legends"] = legends_to_json(profile.legends); header = hj.dump(); }   // item 04: genesis wove them
        recorder.world = &w;
        w.recorder = &recorder;
    }
    run_scenario(w, p, s);
    if (!o.record.empty()) {
        w.recorder = nullptr;
        std::ofstream rec(o.record);
        if (!rec) { std::fprintf(stderr, "cannot write %s\n", o.record.c_str()); return 1; }
        rec << header << "\n";
        for (auto& l : recorder.lines) rec << l << "\n";
        rec << json({{"end", 1}, {"year", w.year}, {"h", hash_hex(state_hash(w).root)}}).dump() << "\n";
    }
    note_death(profile, w, p);   // item 04
    note_exit(profile, w, p);
    if (!o.profile.empty() && !save_profile(o.profile, profile, gerr)) { std::fprintf(stderr, "%s\n", gerr.c_str()); return 1; }
    if (!o.save.empty() && !write_save(o.save, save_json(w, p))) { std::fprintf(stderr, "cannot write %s\n", o.save.c_str()); return 1; }
    if (o.state_hash) std::printf("%s\n", hash_hex(state_hash(w).root).c_str());

    std::string base = out_dir(o.out);
    if (!write_core(base, w, p)) { std::fprintf(stderr, "cannot write to %s\n", o.out.c_str()); return 1; }
    if (o.narrate) {   // narrated.json: the narrator's sentence for every chronicle event, in chronicle order
        json lines = json::array();
        for (auto& e : w.chronicle) lines.push_back(event_sentence(w, p, e));
        if (!write_file(base + "narrated.json", lines.dump(1))) { std::fprintf(stderr, "cannot write narrated.json\n"); return 1; }
    }
    if (!legends.empty() && !write_file(base + "legends.json", legends)) { std::fprintf(stderr, "cannot write legends\n"); return 1; }   // v0.4 world (S2)
    std::string briefing = briefing_json(w, p, w.sleep_started);
    if (!write_file(base + "briefing.json", briefing)) { std::fprintf(stderr, "cannot write briefing\n"); return 1; }
    if (o.briefing) {
        json b = json::parse(briefing, nullptr, false);
        std::printf("== %s\n", b["headline"]["text"].get<std::string>().c_str());
        for (auto& g : b["while_you_slept"]) {
            std::printf("-- %d to %d\n", g["from"].get<int>(), g["to"].get<int>());
            for (auto& l : g["lines"]) std::printf("  %d  %s\n", l["year"].get<int>(), l["text"].get<std::string>().c_str());
        }
        for (const char* k : {"people", "threads", "opportunities"}) {
            std::printf("-- %s\n", k);
            for (auto& l : b[k]) std::printf("  %s\n", l["text"].get<std::string>().c_str());
        }
        std::printf("-- %s\n", b["phase"]["text"].get<std::string>().c_str());
    }
    std::printf("seed %llu year %d factions %zu hash %016llx\n", static_cast<unsigned long long>(w.seed), w.year,
                w.factions.size(), static_cast<unsigned long long>(chronicle_hash(w)));
    return 0;
}
