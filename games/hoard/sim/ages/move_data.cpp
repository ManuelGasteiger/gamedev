#include "data.h"
#include "json.h"

// item 07: data/moves.json; a missing file loads as no moves (people keep the 17.2 record only)

namespace ages {

using nlohmann::json;

namespace {

template <class T>
void get(const json& j, const char* key, T& out) {
    auto it = j.find(key);
    if (it != j.end() && !it->is_null()) out = it->get<T>();
}

MemoryRow row(const json& j, const std::string& ev = "") {
    MemoryRow r;
    r.ev = ev;
    get(j, "ev", r.ev);
    get(j, "sev", r.sev);
    const json& ax = j.contains("ax") ? j["ax"] : j;
    get(ax, "dread", r.dread);
    get(ax, "awe", r.awe);
    get(ax, "debt", r.debt);
    return r;
}

MoveDef move(const std::string& id, const json& v) {
    MoveDef m;
    m.id = id;
    get(v, "intent", m.intent);
    get(v, "access", m.access);
    get(v, "secret", m.secret);
    get(v, "witnessed", m.witnessed);
    get(v, "guise_opens", m.guise_opens);
    get(v, "lore", m.lore);
    get(v, "arcana", m.arcana);
    get(v, "lore_min", m.lore_min);
    get(v, "arcana_min", m.arcana_min);
    get(v, "weights", m.weights);
    get(v, "mods", m.mods);
    get(v, "threshold", m.threshold);
    get(v, "ledger", m.ledger);
    if (v.contains("accept")) m.accept = row(v["accept"]);
    get(v, "acquire", m.acquire);
    get(v, "rel", m.rel);
    if (v.contains("hostage")) {
        m.has_hostage = true;
        m.hostage_giver = row(v["hostage"]["giver"]);
        m.hostage_taken = row(v["hostage"]["taken"]);
    }
    if (v.contains("humiliated")) {
        m.has_humiliated = true;
        m.humiliated = row(v["humiliated"]);
        get(v["humiliated"], "traits", m.humiliated_traits);
    }
    if (v.contains("court")) {
        get(v["court"], "dread", m.court_dread);
        get(v["court"], "awe", m.court_awe);
    }
    return m;
}

}

bool moves_from_json(const DataJson& d, Params& p, std::string& err) {
    if (d.moves.empty()) return true;
    json j = json::parse(d.moves, nullptr, false);
    if (j.is_discarded()) { err = "invalid json in moves"; return false; }
    MoveBook& b = p.moves;
    if (j.contains("traits")) {
        const json& t = j["traits"];
        get(t, "base", b.base);
        get(t, "virtues", b.virtues);
        get(t, "vices", b.vices);
        get(t, "max", b.traits_max);
        get(t, "trait_on", b.trait_on);
        get(t, "extra", b.extra_trait);
    }
    if (j.contains("acquired")) {
        get(j["acquired"], "list", b.acquired);
        get(j["acquired"], "max", b.acquired_max);
    }
    if (j.contains("memory")) {
        const json& m = j["memory"];
        get(m, "src", b.src);
        get(m, "decay", b.decay);
        get(m, "monumental_floor", b.monumental_floor);
        get(m, "drop_below", b.drop_below);
        get(m, "permanent_at", b.permanent_at);
        get(m, "flip_chance", b.flip_chance);
        get(m, "inherit_negative", b.inherit_negative);
        get(m, "inherit_positive", b.inherit_positive);
        get(m, "axis_cap", b.axis_cap);
        get(m, "sev2_at", b.sev2_at);
        get(m, "sev3_at", b.sev3_at);
        get(m, "touched_sev", b.touched_sev);
        get(m, "recent_max", b.recent_max);
    }
    if (j.contains("acts"))
        for (auto it = j["acts"].begin(); it != j["acts"].end(); ++it) b.acts[it.key()] = row(it.value(), "act_" + it.key());
    get(j, "flips", b.flips);
    get(j, "cap", b.cap);
    get(j, "rel", b.rel);
    get(j, "terms_accept", b.terms_accept);
    if (j.contains("memory_terms")) {
        const json& t = j["memory_terms"];
        get(t, "mercy", b.term_mercy);
        get(t, "betrayal", b.term_betrayal);
        get(t, "same_move", b.term_same);
        get(t, "clamp", b.term_clamp);
    }
    if (j.contains("moves"))
        for (auto it = j["moves"].begin(); it != j["moves"].end(); ++it) b.moves[it.key()] = move(it.key(), it.value());
    if (j.contains("release")) {
        const json& r = j["release"];
        get(r, "default", b.release_default);
        get(r, "by_trait", b.release_by_trait);
        if (r.contains("readings"))
            for (auto it = r["readings"].begin(); it != r["readings"].end(); ++it) b.readings[it.key()] = row(it.value(), it.key());
        if (!b.readings.count(b.release_default)) { err = "moves: release default has no reading"; return false; }
    }
    if (j.contains("secret")) {
        get(j["secret"], "base", b.secret_base);
        get(j["secret"], "per_sage", b.secret_per_sage);
        get(j["secret"], "intrigue_mult", b.secret_intrigue_mult);
    }
    if (j.contains("inquisition")) {
        const json& q = j["inquisition"];
        get(q, "joiners", b.inq_joiners);
        get(q, "bonds", b.inq_bonds);
        get(q, "per_decade", b.inq_per_decade);
        get(q, "strength_min", b.inq_strength_min);
    }
    return true;
}

}
