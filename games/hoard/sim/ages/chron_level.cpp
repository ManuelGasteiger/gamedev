#include <algorithm>
#include <cmath>
#include <set>
#include "chron.h"
#include "../sift/sift_util.h"
#include "memory_tiers.h"
#include "world_util.h"

namespace ages {

namespace {

int died_year(const World& w, const std::string& person) {
    if (find_person(w, person)) return -1;
    for (auto& d : w.dead_people) if (d.id == person) return d.died;
    return -2;   // gone from every list
}

std::string court_in(const std::string& source_id) {
    size_t a = source_id.find(':'), b = source_id.find(':', a + 1);
    return b == std::string::npos ? "" : source_id.substr(b + 1);
}

bool held(const World& w, const std::string& id) {
    for (auto& s : w.dragons[kPlayer].chron.sources) if (s.id == id) return true;
    return false;
}

const SiftThread* thread_of(const World& w, int match) {
    for (auto& t : w.sift.threads) if (t.id == match) return &t;
    return nullptr;
}

bool in_cast(const SiftThread& t, const std::string& court) {
    for (auto& kv : t.cast) if (kv.second == court) return true;
    return false;
}

// the year the dragon last burned this court's library; what it wrote before then is ash
int library_burned(const World& w, const std::string& court) {
    for (size_t i = w.chronicle.size(); i > 0; --i) {
        const Event& e = w.chronicle[i - 1];
        if (e.kind == "act_burn" && e.subject == court && e.tags.count("library")) return e.year;
    }
    return -1000000;
}

}

bool historian_at(const World& w, int year) {
    for (auto& t : w.dragons[kPlayer].chron.historians)
        if (t.from <= year && (t.to < 0 || year <= t.to)) return true;
    return false;
}

const Person* historian_now(const World& w) {
    for (auto& t : w.dragons[kPlayer].chron.historians)
        if (t.to < 0) return find_person(w, t.person);
    return nullptr;
}

bool lore_counterfactual(const World& w, const Params& p) {
    auto it = w.dragons[kPlayer].growth.find("lore");
    return historian_now(w) && it != w.dragons[kPlayer].growth.end() && it->second >= p.c.chron.lore_counterfactual;
}

// brief 05 scope 1: a line about court X is at level 1 when the hoard holds a source about X (its books, or a captive of it)
std::vector<Source> sources_for(const World& w, const Params& p, int match, int event) {
    std::vector<Source> out;
    std::vector<std::string> courts;
    if (const SiftThread* t = match ? thread_of(w, match) : nullptr) for (auto& kv : t->cast) courts.push_back(kv.second);
    else if (const Event* e = event > 0 && event < kCompoundBase ? find_event(w, event) : nullptr) courts = {court_of(w, e->subject), court_of(w, e->object)};
    const Dragon& d = w.dragons[kPlayer];
    std::set<std::string> seen;
    for (const std::string& court : courts) {
        if (!find_faction(w, court) || !seen.insert(court).second) continue;
        bool book = false, captive = false;
        for (auto& h : d.chron.sources) if (court_in(h.id) == court) book = true;
        for (auto& id : d.hostages) if (const Person* x = find_person(w, id)) if (x->faction == court) captive = true;
        if (!book && !captive) continue;
        Source s = match ? make_source(w, p, match, court) : make_event_source(w, p, event, court);
        if (s.medium.empty()) continue;
        if (book) { s.medium = "written"; s.archetype = "annalist"; s.hedge = 0; }
        else { s.medium = "oral"; s.archetype = "captive"; }
        out.push_back(s);
    }
    return out;
}

int line_level(const World& w, const Params& p, int year, int match, int event) {
    if (historian_at(w, year)) return lore_counterfactual(w, p) ? 3 : 2;
    return sources_for(w, p, match, event).empty() ? 0 : 1;
}

// a take of books carries off the court's written tellings of the threads it was cast in, newest first
void take_sources(World& w, const Params& p, const Faction& f, double m) {
    if (!keeps_records(w, f)) return;
    int burned = library_burned(w, f.id);
    std::vector<const SiftThread*> pool;
    for (auto& t : w.sift.threads) {
        if (t.state == "latent" || t.state == "dropped" || !in_cast(t, f.id)) continue;
        if (t.advanced <= burned) continue;
        if (held(w, "src:" + std::to_string(t.id) + ":" + f.id)) continue;
        pool.push_back(&t);
    }
    std::sort(pool.begin(), pool.end(), [](const SiftThread* a, const SiftThread* b) { return a->advanced != b->advanced ? a->advanced > b->advanced : a->id > b->id; });
    int n = std::max(1, static_cast<int>(std::round(p.c.chron.take_sources * std::min(1.0, m))));
    std::vector<HeldSource> got;
    for (size_t i = 0; i < pool.size() && static_cast<int>(i) < n; ++i) got.push_back({"src:" + std::to_string(pool[i]->id) + ":" + f.id, w.year, "taken"});
    auto& list = w.dragons[kPlayer].chron.sources;
    list.insert(list.end(), got.begin(), got.end());
}

// a gift of books hands the oldest tellings on
void give_sources(World& w, double m) {
    auto& list = w.dragons[kPlayer].chron.sources;
    int n = std::min(static_cast<int>(list.size()), std::max(1, static_cast<int>(std::round(4 * std::min(1.0, m)))));
    list.erase(list.begin(), list.begin() + n);
}

void close_tenures(World& w) {
    Dragon& d = w.dragons[kPlayer];
    for (auto& t : d.chron.historians) {
        if (t.to >= 0) continue;
        int died = died_year(w, t.person);
        bool kept = std::find(d.hostages.begin(), d.hostages.end(), t.person) != d.hostages.end();
        if (died != -1) t.to = died >= 0 ? died : w.year;
        else if (!kept) t.to = w.year;
    }
}

}
