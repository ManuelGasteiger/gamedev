#include "reaction_lines.h"
#include "json.h"
#include "names.h"
#include "oaths.h"
#include "world_util.h"

// apply_acts' `lines`: what an act meant to each court that reacted, told from the reaction row that fired and the
// court's state. A template key is tried with the court's race first ("burn.strong.dwarf", "burn.strong", then the
// shared harm row "harm.strong"); one more sentence follows a one-sentence line when there is something to add.

namespace ages {

namespace {

bool is_harm(const std::string& a) { return a == "burn" || a == "kill" || a == "take" || a == "show_in_rage" || a == "show_forgotten"; }
bool is_help(const std::string& a) { return a == "defend" || a == "give" || a == "miracle" || a == "accept_offering" || a == "release"; }

// direct harm outranks a direct favour, which outranks watching it happen to someone else
int rank(const ReactionNote& n) { return n.act == "witness_burn" ? 0 : (is_harm(n.act) ? 2 : 1); }

void replace_all(std::string& s, const std::string& from, const std::string& to) {
    for (size_t pos = s.find(from); pos != std::string::npos; pos = s.find(from, pos + to.size())) s.replace(pos, from.size(), to);
}

std::string cap(std::string s) {
    if (!s.empty() && s[0] >= 'a' && s[0] <= 'z') s[0] = static_cast<char>(s[0] - 32);
    return s;
}

bool two_sentences(const std::string& s) { return s.find(". ") != std::string::npos; }

struct Teller {
    const World& w;
    const Params& p;
    size_t first;

    std::string text(const std::string& key) const {
        auto it = p.texts.reactions.find(key);
        return it == p.texts.reactions.end() ? "" : it->second;
    }
    std::string first_of(const std::vector<std::string>& keys) const {
        for (auto& k : keys) { std::string t = text(k); if (!t.empty()) return t; }
        return "";
    }
    std::string race_of(const std::string& faction) const {
        const Faction* f = find_faction(w, faction);
        return f ? dominant_race(*f) : "";
    }
    // a kill aimed at a named person reads as kill_person; a hostage verb with nobody to take or send home as <act>.nobody
    std::string row(const ReactionNote& n) const {
        std::string race = race_of(n.faction), act = n.act, rel = n.rel;
        if ((act == "take_hostage" || act == "release") && person_of(n).empty()) return text(act + ".nobody");
        std::vector<std::string> keys;
        if (act == "kill" && is_person_id(w, n.item)) keys = {"kill_person." + rel, "kill_person.any"};
        for (auto& k : {act + "." + rel + "." + race, act + "." + rel, act + ".any"}) keys.push_back(k);
        if (is_harm(act)) { keys.push_back("harm." + rel + "." + race); keys.push_back("harm." + rel); }
        return first_of(keys);
    }
    const Event* event_here(const std::string& kind, const std::string& subject) const {
        for (size_t i = first; i < w.chronicle.size(); ++i)
            if (w.chronicle[i].kind == kind && (subject.empty() || w.chronicle[i].subject == subject)) return &w.chronicle[i];
        return nullptr;
    }
    // the person the act touched (a hostage taken or sent home), else the court's ruler or heir the dragon knows
    std::string person_of(const ReactionNote& n) const {
        if (!n.item.empty() && is_person_id(w, n.item)) return n.item;
        for (const char* k : {"hostage_taken", "hostage_released"})
            for (size_t i = first; i < w.chronicle.size(); ++i)
                if (w.chronicle[i].kind == k && w.chronicle[i].object == n.faction) return w.chronicle[i].subject;
        return "";
    }
    const Known* known_of(const ReactionNote& n) const {
        std::string id = person_of(n);
        if (const Known* k = id.empty() ? nullptr : find_known(w, id)) return k;
        for (const char* role : {"ruler", "heir"})
            if (const Person* x = find_person_role(w, n.faction, role)) if (const Known* k = find_known(w, x->id)) return k;
        return nullptr;
    }
    std::string oath_word(const std::string& faction) const {
        for (const char* pred : {"defend", "no_harm", "tribute"})
            if (active_oath_with_pred(w, faction, pred)) return first_of({std::string("oath.") + pred, "oath.any"});
        return text("oath.any");
    }
    std::string mood(const ReactionNote& n) const { return is_harm(n.act) ? "harm" : (is_help(n.act) ? "help" : "seen"); }

    std::string fill(std::string s, const ReactionNote& n) const {
        const Person* ruler = find_person_role(w, n.faction, "ruler");
        std::string race = race_of(n.faction);
        replace_all(s, "{F}", cap(display_name(w, n.faction)));
        replace_all(s, "{T}", cap(display_name(w, n.target)));
        replace_all(s, "{region}", n.region.empty() ? text("its_lands") : display_name(w, n.region));
        replace_all(s, "{ruler}", ruler ? ruler->name : text("its_ruler"));
        replace_all(s, "{person}", display_name(w, person_of(n)));
        replace_all(s, "{oath}", oath_word(n.faction));
        replace_all(s, "{memory}", first_of({"memory." + race, "memory.any"}));
        replace_all(s, "{m%}", std::to_string(static_cast<int>(n.magnitude * 100 + 0.5)) + "%");
        replace_all(s, "{loot}", first_of({"loot." + n.loot, "loot.gold"}));
        if (const Known* k = known_of(n)) {
            replace_all(s, "{known}", display_name(w, k->id));
            replace_all(s, "{met}", std::to_string(k->met));
        }
        return s;
    }

    // one more sentence: an offer answered or a lie acted on, a person the dragon knows, then a mark a card left
    std::string extra(const ReactionNote& n) const {
        std::string m = mood(n);
        if (const Event* e = event_here("offer_fulfilled", n.faction)) return with_envoy(text("extra.offer_fulfilled"), *e);
        if (const Event* e = event_here("deceived_dragon", n.faction)) return with_envoy(text("extra.deceived"), *e);
        if (n.act == "show_forgotten") return text("tag.dragon_forgotten");
        if (n.act == "witness_burn") return "";
        const Known* k = known_of(n);
        if (k && k->id != person_of(n)) {
            std::string s = first_of({std::string(k->met == w.year ? "extra.known_new." : "extra.known.") + m});
            if (!s.empty()) return s;
        }
        const Faction* f = find_faction(w, n.faction);
        if (!f) return "";
        for (auto& t : f->tags) {
            size_t colon = t.find(':');
            std::string base = t.substr(0, colon);
            if (colon != std::string::npos && t.substr(colon + 1) != "dragon" && (base == "grudge" || base == "sworn")) continue;
            std::string s = first_of({"tag." + base + "." + m, "tag." + base});
            if (!s.empty()) return s;
        }
        return "";
    }
    std::string with_envoy(std::string s, const Event& e) const {
        replace_all(s, "{envoy}", display_name(w, e.object));
        return s;
    }

    std::string line(const ReactionNote& n) const {
        std::string base = row(n);
        if (base.empty()) return "";
        std::string s = fill(base, n);
        if (!two_sentences(s)) {
            std::string more = fill(extra(n), n);
            if (!more.empty()) s += " " + more;
        }
        return s;
    }
};

}

std::vector<ReactionLine> reaction_lines(const World& w, const Params& p, const std::vector<ReactionNote>& notes, size_t first_event) {
    Teller t{w, p, first_event};
    std::vector<ReactionNote> best;   // per court, in the order the courts first reacted
    for (auto& n : notes) {
        auto it = std::find_if(best.begin(), best.end(), [&](const ReactionNote& b) { return b.faction == n.faction; });
        if (it == best.end()) best.push_back(n);
        else if (rank(n) > rank(*it)) *it = n;
    }
    // a court whose ask was answered by an act on someone else still hears of it
    for (size_t i = first_event; i < w.chronicle.size(); ++i) {
        const Event& e = w.chronicle[i];
        if (e.kind != "offer_fulfilled" && e.kind != "deceived_dragon") continue;
        bool noted = std::any_of(best.begin(), best.end(), [&](const ReactionNote& b) { return b.faction == e.subject; });
        if (!noted) best.push_back({e.subject, "offer", e.kind == "offer_fulfilled" ? "fulfilled" : "deceived", "", "", "", 0, ""});
    }
    auto order = [](const ReactionNote& n) { return n.act != "witness_burn" ? 0 : (n.rel == "neutral" ? 2 : 1); };
    std::stable_sort(best.begin(), best.end(), [&](const ReactionNote& a, const ReactionNote& b) { return order(a) < order(b); });
    std::vector<ReactionLine> out;
    for (auto& n : best) {
        if (!find_faction(w, n.faction)) continue;
        std::string s = n.act == "offer" ? t.with_envoy(t.fill(t.text("offer." + n.rel), n), *t.event_here(n.rel == "fulfilled" ? "offer_fulfilled" : "deceived_dragon", n.faction))
                                         : t.line(n);
        if (!s.empty()) out.push_back({n.faction, s});
    }
    return out;
}

std::string reaction_lines_to_json(const std::vector<ReactionLine>& lines) {
    nlohmann::json arr = nlohmann::json::array();
    for (auto& l : lines) arr.push_back({{"faction", l.faction}, {"text", l.text}});
    return arr.dump();
}

}
