#include "person_record.h"
#include <algorithm>
#include "memory_tiers.h"
#include "rng.h"
#include "world_util.h"

namespace ages {

static bool listed(const std::vector<std::string>& v, const std::string& s) { return std::find(v.begin(), v.end(), s) != v.end(); }

static std::string draw(Rng& r, const std::vector<std::string>& from) {
    if (from.empty()) return "";
    return from[static_cast<size_t>(r.range(0, static_cast<int>(from.size())))];
}

static std::vector<std::string> unused(const MoveBook& b, const std::vector<std::string>& pool, const Person& x) {
    static const char* const kOld[] = {"bold", "pious", "greedy", "cunning"};
    std::vector<std::string> out;
    for (auto& t : pool)
        if (!listed(x.traits.list, t) && std::find(std::begin(kOld), std::end(kOld), t) == std::end(kOld) && listed(b.base, t)) out.push_back(t);
    return out;
}

// the four 17.2 numbers count as traits at trait_on; a virtue and a vice are drawn when the list lacks one, in a
// drawn order, and a third trait by chance; strongest (or first drawn) first
void assign_traits(const World& w, const Params& p, Person& x) {
    const MoveBook& b = p.moves;
    if (b.base.empty()) return;
    std::vector<std::pair<double, std::string>> old = {{x.traits.bold, "bold"}, {x.traits.pious, "pious"}, {x.traits.greedy, "greedy"}, {x.traits.cunning, "cunning"}};
    std::stable_sort(old.begin(), old.end(), [](const auto& a, const auto& c) { return a.first > c.first; });
    x.traits.list.clear();
    for (auto& o : old) if (o.first >= b.trait_on && static_cast<int>(x.traits.list.size()) < b.traits_max) x.traits.list.push_back(o.second);
    Rng r = rng_for(w.seed, "traits", x.id);
    auto first_of = [&](const std::vector<std::string>& kind) {
        for (auto& t : x.traits.list) if (listed(kind, t)) return t;
        return std::string();
    };
    bool vice_first = r.chance(0.5);
    for (int k = 0; k < 2; ++k) {
        const std::vector<std::string>& kind = (k == 0) == vice_first ? b.vices : b.virtues;
        if (!first_of(kind).empty()) continue;
        std::string t = draw(r, unused(b, kind, x));
        if (t.empty()) continue;
        if (static_cast<int>(x.traits.list.size()) >= b.traits_max) x.traits.list.pop_back();
        x.traits.list.push_back(t);
    }
    if (static_cast<int>(x.traits.list.size()) < b.traits_max && r.chance(b.extra_trait)) {
        std::string t = draw(r, unused(b, b.base, x));
        if (!t.empty()) x.traits.list.push_back(t);
    }
    x.virtue = first_of(b.virtues);
    x.vice = first_of(b.vices);
    x.vice_hidden = true;
}

bool has_trait(const Person& x, const std::string& trait) { return listed(x.traits.list, trait); }

bool tracked(const World& w, const Params& p, const Person& x) {
    if (has_tag(x.tags, "hostage")) return true;
    auto it = p.moves.cap.find(x.role);
    if (it == p.moves.cap.end()) return false;
    int before = 0;
    for (auto& y : w.people)
        if (y.faction == x.faction && y.role == x.role && y.id < x.id && !has_tag(y.tags, "hostage")) ++before;
    return before < it->second;
}

double entry_value(const PersonMemory& m, const std::string& axis) {
    double ax = axis == "dread" ? m.dread : axis == "awe" ? m.awe : m.debt;
    return m.s * m.sev * ax;
}

Axes axes(const Params& p, const Person& x) {
    Axes a;
    for (auto* v : {&x.mem, &x.permanent})
        for (auto& m : *v) {
            a.dread += entry_value(m, "dread");
            a.awe += entry_value(m, "awe");
            a.debt += entry_value(m, "debt");
        }
    double c = p.moves.axis_cap;
    auto clamp = [c](double v) { return std::max(-c, std::min(c, v)); };
    return {clamp(a.dread), clamp(a.awe), clamp(a.debt)};
}

static bool acquired_tag(const Params& p, const std::string& tag) {
    for (auto& a : p.moves.acquired) {
        if (a == tag) return true;
        if (!a.empty() && a.back() == '*' && tag.compare(0, a.size() - 1, a, 0, a.size() - 1) == 0) return true;
    }
    return false;
}

int acquired_count(const Params& p, const Person& x) {
    int n = 0;
    for (auto& t : x.tags) if (acquired_tag(p, t)) ++n;
    return n;
}

bool acquire(const Params& p, Person& x, const std::string& tag) {
    if (has_tag(x.tags, tag)) return true;
    if (!acquired_tag(p, tag) || acquired_count(p, x) >= p.moves.acquired_max) return false;
    x.tags.insert(tag);
    return true;
}

PersonMemory memory_entry(const World& w, const Params& p, const Person& x, const MemoryRow& row, const std::string& src, int act) {
    PersonMemory m;
    m.ev = row.ev;
    m.year = w.year;
    m.sev = std::max(1, std::min(3, row.sev));
    m.src = src;
    auto s = p.moves.src.find(src);
    m.s = s == p.moves.src.end() ? 1.0 : s->second;
    m.dread = row.dread;
    m.awe = row.awe;
    m.debt = row.debt;
    m.act = act;
    const Faction* f = find_faction(w, x.faction);
    m.tier = has_tag(x.tags, "lettered") || (f && keeps_records(w, *f)) ? "written" : "oral";
    return m;
}

// the weakest recent entry gives way beyond recent_max
void remember_dragon(const Params& p, Person& x, const PersonMemory& m) {
    x.mem.push_back(m);
    if (static_cast<int>(x.mem.size()) <= p.moves.recent_max) return;
    size_t weakest = 0;
    for (size_t i = 1; i < x.mem.size(); ++i)
        if (x.mem[i].s * x.mem[i].sev < x.mem[weakest].s * x.mem[weakest].sev) weakest = i;
    x.mem.erase(x.mem.begin() + static_cast<long>(weakest));
}

void add_rel(Person& x, const std::string& to, const std::string& kind, int weight) {
    for (auto& r : x.rel)
        if (r.to == to && r.kind == kind) { r.w = weight; return; }
    x.rel.push_back({to, kind, std::max(-100, std::min(100, weight))});
}

void witness_act(World& w, const Params& p, const Faction& f, const Act& a, double m, int act_index) {
    auto row = p.moves.acts.find(a.type);
    if (row == p.moves.acts.end()) return;
    MemoryRow r = row->second;
    r.sev = 1 + (m >= p.moves.sev2_at ? 1 : 0) + (m >= p.moves.sev3_at ? 1 : 0);
    for (auto& x : w.people) {
        bool owner = x.faction == f.id;
        if ((!owner && !contains(a.witnesses, x.faction)) || has_tag(x.tags, "hostage") || !tracked(w, p, x)) continue;
        remember_dragon(p, x, memory_entry(w, p, x, r, owner ? "witness" : "told", act_index));
    }
}

void inherit_memory(World& w, const Params& p, const Person& dead) {
    if (dead.mem.empty() && dead.permanent.empty()) return;
    for (auto& x : w.people) {
        bool child = false;
        for (auto& r : x.rel) if (r.kind == "parent" && r.to == dead.id) child = true;
        if (!child || !tracked(w, p, x)) continue;
        for (auto* v : {&dead.mem, &dead.permanent})
            for (auto m : *v) {
                double net = m.awe + m.debt - m.dread;
                m.s *= net < 0 ? p.moves.inherit_negative : p.moves.inherit_positive;
                if (m.s < p.moves.drop_below) continue;
                m.src = "told";
                remember_dragon(p, x, m);
            }
    }
}

}
