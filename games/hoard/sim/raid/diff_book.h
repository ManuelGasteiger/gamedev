#pragma once
#include <map>
#include <string>
#include <vector>

// item 15: data/raid_diff.json
namespace ages {

struct RaidDiffBook {
    double civilian_toll_factor = 0.2;   // the tactical shell's civilians = burn share x population x this
    double structure_threshold = 0.5;    // a structure burns as its own act above this share of its value
    std::vector<std::string> structures;   // in act order
    std::vector<std::string> plain;        // building classes that count toward the plain burn
    std::map<std::string, double> structure_share;   // of a settlement's value, for each structure its region holds
    double show_land = 0.5, show_roar = 0.3, show_flyover = 0.1, flyover_seconds = 3;
    bool oath_defends = false;           // a hit on the army of a court the dragon is sworn to also defends it (the design table); off, only a repelled army defends
    double kill_person = 0.05, hostage = 0.5, repelled = 0.5, item = 0.05;
    double value_per_wealth = 1, population_per_wealth = 10, garrison_per_power = 25, soldiers_per_power = 100, army_per_power = 100;
    std::map<std::string, double> holdings;   // per loot category, of the court's wealth
};

}
