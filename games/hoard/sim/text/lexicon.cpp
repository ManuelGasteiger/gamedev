#include "lexicon.h"

namespace text {

using nlohmann::json;

namespace {

const json* child(const json& j, const std::string& k) {
    if (!j.is_object()) return nullptr;
    auto it = j.find(k);
    return it == j.end() ? nullptr : &*it;
}

const json* first(const json& j, std::initializer_list<std::string> keys) {
    for (auto& k : keys)
        if (const json* c = child(j, k)) return c;
    return nullptr;
}

bool str(const json* j, std::string& out) {
    if (!j || !j->is_string()) return false;
    out = j->get<std::string>();
    return true;
}

bool starts_with_any(const json* list, const std::string& word) {
    if (!list || !list->is_array()) return false;
    for (auto& w : *list)
        if (w.is_string() && word.compare(0, w.get_ref<const std::string&>().size(), w.get_ref<const std::string&>()) == 0) return true;
    return false;
}

// English a/an: by the first letter, with vowel_words (heir, hour) and consonant_words (unicorn, one) as exceptions
bool vowel_sound(const json& lex, const std::string& word) {
    std::string lower = word;
    for (auto& ch : lower) if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch + 32);
    if (starts_with_any(child(lex, "vowel_words"), lower)) return true;
    if (starts_with_any(child(lex, "consonant_words"), lower)) return false;
    std::string v;
    return str(child(lex, "vowel_start"), v) && v.find(word[0]) != std::string::npos;
}

}

bool is_noun(const json& lex, const std::string& key) {
    const json* n = child(lex, "nouns");
    return n && child(*n, key);
}

bool is_unit(const json& lex, const std::string& name) {
    const json* u = child(lex, "units");
    return u && child(*u, name);
}

NounForm noun_form(const json& lex, const std::string& key, const std::string& sex, const std::string& det, const std::string& cas) {
    NounForm f;
    const json* nouns = child(lex, "nouns");
    const json* entry = nouns ? child(*nouns, key) : nullptr;
    const json* forms = entry ? first(*entry, {sex.empty() ? "*" : sex, "*", "m"}) : nullptr;
    if (!forms) return f;
    std::string c = cas.empty() ? "nom" : cas;
    std::string d = det.empty() ? "none" : det;
    const json* hit = first(*forms, {d + "." + c, d == "poss2" ? "indef." + c : d + "." + c, c, "nom"});
    f.found = str(hit, f.text);
    str(child(*forms, "g"), f.gender);
    return f;
}

std::string article(const json& lex, const std::string& det, const std::string& gender, const std::string& cas, const std::string& next_word) {
    const json* arts = child(lex, "articles");
    const json* d = arts ? child(*arts, det) : nullptr;
    const json* g = d ? first(*d, {gender, "*"}) : nullptr;
    std::string out;
    if (!g || !str(first(*g, {cas.empty() ? "nom" : cas, "*"}), out)) return "";
    const json* vf = child(lex, "vowel_forms");
    if (vf && !next_word.empty() && vowel_sound(lex, next_word)) str(child(*vf, out), out);
    return out;
}

bool pronoun(const json& lex, const std::string& field, const std::string& gender, const std::string& cas, const std::string& possessed,
             std::string& out) {
    const json* all = child(lex, "pronouns");
    const json* set = all ? first(*all, {gender, "*"}) : nullptr;
    if (!set) return false;
    std::string c = cas;
    if (c.empty() && field == "obj") str(child(lex, "obj_case"), c);
    if (!str(first(*set, {field + "." + c, field}), out)) return false;
    if (field != "poss") return true;
    const json* endings = child(lex, "poss_endings");
    const json* e = endings ? first(*endings, {possessed.empty() ? "m" : possessed, "*"}) : nullptr;
    std::string end;
    if (e && str(first(*e, {cas.empty() ? "nom" : cas, "*"}), end)) out += end;
    return true;
}

bool unit(const json& lex, const std::string& name, const std::string& category, const std::string& cas, std::string& out) {
    const json* units = child(lex, "units");
    const json* u = units ? child(*units, name) : nullptr;
    const json* cat = u ? first(*u, {category, "other"}) : nullptr;
    return cat && str(first(*cat, {cas.empty() ? "nom" : cas, "*"}), out);
}

std::string noun_gen_suffix(const json& lex, const std::string& gender) {
    const json* s = child(lex, "noun_gen_suffix");
    std::string out;
    if (s) str(first(*s, {gender, "*"}), out);
    return out;
}

}
