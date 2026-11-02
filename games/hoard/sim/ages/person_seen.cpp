#include "person_seen.h"
#include "chron.h"
#include "world_util.h"

namespace ages {

static bool source_of(const World& w, const std::string& court) {
    std::string tail = ":" + court;
    for (auto& s : w.dragons[kPlayer].chron.sources)
        if (s.id.size() > tail.size() && s.id.compare(s.id.size() - tail.size(), tail.size(), tail) == 0) return true;
    return false;
}

static bool captive_of(const World& w, const std::string& court) {
    for (auto& id : w.dragons[kPlayer].hostages)
        if (const Person* x = find_person(w, id)) if (x->faction == court) return true;
    return false;
}

bool person_seen(const World& w, const Person& x) {
    if (find_known(w, x.id) || has_tag(x.tags, "hostage")) return true;
    for (auto& o : w.offers) if (o.envoy == x.id) return true;
    for (auto& a : w.arrivals) if (a.person == x.id) return true;
    if (x.faction.empty()) return false;
    if (captive_of(w, x.faction) || source_of(w, x.faction)) return true;
    return (x.role == "ruler" || x.role == "heir") && historian_now(w);
}

}
