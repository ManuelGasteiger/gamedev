#include "slayer_view.h"
#include <cctype>
#include <cmath>
#include "memory_tiers.h"
#include "rules/slayers.h"
#include "names.h"
#include "slayer_roster.h"
#include "world_util.h"

namespace ages {

namespace {

std::string misc(const Params& p, const std::string& k) {
    auto it = p.texts.misc.find(k);
    return it == p.texts.misc.end() ? "" : it->second;
}

void replace_all(std::string& s, const std::string& from, const std::string& to) {
    for (size_t pos = s.find(from); pos != std::string::npos; pos = s.find(from, pos + to.size())) s.replace(pos, from.size(), to);
}

std::string trait_word(const Params& p, const Person& x, const std::string& kind, const std::string& id) {
    if (!knows(x, kind)) return "?";
    std::string s = misc(p, "slayer.name." + id);
    return s.empty() ? id : s;
}

std::string scar_words(const Params& p, const Person& x) {
    std::string out;
    for (size_t i = 0; i < x.slayer.scars.size(); ++i) {
        std::string s = misc(p, "slayer.scar." + x.slayer.scars[i]);
        out += (i ? ", " : "") + (s.empty() ? x.slayer.scars[i] : s);
    }
    return out;
}

}

std::string slayer_details(const World& w, const Params& p, const Person& x) {
    (void)w;
    std::string s = misc(p, "slayer.details");
    std::string scars = scar_words(p, x);
    replace_all(s, "{scars}", scars.empty() ? "" : misc(p, "slayer.marked") + scars + "; ");
    replace_all(s, "{counter}", trait_word(p, x, "counter", x.slayer.counter));
    replace_all(s, "{hate}", trait_word(p, x, "hate", x.slayer.hate));
    replace_all(s, "{fear}", trait_word(p, x, "fear", x.slayer.fear));
    return s;
}

int attempt_estimate(const World& w, const Params& p, const Faction& f) {
    const SlayerBook& b = p.slayers;
    int step = p.c.step_years;
    int years = step;
    if (slayer_stage(w, f.id) != "slayers_ready") {
        const Constants& c = p.c;
        double rate = std::max(0.02, std::min(1.0, c.slay_ready_rate * slayer_pressure(f, c.mem) / c.mem.slayer_dread));
        double decades = 1.0 / rate;
        if (const Order* o = find_order(w, f.id)) {
            double gap = slayer_strength(w, p, f) * b.ready_ratio - o->power_total;
            if (gap > 0) decades = std::max(decades, gap / std::max(0.5, 0.3 * static_cast<double>(roster(w, f.id).size())));
        }
        years = static_cast<int>(std::ceil(decades)) * step;
        years = std::max(b.estimate_min, std::min(b.estimate_max, years));
    }
    int round = std::max(1, b.estimate_round);
    return ((w.year + years + round - 1) / round) * round;
}

nlohmann::json slayers_block(const World& w, const Params& p) {
    using nlohmann::json;
    json out = json::array();
    for (auto& o : w.orders) {
        const Faction* f = find_faction(w, o.court);
        const Person* gm = grandmaster(w, o.court);
        std::string stage = slayer_stage(w, o.court);
        bool coming = has_tag(w.tags, "slayers_coming:" + o.court);
        if (!f || !gm || (stage.empty() && !coming)) continue;
        bool ready = stage == "slayers_ready" || coming;
        int year = attempt_estimate(w, p, *f);
        std::string s = misc(p, "slayer_line");
        std::string court = display_name(w, o.court);
        if (!court.empty()) court[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(court[0])));
        replace_all(s, "{court}", court);
        replace_all(s, "{name}", gm->name);
        replace_all(s, "{details}", slayer_details(w, p, *gm));
        replace_all(s, "{power}", std::to_string(static_cast<int>(std::floor(o.power_total))));
        replace_all(s, "{when}", misc(p, ready ? "slayer_when_ready" : "slayer_when_forming"));
        replace_all(s, "{year}", std::to_string(year));
        json traits = {{"counter", knows(*gm, "counter") ? json(gm->slayer.counter) : json("?")},
                       {"fear", knows(*gm, "fear") ? json(gm->slayer.fear) : json("?")},
                       {"hate", knows(*gm, "hate") ? json(gm->slayer.hate) : json("?")}};
        out.push_back({{"court", o.court}, {"text", s}, {"stage", ready ? "ready" : "forming"}, {"estimate_year", year},
                       {"power_total", o.power_total}, {"threat", slayer_strength(w, p, *f)}, {"named", roster(w, o.court).size()},
                       {"grandmaster", {{"id", gm->id}, {"name", gm->name}, {"power", gm->slayer.power}, {"scars", gm->slayer.scars}, {"traits", traits}}}});
    }
    return out;
}

}
