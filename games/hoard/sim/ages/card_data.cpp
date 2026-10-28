#include <algorithm>
#include "card_preds.h"
#include "cards.h"
#include "json.h"

namespace ages {

using nlohmann::json;

namespace {

template <class T>
void get(const json& j, const char* key, T& out) {
    auto it = j.find(key);
    if (it != j.end() && !it->is_null()) out = it->get<T>();
}

std::vector<std::string> strings(const json& v) {
    std::vector<std::string> out;
    if (v.is_string()) out.push_back(v.get<std::string>());
    else if (v.is_array()) for (auto& x : v) out.push_back(x.get<std::string>());
    return out;
}

// checked in this order: a let, find or person condition also carries "field" or "tag" keys
const char* const kPredTypes[] = {"any", "let", "find", "person", "event", "var", "adjacent", "at_war", "lineage", "field", "tag"};

bool pred_from(const json& j, CardPred& p, std::string& err);

bool preds_from(const json& arr, std::vector<CardPred>& out, std::string& err) {
    if (arr.is_object()) { CardPred p; if (!pred_from(arr, p, err)) return false; out.push_back(p); return true; }
    for (auto& v : arr) {
        CardPred p;
        if (!pred_from(v, p, err)) return false;
        out.push_back(p);
    }
    return true;
}

bool pred_from(const json& j, CardPred& p, std::string& err) {
    for (const char* t : kPredTypes) if (j.contains(t)) { p.type = t; break; }
    if (p.type.empty()) { err = "card condition without a type: " + j.dump(); return false; }
    if (p.type == "any") return preds_from(j["any"], p.any, err);
    if (p.type == "event") p.kinds = strings(j["event"]);
    else if (j[p.type].is_string()) p.name = j[p.type].get<std::string>();
    get(j, "on", p.on);
    get(j, "bind", p.bind);
    get(j, "absent", p.absent);
    get(j, "optional", p.optional);
    get(j, "min_age_years", p.min_age);
    get(j, "within_years", p.within);
    get(j, "min_count", p.min_count);
    get(j, "min_magnitude", p.min_magnitude);
    get(j, "op", p.op);
    get(j, "value", p.value);
    get(j, "of", p.of);
    if (j.contains("in")) p.in = strings(j["in"]);
    get(j, "str", p.str);
    get(j, "self_as", p.self_as);
    get(j, "other_from", p.other_from);
    get(j, "region_from", p.region_from);
    get(j, "person_from", p.person_from);
    get(j, "event_tag", p.event_tag);
    get(j, "ref", p.ref);
    get(j, "tag", p.tag);
    get(j, "adjacent_to", p.adjacent_to);
    get(j, "race", p.race);
    if (p.type == "let") get(j, "field", p.str);
    get(j, "this_sleep", p.this_sleep);   // v0.4 hooks (S3)
    for (const char* op : {"lt", "lte", "gt", "gte", "eq", "ne"})
        if (j.contains(op)) {
            p.op = op;
            if (j[op].is_number()) p.value = j[op].get<double>();
            else p.str = j[op].get<std::string>();
        }
    if (!p.in.empty() && p.op.empty()) p.op = "in";
    return true;
}

bool effect_from(const json& j, CardEffect& e, std::string& err) {
    get(j, "do", e.type);
    if (e.type.empty()) { err = "card effect without \"do\": " + j.dump(); return false; }
    for (auto it = j.begin(); it != j.end(); ++it) {
        if (it.key() == "do" || it.key() == "if") continue;
        if (it.key() == "tags") e.tags = strings(it.value());
        else if (it.value().is_object()) for (auto t = it.value().begin(); t != it.value().end(); ++t) e.table[t.key()] = t.value().get<double>();
        else if (it.value().is_string()) e.s[it.key()] = it.value().get<std::string>();
        else if (it.value().is_boolean()) e.n[it.key()] = it.value().get<bool>() ? 1.0 : 0.0;
        else if (it.value().is_number()) e.n[it.key()] = it.value().get<double>();
    }
    return !j.contains("if") || preds_from(j["if"], e.when, err);
}

bool effects_from(const json& arr, std::vector<CardEffect>& out, std::string& err) {
    for (auto& v : arr) {
        CardEffect e;
        if (!effect_from(v, e, err)) return false;
        out.push_back(e);
    }
    return true;
}

bool card_from(const json& j, CardDef& c, std::string& err) {
    get(j, "id", c.id);
    if (c.id.empty()) { err = "card without id"; return false; }
    get(j, "scope", c.scope);
    if (j.contains("era")) { c.era_min = j["era"][0].get<int>(); c.era_max = j["era"][1].get<int>(); }
    if (j.contains("phase")) { c.phase_min = j["phase"][0].get<int>(); c.phase_max = j["phase"][1].get<int>(); }
    if (j.contains("trigger") && !preds_from(j["trigger"], c.trigger, err)) return false;
    get(j, "chance", c.chance);
    if (j.contains("chance_mult"))
        for (auto& v : j["chance_mult"]) {
            ChanceMult m;
            get(v, "mult", m.mult);
            if (v.contains("if") && !preds_from(v["if"], m.when, err)) return false;
            c.chance_mult.push_back(m);
        }
    get(j, "once", c.once);
    get(j, "cooldown_decades", c.cooldown);
    if (j.contains("effects") && !effects_from(j["effects"], c.effects, err)) return false;
    if (j.contains("branches"))
        for (auto& v : j["branches"]) {
            CardBranch b;
            get(v, "weight", b.weight);
            get(v, "text_key", b.text_key);
            get(v, "tag", b.tag);
            if (v.contains("if") && !preds_from(v["if"], b.when, err)) return false;
            if (v.contains("boost"))
                for (auto& bv : v["boost"]) {
                    WeightBoost wb;
                    get(bv, "add", wb.add);
                    get(bv, "mult", wb.mult);
                    if (bv.contains("if") && !preds_from(bv["if"], wb.when, err)) return false;
                    b.boost.push_back(wb);
                }
            if (v.contains("effects") && !effects_from(v["effects"], b.effects, err)) return false;
            c.branches.push_back(b);
        }
    get(j, "text_key", c.text_key);
    if (c.text_key.empty()) c.text_key = c.id;
    get(j, "cause", c.cause);
    if (c.cause.empty()) c.cause = c.id;
    get(j, "object", c.object);
    get(j, "thread", c.thread);
    if (j.contains("tags")) c.tags = strings(j["tags"]);
    get(j, "kind", c.kind);
    get(j, "subject", c.subject);
    get(j, "major", c.major);
    get(j, "needs_dragon", c.needs_dragon);
    return true;
}

}

bool card_preds_from_json(const json& arr, std::vector<CardPred>& out, std::string& err) { return preds_from(arr, out, err); }   // v0.4 hooks (S3)

bool cards_from_json(const std::string& text, std::vector<CardDef>& out, std::string& err) {
    if (text.empty()) return true;
    json j = json::parse(text, nullptr, false);
    if (j.is_discarded()) { err = "invalid json in events"; return false; }
    // a file holds one card or an array of cards; data_io.cpp joins the files into one array
    std::vector<json> items;
    std::vector<json> todo = {j};
    while (!todo.empty()) {
        json v = todo.back();
        todo.pop_back();
        if (v.is_array()) for (auto it = v.rbegin(); it != v.rend(); ++it) todo.push_back(*it);
        else if (v.is_object() && v.contains("id")) items.push_back(v);
        else if (v.is_object()) for (auto it = v.begin(); it != v.end(); ++it) todo.push_back(it.value());
    }
    for (auto& v : items) {
        CardDef c;
        if (!card_from(v, c, err)) { err = "events: " + err; return false; }
        out.push_back(c);
    }
    std::sort(out.begin(), out.end(), [](const CardDef& a, const CardDef& b) { return a.id < b.id; });
    return true;
}

}
