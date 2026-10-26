#pragma once
#include <functional>
#include <cstdint>
#include <string>
#include <vector>
#include "data.h"
#include "state.h"

namespace ages {

struct LoadResult {
    bool ok = false;
    std::string error;
    Params params;
    World world;
};

struct SleepResult {
    std::string wake_reason;     // "planned" | "slay_attempt_failed" | "dragon_slain" | "hunger" | "premonition"
    int years_slept = 0;
    std::vector<Event> chronicle;
    std::map<std::string, double> growth_gained;   // v0.4 (S1): growth points per axis gained in this sleep
    double healed = 0;                             // v0.4 (S1): wounds slept off
    std::vector<Hook> hooks;                       // v0.4 hooks (S3): the situations waiting at this waking
};

struct Forecast {
    int years = 0;
    std::string wake_reason;
    double size = 0, knowledge = 0, magic = 0, hunger = 0;
    std::string strongest;
    int strongest_era = 0;
    bool slay_possible = false;
    std::string hook_kind, hook_title, hook_region;   // v0.4 hooks (S3): the likely hook of the next waking (star-reading)
};

struct CastResult {
    bool cast = false;
    std::string spell, target, region, against, reason;
    bool has_modifier = false;
    Modifier modifier;
    std::string spawned_faction;   // raise_undead
    std::string war_with;          // incite / raise_undead
};

struct TerraformResult {
    bool done = false;
    std::string tag, reason;
    double cost = 0;
    bool has_modifier = false;
    Modifier modifier;
};

// spells.cpp: section 8 verbs resolving to modifiers or spawns, gated and priced by constants.json "spells" / "terraform"
CastResult cast_spell(World& w, const Params& p, const std::string& spell, const std::string& target,
                      const std::string& region, const std::string& against);
TerraformResult terraform(World& w, const Params& p, const std::string& region, const std::string& kind);
std::string cast_result_to_json(const CastResult& r);
std::string terraform_result_to_json(const TerraformResult& r);
std::string spells_catalogue_json(const Params& p);   // gates and costs for the shell's availability display

LoadResult load_data(const std::string& dir, uint64_t seed);                 // data_io.cpp
LoadResult load_data_from_json(const DataJson& d, uint64_t seed);

// what an act meant to one court, in one or two plain sentences (data/text/en.json "reactions")
struct ReactionLine {
    std::string faction;
    std::string text;
};

// applies the act log (section 5) and narrates the reactions it caused, one line per court that reacted
std::vector<ReactionLine> apply_acts(World& w, const Params& p, const std::vector<Act>& acts);
std::string reaction_lines_to_json(const std::vector<ReactionLine>& lines);
SleepResult sleep(World& w, const Params& p, int years);   // 17.4: any years >= 1; a sub-decade sleep is one short step
Forecast forecast(const World& w, const Params& p, int years);   // dry run on a copy, blurred by 1/(1+knowledge)
std::string forecast_to_json(const Forecast& f);
uint64_t chronicle_hash(const World& w);
std::string chronicle_hash_hex(const World& w);
std::string chronicle_text(const std::vector<Event>& events);   // the text golden, one event per line

// offers (section 9, 17.3): w.offers is generated at wake; accept swears the draft, decline is a `show` 0.1 act
bool accept_offer(World& w, const Params& p, size_t index);
bool decline_offer(World& w, const Params& p, size_t index);
// offers.cpp: the dragon kills a named mortal, resolved to a `kill` act of envoy_kill_magnitude on the court with fear
// envoy_kill_fear; `lines` receives the narrated reactions when given
bool kill_person(World& w, const Params& p, const std::string& id, std::string& reason, std::vector<ReactionLine>* lines = nullptr);

// 17.5: the five briefing blocks (headline, while_you_slept, people, threads, opportunities) as JSON text
std::string briefing_json(const World& w, const Params& p, int since_year);
std::string event_sentence(const World& w, const Params& p, const Event& e);   // the narrator's line, cause clause included
std::string people_json(const World& w, const Params& p);

// demand (section 8): the dragon imposes a clause (tribute | no_walls | no_settle) on a court; sworn when the court is
// cowed (fear >= demand_fear or power below the weak ratio of threat), refused otherwise with a resolve gain
struct DemandResult {
    bool sworn = false;
    std::string target, pred, reason;
    Oath oath;
};
DemandResult demand_oath(World& w, const Params& p, const std::string& target, const std::string& pred);
std::string demand_result_to_json(const DemandResult& r);

std::string world_to_json(const World& w);
std::string world_view_json(const World& w, const Params& p);   // world_to_json plus derived values (power, era name, threat, offers)
std::string sleep_result_to_json(const SleepResult& r, const World& w, const Params& p);
bool world_from_json(const std::string& text, World& out, std::string& err);
std::string acts_to_json(const std::vector<Act>& acts);
bool acts_from_json(const std::string& text, std::vector<Act>& out, std::string& err);
std::string chronicle_to_json(const std::vector<Event>& events);

struct Scenario {
    std::vector<Act> first_acts;
    const Policy* policy = nullptr;
    std::string data_dir;
    int years = 3000;
    int sleep = 100;
    std::function<void(const World&)> after_sleep;   // trace and replay checkpoints
    std::function<void(World&, std::vector<Act>&, int)> at_waking;   // the policy's acts before they apply (analysis, perturbation)
    std::function<void(const World&, int)> before_sleep;             // the waking's end, before the sleep starts
    std::function<int(int, int)> adjust_sleep;                       // (years, sleep index) -> years
};

void run_scenario(World& w, const Params& p, const Scenario& s);

std::vector<Act> resolve_policy(const World& w, const Params& p, const Policy& policy, int sleep_index,
                                const std::string& data_dir);

// v0.4 hooks (S3), hook_resolve.cpp: a hook answered by a choice (pilgrims: receive | eat); false with the reason
bool answer_hook(World& w, const Params& p, const std::string& id, const std::string& choice, std::string& reason);

}
