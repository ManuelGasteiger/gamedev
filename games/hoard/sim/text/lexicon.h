#pragma once
#include <string>
#include "ages/json.h"

namespace text {

// data/text/<lang>/lexicon.json:
//   nouns.<key>.<sex m|f|*>: {"g": grammatical gender, "<det>.<case>" | "<case>": form}
//   articles.<det>.<gender|*>.<case|*>, vowel_forms (English a -> an), vowel_start
//   pronouns.<gender>.<field>[.<case>], obj_case, poss_endings.<possessed gender>.<case>
//   units.<unit>.<plural category>.<case|*> with {n}; noun_gen_suffix.<gender>
struct NounForm {
    std::string text, gender;
    bool found = false;
};

NounForm noun_form(const nlohmann::json& lex, const std::string& key, const std::string& sex, const std::string& det, const std::string& cas);
std::string article(const nlohmann::json& lex, const std::string& det, const std::string& gender, const std::string& cas, const std::string& next_word);
bool pronoun(const nlohmann::json& lex, const std::string& field, const std::string& gender, const std::string& cas,
             const std::string& possessed, std::string& out);
bool unit(const nlohmann::json& lex, const std::string& name, const std::string& category, const std::string& cas, std::string& out);
bool is_unit(const nlohmann::json& lex, const std::string& name);
bool is_noun(const nlohmann::json& lex, const std::string& key);
std::string noun_gen_suffix(const nlohmann::json& lex, const std::string& gender);

}
