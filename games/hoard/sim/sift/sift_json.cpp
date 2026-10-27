#include "sift_json.h"
#include "sift.h"

namespace ages {

using nlohmann::json;

namespace {

json thread_json(const SiftThread& t) {
    json shown = json::array();
    for (auto& s : t.shown) shown.push_back({{"waking", s.waking}, {"stage", s.stage}, {"year", s.year}, {"state", s.state}});
    return {{"match", t.id}, {"pattern", t.pattern}, {"cast", t.cast}, {"events", t.events}, {"years", t.years}, {"state", t.state},
            {"opened", t.opened}, {"advanced", t.advanced}, {"ended", t.ended}, {"outcome", t.outcome}, {"cut_by", t.cut_by},
            {"emitted", t.emitted}, {"shown", shown}};
}

SiftThread thread_from(const json& j) {
    SiftThread t;
    t.id = j.value("match", 0);
    t.pattern = j.value("pattern", "");
    t.cast = j.value("cast", std::map<std::string, std::string>());
    t.events = j.value("events", std::vector<int>());
    t.years = j.value("years", std::vector<int>());
    t.state = j.value("state", "latent");
    t.opened = j.value("opened", 0);
    t.advanced = j.value("advanced", 0);
    t.ended = j.value("ended", 0);
    t.outcome = j.value("outcome", "");
    t.cut_by = j.value("cut_by", "");
    t.emitted = j.value("emitted", 0);
    for (auto& s : j.value("shown", json::array()))
        t.shown.push_back({s.value("waking", 0), s.value("stage", 0), s.value("year", 0), s.value("state", "")});
    return t;
}

}

json sift_lines_json(const std::vector<SiftLine>& lines) {
    json out = json::array();
    for (auto& l : lines)
        out.push_back({{"thread", l.thread}, {"event", l.event}, {"tier", l.tier}, {"score", l.score}, {"kind", l.kind}, {"text_key", l.text_key},
                       {"params", l.params}, {"because_chain", l.because}, {"sources", l.sources}});
    return out;
}

json sift_state_json(const SiftState& s) {
    json threads = json::array(), compounds = json::array();
    for (auto& t : s.threads) threads.push_back(thread_json(t));
    for (auto& e : s.compounds) compounds.push_back(event_json(e));
    return {{"cursor", s.cursor}, {"next_thread", s.next_thread}, {"next_event", s.next_event}, {"threads", threads},
            {"compounds", compounds}, {"once", s.once}, {"barred", s.barred}, {"waking", s.waking}, {"headlines", s.headlines},
            {"named", s.named}, {"lines", sift_lines_json(s.lines)}, {"lines_since", s.lines_since}, {"lines_year", s.lines_year}};
}

void sift_state_from_json(const json& j, SiftState& s) {
    s = SiftState();
    s.on = true;
    s.cursor = j.value("cursor", 0);
    s.next_thread = j.value("next_thread", 1);
    s.next_event = j.value("next_event", kCompoundBase);
    for (auto& t : j.value("threads", json::array())) s.threads.push_back(thread_from(t));
    for (auto& e : j.value("compounds", json::array())) s.compounds.push_back(event_from(e));
    s.once = j.value("once", std::set<std::string>());
    s.barred = j.value("barred", std::set<std::string>());
    s.waking = j.value("waking", 0);
    s.headlines = j.value("headlines", std::vector<std::string>());
    s.named = j.value("named", std::set<std::string>());
    for (auto& l : j.value("lines", json::array())) {
        SiftLine x;
        x.thread = l.value("thread", 0);
        x.event = l.value("event", 0);
        x.tier = l.value("tier", 0);
        x.score = l.value("score", 0.0);
        x.kind = l.value("kind", "");
        x.text_key = l.value("text_key", "");
        x.params = l.value("params", std::map<std::string, std::string>());
        x.because = l.value("because_chain", std::vector<int>());
        x.sources = l.value("sources", std::vector<std::string>());
        s.lines.push_back(x);
    }
    s.lines_since = j.value("lines_since", -1);
    s.lines_year = j.value("lines_year", -1);
}

json sift_view_json(const World& w, const Params& p) {
    json out = json::array();
    for (auto& t : w.sift.threads) {
        if (t.state == "latent" || t.state == "dropped") continue;
        json j = thread_json(t);
        j["score"] = thread_score(w, p, t);
        out.push_back(j);
    }
    return out;
}

}
