#include "rules.h"
#include "../world_util.h"

// item 07: a court may discover a secret act on one of its people: per decade base x (piety + sages x per_sage),
// halved for a person schooled in intrigue. Discovery emits secret_exposed, which the inquisition card reads.

namespace ages {

double secret_rate(const World& w, const Params& p, const Secret& s) {
    const Faction* f = find_faction(w, s.holder);
    const Person* x = find_person(w, s.about);
    if (!f || !x) return 0;
    int sages = 0;
    for (auto& y : w.people) if (y.faction == f->id && y.role == "sage") ++sages;
    double rate = p.moves.secret_base * (f->culture.traits.piety + sages * p.moves.secret_per_sage);
    if (has_tag(x->tags, "schooled:intrigue")) rate *= p.moves.secret_intrigue_mult;
    return rate;
}

void rule_secrets(World& w, const Params& p) {
    std::vector<Secret> kept;
    for (auto& s : w.secrets) {
        const Person* x = find_person(w, s.about);
        if (!x || x->faction.empty()) continue;   // the secret dies with the person or the court
        s.holder = x->faction;
        if (s.exposed) { kept.push_back(s); continue; }
        s.discovery_per_decade = secret_rate(w, p, s);
        Rng r = rng_for(w.seed, "secret", s.about + "|" + std::to_string(w.year));
        if (r.chance(s.discovery_per_decade * dts(w, p.c))) {
            s.exposed = w.year;
            emit(w, "secret_exposed", s.holder, s.about, s.discovery_per_decade, {s.kind}, because("secret_found", {}, {{"years", static_cast<double>(w.year - s.year)}}));
        }
        kept.push_back(s);
    }
    w.secrets = kept;
}

}
