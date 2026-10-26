#include "tokens.h"

namespace text {

namespace {

bool ident_char(char c, bool first) { return (c >= 'a' && c <= 'z') || c == '_' || (!first && c >= '0' && c <= '9'); }

bool ident(const std::string& s) {
    if (s.empty() || !ident_char(s[0], true)) return false;
    for (char c : s) if (!ident_char(c, false)) return false;
    return true;
}

// splits at sep where the brace depth is zero; with keyed, only where the next part starts with "key="
std::vector<std::string> split_top(const std::string& s, char sep, bool keyed) {
    std::vector<std::string> out(1);
    int depth = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '{') ++depth;
        if (c == '}') --depth;
        if (c == sep && depth == 0) {
            size_t eq = s.find('=', i + 1);
            if (!keyed || (eq != std::string::npos && ident(s.substr(i + 1, eq - i - 1)))) { out.emplace_back(); continue; }
        }
        out.back() += c;
    }
    return out;
}

bool choices(const std::string& body, char sep, std::vector<Choice>& out) {
    for (auto& part : split_top(body, sep, sep == ',')) {
        size_t eq = part.find('=');
        if (eq == std::string::npos || !(ident(part.substr(0, eq)) || part.substr(0, eq) == "*")) return false;
        out.push_back({part.substr(0, eq), part.substr(eq + 1)});
    }
    return !out.empty();
}

bool head(const std::string& h, Token& t) {
    size_t dot = h.find('.');
    t.role = h.substr(0, dot);
    if (dot != std::string::npos) t.field = h.substr(dot + 1);
    return ident(t.role) && (dot == std::string::npos || ident(t.field));
}

bool token(const std::string& inner, Token& t) {
    size_t q = inner.find('?');
    size_t colon = inner.find(':');
    if (q != std::string::npos && (colon == std::string::npos || q < colon)) {
        t.select = true;
        return head(inner.substr(0, q), t) && choices(inner.substr(q + 1), '|', t.choices);
    }
    if (!head(inner.substr(0, colon), t)) return false;
    if (colon == std::string::npos) return true;
    std::string rest = inner.substr(colon + 1);
    if (rest.rfind("plural(", 0) == 0) {
        if (rest.back() != ')') return false;
        t.plural = true;
        return choices(rest.substr(7, rest.size() - 8), ',', t.choices);
    }
    size_t at = 0;
    while (at <= rest.size()) {
        size_t next = rest.find('.', at);
        std::string m = rest.substr(at, next == std::string::npos ? std::string::npos : next - at);
        if (!ident(m)) return false;
        t.mods.push_back(m);
        if (next == std::string::npos) break;
        at = next + 1;
    }
    return true;
}

}

std::vector<Piece> parse(const std::string& tmpl, std::string& err) {
    std::vector<Piece> out;
    std::string lit;
    for (size_t i = 0; i < tmpl.size(); ++i) {
        char c = tmpl[i];
        if (c == '}') { err = "stray } at " + std::to_string(i); return out; }
        if (c != '{') { lit += c; continue; }
        int depth = 0;
        size_t j = i;
        for (; j < tmpl.size(); ++j) {
            if (tmpl[j] == '{') ++depth;
            if (tmpl[j] == '}' && --depth == 0) break;
        }
        if (j == tmpl.size()) { err = "unclosed { at " + std::to_string(i); return out; }
        if (!lit.empty()) { out.push_back({false, lit, {}}); lit.clear(); }
        Piece p;
        p.is_token = true;
        p.token.source = tmpl.substr(i, j - i + 1);
        if (!token(tmpl.substr(i + 1, j - i - 1), p.token)) { err = "bad token " + p.token.source; return out; }
        out.push_back(p);
        i = j;
    }
    if (!lit.empty()) out.push_back({false, lit, {}});
    return out;
}

}
