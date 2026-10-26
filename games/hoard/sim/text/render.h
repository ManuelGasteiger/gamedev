#pragma once
#include <map>
#include <string>
#include <vector>
#include "bundle.h"

namespace text {

// what a role is bound to when a string is rendered; the sim fills these from records, never from text
struct Entity {
    std::string kind;     // person | faction | region | item | act
    std::string name;     // nominative
    std::string noun;     // person: the closed noun key of the role (envoy, scribe, ...)
    std::string sex;      // person: m | f
    std::string gender;   // item: grammatical gender in this language (m | f | n)
    std::map<std::string, std::string> fields;   // place, verb, ... (already in this language)
    std::map<std::string, long long> numbers;    // years_ago, ...
};

struct Ctx {
    std::map<std::string, Entity> roles;
    std::map<std::string, std::string> vars;   // finished clauses such as {why}
};

// problems lists every token that could not be rendered; such a token stays as written
std::string render(const Bundle& b, const std::string& tmpl, const Ctx& c, std::vector<std::string>* problems = nullptr);

// the string under key in lang; a missing key or a failed token falls back to English in release builds and shows the
// key (or the token) in dev builds
std::string render_key(const Bundles& all, const std::string& lang, const std::string& key, const Ctx& c,
                       std::vector<std::string>* problems = nullptr);

}
