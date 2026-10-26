#include "rules.h"
#include "../world_util.h"

namespace ages {

// 17.8 hysteresis: a realm (own plus vassal regions) of hegemony_share becomes hegemon and stays one until it falls
// below hegemony_end_share or dies; `hegemony` and `hegemony_ended` fire once per episode on the thread hegemony:<id>:<year>
static std::string episode(const World& w, const std::string& id) {
    for (auto& t : w.threads_open) if (t.rfind("hegemony:" + id + ":", 0) == 0) return t;
    return "";
}

static Cause rise_cause(const World& w, const Faction& f) {
    int ev = std::max(last_event_id(w, "region_taken", f.id), last_event_id(w, "vassal_sworn", "", f.id));
    ev = std::max(ev, last_event_id(w, "region_settled", f.id));
    return because(ev ? "conquest" : "growth", {ev}, {{"realm", static_cast<double>(realm_regions(w, f))}});
}

static Cause fall_cause(const World& w, const Faction& f) {
    int ev = std::max(last_event_id(w, "region_taken", "", "", f.id), last_event_id(w, "vassal_freed", "", f.id));
    ev = std::max(ev, std::max(last_event_id(w, "vassal_revolt", "", f.id), last_event_id(w, "coalition_formed", "", f.id)));
    return because("realm_shrank", {ev}, {{"realm", static_cast<double>(realm_regions(w, f))}});
}

void rule_hegemony(World& w, const Params& p) {
    double need = p.c.hegemony_share * static_cast<double>(w.regions.size());
    double keep = p.c.hegemony_end_share * static_cast<double>(w.regions.size());
    for (auto& f : w.factions) {
        int realm = static_cast<int>(realm_regions(w, f));
        f.max_realm = std::max(f.max_realm, realm);
        bool tagged = has_tag(f.tags, "hegemon");
        if (!tagged && static_cast<double>(realm) >= need) {
            f.tags.insert("hegemon");
            w.tags.insert("hegemon:" + f.id);
            std::string thread = "hegemony:" + f.id + ":" + std::to_string(w.year);
            open_thread(w, thread);
            emit(w, "hegemony", f.id, "", static_cast<double>(realm), {}, rise_cause(w, f), thread);
        } else if (tagged && static_cast<double>(realm) < keep) {
            f.tags.erase("hegemon");
            w.tags.erase("hegemon:" + f.id);
            std::string thread = episode(w, f.id);
            emit(w, "hegemony_ended", f.id, "", static_cast<double>(realm), {"shrank"}, fall_cause(w, f), thread);
            close_thread(w, thread);
        }
    }
    for (auto& g : w.graveyard)
        if (g.died == w.year && has_tag(w.tags, "hegemon:" + g.id)) {
            w.tags.erase("hegemon:" + g.id);
            std::string thread = episode(w, g.id);
            int fall = std::max(last_event_id(w, "collapse", g.id), last_event_id(w, "faction_destroyed", g.id));
            emit(w, "hegemony_ended", g.id, "", 0, {fall && find_event(w, fall)->kind == "collapse" ? "collapse" : "destroyed"}, because("fell", {fall}), thread);
            close_thread(w, thread);
        }
}

}
