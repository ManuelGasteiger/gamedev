#include "act_kinds.h"
#include "defences.h"
#include "memory_tiers.h"
#include "moves.h"
#include "person_record.h"
#include "rules/rules.h"
#include "rules/slayers.h"
#include "world_util.h"

// item 09: haunt, guise, consecrate, corrupt on a person, the curse kinds. Rolls draw from rng_for, never world.rng.

namespace ages {

namespace {

std::string arg(const Act& a, const char* k) {
    auto it = a.args.find(k);
    return it == a.args.end() ? "" : it->second;
}

double roll(const World& w, const std::string& tag, const std::string& id) {
    Rng r = rng_for(w.seed, tag, id + ":" + std::to_string(w.year));
    return r.uniform();
}

std::string consecrate_region(const World& w, const Act& a) {
    if (!a.region.empty()) return a.region;
    const Faction* f = find_faction(w, a.target);
    return f ? capital_region(w, *f) : "";
}

void haunt(World& w, const Params& p, Act& a, int act_index) {
    Person* x = find_person(w, a.item);
    if (!x) return;
    const ActNumbers& n = p.act_book.n;
    std::string msg = arg(a, "message");
    auto row = n.haunt.find(msg);
    if (row != n.haunt.end()) remember_dragon(p, *x, memory_entry(w, p, *x, row->second, "dream", act_index));
    x->tags.insert("haunted:" + msg);
    meet(w, x->id, "haunt");
    double r = roll(w, "haunt", x->id);
    std::string id = x->id, court = x->faction;
    if ((x->role == "priest" || x->role == "sage") && msg == "prophecy" && r < n.haunt_cult) {
        if (Faction* f = find_faction(w, court)) f->tags.insert("cult_seed");
        a.args["outcome"] = "cult_seed";
    } else if (x->role == "ruler" && (msg == "warning" || msg == "nightmare") && r < n.haunt_abdicate) {
        a.args["outcome"] = "abdicated";
        fall_person(w, p, id, "abdicated", because("haunted", {}, {{"act", act_index}}));
    } else if (x->role == "hero" && msg == "nightmare" && r < n.haunt_desert * (1.0 - x->traits.bold)) {
        a.args["outcome"] = "deserted";
        fall_person(w, p, id, "deserted", because("haunted", {}, {{"act", act_index}}));
    }
}

void guise(World& w, const Params& p, Act& a, int act_index) {
    Faction* f = find_faction(w, a.target);
    if (!f) return;
    const ActNumbers& n = p.act_book.n;
    int sages = 0;
    for (auto& x : w.people) if (x.faction == f->id && x.role == "sage") ++sages;
    double odds = n.guise_discovery + n.guise_per_sage * sages + (has_tag(f->tags, "forewarned") ? n.guise_forewarned : 0);
    std::string purpose = arg(a, "purpose");
    if (const Person* ruler = find_person_role(w, f->id, "ruler")) meet(w, ruler->id, "guise");
    if (roll(w, "guise", f->id) < odds) {
        a.args["discovered"] = "1";
        w.acts[static_cast<size_t>(act_index)].args = a.args;
        f->tags.insert("deceived");
        learn_act(w, p, *f, act_index);
        Deposit d;
        auto show = p.gains.find("show");
        d.type = "show";
        d.magnitude = n.guise_show;
        d.act = act_index;
        if (show != p.gains.end()) { d.dread = show->second.dread * n.guise_show; d.awe = show->second.awe * n.guise_show * (1.0 - dread_eff(*f)); }
        deposit(w, p, *f, d);
        return;
    }
    if (purpose == "visit") learn_act(w, p, *f, act_index);
    else if (purpose == "court") {
        std::string who = arg(a, "person");
        Person* x = who.empty() ? find_person_role(w, f->id, "heir") : find_person(w, who);
        if (x && x->faction == f->id) { x->tags.insert("willing"); meet(w, x->id, "guise"); a.args["person"] = x->id; }
    } else if (purpose == "learn") {
        if (keeps_records(w, *f)) w.dragons[kPlayer].knowledge += n.guise_learn_knowledge;
    } else if (purpose == "spy") {
        if (has_tag(f->tags, "slayers")) f->tags.insert("slayer_plan_known");
        for (auto& s : w.secrets) if (s.holder == f->id && !s.exposed) meet(w, s.about, "guise");
    } else if (purpose == "preach") {
        f->faith = clamp01(f->faith + n.guise_preach_faith);
        f->tags.insert("cult_seed");
    }
}

void consecrate(World& w, const Params& p, Act& a, int act_index) {
    (void)p;
    Region* r = find_region(w, consecrate_region(w, a));
    if (!r) return;
    Faction* f = find_faction(w, r->owner);
    r->tags.insert("dragon_temple");
    if (!f) return;
    Monument mon;
    mon.bias = "dragon_temple";
    mon.built = w.year;
    mon.act = act_index;
    mon.region = r->id;
    f->monuments.push_back(mon);
    refresh_monumental(*f);
    f->tags.insert("consecrated");
    f->faith = std::max(f->faith, p.c.mem.faith_floor);
    emit(w, "dragon_temple_raised", f->id, r->id, 0, {"consecrated"}, because("consecrated", {}, {{"act", act_index}}));
}

void curse_kind(World& w, const Params& p, const Act& a) {
    std::string kind = arg(a, "kind");
    if (kind.empty() || kind == "lineage") return;
    const ActNumbers& n = p.act_book.n;
    for (size_t i = w.modifiers.size(); i > 0; --i)
        if (w.modifiers[i - 1].source == "dragon.curse" && w.modifiers[i - 1].target == a.target && w.modifiers[i - 1].from == w.year) {
            Modifier m = w.modifiers[i - 1];
            w.modifiers.erase(w.modifiers.begin() + static_cast<std::ptrdiff_t>(i - 1));
            int decades = (m.until - m.from) / p.c.step_years;
            if (kind == "research") {
                int stacks = 0;
                for (auto& x : w.modifiers) if (x.source == "dragon.curse_research" && x.target == a.target) ++stacks;
                if (stacks < n.curse_research_stacks) add_modifier(w, "dragon.curse_research", a.target, "anti_dragon_rate", "mul", n.curse_research_mult, decades);
            } else if (const Faction* f = find_faction(w, a.target)) {
                std::vector<std::string> regions = f->regions;
                for (auto& rid : regions) add_modifier(w, "dragon.curse_blight", rid, "land", "mul", n.curse_blight_land, decades);
            }
            return;
        }
}

}

std::string arcana_refusal(const World& w, const Params& p, const Act& a) {
    if (a.type == "haunt") return find_person(w, a.item) ? "" : "act.no_target";
    if (a.type == "guise") return find_faction(w, a.target) ? "" : "act.no_target";
    if (a.type == "consecrate") {
        const Region* r = find_region(w, consecrate_region(w, a));
        if (!r) return "act.no_target";
        if (has_tag(r->tags, "dragon_temple")) return "consecrate.already";
        const Faction* f = find_faction(w, r->owner);
        bool site = has_tag(r->tags, "temple") || has_tag(r->tags, "altar") || (f && awe_eff(*f) >= p.c.mem.temple_awe);
        return site ? "" : "consecrate.no_temple";
    }
    if (a.type == "corrupt" && !a.item.empty()) {
        const Person* x = find_person(w, a.item);
        if (!x) return "act.no_target";
        MoveCtx c = move_ctx(w, *x, -1);
        c.toward = arg(a, "toward");
        return move_refusal(w, p, a.item, "corrupt", c);
    }
    return "";
}

bool arcana_effects(World& w, const Params& p, Act& a, double m, int act_index) {
    (void)m;
    if (a.type == "haunt") { haunt(w, p, a, act_index); return true; }
    if (a.type == "guise") { guise(w, p, a, act_index); return true; }
    if (a.type == "consecrate") { consecrate(w, p, a, act_index); return true; }
    if (a.type == "corrupt") {
        if (const Person* x = find_person(w, a.item)) {
            MoveCtx c = move_ctx(w, *x, act_index);
            c.toward = arg(a, "toward");
            MoveOutcome o = resolve_move(w, p, a.item, "corrupt", c);
            a.args["outcome"] = o.accepted ? "accepted" : "refused";
        }
        return true;
    }
    if (a.type == "curse") { curse_kind(w, p, a); return true; }
    return false;
}

}
