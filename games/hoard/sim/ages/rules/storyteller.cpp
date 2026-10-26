#include "world_rules.h"
#include "card_ctx.h"
#include "../importance.h"
#include "../world_util.h"

// the storyteller (core-loop.md section 5; RimWorld's Cassandra and Randy), counted in wakings and pressed by quiet
// years: a waking with a major event is followed by a breather; a waking without one brings a storyteller major from the
// deck with chance base + per_window x quiet years / the phase's moult window, forced after forced_after quiet wakings
// or forced_years quiet years; a storyteller major brings a second one with second_chance. Every sleep of at least the
// short window then ends with an event of at least importance_min near its end, a medium one from the wake deck when
// nothing else gave it. Major cards grow likelier the quieter the valley has been, and calmer during a breather.

namespace ages {

namespace {

bool major(const World& w, const Params& p, const Event& e) {
    if (e.id <= 0) return false;
    return (e.importance >= 0 ? e.importance : event_importance(w, p, e)) >= p.genesis.story.major_importance;
}

const CardDef* card_of(const Params& p, const std::string& id) {
    auto it = std::lower_bound(p.cards.begin(), p.cards.end(), id, [](const CardDef& c, const std::string& k) { return c.id < k; });
    return it != p.cards.end() && it->id == id ? &*it : nullptr;
}

// a card of the deck by weight; one that cannot fire ends the draw, unless `cascade` draws again from the rest (forced
// majors and the wake guarantee), so a rare card stays rare when the common ones are spent; true when one fired
bool draw(World& w, const Params& p, std::vector<DeckEntry> deck, bool cascade) {
    while (!deck.empty()) {
        double total = 0;
        for (auto& d : deck) total += std::max(0.0, d.weight);
        if (total <= 0) return false;
        double roll = w.rng.uniform() * total;
        size_t i = 0;
        for (; i + 1 < deck.size(); ++i) {
            if (roll < std::max(0.0, deck[i].weight)) break;
            roll -= std::max(0.0, deck[i].weight);
        }
        const CardDef* c = card_of(p, deck[i].card);
        if (c && fire_card_now(w, p, *c)) return true;
        if (!cascade) return false;
        deck.erase(deck.begin() + static_cast<std::ptrdiff_t>(i));
    }
    return false;
}

bool any_since(const World& w, const Params& p, int after, double importance) {
    for (auto it = w.chronicle.rbegin(); it != w.chronicle.rend() && it->year > after; ++it)
        if (it->id > 0 && (it->importance >= 0 ? it->importance : event_importance(w, p, *it)) >= importance) return true;
    return false;
}

}

void rule_storyteller(World& w, const Params& p) {
    for (const Event& e : events_this_step(w))
        if (major(w, p, e)) { w.last_major = w.year; break; }
}

double storyteller_major_mult(const World& w, const Params& p) {
    const StorytellerDef& s = p.genesis.story;
    double quiet = static_cast<double>(w.year - w.last_major - s.grace_years);
    double m = std::min(s.major_max, 1.0 + s.major_per_year * std::max(0.0, quiet));
    return w.story_breather ? m * s.breather_mult : m;
}

void storyteller_wake(World& w, const Params& p, int slept, bool guarantee) {
    const StorytellerDef& s = p.genesis.story;
    if (!has_tag(w.tags, "generated") || slept <= 0 || has_tag(w.tags, "dragon_slain")) return;
    int from = w.year - slept;
    bool had_major = any_since(w, p, from, s.major_importance);
    if (!had_major && !w.story_breather) {
        double quiet = static_cast<double>(w.year - w.last_major);
        double chance = std::min(s.chance_max, s.chance_base + s.chance_per_window * quiet / std::max(1, long_sleep(w.dragon, p.c)));
        bool forced = w.story_quiet_wakings >= s.forced_after || (s.forced_years > 0 && quiet >= s.forced_years);
        if ((forced || w.rng.chance(chance)) && draw(w, p, s.deck, forced)) {
            had_major = true;
            if (w.rng.chance(s.second_chance)) draw(w, p, s.deck, false);
        }
    }
    double near = std::max(slept * s.near_end_share, static_cast<double>(std::min(slept, p.c.step_years)));
    if (guarantee && slept >= short_sleep(w.dragon, p.c) && !any_since(w, p, w.year - static_cast<int>(near + 0.5), p.c.importance_min))
        draw(w, p, s.wake_deck, true);
    if (had_major) { w.story_breather = true; w.story_quiet_wakings = 0; }
    else { w.story_breather = false; w.story_quiet_wakings += 1; }
    rule_storyteller(w, p);
    score_events(w, p);
}

}
