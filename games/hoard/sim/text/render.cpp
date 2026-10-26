#include "render.h"
#include <set>
#include "lexicon.h"
#include "names.h"
#include "numbers.h"
#include "tokens.h"

namespace text {

using nlohmann::json;

namespace {

const std::set<std::string> kDets = {"def", "indef", "poss2"};
const std::set<std::string> kCases = {"nom", "acc", "dat", "gen"};
const std::set<std::string> kGenders = {"m", "f", "n", "pl"};
const std::set<std::string> kPronouns = {"subj", "obj", "poss", "refl"};

struct Mods {
    std::string det, cas, possessed, unit, noun;
};

struct Renderer {
    const Bundle& b;
    const Ctx& c;
    std::vector<std::string>* problems;

    const json& lex() const { return part(b, "lexicon"); }
    void problem(const std::string& what) { if (problems) problems->push_back(what); }

    bool mods(const Token& t, Mods& m) {
        for (auto& x : t.mods) {
            if (kDets.count(x)) m.det = x;
            else if (kCases.count(x)) m.cas = x;
            else if (kGenders.count(x)) m.possessed = x;
            else if (is_unit(lex(), x)) m.unit = x;
            else if (is_noun(lex(), x)) m.noun = x;
            else { problem(t.source + ": unknown modifier " + x); return false; }
        }
        return true;
    }

    std::string with_article(const std::string& det, const std::string& gender, const std::string& cas, const std::string& word) {
        std::string art = det.empty() ? "" : article(lex(), det, gender, cas, word);
        return art.empty() ? word : art + " " + word;
    }

    static std::string gender_of(const Entity& e) {
        if (e.kind == "person") return e.sex;
        return e.gender.empty() ? "n" : e.gender;
    }

    bool one(const Token& t, std::string& out) {
        if (t.field.empty()) {
            auto v = c.vars.find(t.role);
            if (v == c.vars.end()) { problem(t.source + ": unbound"); return false; }
            out = v->second;
            return true;
        }
        auto r = c.roles.find(t.role);
        if (r == c.roles.end()) { problem(t.source + ": unbound role"); return false; }
        const Entity& e = r->second;
        if (t.select) return select(t, e, out);
        if (t.plural) return plural(t, e, out);
        Mods m;
        if (!mods(t, m)) return false;
        if (t.field == "name") {
            if (e.name.empty()) { problem(t.source + ": no name"); return false; }
            if (m.det.empty()) out = name_in_case(part(b, "names"), e.name, m.cas);
            else out = with_article(m.det, gender_of(e), m.cas, e.name + (m.cas == "gen" ? noun_gen_suffix(lex(), gender_of(e)) : ""));
            return true;
        }
        if (t.field == "noun") {
            std::string key = m.noun.empty() ? e.noun : m.noun;
            NounForm f = noun_form(lex(), key, e.sex, m.det, m.cas);
            if (!f.found) { problem(t.source + ": no noun " + key); return false; }
            out = with_article(m.det, f.gender, m.cas, f.text);
            return true;
        }
        if (kPronouns.count(t.field)) {
            if (pronoun(lex(), t.field, gender_of(e), m.cas, m.possessed, out)) return true;
            problem(t.source + ": no pronoun");
            return false;
        }
        auto n = e.numbers.find(t.field);
        if (n != e.numbers.end()) {
            std::string num = format_int(n->second, part(b, "locale"));
            if (m.unit.empty()) { out = num; return true; }
            if (!unit(lex(), m.unit, plural_category(n->second, part(b, "locale")), m.cas, out)) { problem(t.source + ": no unit form"); return false; }
            for (size_t at; (at = out.find("{n}")) != std::string::npos;) out.replace(at, 3, num);
            return true;
        }
        auto f = e.fields.find(t.field);
        if (f != e.fields.end()) { out = f->second; return true; }
        problem(t.source + ": unknown field");
        return false;
    }

    bool select(const Token& t, const Entity& e, std::string& out) {
        std::string key;
        if (t.field == "gender") key = gender_of(e);
        else if (auto f = e.fields.find(t.field); f != e.fields.end()) key = f->second;
        const Choice* hit = nullptr;
        for (auto& ch : t.choices) if (ch.key == key || (!hit && ch.key == "*")) hit = &ch;
        if (!hit) { problem(t.source + ": no branch for " + key); return false; }
        out = run(hit->body);
        return true;
    }

    bool plural(const Token& t, const Entity& e, std::string& out) {
        auto n = e.numbers.find(t.field);
        if (n == e.numbers.end()) { problem(t.source + ": not a number"); return false; }
        std::string cat = plural_category(n->second, part(b, "locale"));
        const Choice* hit = nullptr;
        for (auto& ch : t.choices) if (ch.key == cat || (!hit && ch.key == "other")) hit = &ch;
        if (!hit) { problem(t.source + ": no " + cat); return false; }
        out = run(hit->body);
        std::string num = format_int(n->second, part(b, "locale"));
        for (size_t at = 0; (at = out.find('#', at)) != std::string::npos; at += num.size()) out.replace(at, 1, num);
        return true;
    }

    static bool sentence_start(const std::string& s) {
        size_t i = s.size();
        while (i > 0) {
            unsigned char ch = static_cast<unsigned char>(s[i - 1]);
            if (ch == ' ' || ch == '"' || ch == '(' || ch == '\'') { --i; continue; }
            if (i >= 3 && (s.compare(i - 3, 3, "\xE2\x80\x9E") == 0 || s.compare(i - 3, 3, "\xE2\x80\x9C") == 0)) { i -= 3; continue; }   // „ “
            if (i >= 2 && s.compare(i - 2, 2, "\xC2\xBB") == 0) { i -= 2; continue; }   // »
            return ch == '.' || ch == '!' || ch == '?';
        }
        return true;
    }

    static void cap_first(std::string& s) {
        if (s.empty()) return;
        unsigned char ch = static_cast<unsigned char>(s[0]);
        if (ch >= 'a' && ch <= 'z') s[0] = static_cast<char>(ch - 32);
        else if (ch == 0xC3 && s.size() > 1) {
            unsigned char nx = static_cast<unsigned char>(s[1]);
            if (nx == 0xA4 || nx == 0xB6 || nx == 0xBC) s[1] = static_cast<char>(nx - 0x20);   // ä ö ü
        }
    }

    std::string run(const std::string& tmpl) {
        std::string err;
        std::vector<Piece> pieces = parse(tmpl, err);
        if (!err.empty()) { problem(err); return tmpl; }
        std::string out;
        for (auto& p : pieces) {
            if (!p.is_token) { out += p.literal; continue; }
            std::string piece;
            if (!one(p.token, piece)) { out += p.token.source; continue; }
            if (sentence_start(out)) cap_first(piece);
            out += piece;
        }
        return out;
    }
};

}

std::string render(const Bundle& b, const std::string& tmpl, const Ctx& c, std::vector<std::string>* problems) {
    Renderer r{b, c, problems};
    return r.run(tmpl);
}

std::string render_key(const Bundles& all, const std::string& lang, const std::string& key, const Ctx& c, std::vector<std::string>* problems) {
    const Bundle* b = all.get(lang);
    const Bundle* en = all.get("en");
    const std::string* s = b ? find_string(*b, key) : nullptr;
    if (!s) {
        if (all.release && en && b != en) return render_key(all, "en", key, c, problems);
        if (problems) problems->push_back(lang + ": missing " + key);
        return key;
    }
    std::vector<std::string> mine;
    std::string out = render(*b, *s, c, &mine);
    if (!mine.empty() && all.release && en && b != en) return render_key(all, "en", key, c, problems);
    if (problems) problems->insert(problems->end(), mine.begin(), mine.end());
    return out;
}

}
