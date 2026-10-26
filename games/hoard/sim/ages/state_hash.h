#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include "json.h"
#include "state.h"

namespace ages {

// the state hash of section 2 of docs/research-determinism-ci.md: values, never text; one part per top-level field of
// the saved world, so whatever a save holds is hashed and nothing else
struct StateHash {
    uint64_t root = 0;
    std::vector<std::pair<std::string, uint64_t>> parts;   // sorted by name
};

StateHash state_hash(const World& w);
StateHash state_hash_of_json(const nlohmann::json& world);   // the same over a saved world object
std::string hash_hex(uint64_t h);
std::string state_hash_json(const World& w);   // {"year", "root", "parts": {...}}, one trace line

}
