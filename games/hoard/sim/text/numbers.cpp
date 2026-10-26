#include "numbers.h"
#include <vector>

namespace text {

using nlohmann::json;

std::string format_int(long long n, const json& locale) {
    std::string digits = std::to_string(n < 0 ? -n : n);
    std::string group = locale.value("group", std::string(","));
    size_t size = locale.value("grouping", 3);
    size_t min_grouping = locale.value("min_grouping", 1);
    std::string out;
    if (size == 0 || digits.size() < size + min_grouping) out = digits;
    else
        for (size_t i = 0; i < digits.size(); ++i) {
            if (i && (digits.size() - i) % size == 0) out += group;
            out += digits[i];
        }
    return n < 0 ? "-" + out : out;
}

namespace {

std::vector<std::string> split(const std::string& s, const std::string& sep) {
    std::vector<std::string> out;
    size_t at = 0;
    for (size_t next; (next = s.find(sep, at)) != std::string::npos; at = next + sep.size()) out.push_back(s.substr(at, next - at));
    out.push_back(s.substr(at));
    return out;
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(' '), b = s.find_last_not_of(' ');
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}

// operands of an integer (CLDR plural rules): n and i are the value, v w f t c e are 0
long long operand(const std::string& name, long long n) { return name == "n" || name == "i" ? n : 0; }

bool in_ranges(long long x, const std::string& list) {
    for (auto& r : split(list, ",")) {
        size_t dots = r.find("..");
        long long lo = std::stoll(trim(r.substr(0, dots)));
        long long hi = dots == std::string::npos ? lo : std::stoll(trim(r.substr(dots + 2)));
        if (x >= lo && x <= hi) return true;
    }
    return false;
}

// "i % 10 in 2..4" | "n = 1" | "v not in 0" | "n != 1"
bool relation(const std::string& rel, long long n) {
    std::string r = trim(rel);
    for (const char* op : {" not in ", " not within ", " != ", " in ", " within ", " = "}) {
        size_t at = r.find(op);
        if (at == std::string::npos) continue;
        std::string expr = trim(r.substr(0, at)), list = trim(r.substr(at + std::string(op).size()));
        size_t mod = expr.find('%');
        long long x = operand(trim(expr.substr(0, mod)), n);
        if (mod != std::string::npos) x %= std::stoll(trim(expr.substr(mod + 1)));
        bool hit = in_ranges(x, list);
        std::string o = op;
        return (o == " not in " || o == " not within " || o == " != ") ? !hit : hit;
    }
    return false;
}

bool rule(const std::string& r, long long n) {
    for (auto& alt : split(r, " or ")) {
        bool all = true;
        for (auto& rel : split(alt, " and ")) all = all && relation(rel, n);
        if (all) return true;
    }
    return false;
}

}

std::string plural_category(long long n, const json& locale) {
    if (n < 0) n = -n;
    auto it = locale.find("plural");
    if (it != locale.end())
        for (auto r = it->begin(); r != it->end(); ++r)
            if (r.value().is_string() && rule(r.value().get<std::string>(), n)) return r.key();
    return "other";
}

}
