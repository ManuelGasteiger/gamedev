#include "moves.h"
#include <algorithm>
#include "memory_tiers.h"
#include "person_record.h"
#include "rules/rules.h"
#include "world_util.h"

namespace ages {

static double growth_of(const MoveCtx& ctx, const char* axis) {
    auto it = ctx.growth.find(axis);
    return it == ctx.growth.end() ? 0 : it->second;
}

static bool at_lair(const World& w, const Person& x) {
    if (has_tag(x.tags, "hostage")) return true;
    for (auto& a : w.arrivals) if (a.person == x.id) return true;
    return false;
}

// a hard precondition that fails returns its reason code (ages.h reason_code style); "" when the move may be tried
static std::string precondition(const World& w, const Person& x, const MoveDef& d, const MoveCtx& ctx) {
    if (d.access && !at_lair(w, x) && !ctx.guise) return "move_no_access";
    double lore = growth_of(ctx, "lore"), arcana = growth_of(ctx, "arcana");
    if (!d.lore.empty()) {
        auto need = d.lore.find(ctx.subject);
        if (need == d.lore.end()) return "move_no_subject|subject=" + ctx.subject;
        if (lore < need->second) return "needs_axis|axis=lore|n=" + std::to_string(static_cast<int>(need->second));
        auto arc = d.arcana.find(ctx.subject);
        if (arc != d.arcana.end() && arcana < arc->second) return "needs_axis|axis=arcana|n=" + std::to_string(static_cast<int>(arc->second));
    }
    if (d.arcana_min > 0 && arcana < d.arcana_min) return "needs_axis|axis=arcana|n=" + std::to_string(static_cast<int>(d.arcana_min));
    if (d.lore_min > 0 && lore < d.lore_min && !(d.guise_opens && ctx.guise)) return "needs_axis|axis=lore|n=" + std::to_string(static_cast<int>(d.lore_min));
    return "";
}

static int count_ev(const Person& x, const std::string& ev) {
    int n = 0;
    for (auto* v : {&x.mem, &x.permanent})
        for (auto& m : *v) if (m.ev == ev) ++n;
    return n;
}

// trait weights + the shared modifiers (each axis up to its cap) + the memory term (mercy, betrayal, earlier
// successes of this move), clamped
double move_score(const World& w, const Params& p, const Person& x, const MoveDef& d) {
    (void)w;
    double score = 0;
    for (auto& t : x.traits.list) {
        auto it = d.weights.find(t);
        if (it != d.weights.end()) score += it->second;
    }
    Axes a = axes(p, x);
    for (auto& mod : d.mods) {
        double v = mod.first == "dread" ? a.dread : mod.first == "awe" ? a.awe : a.debt;
        score += std::min(v, mod.second);
    }
    const MoveBook& b = p.moves;
    double term = b.term_mercy * count_ev(x, "mercy") + b.term_betrayal * count_ev(x, "betrayal") + b.term_same * count_ev(x, d.accept.ev);
    return score + std::max(-b.term_clamp, std::min(b.term_clamp, term));
}

static void write(World& w, const Params& p, Person& x, const MemoryRow& row, const std::string& src, int act, bool hidden, MoveOutcome& out) {
    PersonMemory m = memory_entry(w, p, x, row, src, act);
    m.hidden = hidden;
    remember_dragon(p, x, m);
    out.memory_written.push_back({x.id, m});
}

static std::string substitute(std::string s, const MoveCtx& ctx) {
    auto put = [&](const std::string& token, const std::string& v) {
        size_t at = s.find(token);
        if (at != std::string::npos) s.replace(at, token.size(), v);
    };
    put("$subject", ctx.subject);
    put("$toward", ctx.toward);
    return s;
}

static void add_secret(World& w, const Params& p, const Person& x, const std::string& kind) {
    (void)p;
    Secret s;
    s.holder = x.faction;
    s.about = x.id;
    s.kind = kind;
    s.year = w.year;
    w.secrets.push_back(s);
}

static void hand_over(World& w, const Params& p, Person& giver, const MoveDef& d, const MoveCtx& ctx, MoveOutcome& out) {
    Faction* f = find_faction(w, giver.faction);
    if (!f) return;
    Person* h = ctx.hostage.empty() ? pick_hostage(w, f->id) : find_person(w, ctx.hostage);
    if (!h || h->faction != f->id || has_tag(h->tags, "hostage")) return;
    std::string hid = h->id;
    f->tags.insert("hostage_taken");
    if (!contains(w.dragons[kPlayer].lineages, f->id)) w.dragons[kPlayer].lineages.push_back(f->id);
    take_hostage_person(w, p, *h, true, because("threatened", {out.event}));
    if (Person* t = find_person(w, hid)) write(w, p, *t, d.hostage_taken, "witness", ctx.act, false, out);
}

static void effects(World& w, const Params& p, const std::string& id, const MoveDef& d, const MoveCtx& ctx, MoveOutcome& out) {
    Person* x = find_person(w, id);
    if (!x) return;
    bool giving = d.has_hostage && ctx.demand == "hostage";
    write(w, p, *x, giving ? d.hostage_giver : d.accept, "witness", ctx.act, false, out);
    if (!d.acquire.empty()) {
        auto it = d.acquire.find(ctx.subject);
        if (it == d.acquire.end()) it = d.acquire.find("*");
        if (it != d.acquire.end()) acquire(p, *x, substitute(it->second, ctx));
        if (d.secret) add_secret(w, p, *x, substitute(it == d.acquire.end() ? std::string() : it->second, ctx));
    } else if (d.secret) add_secret(w, p, *x, d.id);
    if (!d.rel.empty()) {
        auto rw = p.moves.rel.find(d.rel);
        add_rel(*x, "dragon", d.rel, rw == p.moves.rel.end() ? 0 : rw->second);
    }
    if (d.witnessed && (d.court_dread != 0 || d.court_awe != 0))
        if (Faction* f = find_faction(w, x->faction)) {
            double told = p.moves.src.count("told") ? p.moves.src.at("told") : 0.6;
            Deposit dep;
            dep.type = "move_" + d.id;
            dep.magnitude = 1;
            dep.act = ctx.act;
            dep.dread = d.court_dread * told;
            dep.awe = d.court_awe * told;
            deposit(w, p, *f, dep);
        }
    if (giving) hand_over(w, p, *x, d, ctx, out);
}

MoveOutcome resolve_move(World& w, const Params& p, const std::string& id, const std::string& move, const MoveCtx& ctx) {
    MoveOutcome out;
    auto def = p.moves.moves.find(move);
    Person* x = find_person(w, id);
    if (def == p.moves.moves.end()) { out.refusal = "move_unknown|move=" + move; return out; }
    if (!x) { out.refusal = "move_no_person"; return out; }
    const MoveDef& d = def->second;
    out.refusal = precondition(w, *x, d, ctx);
    if (!out.refusal.empty()) return out;
    out.score = move_score(w, p, *x, d);
    out.margin = out.score - d.threshold;
    out.accepted = out.score >= d.threshold;
    std::string how = !out.accepted ? "refused" : out.margin < 2 ? "reluctant" : "accepted";
    out.chronicle_line_key = "ev.move_" + d.id + "_" + how;
    bool humiliated = false;
    if (d.has_humiliated) for (auto& t : d.humiliated_traits) if (has_trait(*x, t)) humiliated = true;
    out.event = emit(w, "person_move", id, d.id, out.score, {how, d.secret ? "secret" : "open"},
                     because("move_" + how, {}, {{"score", out.score}, {"threshold", d.threshold}}));
    w.chronicle.back().text_key = out.chronicle_line_key;
    meet(w, id, "move_" + d.id, out.accepted ? d.ledger : 0.0);
    if (out.accepted) effects(w, p, id, d, ctx, out);
    if (humiliated) if (Person* y = find_person(w, id)) write(w, p, *y, d.humiliated, "witness", ctx.act, true, out);
    return out;
}

static std::string reading_of(const Params& p, const Person& x) {
    if (!x.traits.list.empty()) {
        auto it = p.moves.release_by_trait.find(x.traits.list.front());
        if (it != p.moves.release_by_trait.end()) return it->second;
    }
    return p.moves.release_default;
}

static void answer_terms(World& w, const Params& p, const Person& x, ReleaseReading& r) {
    for (auto& o : w.oaths) {
        if (o.status != "draft" || o.kind != "release_terms" || o.envoy != x.id) continue;
        r.terms = true;
        Axes a = axes(p, x);
        r.sworn = a.dread + a.debt >= p.moves.terms_accept;
        o.status = r.sworn ? "active" : "refused";
        o.sworn = w.year;
        o.parties[0] = "dragon";
        o.parties[1] = x.faction;
        emit(w, r.sworn ? "terms_sworn" : "terms_refused", x.faction, x.id, a.dread + a.debt, {}, because("released_with_terms"), "oath:" + o.id);
        return;
    }
}

ReleaseReading read_release(World& w, const Params& p, const std::string& id) {
    ReleaseReading r;
    Person* x = find_person(w, id);
    if (!x || p.moves.readings.empty()) return r;
    r.reading = reading_of(p, *x);
    auto row = p.moves.readings.find(r.reading);
    if (row == p.moves.readings.end()) return r;
    r.m = memory_entry(w, p, *x, row->second, "witness");
    remember_dragon(p, *x, r.m);
    answer_terms(w, p, *x, r);
    return r;
}

void release_terms(World& w, const Oath& draft, const std::string& person) {
    Oath o = draft;
    o.status = "draft";
    o.kind = "release_terms";
    o.envoy = person;
    if (o.id.empty()) o.id = "terms_" + person + "_" + std::to_string(w.year);
    w.oaths.push_back(o);
}

}
