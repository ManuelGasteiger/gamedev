#include <algorithm>
#include "chron.h"
#include "memory_tiers.h"
#include "rules/slayers.h"
#include "world_util.h"
#include "wounds.h"

// story-and-endings section 1: an artifact with an agenda that tells the truth selectively. D7: it speaks only once found.

namespace ages {

namespace {

bool fact(const World& w, const std::string& f) {
    if (f == "slayers") {
        for (auto& x : w.factions) if (slayer_stage(w, x.id) == "slayers_ready") return true;
        return false;
    }
    if (f == "cannons") {
        for (auto& x : w.factions) if (x.era >= 2 && x.anti_dragon >= 4) return true;
        return false;
    }
    if (f == "undead") {
        for (auto& x : w.factions) if (x.race == "undead") return true;
        return false;
    }
    if (f == "cult") {
        for (auto& x : w.factions) if (x.faith >= 0.3 && awe_eff(x) >= 0.3) return true;
        return false;
    }
    if (f == "magic") return w.magic >= 0.6;
    return false;
}

// what each voice watches, most pressing first; it names what helps its ending and leaves out what does not
const std::vector<std::string>& watches(const std::string& agenda) {
    static const std::map<std::string, std::vector<std::string>> m = {
        {"lichdom", {"slayers", "cannons", "undead"}},
        {"ascension", {"cult", "magic", "slayers"}},
        {"emperor", {"cannons", "slayers"}},
        {"sun_eater", {"cannons", "magic"}},
    };
    static const std::vector<std::string> none;
    auto it = m.find(agenda);
    return it == m.end() ? none : it->second;
}

Relic* relic_by_id(World& w, const std::string& id) {
    for (auto& r : w.dragons[kPlayer].chron.relics) if (r.id == id) return &r;
    return nullptr;
}

}

int waking_index(const World& w) {
    int n = 0;
    for (auto& e : w.chronicle) if (e.kind == "wake") ++n;
    return n;
}

// a take of artifacts may carry off one that wants something; at most one per court and relic_max in all
void find_relic(World& w, const Params& p, const Faction& f, double m) {
    const ChronicleConstants& c = p.c.chron;
    auto& list = w.dragons[kPlayer].chron.relics;
    if (static_cast<int>(list.size()) >= c.relic_max) return;
    for (auto& r : list) if (r.court == f.id) return;
    std::vector<const RelicKind*> fit;
    for (auto& k : c.relics) {
        bool held = false;
        for (auto& r : list) if (r.kind == k.id) held = true;
        if (!held && std::find(k.races.begin(), k.races.end(), f.race) != k.races.end()) fit.push_back(&k);
    }
    if (fit.empty()) return;
    Rng r = rng_for(w.seed, "relic", f.id + ":" + std::to_string(w.year));
    if (!r.chance(c.relic_chance * std::min(1.0, m))) return;
    const RelicKind* k = fit[std::min(fit.size() - 1, static_cast<size_t>(r.uniform() * static_cast<double>(fit.size())))];
    list.push_back({k->id + ":" + f.id, k->id, k->agenda, f.id, w.year, -1, -1, ""});
}

const Relic* speaking_relic(const World& w) {
    auto& list = w.dragons[kPlayer].chron.relics;
    int now = waking_index(w);
    for (size_t i = list.size(); i > 0; --i) {
        const Relic& r = list[i - 1];
        if (r.agenda.empty()) continue;
        return r.quiet_until > now ? nullptr : &r;
    }
    return nullptr;
}

std::map<std::string, std::string> voice_line(const World& w, const Params& p) {
    (void)p;
    const Relic* r = speaking_relic(w);
    if (!r) return {};
    std::map<std::string, std::string> out = {{"relic", r->id}, {"kind", r->kind}, {"agenda", r->agenda}, {"court", r->court}};
    if (r->asked == waking_index(w)) {   // commune(ask): the plain state of what it watches, one free level-2 line
        std::string facts;
        for (auto& f : watches(r->agenda)) facts += (facts.empty() ? "" : ",") + f + "=" + (fact(w, f) ? "1" : "0");
        out["state"] = "asked";
        out["facts"] = facts;
        out["key"] = "voice." + r->agenda + ".asked";
        return out;
    }
    std::string state = r->year >= w.sleep_started ? "found" : "idle";
    if (state != "found")
        for (auto& f : watches(r->agenda)) if (fact(w, f)) { state = f; break; }
    out["state"] = state;
    out["key"] = "voice." + r->agenda + "." + state;
    return out;
}

bool commune_unrecorded(World& w, const Params& p, const std::string& relic, const std::string& answer, std::string& reason) {
    if (dragon_dead(w)) { reason = "dragon_slain"; return false; }
    Relic* r = relic_by_id(w, relic);
    if (!r || r->agenda.empty()) { reason = "no_relic"; return false; }
    const Relic* speaking = speaking_relic(w);
    if (!speaking || speaking->id != relic) { reason = "silent"; return false; }
    if (answer != "ask" && answer != "accept" && answer != "refuse") { reason = "no_answer|answer=" + answer; return false; }
    Dragon& d = w.dragons[kPlayer];
    if (answer == "accept" && has_tag(d.tags, "crowned_by:" + relic)) { reason = "already_accepted"; return false; }
    int now = waking_index(w);
    if (answer == "ask") r->asked = now;
    else if (answer == "refuse") r->quiet_until = now + p.c.chron.quiet_wakings;
    else {
        d.tags.insert("crowned_by:" + relic);
        d.tags.insert("boon:" + r->agenda);
        if (r->agenda == "ascension") w.tags.insert("living_unite");
        else for (auto& f : w.factions) set_eff(f.dread, std::min(1.0, dread_eff(f) + 0.1));
    }
    r->answer = answer;
    emit(w, "act_commune", "dragon", relic, 0, {answer, r->agenda});
    return true;
}

}
