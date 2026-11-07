#include "predicate.h"
#include <cstdlib>

namespace scn {

namespace {

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(' '), b = s.find_last_not_of(' ');
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}

bool ident(const std::string& s) {
    if (s.empty() || !(s[0] >= 'a' && s[0] <= 'z')) return false;
    for (char c : s) if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) return false;
    return true;
}

// ordinal words of the closed lists (vocab.json memory_tiers, slayer_stages); booleans as 0 and 1
int rank(const std::string& s) {
    static const char* const tiers[] = {"none", "oral", "written", "monumental"};
    static const char* const stages[] = {"none", "forming", "ready", "marching"};
    for (int i = 0; i < 4; ++i) if (s == tiers[i] || s == stages[i]) return i;
    if (s == "false") return 0;
    if (s == "true") return 1;
    return -1;
}

bool number(const std::string& s, double& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    out = std::strtod(s.c_str(), &end);
    return end && *end == 0;
}

bool parse_clause(std::string s, Clause& c, std::string& err) {
    s = trim(s);
    if (s.rfind("not ", 0) == 0) { c.neg = true; s = trim(s.substr(4)); }
    if (s.rfind("bound(", 0) == 0 && s.back() == ')') {
        c.kind = Clause::bound;
        c.role = trim(s.substr(6, s.size() - 7));
        if (!ident(c.role)) { err = "bad role in " + s; return false; }
        return true;
    }
    if (s.rfind("has_tag(", 0) == 0 && s.back() == ')') {
        c.kind = Clause::has_tag;
        std::string in = s.substr(8, s.size() - 9);
        size_t comma = in.find(',');
        if (comma == std::string::npos) { err = "has_tag needs a role and a tag: " + s; return false; }
        c.role = trim(in.substr(0, comma));
        c.tag = trim(in.substr(comma + 1));
        return ident(c.role) && !c.tag.empty() ? true : (err = "bad has_tag: " + s, false);
    }
    static const char* const ops[] = {">=", "<=", "==", "!=", ">", "<"};
    for (const char* op : ops) {
        size_t at = s.find(op);
        if (at == std::string::npos) continue;
        std::string path = trim(s.substr(0, at));
        c.op = op;
        c.value = trim(s.substr(at + std::string(op).size()));
        if (c.value.size() >= 2 && c.value.front() == '"' && c.value.back() == '"') c.value = c.value.substr(1, c.value.size() - 2);
        size_t dot = path.find('.');
        if (dot == std::string::npos) { err = "path needs role.field: " + s; return false; }
        c.role = path.substr(0, dot);
        c.field = path.substr(dot + 1);
        if (!ident(c.role) || !ident(c.field) || c.value.empty()) { err = "cannot parse clause: " + s; return false; }
        return true;
    }
    err = "cannot parse clause: " + s;
    return false;
}

}

bool parse_pred(const std::string& text, Pred& out, std::string& err) {
    out = Pred();
    out.text = text;
    if (text.find(" or ") != std::string::npos) { err = "`or` is not in the grammar: " + text; return false; }
    for (size_t at = 0; at <= text.size();) {
        size_t next = text.find(" and ", at);
        std::string part = text.substr(at, next == std::string::npos ? std::string::npos : next - at);
        Clause c;
        if (!parse_clause(part, c, err)) return false;
        out.clauses.push_back(c);
        if (next == std::string::npos) break;
        at = next + 5;
    }
    return true;
}

bool compare(const Value& v, const std::string& op, const std::string& rhs) {
    if (!v.known) return false;
    double a = 0, b = 0;
    bool numeric = false;
    if (v.is_num && number(rhs, b)) { a = v.num; numeric = true; }
    else if (v.is_num && rank(rhs) >= 0) { a = v.num; b = rank(rhs); numeric = true; }
    else if (!v.is_num && rank(v.str) >= 0 && rank(rhs) >= 0) { a = rank(v.str); b = rank(rhs); numeric = true; }
    if (!numeric) {
        if (op == "==") return v.str == rhs;
        if (op == "!=") return v.str != rhs;
        return false;
    }
    if (op == ">=") return a >= b;
    if (op == "<=") return a <= b;
    if (op == ">") return a > b;
    if (op == "<") return a < b;
    if (op == "==") return a == b;
    return a != b;
}

bool eval(const Pred& p, const PredEnv& env) {
    for (auto& c : p.clauses) {
        bool r = false;
        if (c.kind == Clause::bound) r = env.bound(c.role);
        else if (c.kind == Clause::has_tag) r = env.has_tag(c.role, c.tag);
        else if (int m = env.member ? env.member(c.role, c.field, c.value) : -1; m >= 0) {
            if (c.op != "==" && c.op != "!=") return false;
            r = (m == 1) == (c.op == "==");
        } else {
            Value v = env.value(c.role, c.field);
            if (!v.known) return false;
            r = compare(v, c.op, c.value);
        }
        if (r == c.neg) return false;
    }
    return true;
}

std::vector<std::string> pred_roles(const Pred& p) {
    std::vector<std::string> out;
    for (auto& c : p.clauses) out.push_back(c.role);
    return out;
}

}
