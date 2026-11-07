#pragma once
#include <string>
#include "ages/data.h"

namespace ages {

// the line in words when its key is a pat.* key, else ""; without its cause the key's .bare variant where one exists
std::string sift_line_text(const World& w, const Params& p, const SiftLine& l, const std::string& lang = "en", bool with_cause = true);
bool sift_text_exists(const Params& p, const std::string& key);   // in English, with the fallbacks

}
