#include "act_kinds.h"
#include "act_gates.h"
#include "hooks.h"
#include "names.h"
#include "rules/rules.h"
#include "world_util.h"

// item 09: keep (every role but historian, which keep_person serves), dismiss, build, sponsor, ignore

namespace ages {

namespace {

std::string arg(const Act& a, const char* k) {
    auto it = a.args.find(k);
    return it == a.args.end() ? "" : it->second;
}

bool in_lair(const World& w, const std::string& id) { return contains(w.dragons[kPlayer].hostages, id); }

bool at_ledge(const World& w, const std::string& id) {
    for (auto& a : w.arrivals) if (a.person == id) return true;
    return false;
}

const Hook* open_hook(const World& w, const std::string& id) {
    for (auto& h : w.hooks) if (h.id == id) return &h;
    return nullptr;
}

void drop_kept(World& w, Person& x) {
    for (auto it = x.tags.begin(); it != x.tags.end();) it = it->rfind("kept:", 0) == 0 ? x.tags.erase(it) : std::next(it);
    if (Faction* f = find_faction(w, x.faction)) f->tags.erase("priest_kept");
}

void keep(World& w, const Params& p, Act& a, int act_index) {
    Person* x = find_person(w, a.item);
    if (!x) return;
    std::string role = arg(a, "role");
    if (!in_lair(w, x->id)) take_hostage_person(w, p, *x, true, because("kept", {}, {{"act", act_index}}));   // a willing guest or a visitor who stays
    x = find_person(w, a.item);
    if (!x) return;
    drop_kept(w, *x);
    x->tags.insert("kept:" + role);
    if (role == "priest") if (Faction* f = find_faction(w, x->faction)) f->tags.insert("priest_kept");
    if (role == "ward") x->tags.insert("ward");
}

void dismiss(World& w, const Params& p, Act& a, int act_index) {
    Person* x = find_person(w, a.item);
    if (!x) return;
    drop_kept(w, *x);
    std::string court = x->faction;
    if (Faction* f = find_faction(w, court)) {
        f->tags.erase("hostage_taken");
        erase_value(w.dragons[kPlayer].lineages, court);
    }
    release_hostage_person(w, p, court, because("dismissed", {}, {{"act", act_index}}));
}

void sponsor(World& w, const Params& p, Act& a, int act_index) {
    std::string kind = arg(a, "kind");
    if (kind == "remnant") {
        Act s = a;
        s.race = a.item.empty() ? arg(a, "race") : a.item;
        sponsor_remnant(w, p, s);
        return;
    }
    Faction* f = find_faction(w, a.target);
    if (!f) return;
    Person x = new_adult(w, p, dominant_race(*f), f->id, "hero");
    x.tags.insert(kind == "claimant" ? "claimant" : "sworn_to_dragon");
    add_person(w, x);
    meet(w, x.id, "sponsor");
    a.args["person"] = x.id;
    emit(w, "person_rose", x.id, f->id, 0, {"hero", kind == "claimant" ? "claimant" : "sworn_to_dragon"}, because("sponsored", {}, {{"act", act_index}}));
}

void ignore(World& w, const Params& p, Act& a) {
    if (a.target_kind == "scenario") { w.tags.insert("ignored:" + a.item); return; }
    const Hook* h = open_hook(w, a.item);
    if (!h) return;
    Hook copy = *h;
    w.hooks.erase(std::remove_if(w.hooks.begin(), w.hooks.end(), [&](const Hook& x) { return x.id == copy.id; }), w.hooks.end());
    apply_hook_ignore(w, p, copy);
    emit(w, "hook_ignored", copy.faction, copy.region, 0, {copy.kind, "by_act"});
    w.tags.insert("ignored:" + copy.kind);
}

}

std::string lair_refusal(const World& w, const Params& p, const Act& a) {
    const ActBook& b = p.act_book;
    if (a.type == "keep") {
        const Person* x = find_person(w, a.item);
        if (!x) return "act.no_target";
        std::string role = arg(a, "role");
        bool here = in_lair(w, x->id) || at_ledge(w, x->id) || (role == "consort" && has_tag(x->tags, "willing"));
        if (!here) return "keep.not_in_lair";
        if (has_tag(x->tags, "kept:" + role)) return "keep.already";
        auto g = b.keep_roles.find(role);
        return g == b.keep_roles.end() ? "" : gates_refusal(w, p, g->second);
    }
    if (a.type == "dismiss") {
        const Person* x = find_person(w, a.item);
        if (!x) return "act.no_target";
        return in_lair(w, x->id) ? "" : "keep.not_in_lair";
    }
    if (a.type == "build") {
        std::string what = arg(a, "what");
        if (has_tag(w.dragons[kPlayer].tags, "built:" + what)) return "build.already";
        auto g = b.build_what.find(what);
        return g == b.build_what.end() ? "" : gates_refusal(w, p, g->second);
    }
    if (a.type == "sponsor") {
        std::string kind = arg(a, "kind");
        if (kind == "remnant") {
            if (w.dragons[kPlayer].knowledge < p.c.remnant_sponsor_knowledge) return "needs_knowledge|n=" + std::to_string(static_cast<int>(p.c.remnant_sponsor_knowledge));
            std::string race = a.item.empty() ? arg(a, "race") : a.item;
            for (auto& r : w.remnants) if (r.race == race) return "";
            return "sponsor.no_remnant";
        }
        if (!find_faction(w, a.target)) return "act.no_target";
        return gates_refusal(w, p, b.sponsor_person);
    }
    if (a.type == "ignore") {
        if (a.target_kind == "scenario") return "";
        return open_hook(w, a.item) ? "" : "ignore.no_hook";
    }
    return "";
}

bool lair_effects(World& w, const Params& p, Act& a, double m, int act_index) {
    (void)m;
    if (a.type == "keep") { keep(w, p, a, act_index); return true; }
    if (a.type == "dismiss") { dismiss(w, p, a, act_index); return true; }
    if (a.type == "build") { w.dragons[kPlayer].tags.insert("built:" + arg(a, "what")); return true; }
    if (a.type == "sponsor") { sponsor(w, p, a, act_index); return true; }
    if (a.type == "ignore") { ignore(w, p, a); return true; }
    return false;
}

// a kept consort founds a line once consort_line_years have passed at the lair: a dragon-blooded heir at their court
void lair_roles_after_sleep(World& w, const Params& p) {
    std::vector<std::string> ids;
    for (auto& x : w.people)
        if (has_tag(x.tags, "kept:consort") && !has_tag(x.tags, "line_founded") && w.year - x.hostage_since >= p.act_book.n.consort_line_years) ids.push_back(x.id);
    for (auto& id : ids) {
        Person* x = find_person(w, id);
        if (!x) continue;
        x->tags.insert("line_founded");
        std::string court = x->faction, race = x->race;
        if (!find_faction(w, court)) continue;
        Person child = new_adult(w, p, race, court, "hero");
        child.tags.insert("dragon_blood");
        child.tags.insert("half_dragon");
        add_person(w, child);
        emit(w, "person_rose", child.id, court, 0, {"hero", "half_dragon"}, because("consort_line", {}, {}));
    }
}

}
