#pragma once
#include <functional>
#include <string>
#include "bank.h"

namespace scn {

// a field's value: a number, or a word (ordinal words compare by their rank in the closed lists)
struct Value {
    bool known = false, is_num = false;
    double num = 0;
    std::string str;
};

bool parse_pred(const std::string& text, Pred& out, std::string& err);

struct PredEnv {
    std::function<bool(const std::string&)> bound;
    std::function<Value(const std::string& role, const std::string& field)> value;
    std::function<bool(const std::string& role, const std::string& tag)> has_tag;
    // set-valued fields (person.trait, dragon.talents): 1 or 0 for membership of the value, -1 for any other field
    std::function<int(const std::string& role, const std::string& field, const std::string& value)> member;
};

// a clause on an unbound role or an unknown field is false (not true under `not`)
bool eval(const Pred& p, const PredEnv& env);
bool compare(const Value& v, const std::string& op, const std::string& rhs);
std::vector<std::string> pred_roles(const Pred& p);

}
