#include "endings_json.h"
#include "endings.h"
#include "names.h"
#include "profile.h"
#include "wounds.h"
#include "world_util.h"
#include "text/render.h"

namespace ages {

using nlohmann::json;

void endings_fields_to_json(const World& w, json& j) {
    if (!w.endings.empty()) {
        json arr = json::array();
        for (auto& s : w.endings) {
            json conds = json::array();
            for (auto& h : s.conds)
                conds.push_back({{"held_since", h.held_since}, {"last_met", h.last_met}, {"met", h.met}, {"ever_met", h.ever_met},
                                 {"best_years", h.best_years}, {"best_value", h.best_value}});
            arr.push_back({{"id", s.id}, {"ready", s.ready}, {"court", s.court}, {"conds", conds}});
        }
        j["endings"] = arr;
    }
    const Prophecy& pr = w.prophecy;
    if (!pr.ending.empty()) {
        json lines = json::array();
        for (auto& l : pr.lines) lines.push_back({{"key", l.key}, {"cond", l.cond}, {"slots", l.slots}});
        j["prophecy"] = {{"ending", pr.ending}, {"lines", lines}, {"known_by", pr.known_by}, {"year", pr.year}, {"event", pr.event}, {"book", pr.book}};
    }
    if (!w.ended.empty()) j["ended"] = w.ended;
}

void endings_fields_from_json(const json& j, World& w) {
    w.endings.clear();
    for (auto& v : j.value("endings", json::array())) {
        EndingState s;
        s.id = v.value("id", "");
        s.ready = v.value("ready", false);
        s.court = v.value("court", "");
        for (auto& c : v.value("conds", json::array())) {
            CondHold h;
            h.held_since = c.value("held_since", -1);
            h.last_met = c.value("last_met", -1);
            h.met = c.value("met", false);
            h.ever_met = c.value("ever_met", false);
            h.best_years = c.value("best_years", 0);
            h.best_value = c.value("best_value", 0.0);
            s.conds.push_back(h);
        }
        w.endings.push_back(s);
    }
    w.prophecy = Prophecy();
    if (j.contains("prophecy")) {
        const json& pj = j["prophecy"];
        Prophecy& pr = w.prophecy;
        pr.ending = pj.value("ending", "");
        for (auto& l : pj.value("lines", json::array())) {
            ProphecyLine line;
            line.key = l.value("key", "");
            line.cond = l.value("cond", "");
            if (l.contains("slots")) line.slots = l["slots"].get<std::map<std::string, std::string>>();
            pr.lines.push_back(line);
        }
        if (pj.contains("known_by")) pr.known_by = pj["known_by"].get<std::vector<std::string>>();
        pr.year = pj.value("year", 0);
        pr.event = pj.value("event", 0);
        pr.book = pj.value("book", "");
    }
    w.ended = j.value("ended", "");
}

std::string endings_text(const World& w, const Params& p, const std::string& key, const std::map<std::string, std::string>& vars,
                         const std::string& court, const std::string& lang) {
    if (key.empty() || !p.sift.texts) return "";
    text::Ctx c;
    c.vars = vars;
    if (!court.empty()) {
        text::Entity e;
        e.kind = "faction";
        e.name = display_name(w, court);
        c.roles["court"] = e;
    }
    return text::render_key(*p.sift.texts, lang, key, c);
}

json endings_view_json(const World& w, const Params& p) {
    json out = json::array();
    for (auto& e : p.endings.endings) {
        if (!e.enabled) continue;
        const EndingState* s = ending_state(w, e.id);
        std::string court;
        std::vector<bool> now = conditions_met(w, p, e, &court);
        if (s && !s->court.empty()) court = s->court;
        json conds = json::array();
        int met = 0;
        for (size_t i = 0; i < e.conds.size(); ++i) {
            const EndingCond& c = e.conds[i];
            CondHold h = s && i < s->conds.size() ? s->conds[i] : CondHold();
            bool is = now[i];
            met += is ? 1 : 0;
            std::string key = "endings." + e.id + "." + c.id + (is ? ".met" : ".missing");
            int held = is ? years_held(h, w.year) : 0;
            std::string held_text = c.hold_years > 0 && is ? endings_text(w, p, "endings.held", {{"years", std::to_string(held)}, {"hold", std::to_string(c.hold_years)}}) : "";
            conds.push_back({{"id", c.id}, {"met", is}, {"years_held", held}, {"hold_years", c.hold_years}, {"lapse_years", c.lapse_years},
                             {"ever_met", h.ever_met}, {"best_years", h.best_years}, {"best_value", h.best_value},
                             {"text_key", key}, {"text", endings_text(w, p, key, {}, court)}, {"held_text", held_text}});
        }
        std::string name = endings_text(w, p, "endings." + e.id + ".name", {});
        bool ready = s && s->ready;
        std::string line = ready ? endings_text(w, p, "endings.ready", {{"name", name}})
                                 : endings_text(w, p, "endings.progress", {{"name", name}, {"met", std::to_string(met)}, {"total", std::to_string(e.conds.size())}});
        out.push_back({{"id", e.id}, {"exit_kind", e.exit_kind}, {"ready", ready}, {"court", court}, {"name", name}, {"text", line},
                       {"conditions", conds}});
    }
    return out;
}

json prophecy_view_json(const World& w, const Params& p) {
    const Prophecy& pr = w.prophecy;
    if (pr.ending.empty()) return nullptr;
    json lines = json::array();
    for (auto& l : pr.lines) {
        std::map<std::string, std::string> words = l.slots;   // a riddle spells its numbers: prophecy.num.<n>
        for (auto& kv : words) {
            std::string word = endings_text(w, p, "prophecy.num." + kv.second, {});
            if (!word.empty() && word.find("prophecy.num") == std::string::npos) kv.second = word;
        }
        lines.push_back({{"key", l.key}, {"slots", l.slots}, {"cond", l.cond}, {"text", endings_text(w, p, l.key, words)}});
    }
    lines.push_back({{"key", "prophecy.ending_noun"}, {"slots", json::object()}, {"cond", ""}, {"text", endings_text(w, p, "prophecy.ending_noun", {})}});
    return {{"ending", pr.ending}, {"year", pr.year}, {"book", pr.book}, {"known_by", pr.known_by}, {"lines", lines}};
}

json myth_view_json(const World& w, const Params& p) {
    if (!dragon_dead(w)) return nullptr;
    bool q = myth_qualifies(w, p);
    std::string missing;
    if (const EndingDef* e = ending_def(p.endings, "myth")) {
        std::vector<bool> met = conditions_met(w, p, *e);
        for (size_t i = 0; i < met.size(); ++i)
            if (!met[i]) missing += (missing.empty() ? "" : " ") + endings_text(w, p, "endings.myth." + e->conds[i].id + ".missing", {});
    }
    std::string key = q ? "myth.qualifies" : "myth.not_yet";
    return {{"qualifies", q}, {"text_key", key}, {"text", endings_text(w, p, key, {{"missing", missing}})}};
}

json ending_view_json(const World& w, const Params& p) {
    if (w.ended.empty()) return nullptr;
    const EndingDef* def = nullptr;
    for (auto& e : p.endings.endings) if (e.exit_kind == w.ended) def = &e;
    const EndingState* s = def ? ending_state(w, def->id) : nullptr;
    std::string court = s ? s->court : "";
    std::string key = def ? def->epilogue_text_key : "";
    int from = 0;
    for (auto& e : w.chronicle) if (e.kind == "ending") from = e.year;
    return {{"kind", w.ended}, {"court", court}, {"year", from}, {"epilogue_key", key}, {"epilogue", endings_text(w, p, key, {}, court)}};
}

}
