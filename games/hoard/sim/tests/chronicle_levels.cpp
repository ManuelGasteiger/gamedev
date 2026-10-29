#include "fixtures.h"
#include "ages/json.h"
#include "sift/sift.h"

using namespace ages;
using nlohmann::json;

static int at(World& w, const Params& p, int year, const std::string& kind, const std::string& subject, const std::string& object = "",
              double m = 0, std::vector<int> refs = {}) {
    w.year = year;
    Cause c;
    c.refs = refs;
    int id = emit(w, kind, subject, object, m, {}, c);
    sift_events(w, p);
    return id;
}

// the story's line in a briefing of the years since 90
static json line_of(const World& w, const Params& p) {
    json b = json::parse(briefing_json(w, p, 90));
    for (auto& l : b["lines"]) if (l.value("sift_thread", 0) == w.sift.threads.front().id) return l;
    return json();
}

int main() {
    LoadResult r = load_or_die(1);
    Params& p = r.params;
    std::vector<SiftPattern> keep;
    for (auto& x : p.sift.patterns) if (x.id == "vengeance_for_burning") keep.push_back(x);
    p.sift.patterns = keep;
    World& w = r.world;
    w.chronicle.clear();
    w.sift = SiftState();
    w.year = 90;
    emit(w, "sleep_begin", "dragon", "", 60, {}, {}, "", false);
    int war = at(w, p, 95, "war_started", "aldmere", "vessary");
    int a = at(w, p, 100, "act_burn", "aldmere", "east_vale", 0.5, {war});
    int b = at(w, p, 120, "slayers_founded", "aldmere", "", 0, {a});
    at(w, p, 150, "slay_attempt_failed", "aldmere", "", 0.1, {b});
    CHECK(!w.sift.threads.empty(), "the story was matched");
    int match = w.sift.threads.front().id;

    json l0 = line_of(w, p);
    CHECK(!l0.is_null(), "the story has a line");
    CHECK(l0["level"] == 0 && !l0["cause_shown"].get<bool>() && l0["sources"].empty() && !l0.contains("because"), "level 0: no cause");

    w.dragons[kPlayer].chron.sources.push_back({"src:" + std::to_string(match) + ":aldmere", 160, "taken"});
    json l1 = line_of(w, p);
    CHECK(l1["level"] == 1 && l1["cause_shown"].get<bool>() && l1["sources"].size() == 1 && !l1.contains("because"), "level 1: a source's cause");
    json b1 = json::parse(briefing_json(w, p, 90));
    std::string said = b1["sources"][0]["text"];
    CHECK(!said.empty() && l1["text"].get<std::string>().find(said) != std::string::npos, "level 1 prints what the source says");

    w.dragons[kPlayer].chron.historians.push_back({"nobody", 90, -1});
    json l2 = line_of(w, p);
    CHECK(l2["level"] == 2 && l2.contains("because") && l2["sources"].empty(), "level 2: the true cause");
    CHECK(l2["text"].get<std::string>().find(l2["because"].get<std::string>()) != std::string::npos, "level 2 prints it");
    std::printf("chronicle_levels ok\n");
    return 0;
}
