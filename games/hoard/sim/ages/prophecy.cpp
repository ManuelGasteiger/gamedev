#include <tuple>
#include "prophecy.h"
#include "memory_tiers.h"
#include "rules/card_ctx.h"
#include "world_util.h"

namespace ages {

namespace {

// keeps records first, then the highest era, then the most written memory, then id
const Faction* most_lettered(const World& w) {
    const Faction* best = nullptr;
    auto score = [&](const Faction& f) { return std::make_tuple(keeps_records(w, f) ? 1 : 0, f.era, f.dread.written + f.awe.written); };
    for (auto& f : w.factions) if (!best || score(f) > score(*best)) best = &f;
    return best;
}

void learn(World& w, const Params& p, const std::string& court, const std::string& from) {
    const std::string& tag = p.endings.prophecy.known_tag;
    const Faction* f = find_faction(w, court);
    if (!f || has_tag(f->tags, tag)) return;
    emit(w, "prophecy_spread", court, from, 0, {}, because("prophecy_spread", {w.prophecy.event}));
    write_tag(w, court, tag, 0, true, w.prophecy.event);
    refresh_prophecy(w, p);
}

bool knows(const World& w, const Params& p, const std::string& court) {
    const Faction* f = find_faction(w, court);
    return f && has_tag(f->tags, p.endings.prophecy.known_tag);
}

}

int prophecy_year(const World& w, const Params& p) {
    const ProphecyDef& d = p.endings.prophecy;
    if (d.ending.empty() || d.lines.empty()) return 0;
    int step = p.c.step_years;
    Rng r = rng_for(w.seed, "prophecy", "year");
    int before = r.range(d.years_lo / step, d.years_hi / step + 1) * step;
    return std::max(step, w.genesis.hatched_year - before);
}

void write_prophecy(World& w, const Params& p) {
    const ProphecyDef& d = p.endings.prophecy;
    const Faction* book = most_lettered(w);
    if (!book || !w.prophecy.ending.empty()) return;
    Prophecy& pr = w.prophecy;
    pr.ending = d.ending;
    pr.year = w.year;
    pr.book = book->id;
    for (auto& l : d.lines) {
        Rng r = rng_for(w.seed, "prophecy", l.key);
        ProphecyLine line;
        line.key = l.key + "." + std::to_string(r.range(1, std::max(1, l.variants) + 1));
        line.cond = l.cond;
        line.slots = l.slots;
        pr.lines.push_back(line);
    }
    pr.event = emit(w, "prophecy_written", book->id, d.ending, 0, {}, because("prophecy_written"));
    write_tag(w, book->id, d.known_tag, 0, true, pr.event);
    write_tag(w, book->id, d.book_tag, 0, false, pr.event);
    refresh_prophecy(w, p);
}

void refresh_prophecy(World& w, const Params& p) {
    if (w.prophecy.ending.empty()) return;
    w.prophecy.known_by.clear();
    for (auto& f : w.factions) if (has_tag(f.tags, p.endings.prophecy.known_tag)) w.prophecy.known_by.push_back(f.id);
}

void prophecy_after_act(World& w, const Params& p, const Act& act) {
    if (w.prophecy.ending.empty() || act.type != "take" || !knows(w, p, act.target)) return;
    auto books = act.payload.find("books");
    if (books == act.payload.end() || books->second <= 0) return;
    for (auto& id : act.witnesses) if (id != act.target) learn(w, p, id, act.target);
}

void prophecy_after_teach(World& w, const Params& p, const std::string& court) {
    if (w.prophecy.ending.empty() || !knows(w, p, court)) return;
    const Faction* f = find_faction(w, court);
    if (!f) return;
    std::vector<std::string> near;
    for (auto& g : w.factions) if (g.id != f->id && adjacent(w, *f, g)) near.push_back(g.id);
    for (auto& id : near) learn(w, p, id, court);
}

}
