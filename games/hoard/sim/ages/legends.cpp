#include "ages.h"
#include "endings_json.h"
#include "genesis.h"
#include "json.h"
#include "world_util.h"

// the first screen's legends: the most important events of the pre-history, at most per_kind of a kind, per_century of
// a century and one per thread, told by the narrator in the order they happened; the `always` kinds (the prophecy,
// woven legends) are told besides

namespace ages {

std::string legends_json(const World& w, const Params& p) {
    const LegendsDef& l = p.genesis.legends;
    std::vector<const Event*> pool, picked;
    for (auto& e : w.chronicle) {
        if (e.id <= 0 || !e.tags.count("prehistory")) continue;
        if (contains(l.always, e.kind)) picked.push_back(&e);
        else if (!contains(l.skip, e.kind)) pool.push_back(&e);
    }
    size_t told = picked.size();
    std::stable_sort(pool.begin(), pool.end(), [](const Event* a, const Event* b) { return a->importance > b->importance; });
    std::map<std::string, int> kinds;
    std::map<int, int> centuries;
    std::vector<std::string> threads;
    for (const Event* e : pool) {
        if (static_cast<int>(picked.size() - told) >= l.max) break;
        if (kinds[e->kind] >= l.per_kind || centuries[e->year / 100] >= l.per_century || (!e->thread.empty() && contains(threads, e->thread))) continue;
        kinds[e->kind] += 1;
        centuries[e->year / 100] += 1;
        if (!e->thread.empty()) threads.push_back(e->thread);
        picked.push_back(e);
    }
    std::sort(picked.begin(), picked.end(), [](const Event* a, const Event* b) { return a->id < b->id; });
    nlohmann::json events = nlohmann::json::array();
    for (const Event* e : picked) events.push_back({{"year", e->year}, {"text", event_sentence(w, p, *e)}, {"importance", e->importance}, {"kind", e->kind}});
    return nlohmann::json{{"ok", true}, {"events", events}, {"prophecy", prophecy_view_json(w, p)}}.dump();   // item 04: the riddle
}

}
