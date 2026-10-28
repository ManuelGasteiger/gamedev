#include <algorithm>
#include "chron.h"
#include "importance.h"
#include "unrecorded.h"
#include "world_util.h"

// level 3: the same waking and sleep run twice from the world at the last wake, with every act and without one, so the
// difference is that act alone. Only the acts of the last waking can be taken out.

namespace ages {

namespace {

bool is_copy(const World& w) { return w.counterfactual.count("copy") > 0; }

bool reaches(const World& w, const Event& e, int lo, int hi, int depth, int& act) {
    for (int r : e.cause.refs) {
        if (r >= lo && r < hi) {
            const Event* x = find_event(w, r);
            if (x && x->kind.rfind("act_", 0) == 0) { act = r; return true; }
        }
        if (depth <= 0) continue;
        if (const Event* x = find_event(w, r)) if (reaches(w, *x, lo, hi, depth - 1, act)) return true;
    }
    return false;
}

bool has_like(const World& w, const Event& e, int from_year) {
    for (auto& x : w.chronicle)
        if (x.year >= from_year && x.kind == e.kind && x.subject == e.subject && x.object == e.object) return true;
    return false;
}

}

void snapshot_at_wake(World& w, const Params& p) {
    w.wake_snapshot.reset();
    if (is_copy(w) || !lore_counterfactual(w, p)) return;
    auto s = std::make_shared<World>(w);
    s->recorder = nullptr;
    s->counterfactual = {{"copy", "1"}};
    w.wake_snapshot = s;
}

void counterfactual_after_sleep(World& w, const Params& p, int years, size_t acts_end) {
    if (is_copy(w)) return;
    w.counterfactual.clear();
    std::shared_ptr<const World> snap = w.wake_snapshot;
    if (!snap || !lore_counterfactual(w, p) || acts_end <= snap->acts.size() || acts_end > w.acts.size()) return;
    int lo = snap->next_event_id, hi = lo;
    for (auto& e : w.chronicle) if (e.kind == "sleep_begin" && e.year == w.sleep_started) break; else if (e.id >= hi) hi = e.id + 1;
    const Event* best = nullptr;
    int best_act = 0;
    for (auto& e : w.chronicle) {
        if (e.id < hi || e.year < w.sleep_started) continue;
        double imp = e.importance >= 0 ? e.importance : event_importance(w, p, e);
        int act = 0;
        if (imp < p.c.chron.cf_min_importance || !reaches(w, e, lo, hi, 6, act)) continue;
        double top = best ? (best->importance >= 0 ? best->importance : event_importance(w, p, *best)) : -1;
        if (imp > top) { best = &e; best_act = act; }
    }
    if (!best) return;
    const Event* ae = find_event(w, best_act);
    std::vector<Act> all(w.acts.begin() + static_cast<std::ptrdiff_t>(snap->acts.size()), w.acts.begin() + static_cast<std::ptrdiff_t>(acts_end));
    std::vector<Act> without;
    bool dropped = false;
    for (auto& a : all) {
        if (!dropped && ae && "act_" + a.type == ae->kind && a.target == ae->subject) { dropped = true; continue; }
        without.push_back(a);
    }
    if (!dropped) return;
    auto run = [&](const std::vector<Act>& acts) {
        World c = *snap;
        apply_acts_unrecorded(c, p, acts);
        sleep_unrecorded(c, p, years);
        return c;
    };
    World with = run(all), wo = run(without);
    if (!has_like(with, *best, w.sleep_started)) return;   // the run does not tell it again: other inputs mattered
    w.counterfactual = {{"act", std::to_string(best_act)}, {"event", std::to_string(best->id)}, {"outcome", has_like(wo, *best, w.sleep_started) ? "anyway" : "not"}};
}

}
