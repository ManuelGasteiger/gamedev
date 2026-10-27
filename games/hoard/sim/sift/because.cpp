#include <algorithm>
#include "candidate.h"
#include "sift.h"
#include "sift_util.h"
#include "ages/importance.h"

// Labov: start at the most reportable event, walk its causes back, stop at the first one that needs no explanation
// (importance at or below because_stop_importance) or has no cause. An episode or story starts from its kernel.

namespace ages {

std::vector<int> because_chain(const World& w, const Params& p, int event, int hops) {
    std::vector<int> out;
    const Event* e = sift_event(w, event);
    if (!e) return out;
    if (e->id >= kCompoundBase) {
        for (size_t i = e->cause.refs.size(); i > 0; --i) out.push_back(e->cause.refs[i - 1]);
        if (e->cause.refs.empty()) return out;
        e = sift_event(w, e->cause.refs.front());
        if (!e) return out;
    }
    double stop = p.sift.c.because_stop_importance;
    for (int h = 0; h < hops; ++h) {
        const Event* best = nullptr;
        for (int r : e->cause.refs) {
            const Event* x = sift_event(w, r);
            if (x && (!best || reportable(w, p, *x) > reportable(w, p, *best))) best = x;
        }
        if (!best) break;
        if (std::find(out.begin(), out.end(), best->id) == out.end()) out.push_back(best->id);
        double imp = best->importance >= 0 ? best->importance : event_importance(w, p, *best);
        if (imp <= stop) break;
        e = best;
    }
    return out;
}

}
