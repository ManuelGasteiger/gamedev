#pragma once
#include <string>
#include "ages/data.h"

namespace ages {

// the line in words when its key is a pat.* key, else ""
std::string sift_line_text(const World& w, const Params& p, const SiftLine& l, const std::string& lang = "en");

}
