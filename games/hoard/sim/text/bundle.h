#pragma once
#include <map>
#include <string>
#include "ages/json.h"

namespace text {

// one language: {"lang", "ui", "lexicon", "names", "narrator", "scenarios", "scenarios.meta", "patterns", "locale"},
// each part the file data/text/<lang>/<part>.json as it is
struct Bundle {
    std::string lang;
    nlohmann::json j;
};

// the flat string tables a key is looked up in
extern const char* const kStringParts[3];   // ui, scenarios, patterns

const std::string* find_string(const Bundle& b, const std::string& key);
const nlohmann::json& part(const Bundle& b, const char* name);

bool bundle_from_json(const std::string& text, Bundle& out, std::string& err);
Bundle pseudo_bundle(const Bundle& en);   // accented letters, 35 % longer, in brackets; tokens kept

// en, de and pseudo from their JSON texts; pseudo is made from en. A release build renders a missing string or a
// failed token in English with English rules; a dev build shows the key or the token as written.
struct Bundles {
    std::map<std::string, Bundle> by_lang;
#ifdef HOARD_TEXT_RELEASE
    bool release = true;
#else
    bool release = false;
#endif
    const Bundle* get(const std::string& lang) const;
};
bool bundles_from_json(const std::map<std::string, std::string>& texts, Bundles& out, std::string& err);

}
