#include "rules.h"
#include "core/detmath.h"
#include "../memory_tiers.h"
#include "../person_record.h"
#include "../world_util.h"

// item 07: a person's memory of the dragon ages each step. Recent entries decay per generation by tier (oral halves,
// written loses 10 %, monumental floors at 0.3); one whose sev x s reaches permanent_at becomes permanent and may take
// away one trait (flip, chance flip_chance x s). A lettered ruler writes the court's memory down.

namespace ages {

static double generation(const Params& p, const std::string& race) {
    auto it = p.races.find(race);
    return it == p.races.end() || it->second.person.generation <= 0 ? 25.0 : it->second.person.generation;
}

static void decay(World& w, const Params& p, Person& x) {
    const MoveBook& b = p.moves;
    double gens = static_cast<double>(w.dt) / generation(p, x.race);
    for (auto& m : x.mem) {
        auto f = b.decay.find(m.tier);
        if (f == b.decay.end()) continue;
        double floor = m.tier == "monumental" ? b.monumental_floor : 0;
        double before = m.s;
        m.s *= det::pow(f->second, gens);
        if (m.s < floor) m.s = std::min(before, floor);
    }
    x.mem.erase(std::remove_if(x.mem.begin(), x.mem.end(), [&](const PersonMemory& m) { return m.s < b.drop_below; }), x.mem.end());
}

static std::string dominant_axis(const PersonMemory& m) {
    std::string best = "dread";
    double top = entry_value(m, "dread");
    for (const char* a : {"awe", "debt"})
        if (entry_value(m, a) > top) { top = entry_value(m, a); best = a; }
    return top > 0 ? best : "";
}

static void flip(World& w, const Params& p, Person& x, const PersonMemory& m, size_t index) {
    std::string axis = dominant_axis(m);
    auto row = p.moves.flips.find(axis);
    if (row == p.moves.flips.end()) return;
    std::string trait;
    for (auto& t : row->second) if (has_trait(x, t)) { trait = t; break; }
    if (trait.empty()) return;
    Rng r = rng_for(w.seed, "flip", x.id + "|" + std::to_string(w.year) + "|" + std::to_string(index));
    if (!r.chance(p.moves.flip_chance * m.s)) return;
    x.traits.list.erase(std::find(x.traits.list.begin(), x.traits.list.end(), trait));
    if (x.virtue == trait) x.virtue.clear();
    if (x.vice == trait) x.vice.clear();
    ++x.flips;
    emit(w, "trait_flipped", x.id, trait, m.s, {m.ev, axis}, because("memory_of_dragon", {}, {{"years", static_cast<double>(w.year - m.year)}}));
}

static void make_permanent(World& w, const Params& p, Person& x) {
    for (size_t i = 0; i < x.mem.size();) {
        PersonMemory m = x.mem[i];
        if (m.sev * m.s < p.moves.permanent_at) { ++i; continue; }
        x.mem.erase(x.mem.begin() + static_cast<long>(i));
        x.permanent.push_back(m);
        if (m.sev >= p.moves.touched_sev && m.src == "witness") acquire(p, x, "dragon_touched");
        flip(w, p, x, m, x.permanent.size());
    }
}

static void lettered_courts(World& w) {
    for (auto& f : w.factions) {
        const Person* ruler = find_person_role(w, f.id, "ruler");
        if (!ruler || !has_tag(ruler->tags, "lettered")) continue;
        f.dread.written = std::max(f.dread.written, f.dread.oral);
        f.awe.written = std::max(f.awe.written, f.awe.oral);
    }
}

void rule_person_memory(World& w, const Params& p) {
    if (p.moves.base.empty()) return;
    for (auto& x : w.people) {
        if (x.mem.empty()) continue;
        make_permanent(w, p, x);   // before the decay, so an entry is judged at the strength it was written with
        decay(w, p, x);
    }
    lettered_courts(w);
}

}
