#include "bundle.h"
#include <cstring>
#include "tokens.h"

namespace text {

using nlohmann::json;

const char* const kStringParts[4] = {"ui", "scenarios", "patterns", "endings"};

const json& part(const Bundle& b, const char* name) {
    static const json empty = json::object();
    auto it = b.j.find(name);
    return it == b.j.end() ? empty : *it;
}

const std::string* find_string(const Bundle& b, const std::string& key) {
    for (const char* p : kStringParts) {
        const json& t = part(b, p);
        auto it = t.find(key);
        if (it != t.end() && it->is_string()) return &it->get_ref<const std::string&>();
    }
    return nullptr;
}

bool bundle_from_json(const std::string& text, Bundle& out, std::string& err) {
    out.j = json::parse(text, nullptr, false);
    if (out.j.is_discarded() || !out.j.is_object()) { err = "text bundle is not a json object"; return false; }
    out.lang = out.j.value("lang", std::string());
    if (out.lang.empty()) { err = "text bundle without lang"; return false; }
    return true;
}

namespace {

std::string accent(const std::string& s, size_t& letters) {
    static const char* const from = "aeiouycnAEIOUYCN";
    static const char* const to[] = {"á", "é", "í", "ö", "ü", "ý", "ç", "ñ", "Å", "É", "Î", "Ö", "Ü", "Ý", "Ç", "Ñ"};
    std::string out;
    for (char c : s) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) ++letters;
        const char* hit = c ? std::strchr(from, c) : nullptr;
        if (hit) out += to[hit - from];
        else out += c;
    }
    return out;
}

std::string pseudo_template(const std::string& tmpl, bool brackets) {
    std::string err;
    std::vector<Piece> pieces = parse(tmpl, err);
    if (!err.empty()) return tmpl;
    size_t letters = 0;
    std::string out;
    for (auto& p : pieces) out += p.is_token ? p.token.source : accent(p.literal, letters);
    if (!brackets) return out;
    return "[" + out + " " + std::string((letters * 35 + 99) / 100, '~') + "]";
}

void pseudo_values(json& j, bool brackets) {
    if (j.is_string()) j = pseudo_template(j.get<std::string>(), brackets);
    else if (j.is_object() || j.is_array())
        for (auto& v : j) pseudo_values(v, brackets);
}

}

Bundle pseudo_bundle(const Bundle& en) {
    Bundle b = en;
    b.lang = "pseudo";
    b.j["lang"] = "pseudo";
    for (const char* p : kStringParts)
        if (b.j.contains(p)) pseudo_values(b.j[p], true);
    if (b.j.contains("lexicon")) {
        json& lex = b.j["lexicon"];
        for (const char* k : {"nouns", "units"})
            if (lex.contains(k)) pseudo_values(lex[k], false);
    }
    return b;
}

const Bundle* Bundles::get(const std::string& lang) const {
    auto it = by_lang.find(lang);
    return it == by_lang.end() ? nullptr : &it->second;
}

bool bundles_from_json(const std::map<std::string, std::string>& texts, Bundles& out, std::string& err) {
    out.by_lang.clear();
    for (auto& t : texts) {
        Bundle b;
        if (!bundle_from_json(t.second, b, err)) { err = t.first + ": " + err; return false; }
        out.by_lang[b.lang] = b;
    }
    auto en = out.by_lang.find("en");
    if (en == out.by_lang.end()) { err = "no English text bundle"; return false; }
    out.by_lang["pseudo"] = pseudo_bundle(en->second);
    return true;
}

}
