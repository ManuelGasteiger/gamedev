#include <algorithm>
#include <cmath>
#include "chron.h"
#include "world_util.h"
#include "wounds.h"

namespace ages {

namespace {

bool lettered(const Person& x) {
    return x.role == "sage" || x.role == "priest" || x.role == "scribe" || has_tag(x.tags, "lettered");
}

}

// a captive who can write keeps the record; one historian at a time, the new one ends the old one's book
bool keep_person_unrecorded(World& w, const Params& p, const std::string& id, const std::string& role, std::string& reason) {
    (void)p;
    if (dragon_dead(w)) { reason = "dragon_slain"; return false; }
    if (role != "historian") { reason = "no_role|role=" + role; return false; }
    Dragon& d = w.dragons[kPlayer];
    Person* x = find_person(w, id);
    if (!x || std::find(d.hostages.begin(), d.hostages.end(), id) == d.hostages.end()) { reason = "not_in_lair"; return false; }
    if (!lettered(*x)) { reason = "unlettered"; return false; }
    close_tenures(w);
    for (auto& t : d.chron.historians) {
        if (t.to >= 0) continue;
        if (t.person == id) { reason = "already_kept"; return false; }
        t.to = w.year;
    }
    d.chron.historians.push_back({id, w.year, -1});
    emit(w, "historian_kept", id, x->faction, 0, {x->role});
    return true;
}

// the hoard keeps such a sleeper: it ages at a tenth of the years slept
void age_in_hoard(World& w, int years) {
    const Person* h = historian_now(w);
    if (!h || !has_tag(h->tags, "sleeps_in_hoard")) return;
    if (Person* x = find_person(w, h->id)) x->dies += static_cast<int>(std::round(0.9 * years));
}

}
