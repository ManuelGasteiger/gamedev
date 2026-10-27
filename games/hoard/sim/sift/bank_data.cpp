#include "ages/data.h"
#include "ages/chronicle.h"
#include "ages/json.h"
#include "text/bundle.h"

namespace ages {

using nlohmann::json;

namespace {

std::vector<std::string> strings(const json& j, const char* key) {
    std::vector<std::string> out;
    auto it = j.find(key);
    if (it == j.end()) return out;
    if (it->is_string()) out.push_back(it->get<std::string>());
    else for (auto& v : *it) out.push_back(v.get<std::string>());
    return out;
}

std::string str(const json& j, const char* key) {
    auto it = j.find(key);
    return it == j.end() || !it->is_string() ? "" : it->get<std::string>();
}

SiftMatch match_of(const json& j) {
    SiftMatch m;
    m.kinds = strings(j, "kind");
    m.subject = str(j, "subject");
    m.object = str(j, "object");
    m.tags_all = strings(j, "tags_all");
    m.tags_any = strings(j, "tags_any");
    m.tags_none = strings(j, "tags_none");
    if (j.contains("magnitude_gte")) { m.has_magnitude = true; m.magnitude_gte = j["magnitude_gte"].get<double>(); }
    return m;
}

int clause_index(const std::vector<SiftClause>& cs, const std::string& as) {
    for (size_t i = 0; i < cs.size(); ++i) if (cs[i].as == as) return static_cast<int>(i);
    return -1;
}

SiftRef ref_of(const json& j, const std::vector<SiftClause>& cs) {
    SiftRef r;
    r.clause = clause_index(cs, str(j, "ref"));
    if (j.contains("max_hops")) r.max_hops = j["max_hops"].get<int>();
    return r;
}

std::vector<std::pair<std::string, std::string>> pairs(const json& j, const char* key) {
    std::vector<std::pair<std::string, std::string>> out;
    auto it = j.find(key);
    if (it == j.end()) return out;
    for (auto kv = it->begin(); kv != it->end(); ++kv) out.emplace_back(kv.key(), kv.value().get<std::string>());
    return out;
}

bool pattern_of(const json& j, SiftPattern& p, std::string& err) {
    p.id = str(j, "id");
    if (j.contains("level")) p.level = j["level"].get<int>();
    if (j.contains("window_years")) p.window_years = j["window_years"].get<int>();
    if (j.contains("stakes_for_dragon")) p.stakes = j["stakes_for_dragon"].get<double>();
    if (j.contains("survives_death")) p.survives_death = j["survives_death"].get<bool>();
    for (auto& t : strings(j, "tags")) p.tags.insert(t);
    for (auto& c : j.value("clauses", json::array())) {
        SiftClause cl;
        cl.as = str(c, "as");
        if (c.contains("any_of")) for (auto& a : c["any_of"]) cl.alts.push_back(match_of(a));
        else cl.alts.push_back(match_of(c));
        if (c.contains("caused_by")) cl.caused_by = ref_of(c["caused_by"], p.clauses);
        if (c.contains("not_caused_by")) cl.not_caused_by = ref_of(c["not_caused_by"], p.clauses);
        if (c.contains("same_thread_as")) cl.same_thread_as = clause_index(p.clauses, str(c, "same_thread_as"));
        if (c.contains("within_years")) cl.within_years = c["within_years"].get<int>();
        cl.bind = pairs(c, "bind");
        cl.where = pairs(c, "where");
        p.clauses.push_back(cl);
    }
    if (p.clauses.size() < 2) { err = "pattern " + p.id + ": fewer than two clauses"; return false; }
    for (const char* key : {"unless", "unless_forever"})
        for (auto& u : j.value(key, json::array())) {
            SiftGuard g;
            g.m = match_of(u);
            if (u.contains("between")) {
                g.from = clause_index(p.clauses, u["between"][0].get<std::string>());
                g.to = clause_index(p.clauses, u["between"][1].get<std::string>());
            }
            (std::string(key) == "unless" ? p.unless : p.unless_forever).push_back(g);
        }
    p.once_per = strings(j, "once_per");
    if (j.contains("outcome_by"))
        for (auto it = j["outcome_by"].begin(); it != j["outcome_by"].end(); ++it) {
            std::string field = it.key();
            size_t dot = field.find('.');
            p.outcome_clause = clause_index(p.clauses, field.substr(0, dot));
            p.outcome_field = dot == std::string::npos ? "kind" : field.substr(dot + 1);
            for (auto o = it.value().begin(); o != it.value().end(); ++o) p.outcomes[o.key()] = o.value().get<std::string>();
        }
    const json& e = j.value("emit", json::object());
    p.emit_kind = str(e, "kind");
    p.emit_subject = str(e, "subject");
    p.emit_object = str(e, "object");
    p.emit_magnitude = str(e, "magnitude");
    for (auto& t : strings(e, "tags")) p.emit_tags.insert(t);
    if (e.contains("tier")) p.tier = e["tier"].get<int>();
    return true;
}

void constants_of(const json& j, SiftConstants& c) {
    auto get = [&](const char* k, auto& out) { auto it = j.find(k); if (it != j.end()) out = it->get<std::decay_t<decltype(out)>>(); };
    if (j.contains("weights")) {
        const json& w = j["weights"];
        auto gw = [&](const char* k, double& out) { auto it = w.find(k); if (it != w.end()) out = it->get<double>(); };
        gw("dragon", c.w.dragon); gw("stakes", c.w.stakes); gw("causal", c.w.causal); gw("continuity", c.w.continuity);
        gw("proximity", c.w.proximity); gw("surprise", c.w.surprise); gw("reversals", c.w.reversals); gw("irreversible", c.w.irreversible);
    }
    get("shown_end_bonus", c.shown_end_bonus); get("tier5_bonus", c.tier5_bonus);
    get("recent_headline_penalty", c.recent_headline_penalty); get("recent_headline_wakings", c.recent_headline_wakings);
    get("floor", c.floor); get("same_pattern_penalty", c.same_pattern_penalty); get("generation_years", c.generation_years);
    get("diversity_start", c.diversity_start); get("diversity_end", c.diversity_end); get("run_bonus_max", c.run_bonus_max);
    get("run_phase_wakings", c.run_phase_wakings); get("t_major", c.t_major); get("t_breather", c.t_breather); get("arc_k", c.arc_k);
    get("headline_pool", c.headline_pool); get("max_callbacks", c.max_callbacks); get("min_lines", c.min_lines);
    get("max_lines", c.max_lines); get("max_ages", c.max_ages); get("partial_cap", c.partial_cap); get("dragon_hops", c.dragon_hops);
    get("because_hops", c.because_hops); get("because_stop_importance", c.because_stop_importance);
    get("oral_reroll", c.oral_reroll); get("blame_dread", c.blame_dread); get("reversal_score", c.reversal_score);
    get("fortune", c.fortune);
    for (auto& k : strings(j, "irreversible")) c.irreversible.insert(k);
    for (auto& k : strings(j, "motivating")) c.motivating.insert(k);
    for (auto& k : strings(j, "harm")) c.harm.insert(k);
}

}

bool sift_bank_from_json(const DataJson& d, SiftBank& b, std::string& err) {
    b = SiftBank();
    json j = json::parse(d.constants, nullptr, false);
    if (!j.is_discarded() && j.contains("sift")) constants_of(j["sift"], b.c);
    if (!d.importance.empty()) {
        json imp = json::parse(d.importance, nullptr, false);
        if (!imp.is_discarded() && imp.contains("tiers")) b.tiers = imp["tiers"].get<std::map<std::string, int>>();
    }
    if (!d.base_rates.empty()) {
        json r = json::parse(d.base_rates, nullptr, false);
        if (r.is_discarded()) { err = "invalid json in patterns/base_rates.json"; return false; }
        if (r.contains("rates")) b.rates = r["rates"].get<std::map<std::string, std::map<std::string, double>>>();
        if (r.contains("magnitudes")) b.magnitudes = r["magnitudes"].get<std::map<std::string, std::vector<double>>>();
    }
    if (!d.text_bundles.empty()) {
        auto texts = std::make_shared<text::Bundles>();
        std::string terr;
        if (text::bundles_from_json(d.text_bundles, *texts, terr)) b.texts = texts;
    }
    if (d.patterns.empty()) return true;
    json all = json::parse(d.patterns, nullptr, false);
    if (all.is_discarded() || !all.is_array()) { err = "invalid json in data/patterns"; return false; }
    for (auto& file : all)
        for (auto& pj : file.is_array() ? file : file.value("patterns", json::array())) {
            SiftPattern p;
            if (!pattern_of(pj, p, err)) return false;
            b.patterns.push_back(p);
        }
    b.hash = fnv1a(all.dump());
    return true;
}

}
