#include "rules.h"
#include <algorithm>
#include "people_life.h"
#include "slayers.h"
#include "../person_record.h"
#include "../slayer_roster.h"
#include "../talents.h"
#include "../world_util.h"

// item 08: the order turn. Founds the roster of a court that holds the slayers tag and a grief about the dragon, then
// each decade every named slayer rolls a mission, the order recruits from its hunters, ranks are repaired, and the
// dragon learns what hostages, sources and Dragon-sight tell. Rolls come from keyed streams.

namespace ages {

namespace {

bool dread_grief(const PersonMemory& m, const SlayerBook& b) {
    return m.sev >= b.person_sev && m.s >= b.person_s && m.awe + m.debt - m.dread < 0;
}

bool grieving(const World& w, const Params& p, const Faction& f) {
    const SlayerBook& b = p.slayers;
    for (auto& m : f.memory)
        if (m.axis == "dread" && m.magnitude >= b.court_magnitude && m.weight >= b.court_weight) return true;
    for (auto& x : w.people) {
        if (x.faction != f.id) continue;
        for (auto* v : {&x.mem, &x.permanent})
            for (auto& m : *v) if (dread_grief(m, b)) return true;
    }
    return false;
}

void bond(World& w, const Params& p, const Order& o, const std::string& id) {
    const SlayerBook& b = p.slayers;
    Rng r = rng_for(w.seed, "bond", id);
    Person* x = find_person(w, id);
    if (!x) return;
    Person* mentor = nullptr;
    Person* rival = nullptr;
    for (Person* y : roster(w, o.court)) {
        if (y->id == id) continue;
        if (!mentor || y->slayer.power > mentor->slayer.power) mentor = y;
    }
    for (Person* y : roster(w, o.court)) {
        if (y->id == id) continue;
        if (!rival || std::abs(y->slayer.power - x->slayer.power) < std::abs(rival->slayer.power - x->slayer.power)) rival = y;
    }
    auto weight = [&](const char* k) { auto it = b.rel.find(k); return it == b.rel.end() ? 0 : it->second; };
    if (mentor && mentor->slayer.power > x->slayer.power) add_rel(*x, mentor->id, "mentor", weight("mentor"));
    if (rival) {
        add_rel(*x, rival->id, "rival", weight("rival"));
        add_rel(*rival, id, "rival", weight("rival"));
    }
    double kin_roll = r.uniform();
    if (rival && kin_roll < b.kin_chance) {
        add_rel(*x, rival->id, "kin", weight("kin"));
        add_rel(*rival, id, "kin", weight("kin"));
    }
}

void found(World& w, const Params& p, Faction& f) {
    const SlayerBook& b = p.slayers;
    Order o;
    o.id = "order:" + f.id;
    o.court = f.id;
    o.founded = w.year;
    o.hunters = b.hunters_start;
    int slayers_ev = last_event_id(w, "slayers_founded", f.id);
    o.grievance = slayers_ev ? slayers_ev : last_event_id(w, "act_burn", f.id);
    std::vector<std::string> ids;
    for (auto& x : w.people)
        if (x.faction == f.id && x.role == "hero" && x.slayer.rank.empty() && !has_tag(x.tags, "hostage") && static_cast<int>(ids.size()) < b.slayers) ids.push_back(x.id);
    for (auto& id : ids) enrol(w, p, *find_person(w, id), "slayer");
    w.orders.push_back(o);
    std::sort(w.orders.begin(), w.orders.end(), [](const Order& a, const Order& c) { return a.id < c.id; });
    Order* mine = find_order(w, f.id);
    while (static_cast<int>(roster(w, f.id).size()) < b.first_slayers)
        if (!recruit(w, p, *mine, "slayer")) break;
    std::string lead;
    for (Person* x : roster(w, f.id)) if (lead.empty() || x->slayer.power > find_person(w, lead)->slayer.power) lead = x->id;
    if (lead.empty()) { w.orders.erase(std::remove_if(w.orders.begin(), w.orders.end(), [&](const Order& c) { return c.court == f.id; }), w.orders.end()); return; }
    find_person(w, lead)->slayer.rank = "grandmaster";
    for (Person* x : roster(w, f.id)) bond(w, p, *mine, x->id);
    mine->power_total = power_total(w, p, *mine);
    emit(w, "order_formed", f.id, lead, mine->power_total, {}, because("order_formed", {slayers_ev}, {{"named", static_cast<double>(roster(w, f.id).size())}}));
}

// a vanished court's order follows its people into the court they went to, or ends
void keep(World& w, const Params& p) {
    for (auto& x : w.people)
        if (!x.slayer.rank.empty() && x.role != "hero") x.slayer = Slayer();
    std::vector<std::string> gone;
    for (auto& o : w.orders) {
        if (find_faction(w, o.court)) continue;
        std::string court;
        int best = 0;
        for (auto& x : w.people)
            if (!x.slayer.rank.empty() && find_faction(w, x.faction) && !find_order(w, x.faction) && x.slayer.power > best) { best = x.slayer.power; court = x.faction; }
        if (court.empty()) gone.push_back(o.id); else o.court = court;
    }
    w.orders.erase(std::remove_if(w.orders.begin(), w.orders.end(), [&](const Order& o) { return contains(gone, o.id); }), w.orders.end());
    (void)p;
}

bool owes_dragon(const World& w, const std::string& court) {
    for (auto& o : w.oaths) if (o.status == "active" && (o.parties[0] == court || o.parties[1] == court) && (o.parties[0] == "dragon" || o.parties[1] == "dragon")) return true;
    return false;
}

void mission(World& w, const Params& p, Order& o, Person& x, bool ready) {
    const SlayerBook& b = p.slayers;
    Rng r = rng_for(w.seed, "mission", x.id + "|" + std::to_string(w.year));
    double rel = std::max(0.25, std::min(2.0, x.slayer.power / std::max(1.0, b.relative_div * dragon_points(w, p))));
    double train = b.w_train * std::max(0.1, 2.0 - rel);
    double hunt = b.w_hunt * rel;
    double seek = w.magic >= b.seek_magic_min ? b.w_seek * rel : 0;
    double march = ready ? b.w_march * rel : 0;
    double pickv = r.uniform() * (train + hunt + seek + march);
    double roll = r.uniform();
    if (pickv < train) { o.hunters = std::min(b.hunters_max, o.hunters + b.train_hunters); return; }
    pickv -= train;
    if (pickv < hunt) {
        if (roll < b.hunt_power * adaptation_factor(w, p)) raise_power(p, x, 1);
        return;
    }
    pickv -= hunt;
    if (pickv < seek && roll < b.seek_artifact && !x.slayer.armed) {
        x.slayer.armed = true;
        emit(w, "slayer_armed", x.id, o.court, 0, {}, because("seek"));
    }
}

bool source_told(World& w, const Person& gm, const std::string& court, const std::string& source) {
    (void)gm;
    std::string tail = ":" + court;
    return source.size() > tail.size() && source.compare(source.size() - tail.size(), tail.size(), tail) == 0;
}

void learn(World& w, const Params& p, Order& o) {
    Person* gm = grandmaster(w, o.court);
    if (!gm) return;
    for (auto& id : w.dragons[kPlayer].hostages) {
        Person* h = find_person(w, id);
        if (!h || h->faction != o.court || has_tag(h->tags, "told_order")) continue;
        h->tags.insert("told_order");
        reveal(*gm, p.slayers.reveal_hostage);
    }
    for (auto& s : w.dragons[kPlayer].chron.sources) {
        std::string mark = "told:" + s.id;
        if (!source_told(w, *gm, o.court, s.id) || has_tag(gm->tags, mark)) continue;
        gm->tags.insert(mark);
        reveal(*gm, p.slayers.reveal_source);
    }
    if (effect(w, p, "sight") > 0) reveal_all(*gm);
}

void repair(World& w, const Params& p, Order& o) {
    const SlayerBook& b = p.slayers;
    if (!grandmaster(w, o.court)) promote(w, p, o.court, "grandmaster");
    int masters = 0, slayers = 0;
    for (const Person* x : roster(w, o.court)) { if (x->slayer.rank == "master") ++masters; if (x->slayer.rank == "slayer") ++slayers; }
    while (masters < b.masters && slayers > 1 && promote(w, p, o.court, "master")) { ++masters; --slayers; }
    int named = static_cast<int>(roster(w, o.court).size());
    Rng r = rng_for(w.seed, "recruit_roll", o.id + "|" + std::to_string(w.year));
    double roll = r.uniform();
    if (o.hunters >= b.hunters_per_slayer && named < 1 + b.masters + b.slayers && roll < b.recruit_chance) {
        if (Person* x = recruit(w, p, o, "slayer")) { o.hunters -= b.hunters_per_slayer; bond(w, p, o, x->id); }
    }
}

void peace(World& w, const Params& p, Person& x) {
    if (w.year - x.slayer.last_fight < p.slayers.peace_years) return;
    x.slayer.last_fight = w.year;
    if (!is_zealot(p, x)) raise_power(p, x, -p.slayers.peace_loss);
}

void allies(World& w, const Params& p, Order& o) {
    for (const Event& e : events_this_step(w)) {
        if (e.kind != "war_ended" || e.subject != o.court || !(e.tags.count("conquest") || e.tags.count("annihilation")) || !owes_dragon(w, e.object)) continue;
        Person* best = nullptr;
        for (Person* x : roster(w, o.court)) if (x->slayer.rank != "grandmaster" && (!best || x->slayer.power > best->slayer.power)) best = x;
        if (best) raise_power(p, *best, p.slayers.ally_promote);
        break;
    }
}

}

void rule_slayer_order(World& w, const Params& p) {
    if (!p.slayers.present || !w.ended.empty()) return;
    keep(w, p);
    for (auto& f : w.factions)
        if (has_tag(f.tags, "slayers") && !find_order(w, f.id) && grieving(w, p, f)) found(w, p, f);
    for (auto& o : w.orders) {
        bool ready = slayer_stage(w, o.court) == "slayers_ready";
        std::vector<std::string> ids;
        for (const Person* x : roster(w, o.court)) ids.push_back(x->id);
        for (auto& id : ids) {
            Person* x = find_person(w, id);
            if (!x) continue;
            peace(w, p, *x);
            mission(w, p, o, *x, ready);
        }
        allies(w, p, o);
        repair(w, p, o);
        learn(w, p, o);
        o.power_total = power_total(w, p, o);
    }
}

}
