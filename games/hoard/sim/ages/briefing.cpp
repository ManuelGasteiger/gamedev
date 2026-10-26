#include <cstdio>
#include "ages.h"
#include "importance.h"
#include "json.h"
#include "names.h"
#include "offers.h"
#include "world_util.h"

// 17.5: the narrator. Every sentence is a template from data/text/en/narrator.json filled with names and numbers; a cause clause
// is appended when the event's refs point at an earlier event (a "<cause>@<kind>" text, keyed by the kind of the cited
// event, wins over the plain cause text). Threads collapse to one paragraph. "While you slept" keeps the events at or
// above importance_min and every event of a thread that holds one.

namespace ages {

using nlohmann::json;

namespace {

struct Narrator {
    const World& w;
    const Params& p;
    const Texts& t;
    Narrator(const World& world, const Params& params) : w(world), p(params), t(params.texts) {}

    static std::string num(double v, int digits) { char b[32]; std::snprintf(b, sizeof b, digits == 0 ? "%.0f" : "%.2f", v); return b; }
    static std::string pct(double v) { char b[32]; std::snprintf(b, sizeof b, "%.0f%%", v * 100); return b; }
    static std::string cap(std::string s) { if (!s.empty() && s[0] >= 'a' && s[0] <= 'z') s[0] = static_cast<char>(s[0] - 32); return s; }

    std::string word(const std::string& k) const {
        auto it = t.words.find(k);
        if (it != t.words.end()) return it->second;
        auto tl = p.talents.index.find(k);   // v0.4: a talent id reads as its name
        return tl == p.talents.index.end() ? k : p.talents.talents[tl->second].name;
    }
    std::string role(const std::string& k) const { auto it = t.roles.find(k); return it == t.roles.end() ? k : it->second; }
    std::string misc(const std::string& k) const { auto it = t.misc.find(k); return it == t.misc.end() ? "" : it->second; }
    std::string misc_with(const std::string& k, const std::string& token, const std::string& v) const {
        std::string s = misc(k);
        replace_all(s, token, v);
        return s;
    }
    std::string list(const std::vector<std::string>& v) const {
        std::string s;
        for (size_t i = 0; i < v.size(); ++i) s += (i == 0 ? "" : (i + 1 == v.size() ? misc("and") : ", ")) + v[i];
        return s;
    }

    std::string name(const std::string& id) const { return display_name(w, id); }
    std::string bare_region(const std::string& id) const { std::string s = name(id); return s.rfind("the ", 0) == 0 ? s.substr(4) : s; }
    std::string race_word(const std::string& r) const { return r == "dwarf" ? "dwarves" : (r == "elf" ? "elves" : (r == "undead" ? "undead" : r + "s")); }

    std::string tag_at(const Event& e, size_t i) const {
        size_t n = 0;
        for (auto& x : e.tags) { if (x.rfind("successor:", 0) == 0 || x == "prehistory") continue; if (n++ == i) return x; }
        return "";
    }

    static void replace_all(std::string& s, const std::string& from, const std::string& to) {
        for (size_t pos = s.find(from); pos != std::string::npos; pos = s.find(from, pos + to.size())) s.replace(pos, from.size(), to);
    }

    // {p:name}, {p:name0}, {p:name%} read the cause params
    static void fill_params(std::string& s, const std::map<std::string, double>& params) {
        for (auto& kv : params) {
            replace_all(s, "{p:" + kv.first + "0}", num(kv.second, 0));
            replace_all(s, "{p:" + kv.first + "%}", pct(kv.second));
            replace_all(s, "{p:" + kv.first + "}", num(kv.second, 2));
        }
    }

    static bool vowel(const std::string& w) { return !w.empty() && std::string("aeiouAEIOU").find(w[0]) != std::string::npos; }
    static std::string a_an(const std::string& w) { return (vowel(w) ? "an " : "a ") + w; }

    // the person's place in the court (ruler, heir, sage, ...) and, for a hero, what made them one (slayer, general, ...)
    std::string person_role(const Event& e) const {
        static const char* const roles[] = {"ruler", "heir", "sage", "priest", "hostage_ruler", "hero"};
        for (const char* r : roles) if (e.tags.count(r)) return role(r);
        return role("hero");
    }
    std::string rose_role(const Event& e) const {
        if (!e.tags.count("hero")) return person_role(e);
        for (auto& x : e.tags) if (x != "hero" && x != "prehistory") return role(x);
        return role("hero");
    }

    std::string fill(std::string s, const Event& e) const {
        std::string t0 = tag_at(e, 0), t1 = tag_at(e, 1);
        if (s.find("{A_rose_role}") != std::string::npos) replace_all(s, "{A_rose_role}", cap(a_an(rose_role(e))));
        if (s.find("{role}") != std::string::npos) replace_all(s, "{role}", person_role(e));
        replace_all(s, "{S}", cap(name(e.subject)));
        replace_all(s, "{s}", name(e.subject));
        replace_all(s, "{O}", cap(name(e.object)));
        replace_all(s, "{o}", name(e.object));
        replace_all(s, "{o_bare}", bare_region(e.object));
        replace_all(s, "{T0}", cap(name(t0)));
        replace_all(s, "{t0_name}", name(t0));
        replace_all(s, "{t0_word}", word(t0));
        replace_all(s, "{t1_word}", word(t1));
        replace_all(s, "{t0_word_the}", word(t0));
        replace_all(s, "{t0_role}", role(t0));
        replace_all(s, "{t1_role}", role(t1));
        replace_all(s, "{t0_race}", race_word(t0));
        replace_all(s, "{s_race}", race_word(e.subject));
        replace_all(s, "{m%}", pct(e.magnitude));
        if (s.find("{m_word}") != std::string::npos) {
            size_t i = static_cast<size_t>(std::max(0.0, e.magnitude + 0.5));
            replace_all(s, "{m_word}", i < t.numbers.size() ? t.numbers[i] : num(e.magnitude, 0));
        }
        replace_all(s, "{o_word}", word(e.object));
        replace_all(s, "{m0}", num(e.magnitude, 0));
        replace_all(s, "{m}", num(e.magnitude, 2));
        replace_all(s, "{year}", std::to_string(e.year));
        if (s.find("{era}") != std::string::npos) {
            size_t i = static_cast<size_t>(std::max(0.0, e.magnitude));
            replace_all(s, "{era}", i < t.era_names.size() ? t.era_names[i] : std::to_string(i));
        }
        if (s.find("{phase_word}") != std::string::npos) {
            size_t i = static_cast<size_t>(std::max(0.0, e.magnitude - 1));
            replace_all(s, "{phase_word}", i < t.phase_names.size() ? t.phase_names[i] : std::to_string(i + 1));
        }
        replace_all(s, "{wake_word}", word(e.object));
        if (s.find("{successors}") != std::string::npos) {
            std::vector<std::string> ids;
            for (auto& x : e.tags) if (x.rfind("successor:", 0) == 0) ids.push_back(name(x.substr(10)));
            replace_all(s, "{successors}", list(ids));
        }
        if (s.find("{led_by}") != std::string::npos) replace_all(s, "{led_by}", e.object.empty() ? "" : misc_with("led_by", "{name}", name(e.object)));
        fill_params(s, e.cause.params);
        return s;
    }

    const TextTemplate* tmpl(const std::string& key) const { auto it = t.events.find(key); return it == t.events.end() ? nullptr : &it->second; }

    double importance(const Event& e) const { return e.importance >= 0 ? e.importance : event_importance(w, p, e); }

    // the headline ranks by importance, then by magnitude where it is a share (counts and eras do not weigh)
    double weight(const Event& e) const { return importance(e) * (1.0 + (e.magnitude <= 1 ? std::max(0.0, e.magnitude) : 0.0)); }

    std::string brief(const Event& e) const {
        const TextTemplate* tt = tmpl(ages::template_key(e));
        if (!tt || tt->brief.empty()) {
            std::string s = misc_with("brief_fallback", "{kind}", e.kind);
            replace_all(s, "{year}", std::to_string(e.year));
            return s;
        }
        return fill(tt->brief, e);
    }

    // "because the plague of 1140 had emptied its treasury": the cause template with {ref} as the brief of the newest ref
    // a cause text for this kind of event ("person_died_deposed.collapse") wins over the shared one ("collapse")
    std::map<std::string, std::string>::const_iterator cause_text(const Event& e) const {
        for (const std::string& k : {ages::template_key(e) + "." + e.cause.text_key, e.kind + "." + e.cause.text_key}) {
            auto it = t.causes.find(k);
            if (it != t.causes.end()) return it;
        }
        return t.causes.find(e.cause.text_key);
    }

    std::string cause_clause(const Event& e) const {
        if (e.cause.text_key.empty()) return "";
        auto it = cause_text(e);
        if (it == t.causes.end() || it->second.empty()) return "";
        std::string s = it->second;
        if (s.find("{ref_oldest}") != std::string::npos) {
            const Event* old = e.cause.refs.empty() ? nullptr : find_event(w, e.cause.refs.front());
            if (!old) return "";
            replace_all(s, "{ref_oldest}", brief(*old));
        }
        if (s.find("{ref}") != std::string::npos || s.find("{ref_year}") != std::string::npos) {
            // the newest ref that is not the opening of the event's own arc; the opening only when nothing else explains it
            const Event* pick = nullptr;
            const Event* opening = nullptr;
            for (size_t i = e.cause.refs.size(); i > 0 && !pick; --i) {
                const Event* r = find_event(w, e.cause.refs[i - 1]);
                if (!r) continue;
                if (!e.thread.empty() && r->thread == e.thread && r->id != e.id) { if (!opening) opening = r; continue; }
                pick = r;
            }
            auto nr = t.causes.find(e.cause.text_key + "_noref");
            if (!pick && opening && nr == t.causes.end()) pick = opening;
            if (!pick) {
                if (nr == t.causes.end()) return "";
                s = nr->second;
            } else {
                // "<cause>@<kind of the cited event>" tells the same cause in words that fit what it cites
                auto by_kind = t.causes.find(e.cause.text_key + "@" + ages::template_key(*pick));
                if (by_kind != t.causes.end()) s = by_kind->second;
                if (s.empty()) return "";
                replace_all(s, "{ref}", brief(*pick));
                replace_all(s, "{ref_year}", std::to_string(pick->year));
            }
        }
        fill_params(s, e.cause.params);
        replace_all(s, "{O}", cap(name(e.object)));
        return s;
    }

    // the cause clause goes where the template puts {why} (after a space), {,why} (after a comma) or {,why,} (between
    // commas, mid-sentence), else at the end
    std::string sentence(const Event& e) const {
        const TextTemplate* tt = tmpl(ages::template_key(e));
        std::string s = tt && !tt->text.empty() ? fill(tt->text, e) : cap(name(e.subject)) + ": " + e.kind + ".";
        std::string why = cause_clause(e);
        if (s.find("{why}") != std::string::npos || s.find("{,why") != std::string::npos) {
            replace_all(s, "{why}", why.empty() ? "" : " " + why);
            replace_all(s, "{,why,}", why.empty() ? "" : ", " + why + ",");
            replace_all(s, "{,why}", why.empty() ? "" : ", " + why);
        } else if (!why.empty()) {
            if (!s.empty() && s.back() == '.') s.pop_back();
            s += ", " + why + ".";
        }
        if (e.count > 1) s += " (x" + std::to_string(e.count) + ")";
        return s;
    }

    // a war that ends in submission is told by the oath of fealty that follows it in the same year
    bool told_by_fealty(const std::vector<Event>& evs, size_t i) const {
        const Event& e = evs[i];
        if (e.kind != "war_ended" || !e.tags.count("submission")) return false;
        for (auto& x : evs) if (x.kind == "vassal_sworn" && x.year == e.year && (x.subject == e.object || x.subject == e.subject)) return true;
        return false;
    }

    // a succession reads as one line: "X of Y died; Z took the throne"
    bool merge_succession(const std::vector<Event>& evs, size_t i, std::string& out) const {
        if (evs[i].kind != "person_died" || !evs[i].tags.count("ruler") || i + 1 >= evs.size()) return false;
        const Event& c = evs[i + 1];
        if (c.kind != "person_crowned" || c.object != evs[i].object || c.year != evs[i].year) return false;
        const TextTemplate* tt = tmpl("succession");
        if (!tt) return false;
        std::string s = fill(tt->text, c);
        replace_all(s, "{dead}", cap(name(evs[i].subject)));
        std::string why = cause_clause(evs[i]);
        if (!why.empty() && evs[i].cause.text_key != "old_age") { s.pop_back(); s += ", " + why + "."; }
        out = s;
        return true;
    }
};

// the sleep bookkeeping, heirs being named, tribute, the lives of courtiers the dragon never met, and the lines a
// collapse sentence already covers (its successors, the deposed ruler); great rulers are told through reign_named
bool skip_in_briefing(const World& w, const Event& e) {
    if (e.kind == "sleep_begin" || e.kind == "wake") return true;
    if (e.kind == "tribute_paid" || e.kind == "bad_century" || e.kind == "tyrant_rises" || e.kind == "golden_age") return true;
    if (e.kind == "successor_founded" && e.cause.text_key == "collapse") return true;
    if (e.kind == "person_rose") return e.tags.count("heir") || ((e.tags.count("sage") || e.tags.count("priest")) && !find_known(w, e.subject));
    if (e.kind == "person_died") {
        if (e.tags.count("deposed") && e.cause.text_key == "collapse") return true;
        return !e.tags.count("ruler") && !find_known(w, e.subject) && !e.tags.count("slain_by_dragon");
    }
    return false;
}

}

std::string event_sentence(const World& w, const Params& p, const Event& e) { return Narrator(w, p).sentence(e); }

std::string briefing_json(const World& w, const Params& p, int since_year) {
    Narrator n(w, p);
    const Constants& c = p.c;
    // everything from the last sleep_begin at or after since_year (the acts of that raid included)
    std::vector<Event> slept;
    size_t start = 0;
    for (size_t i = 0; i < w.chronicle.size(); ++i) if (w.chronicle[i].kind == "sleep_begin" && w.chronicle[i].year >= since_year) { start = i; break; }
    for (size_t i = start; i < w.chronicle.size(); ++i) if (w.chronicle[i].year >= since_year) slept.push_back(w.chronicle[i]);

    json out;
    out["ok"] = true;
    out["since"] = since_year;
    out["year"] = w.year;

    // 1. headline
    const Event* top = nullptr;
    for (auto& e : slept) if (!skip_in_briefing(w, e) && (!top || n.weight(e) > n.weight(*top))) top = &e;
    out["headline"] = top ? json{{"text", n.sentence(*top)}, {"event", top->id}, {"year", top->year}, {"kind", top->kind}, {"importance", n.importance(*top)}}
                          : json{{"text", n.misc("headline_none")}};

    // 2. while you slept: chronological groups of briefing_group_years; a thread with 3+ lines in a group collapses to
    // first ... last; events below importance_min drop out unless their thread holds an important one
    std::set<std::string> weighty;
    for (auto& e : slept) if (!e.thread.empty() && n.importance(e) >= c.importance_min) weighty.insert(e.thread);
    json groups = json::array();
    std::map<int, std::vector<Event>> by_group;
    for (auto& e : slept) {
        if (skip_in_briefing(w, e)) continue;
        if (n.importance(e) < c.importance_min && !weighty.count(e.thread)) continue;
        by_group[(e.year - since_year) / std::max(1, c.briefing_group_years)].push_back(e);
    }
    for (auto& kv : by_group) {
        std::vector<Event>& evs = kv.second;
        json lines = json::array();
        std::map<std::string, int> thread_count;
        for (auto& e : evs) if (!e.thread.empty()) thread_count[e.thread] += 1;
        std::set<std::string> threads_done;
        for (size_t i = 0; i < evs.size(); ++i) {
            const Event& e = evs[i];
            if (!e.thread.empty() && thread_count[e.thread] >= 3) {
                if (threads_done.count(e.thread)) continue;
                threads_done.insert(e.thread);
                const Event* last = &e;
                for (size_t j = i; j < evs.size(); ++j) if (evs[j].thread == e.thread) last = &evs[j];
                std::string para = n.sentence(e);
                if (thread_count[e.thread] == 3) {
                    for (size_t j = i + 1; j < evs.size(); ++j) if (evs[j].thread == e.thread && &evs[j] != last) { para += " " + n.sentence(evs[j]); break; }
                    para += " " + n.sentence(*last);
                } else {
                    std::string more = n.misc("thread_more");
                    Narrator::replace_all(more, "{n}", std::to_string(thread_count[e.thread] - 2));
                    para += more + n.sentence(*last);
                }
                double top_imp = 0;
                for (auto& x : evs) if (x.thread == e.thread) top_imp = std::max(top_imp, n.importance(x));
                lines.push_back({{"text", para}, {"year", e.year}, {"thread", e.thread}, {"events", thread_count[e.thread]}, {"importance", top_imp}});
                continue;
            }
            if (n.told_by_fealty(evs, i)) continue;
            std::string merged;
            if (n.merge_succession(evs, i, merged)) {
                lines.push_back({{"text", merged}, {"year", e.year}, {"event", evs[i + 1].id}, {"importance", std::max(n.importance(e), n.importance(evs[i + 1]))}});
                i += 1;
                continue;
            }
            json line = {{"text", n.sentence(e)}, {"year", e.year}, {"event", e.id}, {"importance", n.importance(e)}};
            if (!e.thread.empty()) line["thread"] = e.thread;
            lines.push_back(line);
        }
        int from = since_year + kv.first * c.briefing_group_years;
        groups.push_back({{"from", from}, {"to", std::min(w.year, from + c.briefing_group_years - 1)}, {"lines", lines}});
    }
    out["while_you_slept"] = groups;

    // 3. people: known people who died, rose or were crowned; the rulers of every court the dragon has an oath with
    json people = json::array();
    std::set<std::string> named;
    for (auto& e : slept) {
        if (e.kind != "person_died" && e.kind != "person_crowned" && e.kind != "raised_in_lair" && e.kind != "hostage_taken" && e.kind != "hostage_released") continue;
        if (!find_known(w, e.subject)) continue;
        people.push_back({{"text", n.sentence(e)}, {"person", e.subject}, {"year", e.year}, {"kind", e.kind}, {"importance", n.importance(e)}});
        named.insert(e.subject);
    }
    for (auto& o : w.oaths) {
        if (o.status != "active") continue;
        const Faction* f = find_faction(w, o.parties[1]);
        const Person* r = f ? find_person_role(w, f->id, "ruler") : nullptr;
        if (!r || named.count(r->id)) continue;
        named.insert(r->id);
        std::string s = n.misc("people_ruler");
        Narrator::replace_all(s, "{faction}", Narrator::cap(f->id));
        Narrator::replace_all(s, "{name}", r->name);
        std::string known;
        if (const Known* k = find_known(w, r->id)) {
            known = n.misc("known_suffix");
            const char* rel = k->relationship > 0.5 ? "relation_friend" : (k->relationship > 0.1 ? "relation_warm" : (k->relationship < -0.5 ? "relation_enemy" : (k->relationship < -0.1 ? "relation_cold" : "relation_neutral")));
            Narrator::replace_all(known, "{relation}", n.misc(rel));
        }
        Narrator::replace_all(s, "{known}", known);
        people.push_back({{"text", s}, {"person", r->id}, {"faction", f->id}, {"kind", "ruler"}});
    }
    out["people"] = people;

    // 4. threads: oaths, hostages, spells, remnants, tags
    json threads = json::array();
    for (auto& o : w.oaths) {
        if (o.status == "expired") continue;
        if (o.status != "active" && last_event_id(w, "", o.id) && find_event(w, last_event_id(w, "", o.id))->year < since_year) continue;
        std::string s = n.misc("oath_line");
        std::vector<std::string> cl;
        for (auto& x : o.clauses) cl.push_back(n.word(x.pred));
        Narrator::replace_all(s, "{status}", Narrator::cap(o.status == "active" ? "active" : o.status));
        Narrator::replace_all(s, "{faction}", o.parties[1]);
        Narrator::replace_all(s, "{clauses}", cl.empty() ? n.word(o.kind.empty() ? "offer.tribute" : "offer." + o.kind) : n.list(cl));
        Narrator::replace_all(s, "{expires}", std::to_string(o.expires));
        Narrator::replace_all(s, "_", " ");
        threads.push_back({{"text", s}, {"kind", "oath"}, {"id", o.id}, {"status", o.status}});
    }
    for (auto& id : w.dragons[kPlayer].hostages) {
        const Person* x = find_person(w, id);
        if (!x) continue;
        std::string s = n.misc("hostage_line");
        Narrator::replace_all(s, "{name}", x->name);
        Narrator::replace_all(s, "{role}", n.role(x->role));
        Narrator::replace_all(s, "{faction}", x->faction);
        Narrator::replace_all(s, "{years}", std::to_string(w.year - x->hostage_since));
        Narrator::replace_all(s, "{raised}", has_tag(x->tags, "dragon_raised") ? n.misc("hostage_raised") : "");
        threads.push_back({{"text", s}, {"kind", "hostage"}, {"id", id}});
    }
    for (auto& m : w.modifiers) {
        if (m.source.rfind("dragon.", 0) != 0) continue;
        std::string s = n.misc("spell_line");
        Narrator::replace_all(s, "{spell}", Narrator::cap(m.source.substr(7)));
        Narrator::replace_all(s, "{target}", n.name(m.target));
        Narrator::replace_all(s, "{until}", std::to_string(m.until));
        threads.push_back({{"text", s}, {"kind", "spell"}, {"id", m.source + ":" + m.target}});
    }
    for (auto& r : w.remnants) {
        if (r.strength <= c.remnant_min_strength) continue;
        std::string s = n.misc("remnant_line");
        Narrator::replace_all(s, "{race}", r.race);
        Narrator::replace_all(s, "{race_plural}", n.race_word(r.race));
        Narrator::replace_all(s, "{strength}", Narrator::num(r.strength, 2));
        Narrator::replace_all(s, "{origin}", n.name(r.origin));
        threads.push_back({{"text", s}, {"kind", "remnant"}, {"id", r.race}, {"strength", r.strength}});
    }
    for (auto& f : w.factions)
        for (auto& tag : f.tags) {
            if (tag.rfind("deceived_dragon:", 0) != 0 && tag != "slayers" && tag != "dragon_raised" && tag != "hegemon") continue;
            std::string s = n.misc("tag_line");
            Narrator::replace_all(s, "{faction}", Narrator::cap(f.id));
            Narrator::replace_all(s, "{tag}", tag.substr(0, tag.find(':')));
            threads.push_back({{"text", s}, {"kind", "tag"}, {"id", f.id + ":" + tag}});
        }
    for (auto& tid : w.threads_open) {
        int last = 0;
        for (size_t i = w.chronicle.size(); i > 0; --i) if (w.chronicle[i - 1].thread == tid) { last = w.chronicle[i - 1].id; break; }
        if (const Event* e = find_event(w, last)) threads.push_back({{"text", n.sentence(*e)}, {"kind", "arc"}, {"id", tid}});
    }
    out["threads"] = threads;

    // 5. opportunities: offers with their stated reason, wars to join, rich undefended courts, slayer orders forming
    json opps = json::array();
    for (auto& of : w.offers) {
        std::string s = n.misc("opportunity_offer");
        Narrator::replace_all(s, "{envoy}", of.envoy.empty() ? n.misc("an_envoy") : n.name(of.envoy));
        Narrator::replace_all(s, "{faction}", Narrator::cap(of.from_faction));
        auto ask = p.texts.asks.find(of.kind);
        std::string a = ask == p.texts.asks.end() ? of.kind : ask->second;
        Narrator::replace_all(a, "{target}", of.target.empty() ? of.from_faction : n.name(of.target));
        Narrator::replace_all(a, "{price0}", Narrator::num(of.price, 0));
        Narrator::replace_all(s, "{ask}", a);
        std::string stated;
        auto pre = p.texts.pretexts.find(of.stated_reason);
        auto mot = p.texts.motives.find(of.stated_reason);
        if (pre != p.texts.pretexts.end()) { stated = pre->second; Narrator::replace_all(stated, "{target}", of.target.empty() ? Narrator::cap(of.from_faction) : n.name(of.target)); }
        else if (mot != p.texts.motives.end()) { stated = mot->second; Narrator::replace_all(stated, "{envoy}", n.name(of.envoy)); }
        else stated = of.stated_reason;
        Narrator::replace_all(s, "{stated}", stated);
        std::string eyes;
        if (w.dragons[kPlayer].knowledge >= c.offer_truth_knowledge && of.is_lie) eyes += n.misc("eyes_say_otherwise");
        if (of.marked) eyes += n.misc("marked_envoy");
        Narrator::replace_all(s, "{eyes}", eyes);
        opps.push_back({{"text", s}, {"kind", "offer"}, {"id", of.id}, {"offer_kind", of.kind}, {"faction", of.from_faction}});
    }
    for (auto& f : w.factions) {
        if (!losing(w, c, f)) continue;
        std::string enemy;
        for (auto& e : f.at_war_with) { enemy = e; break; }
        std::string s = n.misc("opportunity_war");
        Narrator::replace_all(s, "{faction}", Narrator::cap(f.id));
        Narrator::replace_all(s, "{enemy}", Narrator::cap(enemy));
        opps.push_back({{"text", s}, {"kind", "war"}, {"faction", f.id}});
    }
    std::vector<const Faction*> rich;
    for (auto& f : w.factions) if (f.fear < c.briefing_rich_fear) rich.push_back(&f);
    std::sort(rich.begin(), rich.end(), [](const Faction* a, const Faction* b) { return a->wealth > b->wealth || (a->wealth == b->wealth && a->id < b->id); });
    for (size_t i = 0; i < rich.size() && i < static_cast<size_t>(c.briefing_rich_count); ++i) {
        std::string s = n.misc("opportunity_rich");
        Narrator::replace_all(s, "{faction}", Narrator::cap(rich[i]->id));
        Narrator::replace_all(s, "{wealth}", Narrator::num(rich[i]->wealth, 0));
        Narrator::replace_all(s, "{fear}", Narrator::num(rich[i]->fear, 2));
        opps.push_back({{"text", s}, {"kind", "rich"}, {"faction", rich[i]->id}, {"wealth", rich[i]->wealth}, {"fear", rich[i]->fear}});
    }
    for (auto& f : w.factions) {
        if (has_tag(f.tags, "slayers") || f.anti_dragon < c.briefing_slayers_forming) continue;
        std::string s = n.misc("opportunity_slayers");
        Narrator::replace_all(s, "{faction}", Narrator::cap(f.id));
        Narrator::replace_all(s, "{progress}", Narrator::num(f.anti_dragon, 2));
        opps.push_back({{"text", s}, {"kind", "slayers"}, {"faction", f.id}, {"progress", f.anti_dragon}});
    }
    out["opportunities"] = opps;

    std::string ph = n.misc("phase_line");
    size_t pi = static_cast<size_t>(phase_index(w.dragons[kPlayer]));
    Narrator::replace_all(ph, "{phase}", std::to_string(w.dragons[kPlayer].phase));
    Narrator::replace_all(ph, "{phase_word}", pi < p.texts.phase_names.size() ? p.texts.phase_names[pi] : "");
    Narrator::replace_all(ph, "{short}", std::to_string(short_sleep(w.dragons[kPlayer], c)));
    Narrator::replace_all(ph, "{long}", std::to_string(long_sleep(w.dragons[kPlayer], c)));
    Narrator::replace_all(ph, "{ready}", std::string(ready_to_moult(w.dragons[kPlayer], c) ? n.misc("ready_to_moult") : "") + (has_tag(w.dragons[kPlayer].tags, "stiff") ? n.misc("stiff") : ""));
    out["phase"] = {{"text", ph}, {"phase", w.dragons[kPlayer].phase}, {"ready_to_moult", ready_to_moult(w.dragons[kPlayer], c)}};
    return out.dump();
}

}
