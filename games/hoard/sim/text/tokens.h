#pragma once
#include <string>
#include <vector>

namespace text {

struct Choice {
    std::string key, body;   // body is a template of its own
};

// {var}, {role.field}, {role.field:mod.mod}, {role.field?k=...|k=...} (select), {role.field:plural(one=...,other=...)}
struct Token {
    std::string source;   // as written, braces included
    std::string role, field;
    std::vector<std::string> mods;
    bool select = false, plural = false;
    std::vector<Choice> choices;
};

struct Piece {
    bool is_token = false;
    std::string literal;
    Token token;
};

std::vector<Piece> parse(const std::string& tmpl, std::string& err);

}
