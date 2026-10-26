#include "rules.h"
#include "../world_util.h"

namespace ages {

// 17.8: threads open where their first event is emitted (war, coalition, hegemony, vassalage, collapse). This rule
// closes the ones whose arc has no live state left, so the briefing knows which threads are still running.
void rule_threads(World& w, const Params&) {
    std::vector<std::string> open = w.threads_open;
    for (auto& t : open) {
        size_t a = t.find(':');
        std::string kind = t.substr(0, a);
        size_t b = t.find(':', a + 1);
        std::string id = t.substr(a + 1, b == std::string::npos ? std::string::npos : b - a - 1);
        bool live = true;
        if (kind == "war" || kind == "coalition") {
            live = false;
            for (auto& x : w.wars) if (war_thread(x) == t) live = true;
        } else if (kind == "hegemony") {
            live = has_tag(w.tags, "hegemon:" + id);
        } else if (kind == "vassalage") {
            const Faction* f = find_faction(w, id);
            live = f && is_vassal(*f);
        } else if (kind == "collapse" || kind == "civil_war") {
            live = false;
            for (auto& f : w.factions) if (has_tag(f.tags, "successor_of:" + id) && has_tag(f.tags, "civil_war") && !f.at_war_with.empty()) live = true;
        }
        if (!live) close_thread(w, t);
    }
    for (auto& f : w.factions)
        if (has_tag(f.tags, "civil_war") && f.at_war_with.empty()) f.tags.erase("civil_war");
}

}
