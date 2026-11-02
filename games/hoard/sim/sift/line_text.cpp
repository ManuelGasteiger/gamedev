#include <algorithm>
#include "line_text.h"
#include "sift_util.h"
#include "ages/names.h"
#include "ages/world_util.h"
#include "text/render.h"

// pat.* keys from data/text/<lang>/patterns.json. A missing key falls back (pat.x.line.fell -> pat.x.line; a closed
// callback to the pattern's own line; pat.x.cb.cut -> pat.cb.cut), so a pattern needs only its plain line.
// Without its cause a key is tried as <key>.bare first; a line that carries no cause has no .bare.

namespace ages {

namespace {

text::Entity entity(const World& w, const std::string& id) {
    text::Entity e;
    e.kind = find_person(w, id) ? "person" : (find_region(w, id) ? "region" : "faction");
    e.name = display_name(w, id);
    if (e.kind == "person") {
        const Person* x = find_person(w, id);
        e.sex = x && !x->sex.empty() ? x->sex : "m";
    }
    return e;
}

// the keys to try, most specific first
std::vector<std::string> keys_for(const std::string& key) {
    std::vector<std::string> out;
    size_t dot = key.find('.', 4);
    std::string id = key.substr(4, dot == std::string::npos ? std::string::npos : dot - 4);
    std::string rest = dot == std::string::npos ? "" : key.substr(dot + 1);
    std::string base = "pat." + id;
    auto outcome = [&](const std::string& prefix) { return rest.size() > prefix.size() ? rest.substr(prefix.size() + 1) : std::string(); };
    if (rest.rfind("cb.closed", 0) == 0) {
        std::string o = outcome("cb.closed");
        if (!o.empty()) out.push_back(base + ".cb.closed." + o);
        out.push_back(base + ".cb.closed");
        if (!o.empty()) out.push_back(base + ".line." + o);
        out.push_back(base + ".line");
        out.push_back("pat.cb.closed");
    } else if (rest == "cb.cut") {
        out = {base + ".cb.cut", "pat.cb.cut"};
    } else if (rest == "cb.advanced") {
        out = {base + ".cb.advanced", base + ".open", "pat.cb.advanced"};
    } else if (rest.rfind("line", 0) == 0) {
        std::string o = outcome("line");
        if (!o.empty()) out.push_back(base + ".line." + o);
        out.push_back(base + ".line");
    } else if (rest.rfind("fold", 0) == 0 && rest != "fold.open" && id != "fold") {
        std::string o = outcome("fold");
        if (!o.empty()) out.push_back(base + ".fold." + o);
        out.push_back(base + ".fold");
    } else out.push_back(key);
    return out;
}

const std::string* lookup(const text::Bundle& b, const std::string& key, bool with_cause) {
    for (auto& k : keys_for(key)) {
        if (!with_cause)
            if (const std::string* s = text::find_string(b, k + ".bare")) return s;
        if (const std::string* s = text::find_string(b, k)) return s;
    }
    return nullptr;
}

std::string joined(const World& w, const text::Bundle& b, const std::string& ids) {
    std::vector<std::string> names;
    for (size_t at = 0; at <= ids.size();) {
        size_t end = std::min(ids.find(',', at), ids.size());
        if (end > at) names.push_back(display_name(w, ids.substr(at, end - at)));
        at = end + 1;
    }
    const std::string* a = text::find_string(b, "pat.list.and");
    std::string out;
    for (size_t i = 0; i < names.size(); ++i) {
        if (i) out += i + 1 == names.size() ? " " + (a ? *a : std::string("and")) + " " : ", ";
        out += names[i];
    }
    return out;
}

}

bool sift_text_exists(const Params& p, const std::string& key) {
    const text::Bundle* b = p.sift.texts ? p.sift.texts->get("en") : nullptr;
    return b && lookup(*b, key, true);
}

std::string sift_line_text(const World& w, const Params& p, const SiftLine& l, const std::string& lang, bool with_cause) {
    if (l.text_key.rfind("pat.", 0) != 0 || !p.sift.texts) return "";
    const text::Bundle* b = p.sift.texts->get(lang);
    if (!b) return "";
    const std::string* tmpl = lookup(*b, l.text_key, with_cause);
    if (!tmpl) return "";
    text::Ctx c;
    for (auto& kv : l.params) {
        if (kv.first.rfind("role:", 0) == 0) c.roles[kv.first.substr(5)] = entity(w, kv.second);
        else if (kv.first.rfind("ev:", 0) == 0) {
            const Event* e = sift_event(w, std::atoi(kv.second.c_str()));
            if (!e) continue;
            text::Entity x;
            x.kind = "act";
            x.fields["year"] = std::to_string(e->year);   // a year, never grouped like a number
            x.numbers["years_ago"] = w.year - e->year;
            if (find_region(w, e->object)) x.fields["place"] = display_name(w, e->object);
            else if (find_region(w, e->subject)) x.fields["place"] = display_name(w, e->subject);
            std::string court = court_of(w, e->subject);
            if (!court.empty()) x.fields["court"] = display_name(w, court);
            c.roles[kv.first.substr(3)] = x;
        } else c.vars[kv.first] = kv.second;
    }
    if (l.params.count("ago")) {
        int ago = std::atoi(l.params.at("ago").c_str());
        std::string k = ago <= 5 ? "pat.ago." + std::to_string(ago) : "pat.ago.year";
        const std::string* s = text::find_string(*b, k);
        std::string v = s ? *s : "";
        for (size_t at; (at = v.find("{year}")) != std::string::npos;) v.replace(at, 6, l.params.count("shown_year") ? l.params.at("shown_year") : "");
        c.vars["ago"] = v;
    }
    int nth = l.params.count("nth") ? std::atoi(l.params.at("nth").c_str()) : 1;
    const std::string* ord = nth > 1 ? text::find_string(*b, "pat.nth." + std::to_string(std::min(nth, 9))) : nullptr;
    c.vars["nth"] = ord ? *ord : "";
    if (l.params.count("era")) {
        size_t i = static_cast<size_t>(std::atoi(l.params.at("era").c_str()));
        c.vars["era"] = i < p.texts.era_names.size() ? p.texts.era_names[i] : l.params.at("era");
    }
    if (l.params.count("fold_names")) {
        c.vars["names"] = joined(w, *b, l.params.at("fold_names"));
        if (const Event* e = sift_event(w, l.event)) c.vars["year"] = std::to_string(e->year);
    }
    if (l.text_key.rfind("pat.", 0) == 0 && l.text_key.rfind("pat.cb.", 0) != 0 && l.text_key.rfind("pat.age.", 0) != 0) {
        std::string id = l.text_key.substr(4, l.text_key.find('.', 4) - 4);
        const std::string* t = with_cause ? nullptr : text::find_string(*b, "pat." + id + ".title.bare");
        if (!t) t = text::find_string(*b, "pat." + id + ".title");
        if (t) c.vars["title"] = text::render(*b, *t, c);
    }
    std::vector<std::string> problems;
    return text::render(*b, *tmpl, c, &problems);
}

}
