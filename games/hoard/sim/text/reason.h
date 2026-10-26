#pragma once
#include <string>
#include "bundle.h"

namespace text {

// a sim refusal "code|key=value|..." (ages.h reason_code) worded by ui.json "reason.<code>"; values that are numbers
// take the language's number format, values with a ui.json "word.<value>" take that word
std::string reason_text(const Bundles& all, const std::string& lang, const std::string& encoded);

}
