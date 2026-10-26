#pragma once
#include <string>
#include "ages/json.h"

namespace text {

// locale is data/text/<lang>/locale.json (CLDR via Babel, tools/text/cldr.py)
std::string format_int(long long n, const nlohmann::json& locale);
std::string plural_category(long long n, const nlohmann::json& locale);   // "one", "other", ... for an integer

}
