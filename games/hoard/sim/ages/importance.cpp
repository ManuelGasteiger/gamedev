#include "importance.h"
#include "world_util.h"

namespace ages {

static const char* const kDeathReasons[] = {"old_age", "deposed", "realm_fell", "slain_by_dragon", "killed_by_dragon", "warlord"};

std::string template_key(const Event& e) {
    if (e.kind == "collapse" && e.magnitude <= 1) return "collapse_one";
    if (e.kind == "remnant_formed" && e.tags.count("off_map")) return "remnant_formed_off_map";
    if (e.kind == "person_rose" && e.tags.count("heir")) return "person_rose_heir";
    if (e.kind == "dragon_raised_lineage" && e.object.empty()) return "dragon_raised_lineage_digested";
    if (e.kind == "act_burn" && e.object.empty()) return "act_burn_lands";
    if (e.kind == "hostage_taken" && e.tags.count("consent")) return "hostage_taken_consent";
    if (e.kind == "person_died")
        for (const char* r : kDeathReasons) if (e.tags.count(r)) return std::string("person_died_") + r;
    if (e.text_key.rfind("ev.", 0) == 0 && e.text_key.size() > 3 && e.text_key.compare(3, std::string::npos, e.kind) != 0) return e.text_key.substr(3);
    return e.kind;
}

static double base_of(const Params& p, const Event& e) {
    auto it = p.texts.events.find(template_key(e));
    if (it == p.texts.events.end()) it = p.texts.events.find(e.kind);
    return it == p.texts.events.end() ? p.c.importance_default : it->second.importance;
}

static bool under_oath(const World& w, const std::string& id) {
    if (id.empty()) return false;
    for (auto& o : w.oaths) if (o.status == "active" && (o.parties[0] == id || o.parties[1] == id)) return true;
    return false;
}

// a person in the lair, or a court with one of its people in the lair now
static bool hostage_of(const World& w, const std::string& id) {
    if (contains(w.dragons[kPlayer].hostages, id)) return true;
    for (auto& h : w.dragons[kPlayer].hostages)
        if (const Person* x = find_person(w, h)) if (x->faction == id) return true;
    return false;
}

double event_importance(const World& w, const Params& p, const Event& e) {
    const Constants& c = p.c;
    double v = base_of(p, e);
    bool known = false, oath = false, hostage = false;
    for (const std::string* id : {&e.subject, &e.object}) {
        if (find_known(w, *id)) known = true;
        if (under_oath(w, *id)) oath = true;
        if (hostage_of(w, *id)) hostage = true;
    }
    if (known) v += c.importance_known;
    if (oath) v += c.importance_oath;
    if (hostage) v += c.importance_hostage;
    return clamp01(v);
}

void score_events(World& w, const Params& p) {
    for (size_t i = w.chronicle.size(); i > 0 && w.chronicle[i - 1].importance < 0; --i)
        w.chronicle[i - 1].importance = event_importance(w, p, w.chronicle[i - 1]);
}

}
