#include "endings.h"
#include "names.h"
#include "rules/rules.h"
#include "step.h"
#include "world_util.h"
#include "wounds.h"

// item 04: the exit act. A ready ending with an exit kind ends the run: the `ending` event, Mortality's mortal in its
// court, then epilogue_decades idle decades with no dragon in them (cards that need it, slayers and visitors stop)

namespace ages {

namespace {

const EndingDef* exit_def(const Params& p, const std::string& kind) {
    for (auto& e : p.endings.endings) if (e.enabled && !e.exit_kind.empty() && e.exit_kind == kind) return &e;
    return nullptr;
}

const Person* voice_of(const World& w, const std::string& court) {
    for (const char* role : {"priest", "sage"}) if (const Person* x = find_person_role(w, court, role)) return x;
    return nullptr;
}

}

bool exit_unrecorded(World& w, const Params& p, const std::string& kind, std::string& reason) {
    if (dragon_dead(w)) { reason = "dragon_slain"; return false; }
    if (run_ended(w)) { reason = "run_ended"; return false; }
    const EndingDef* def = exit_def(p, kind);
    if (!def) { reason = "unknown_ending|kind=" + kind; return false; }
    const EndingState* s = ending_state(w, def->id);
    if (!s || !s->ready) { reason = "ending_not_ready|kind=" + kind; return false; }
    std::string court = s->court;
    Act a;
    a.year = w.year;
    a.type = "exit";
    a.target = court;
    a.item = kind;
    w.acts.push_back(a);
    int ready = last_event_id(w, "ending_ready", def->id);
    emit(w, "ending", "dragon", court, 0, {kind}, because("ending", {ready}));
    w.ended = kind;
    w.tags.insert("ended:" + kind);
    w.offers.clear();
    w.hooks.clear();
    if (kind == "mortality")
        if (const Faction* f = find_faction(w, court)) {
            Person x = new_adult(w, p, dominant_race(*f), f->id, "sage");
            x.tags.insert("was_dragon");
            add_person(w, x);
            emit(w, "person_rose", x.id, f->id, 0, {"sage", "was_dragon"}, because("ending"));
        }
    for (int i = 0; i < p.endings.epilogue_decades; ++i) decade_step(w, p);
    emit(w, "epilogue_over", "dragon", court, p.endings.epilogue_decades * p.c.step_years, {kind}, because("ending"));
    return true;
}

void exit_offers(World& w, const Params& p) {
    for (auto& e : p.endings.endings) {
        if (!e.enabled || e.exit_kind.empty() || !ending_ready(w, e.id)) continue;
        const EndingState* s = ending_state(w, e.id);
        Offer of;
        of.kind = "exit";
        of.id = "offer_exit_" + e.exit_kind + "_" + std::to_string(w.year);
        of.from_faction = s->court;
        if (const Person* x = voice_of(w, s->court)) of.envoy = x->id;
        of.motive = e.exit_kind;
        of.stated_reason = e.exit_kind;
        of.text_key = e.offer_text_key;
        of.expires = w.year;
        of.draft.id = of.id;
        w.offers.push_back(of);
    }
}

}
