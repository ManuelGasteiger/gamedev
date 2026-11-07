#pragma once
#include <map>
#include <string>
#include <vector>

// item 15: what the courts held when a sortie began; raid_diff reads shares against this, never the current state.
// World.sortie is saved and hashed only while open, so closed it leaves goldens and saves as they were.
namespace ages {

struct SortieSettlement {
    std::string region, owner;
    double value = 0, population = 0, garrison = 0, soldiers = 0;
    std::map<std::string, double> structures;   // value of each structure the region holds
};

struct SortieCourt {
    std::string id;
    double wealth = 0;
    std::map<std::string, double> holds;   // per loot category
};

struct SortieArmy {
    std::string attacker, defender, region;
    double size = 0;
};

struct SortieState {
    bool open = false;
    int number = 0, year = 0;
    std::vector<SortieSettlement> settlements;
    std::vector<SortieCourt> courts;
    std::vector<SortieArmy> armies;
};

}
