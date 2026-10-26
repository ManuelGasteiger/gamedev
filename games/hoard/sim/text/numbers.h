#pragma once
#include <string>
#include "ages/json.h"

namespace text {

// locale is data/text/<lang>/locale.json (CLDR via Babel, tools/text/cldr.py)
std::string format_int(long long n, const nlohmann::json& locale);
// "1200.5" as the language writes it; false when s is not a plain decimal number
bool format_decimal(const std::string& s, const nlohmann::json& locale, std::string& out);
std::string plural_category(long long n, const nlohmann::json& locale);   // "one", "other", ... for an integer

}
