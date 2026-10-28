#pragma once
#include <map>
#include <string>
#include <vector>
#include "data.h"
#include "state.h"

// item 07: the six social moves, responder side (data/moves.json). Item 09 plumbs the acts and calls these.

namespace ages {

struct MoveCtx {
    std::map<std::string, double> growth;   // the dragon's growth points (lore, arcana, ...)
    std::vector<std::string> talents;
    bool guise = false;                     // the dragon is in guise at the person's court
    int act = -1;                           // index in World.acts, -1 when none
    std::string subject;                    // teach: letters | craft | magic | war | intrigue | dragon_lore
    std::string toward = "dragon";          // corrupt: dark | undead | chaos | dragon
    std::string demand;                     // threaten: "hostage" hands over the person in `hostage` (default the heir)
    std::string hostage;
};

struct WrittenMemory {
    std::string person;
    PersonMemory m;
};

struct MoveOutcome {
    bool accepted = false;
    double score = 0, margin = 0;
    std::string refusal;   // reason code when a precondition failed (no roll, nothing written)
    std::vector<WrittenMemory> memory_written;
    std::string chronicle_line_key;
    int event = 0;
};

struct ReleaseReading {
    std::string reading;   // mercy | insult | weakness | reprieve
    PersonMemory m;
    bool terms = false, sworn = false;
};

double move_score(const World& w, const Params& p, const Person& x, const MoveDef& def);
MoveOutcome resolve_move(World& w, const Params& p, const std::string& person, const std::string& move, const MoveCtx& ctx);
// writes the reading on the person; release_hostage_person calls it
ReleaseReading read_release(World& w, const Params& p, const std::string& person);
// an oath draft the court answers when the person comes home (status "draft" until then)
void release_terms(World& w, const Oath& draft, const std::string& person);

}
