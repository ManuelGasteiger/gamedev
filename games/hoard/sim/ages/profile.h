#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "data.h"
#include "json.h"
#include "state.h"

// item 04: what carries between runs, all knowledge (research-meta-progression.md section 1). The sim never stores it
// in a world or a save; the shell or the runner keeps the five files and passes them in.

namespace ages {

struct Deed {
    std::string type, target;
    int year = 0;
    double weight = 0;   // magnitude x witnesses
};

struct LegendEntry {
    std::string id, name, killer, tier, ending = "myth";
    uint64_t world_seed = 0;
    int reign = 0, hatched = 0, died = 0;
    bool slain_by_order = false;
    double dread = 0, awe = 0;
    std::vector<Deed> deeds;
};

struct Atom {
    std::string state;   // rumor | witnessed | used
    std::string how;
};

struct BestiaryEntry {
    std::string kind;    // order | defence
    int waking = 0, years_after_hatching = 0, era = 0;
};

struct GlossaryEntry {
    std::string ending, cond;
    bool confirmed = false;
};

struct Profile {
    std::vector<LegendEntry> legends;
    std::map<std::string, std::map<std::string, Atom>> atoms;   // ending -> condition
    nlohmann::json seen = nlohmann::json::object();             // item 10 owns the shape
    std::map<std::string, BestiaryEntry> bestiary;
    std::map<std::string, GlossaryEntry> glossary;              // prophecy line key -> the condition it encodes
};

// the five files as one object {legends, ending_atoms, seen, bestiary, glossary}; each part may be missing
nlohmann::json profile_to_json(const Profile& pr);
Profile profile_from_json(const nlohmann::json& j);
nlohmann::json legends_to_json(const std::vector<LegendEntry>& legends);
std::vector<LegendEntry> legends_from_json(const nlohmann::json& j);

// file IO, data_io.cpp (not part of the wasm build): <dir>/legends.json, ending_atoms.json, seen.json, bestiary.json,
// glossary.json; a missing file is empty
bool load_profile(const std::string& dir, Profile& out, std::string& err);
bool save_profile(const std::string& dir, const Profile& pr, std::string& err);

// genesis: the woven legends (1 to 3 pre-history events each, world tag legend:<id>), legend_weave.cpp
struct Weave {
    LegendEntry legend;
    int year = 0, index = 0;
};
std::vector<Weave> plan_weaves(const World& w, const Params& p, const Profile& pr);
void weave_legend(World& w, const Params& p, const Weave& wv);

// profile_notes.cpp: what a run teaches
void note_hatching(Profile& pr, const World& w, const Params& p);
void note_waking(Profile& pr, const World& w, const Params& p);
bool myth_qualifies(const World& w, const Params& p);
LegendEntry myth_entry(const World& w, const Params& p);
void note_death(Profile& pr, const World& w, const Params& p);   // closest conditions as rumors; the legend on a Myth
void note_exit(Profile& pr, const World& w, const Params& p);
bool atom_known(const Profile& pr, const std::string& ending, const std::string& cond);
std::vector<std::string> closest_conditions(const World& w, const Params& p);   // "ending.cond", one to three

}
