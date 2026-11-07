#pragma once
#include <map>
#include <string>
#include <vector>
#include "ages/ages.h"
#include "ages/state.h"

// item 15: RaidReport (raid-design.md section 3), the facts a shell sends at the end of a sortie; raid_diff turns them into acts
namespace ages {

struct RaidSettlement {
    std::string region, owner;
    std::map<std::string, double> destroyed;   // by building class: house, keep, library, temple, foundry, walls, granary, tower, altar, monument
    std::map<std::string, double> collected;   // gold, books, artifacts, art, food, royals
    std::vector<std::string> items;
    double soldiers = 0, garrison = 0, civilians = 0;
    std::vector<std::string> persons;
    std::vector<std::string> taken;            // hostages carried off; "" is the court's heir
    double lowest_altitude = 0, seconds_below_150 = 0;
    bool roared = false, landed_in_square = false;
};

struct RaidArmy {
    std::string attacker, defender, region;
    double killed_share = 0;
    bool repelled = false;   // the attacker's assault was broken
};

struct RaidReceived {
    std::string id, outcome;
};

struct RaidLair {
    bool has_dropped = false;
    std::map<std::string, double> dropped;
    std::vector<std::string> dropped_items;
    std::vector<RaidReceived> received;
    std::vector<std::string> fought;
};

struct RaidReport {
    int sortie = 0;
    std::string shell;   // "tactical" | "flight"; echoed in the result
    std::vector<RaidSettlement> settlements;
    std::vector<RaidArmy> armies;
    RaidLair lair;
    std::vector<Act> spells, speech;   // acts already
    std::map<std::string, double> wounds;   // "<region>|<defence>" or "<defence>" -> damage before armour
};

struct DiffResult {
    std::vector<Act> acts;
    std::vector<std::string> warnings;   // reason codes
};

struct SortieResult {
    bool ok = false;
    std::string reason;
    std::string shell;
    size_t first_act = 0, last_act = 0;   // the logged acts of this sortie are World.acts[first_act, last_act)
    std::vector<Act> derived;
    std::vector<ActRefusal> refused;
    std::vector<ReactionLine> lines;
    std::vector<std::string> warnings;
};

bool report_from_json(const std::string& text, RaidReport& out, std::string& err);
std::string report_to_json(const RaidReport& r);
std::string sortie_to_json(const SortieState& s);
bool sortie_from_json(const std::string& text, SortieState& out);
std::string sortie_view_json(const Params& p, const SortieState& s);   // what begin_sortie returns to the shell

SortieState take_snapshot(const World& w, const Params& p, int number);
DiffResult raid_diff(const World& w, const Params& p, const RaidReport& r);

// state-changing; recorded.cpp wraps each
bool begin_sortie_unrecorded(World& w, const Params& p, int number, std::string& reason);
SortieResult end_sortie_unrecorded(World& w, const Params& p, const RaidReport& r);

bool begin_sortie(World& w, const Params& p, int number, std::string& reason);
SortieResult end_sortie(World& w, const Params& p, const RaidReport& r);

}
