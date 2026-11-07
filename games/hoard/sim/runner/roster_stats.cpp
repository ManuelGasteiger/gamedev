#include "roster_stats.h"
#include "ages/names.h"
#include "ages/slayer_roster.h"
#include "ages/world_util.h"

namespace ages {

using nlohmann::json;

json roster_json(const World& w) {
    json orders = json::array();
    int ready = 0, ready_named = 0, failed = 0, slain = 0, scarred = 0, promoted = 0, fought = 0;
    for (auto& e : w.chronicle) {
        if (e.kind == "slayers_ready") { ++ready; if (is_person_id(w, e.object)) ++ready_named; }
        else if (e.kind == "slay_attempt_failed") ++failed;
        else if (e.kind == "dragon_slain" && e.cause.text_key != "defences" && is_person_id(w, e.object)) ++slain;
        else if (e.kind == "slayer_scarred") ++scarred;
        else if (e.kind == "slayer_promoted") ++promoted;
        else if (e.kind == "slayer_fought") ++fought;
    }
    for (auto& e : w.chronicle) {
        if (e.kind != "order_formed") continue;
        json o = {{"court", e.subject}, {"founded", e.year}, {"first_grandmaster", display_name(w, e.object)}};
        int attempts = 0, lost = 0;
        for (auto& x : w.chronicle) {
            if (x.subject != e.subject) continue;
            if (x.kind == "slay_attempt_failed") { ++attempts; ++lost; }
            if (x.kind == "dragon_slain") ++attempts;
        }
        o["attempts"] = attempts;
        o["failed"] = lost;
        const Order* ord = find_order(w, e.subject);
        const Person* gm = grandmaster(w, e.subject);
        if (ord && gm) {
            json known = json::array();
            for (auto& k : gm->slayer.known) known.push_back(k);
            o["grandmaster"] = {{"name", gm->name}, {"power", gm->slayer.power}, {"scars", gm->slayer.scars}, {"known", known},
                                {"counter", gm->slayer.counter}, {"hate", gm->slayer.hate}, {"fear", gm->slayer.fear}};
            o["named"] = roster(w, e.subject).size();
            o["power_total"] = ord->power_total;
            o["hunters"] = ord->hunters;
        }
        orders.push_back(o);
    }
    return {{"orders", orders}, {"ready", ready}, {"ready_named", ready_named}, {"failed", failed}, {"slain_by_band", slain},
            {"scarred", scarred}, {"promoted", promoted}, {"fought", fought}};
}

}
