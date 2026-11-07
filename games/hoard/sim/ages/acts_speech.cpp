#include "act_kinds.h"
#include "act_gates.h"
#include "memory_tiers.h"
#include "moves.h"
#include "oaths.h"
#include "rules/rules.h"
#include "rules/slayers.h"
#include "world_util.h"

// item 09: teach (on a court with a subject, on a person as the Teach move), broker, reveal, judge

namespace ages {

namespace {

std::string arg(const Act& a, const char* k) {
    auto it = a.args.find(k);
    return it == a.args.end() ? "" : it->second;
}

bool tagged_for_judgement(const World& w, const Person& x) {
    const Faction* f = find_faction(w, x.faction);
    for (const char* t : {"false_dragon", "usurper", "necromancer", "tyrant"}) {
        if (has_tag(x.tags, t)) return true;
        if (f && (has_tag(f->tags, std::string(t) + ":" + x.id))) return true;
    }
    return false;
}

const Secret* open_secret(const World& w, const std::string& court) {
    for (auto& s : w.secrets) if (s.holder == court && !s.exposed) return &s;
    return nullptr;
}

const Region* hidden_region(const World& w, const Act& a) {
    if (const Region* r = find_region(w, arg(a, "about"))) return has_tag(r->tags, "hidden") ? r : nullptr;
    for (auto& r : w.regions) if (has_tag(r.tags, "hidden")) return &r;
    return nullptr;
}

const Faction* plotting_order(const World& w, const Act& a) {
    if (const Faction* f = find_faction(w, arg(a, "about"))) return has_tag(f->tags, "slayers") ? f : nullptr;
    for (auto& f : w.factions) if (f.id != a.target && has_tag(f.tags, "slayers") && !slayer_stage(w, f.id).empty()) return &f;
    return nullptr;
}

const Offer* lie_of(const World& w, const Act& a) {
    std::string liar = arg(a, "about");
    for (auto& o : w.offers) if (o.is_lie && (liar.empty() || o.from_faction == liar)) return &o;
    return nullptr;
}

bool has_pretender(const Faction& f) {
    for (auto& t : f.tags) if (t.rfind("pretender:", 0) == 0) return true;
    return false;
}

void untag_prefix(World& w, Faction& f, const std::string& prefix) {
    for (auto it = f.tags.begin(); it != f.tags.end();) it = it->rfind(prefix, 0) == 0 ? f.tags.erase(it) : std::next(it);
    w.timed_tags.erase(std::remove_if(w.timed_tags.begin(), w.timed_tags.end(),
                                      [&](const TimedTag& t) { return t.on == "faction" && t.target == f.id && t.tag.rfind(prefix, 0) == 0; }),
                       w.timed_tags.end());
}

void betrayed(World& w, const Params& p, const std::string& court) {
    for (auto& o : w.oaths)
        if (o.status == "active" && (o.parties[0] == court || o.parties[1] == court)) { apply_oath_effect(w, p, "betrayal", court); return; }
}

void teach_court(World& w, const Params& p, Faction& f, const std::string& subject) {
    const ActNumbers& n = p.act_book.n;
    if (subject == "craft") f.era_progress += n.craft_era_progress;
    else if (subject == "war") {
        f.culture.traits.militarism = clamp01(f.culture.traits.militarism + n.war_militarism);
        f.anti_dragon += n.war_anti_dragon;
    } else if (subject == "magic") {
        w.magic = clamp01(w.magic + n.magic_world);
        f.tags.insert("arcane");
    } else if (subject == "intrigue") {
        if (Person* r = find_person_role(w, f.id, "ruler")) r->traits.cunning = clamp01(r->traits.cunning + n.intrigue_cunning);
    } else if (subject == "dragon_lore") {
        f.dread.written = std::max(f.dread.written, f.dread.oral);
        f.awe.written = std::max(f.awe.written, f.awe.oral);
    }
}

void broker(World& w, const Params& p, Act& a, int act_index) {
    Faction* x = find_faction(w, a.target);
    Faction* y = find_faction(w, a.against);
    if (!x || !y) return;
    std::string pact = arg(a, "pact");
    auto ally = [&]() { x->tags.insert("ally:" + y->id); y->tags.insert("ally:" + x->id); };
    if (pact == "marriage") { x->tags.insert("married:" + y->id); y->tags.insert("married:" + x->id); ally(); }
    else if (pact == "alliance") ally();
    else if (pact == "trade") {
        add_modifier(w, "dragon.broker_trade", x->id, "growth", "mul", p.act_book.n.trade_growth, p.act_book.n.trade_decades);
        add_modifier(w, "dragon.broker_trade", y->id, "growth", "mul", p.act_book.n.trade_growth, p.act_book.n.trade_decades);
    } else if (pact == "peace") {
        std::vector<War> ended;
        for (auto& war : w.wars) if ((war.a == x->id && war.b == y->id) || (war.a == y->id && war.b == x->id)) ended.push_back(war);
        for (auto& war : ended) end_war(w, war, "brokered_by_dragon", because("brokered", {}, {{"act", act_index}}));
        x = find_faction(w, a.target);
        y = find_faction(w, a.against);
        if (x && y) { erase_value(x->at_war_with, y->id); erase_value(y->at_war_with, x->id); }
    } else if (pact == "vassalage") {
        Faction* small = power(*x, p.c) < power(*y, p.c) ? x : y;
        Faction* big = small == x ? y : x;
        small->tags.insert("vassal_of:" + big->id);
        emit(w, "vassal_sworn", small->id, big->id, 0, {"brokered_by_dragon"}, because("brokered", {}, {{"act", act_index}}));
    }
    if (x && y) emit(w, "pact_brokered", x->id, y->id, 0, {pact}, because("brokered", {}, {{"act", act_index}}));
}

void reveal(World& w, const Params& p, Act& a, int act_index) {
    Faction* f = find_faction(w, a.target);
    if (!f) return;
    std::string s = arg(a, "secret");
    if (s == "hidden_region") {
        if (const Region* r = hidden_region(w, a)) find_region(w, r->id)->tags.erase("hidden");
    } else if (s == "plot") {
        for (auto& sec : w.secrets) {
            if (sec.holder != f->id || sec.exposed) continue;
            sec.exposed = w.year;
            emit(w, "secret_exposed", sec.holder, sec.about, 0, {sec.kind}, because("revealed", {}, {{"act", act_index}}));
            for (size_t i = w.acts.size(); i > 0; --i) {
                const Act& x = w.acts[i - 1];
                if (x.item == sec.about && (x.type == "corrupt" || x.type == "teach")) { learn_act(w, p, *find_faction(w, a.target), static_cast<int>(i - 1)); break; }
            }
            break;
        }
    } else if (s == "heir_location") {
        untag_prefix(w, *f, "pretender:");
    } else if (s == "slayer_plan") {
        if (const Faction* o = plotting_order(w, a)) {
            std::string order = o->id;
            find_faction(w, order)->tags.insert("slayer_plan_known");
            find_faction(w, a.target)->tags.insert("forewarned");
            betrayed(w, p, order);
        }
    } else if (s == "lie") {
        if (const Offer* o = lie_of(w, a)) {
            std::string liar = o->from_faction, id = o->id;
            if (Faction* l = find_faction(w, liar)) l->tags.insert("lie_exposed");
            if (liar != a.target) find_faction(w, a.target)->tags.insert("deceived");
            w.offers.erase(std::remove_if(w.offers.begin(), w.offers.end(), [&](const Offer& x) { return x.id == id; }), w.offers.end());
            betrayed(w, p, liar);
        }
    }
}

void judge(World& w, const Params& p, Act& a, int act_index) {
    Person* x = find_person(w, a.item);
    if (!x) return;
    if (!tagged_for_judgement(w, *x)) a.args["untagged"] = "1";
    if (arg(a, "verdict") == "guilty") kill_person_now(w, p, a.item, "judged_by_dragon", because("judged", {}, {{"act", act_index}}));
    else x->tags.insert("pardoned");
}

}

MoveCtx move_ctx(const World& w, const Person& x, int act_index) {
    MoveCtx c;
    const Dragon& d = w.dragons[kPlayer];
    c.growth = d.growth;
    c.talents = d.talents;
    c.act = act_index;
    for (size_t i = w.acts.size(); i > 0; --i) {
        const Act& g = w.acts[i - 1];
        if (g.year < w.year) break;
        if (g.type == "guise" && g.target == x.faction && !g.args.count("discovered")) c.guise = true;
    }
    return c;
}

std::string speech_refusal(const World& w, const Params& p, const Act& a) {
    if (a.type == "teach" && !a.item.empty()) {
        const Person* x = find_person(w, a.item);
        if (!x) return "act.no_target";
        MoveCtx c = move_ctx(w, *x, -1);
        c.subject = arg(a, "subject");
        return move_refusal(w, p, a.item, "teach", c);
    }
    if (a.type == "broker") {
        const Faction* x = find_faction(w, a.target);
        const Faction* y = find_faction(w, a.against);
        if (!x || !y || x == y) return "act.no_target";
        std::string pact = arg(a, "pact");
        if (pact == "peace" && !at_war(*x, y->id)) return "broker.not_at_war";
        if ((pact == "alliance" || pact == "marriage") && allied(*x, *y)) return "broker.already";
        if (pact == "vassalage" && (is_vassal(*x) || is_vassal(*y))) return "broker.already";
        return "";
    }
    if (a.type == "reveal") {
        const Faction* f = find_faction(w, a.target);
        if (!f) return "act.no_target";
        std::string s = arg(a, "secret");
        bool known = s == "hidden_region" ? hidden_region(w, a) != nullptr
                   : s == "plot" ? open_secret(w, f->id) != nullptr
                   : s == "heir_location" ? has_pretender(*f)
                   : s == "slayer_plan" ? plotting_order(w, a) != nullptr
                   : lie_of(w, a) != nullptr;
        if (!known) return "reveal.nothing|secret=" + s;
        if (s == "lie") return gates_refusal(w, p, p.act_book.reveal_lie);
        return "";
    }
    if (a.type == "judge") {
        const Person* x = find_person(w, a.item);
        if (!x || x->faction.empty()) return "act.no_target";
        if (has_tag(x->tags, "hostage")) return "judge.in_lair";
        return "";
    }
    return "";
}

bool speech_effects(World& w, const Params& p, Act& a, double m, int act_index) {
    (void)m;
    if (a.type == "teach") {
        if (a.item.empty()) { if (Faction* f = find_faction(w, a.target)) teach_court(w, p, *f, arg(a, "subject")); }
        else if (const Person* x = find_person(w, a.item)) {
            MoveCtx c = move_ctx(w, *x, act_index);
            c.subject = arg(a, "subject");
            MoveOutcome o = resolve_move(w, p, a.item, "teach", c);
            a.args["outcome"] = o.accepted ? "accepted" : "refused";
        }
        return true;
    }
    if (a.type == "broker") { broker(w, p, a, act_index); return true; }
    if (a.type == "reveal") { reveal(w, p, a, act_index); return true; }
    if (a.type == "judge") { judge(w, p, a, act_index); return true; }
    return false;
}

}
