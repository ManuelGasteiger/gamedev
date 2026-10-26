#pragma once
#include <string>
#include "data.h"
#include "state.h"

namespace ages {

constexpr int kSaveFormat = 1;

// docs/research-determinism-ci.md section 3: a header, then the world; loading checks the header first and the state
// hash last, so a corrupt or foreign save is refused instead of continuing a different history
std::string save_json(const World& w, const Params& p);

enum class LoadSave { ok, bad_json, not_a_save, newer_format, hash_mismatch, bad_world };
LoadSave load_save(const std::string& text, World& out, std::string& err, bool* replay_verifiable = nullptr);

}
