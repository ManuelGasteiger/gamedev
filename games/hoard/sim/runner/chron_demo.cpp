#include "chron_demo.h"
#include <cstdio>
#include "ages/ages.h"
#include "ages/chron.h"
#include "ages/genesis.h"
#include "ages/json.h"
#include "ages/memory_tiers.h"
#include "ages/talents.h"
#include "ages/world_util.h"

// item 05 report: one seed under the played policy, plus a kept historian, books taken from every court that writes,
// and the historian's death after three wakings; prints the three wakings of paper-playtest-wakings.md as found

namespace ages {

using nlohmann::json;

namespace {

void print(const json& b, const char* title) {
    std::printf("\n## %s · year %d · slept %d years\n\n", title, b.value("year", 0), b.value("year", 0) - b.value("since", 0));
    const json& c = b["chronicle"];
    std::printf("Chronicle level %d", c.value("level", 0));
    if (c.contains("historian")) std::printf(", historian %s (%s, born %d)", c["historian"].value("name", "").c_str(), c["historian"].value("race", "").c_str(), c["historian"].value("born", 0));
    std::printf(", %d sources held\n", c.value("sources_held", 0));
    if (c.contains("ended")) std::printf("%s\n", c["ended"].value("text", "").c_str());
    if (b.contains("voice")) std::printf("\nVoice: %s\n", b["voice"].value("text", "").c_str());
    std::printf("\n");
    for (auto& l : b.value("lines", json::array()))
        std::printf("- [%s, level %d] %s\n", l.value("kind", "").c_str(), l.value("level", 0), l.value("text", "").c_str());
}

bool counterfactual(const json& b) {
    for (auto& l : b.value("lines", json::array())) if (l.contains("counterfactual")) return true;
    return false;
}

bool disputed(const json& b) {
    for (auto& s : b.value("sources", json::array())) if (s.value("disputed", false)) return true;
    return false;
}

}

int chronicle_demo(const std::string& data, uint64_t seed) {
    LoadResult r = load_data(data, seed);
    std::string err;
    if (!r.ok || !make_genesis(r.world, r.params, err)) { std::fprintf(stderr, "load failed: %s %s\n", r.error.c_str(), err.c_str()); return 1; }
    Policy policy;
    if (!load_policy(data, "played", policy, err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
    World& w = r.world;
    const Params& p = r.params;
    std::string historian;
    int kept_wakings = 0, voices = 0;
    bool shown0 = false, shown2 = false, shown1 = false, shown3 = false;
    Scenario s;
    s.policy = &policy;
    s.data_dir = data;
    s.years = 3000;
    s.at_waking = [&](World& x, std::vector<Act>& acts, int index) {
        if (index == 0) return;
        if (!historian_now(x) && (historian.empty() || x.year >= 1500)) {
            const Person* pick = nullptr;
            for (auto& q : x.people)
                if ((q.role == "sage" || q.role == "priest") && !has_tag(q.tags, "hostage") && find_faction(x, q.faction) && (!pick || q.dies > pick->dies)) pick = &q;
            if (pick) {
                Act a;
                a.type = "take_hostage";
                a.target = pick->faction;
                a.item = pick->id;
                a.magnitude = 0.2;
                std::string id = pick->id, why;
                apply_acts(x, p, {a});
                if (keep_person(x, p, id, "historian", why)) historian = id;
            }
        } else if (historian_now(x) && ++kept_wakings == 8 && find_person(x, historian)) {
            std::string why;
            kill_person(x, p, historian, why);
        }
        if (x.year >= 2000 && x.dragons[kPlayer].chron.relics.empty())   // a raid for an artifact with a voice
            for (auto& f : x.factions) {
                Act t;
                t.type = "take";
                t.target = f.id;
                t.magnitude = 1.0;
                t.payload["artifacts"] = 1;
                acts.push_back(t);
            }
        if (const Relic* r = speaking_relic(x))
            if (r->asked < 0 && r->year < x.year) {
                std::string why;
                if (commune(x, p, r->id, "ask", why))
                    std::printf("\n## commune(ask) · year %d\n\nVoice: %s\n", x.year, json::parse(briefing_json(x, p, x.sleep_started))["voice"].value("text", "").c_str());
            }
        for (auto& f : x.factions) {
            if (!keeps_records(x, f)) continue;
            Act t;
            t.type = "take";
            t.target = f.id;
            t.magnitude = 0.1;
            t.payload["books"] = 1;
            acts.push_back(t);
        }
    };
    s.after_sleep = [&](const World& x) {
        json b = json::parse(briefing_json(x, p, x.sleep_started), nullptr, false);
        int level = b["chronicle"].value("level", 0);
        if (!shown0 && level == 0) { print(b, "Waking A: dreams"); shown0 = true; }
        else if (!shown2 && level >= 2) { print(b, "Waking B: a kept historian"); shown2 = true; }
        else if (!shown3 && level >= 3 && counterfactual(b)) { print(b, "Waking B2: a historian and high Lore"); shown3 = true; }
        else if (b.contains("voice") && !voices) { print(b, "Waking D: a relic speaks"); ++voices; }
        else if (shown2 && !shown1 && level < 2 && disputed(b)) { print(b, "Waking C: sources that disagree"); shown1 = true; }
    };
    run_scenario(w, p, s);
    std::printf("\nAt the end: Lore rank %d (%.0f points), %zu relics\n", ranks_of(w.dragons[kPlayer], p.talents, "lore"), w.dragons[kPlayer].growth["lore"], w.dragons[kPlayer].chron.relics.size());
    if (!shown1) std::printf("\n(no waking after the historian with two disagreeing sources)\n");
    return 0;
}

}
