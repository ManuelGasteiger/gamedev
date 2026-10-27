#include "arrivals.h"
#include "names.h"
#include "world_util.h"

namespace ages {

namespace {

// exact knowledge comes from someone who stood at the lair and walked home
bool returns_home(const std::string& kind) { return kind != "slayer_band" && kind != "thief"; }

void leave(World& w, const Arrival& a, const char* why) {
    emit(w, "arrival_left", a.court, a.kind, 0, {why}, because("arrival_left"));
}

}

const ArrivalDef* arrival_def(const Params& p, const std::string& kind) {
    for (auto& d : p.arrivals.kinds) if (d.kind == kind) return &d;
    return nullptr;
}

const Arrival* arrival_for(const World& w, const std::string& kind, const std::string& court) {
    for (auto& a : w.arrivals) if (a.kind == kind && a.court == court) return &a;
    return nullptr;
}

Arrival* find_arrival(World& w, const std::string& id) {
    for (auto& a : w.arrivals) if (a.id == id) return &a;
    return nullptr;
}

void arrival_consume(World& w, const std::string& id) {
    w.arrivals.erase(std::remove_if(w.arrivals.begin(), w.arrivals.end(), [&](const Arrival& a) { return a.id == id; }), w.arrivals.end());
}

void arrivals_depart(World& w, const Params&) {
    for (auto& a : w.arrivals) {
        if (Faction* f = find_faction(w, a.court)) if (returns_home(a.kind)) learn_lair(w, *f, true);
        leave(w, a, "slept");
    }
    w.arrivals.clear();
}

void arrivals_at_wake(World& w, const Params& p) {
    int window = short_sleep(w.dragons[kPlayer], p.c);
    std::vector<Arrival> kept;
    for (auto& a : w.arrivals) {
        const ArrivalDef* d = arrival_def(p, a.kind);
        const OfferDef* o = a.kind == "envoy" ? offer_def(p, a.offer_kind) : nullptr;
        bool gone = !find_faction(w, a.court);
        bool short_gone = d && d->window == "short" && a.year < w.year - window;
        bool expired = o && a.year + o->expires_years <= w.year;
        if (gone || short_gone || expired) {
            if (!gone) leave(w, a, short_gone ? "window" : "expired");
            continue;
        }
        kept.push_back(a);
    }
    w.arrivals = kept;
    for (auto it = w.arrival_fired.begin(); it != w.arrival_fired.end();) it = w.year - it->second > 300 ? w.arrival_fired.erase(it) : std::next(it);
}

// a hook of an arrival went unanswered: a thief gets away with what he saw
void arrival_ignored(World& w, const Params& p, const Hook& h) {
    const HookDef* t = hook_def(p, h);
    if (!t || t->arrival.empty()) return;
    const Arrival* a = arrival_for(w, t->arrival, h.faction);
    if (!a) return;
    std::string id = a->id;
    if (t->arrival == "thief") if (Faction* f = find_faction(w, h.faction)) learn_lair(w, *f, true);
    arrival_consume(w, id);
}

void envoy_returned(World& w, const Offer& of) {
    if (Faction* f = find_faction(w, of.from_faction)) learn_lair(w, *f, true);
    for (auto& a : w.arrivals)
        if (a.kind == "envoy" && a.court == of.from_faction && a.offer_kind == of.kind && a.target == of.target) { arrival_consume(w, a.id); break; }
}

std::string ledge_text(const World& w, const Params& p, const Arrival& a) {
    const Texts& t = p.texts;
    auto misc = [&](const std::string& k) { auto it = t.misc.find(k); return it == t.misc.end() ? std::string() : it->second; };
    auto replace = [](std::string& s, const std::string& from, const std::string& to) {
        for (size_t pos = s.find(from); pos != std::string::npos; pos = s.find(from, pos + to.size())) s.replace(pos, from.size(), to);
    };
    std::string head = misc("ledge." + a.kind);
    replace(head, "{court}", display_name(w, a.court));
    std::string list;
    for (size_t i = 0; i < a.because.size(); ++i) {
        const std::string& k = a.because[i];
        std::string clause;
        if (k.rfind("motive.", 0) == 0) { auto m = t.motives.find(k.substr(7)); clause = m == t.motives.end() ? k : m->second; }
        else clause = misc("because." + k);
        replace(clause, "{court}", display_name(w, a.court));
        list += (i == 0 ? "" : (i + 1 == a.because.size() ? misc("and") : ", ")) + clause;
    }
    std::string line = misc("ledge_line");
    replace(line, "{head}", head);
    replace(line, "{because}", list);
    return line;
}

}
