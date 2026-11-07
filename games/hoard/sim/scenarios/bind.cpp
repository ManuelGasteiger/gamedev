#include "bind.h"
#include <algorithm>
#include <cstdlib>
#include "ages/memory_tiers.h"
#include "ages/person_record.h"
#include "ages/world_util.h"

namespace scn {

using namespace ages;
using nlohmann::json;

namespace {

std::string ref(const json& v) {   // "$court" -> "court"; "$a.target" -> "a"
    if (!v.is_string()) return "";
    std::string s = v.get<std::string>();
    if (s.size() < 2 || s[0] != '$') return "";
    return s.substr(1, s.find('.') == std::string::npos ? std::string::npos : s.find('.') - 1);
}

bool in_range(const json& spec, const char* k, double v) {
    if (!spec.contains(k)) return true;
    const json& r = spec[k];
    if (r.is_array() && r.size() == 2) return v >= r[0].get<double>() && v <= r[1].get<double>();
    if (r.is_number()) return v >= r.get<double>();
    return true;
}

bool flag_is(const json& spec, const char* k, bool v) {
    return !spec.contains(k) || !spec[k].is_boolean() || spec[k].get<bool>() == v;
}

std::vector<std::string> types_of(const json& pat) {
    if (!pat.contains("type")) return {};
    if (pat["type"].is_string()) return {pat["type"].get<std::string>()};
    return pat["type"].get<std::vector<std::string>>();
}

// the argument of an `is` expression: "owner_of($a.target)" -> ("owner_of", "a")
std::pair<std::string, std::string> call_of(const std::string& is) {
    size_t open = is.find('(');
    if (open == std::string::npos) return {is, ""};
    std::string fn = is.substr(0, open), arg = is.substr(open + 1, is.rfind(')') - open - 1);
    size_t inner = arg.find('(');
    if (inner != std::string::npos) return {fn + ":" + arg.substr(0, inner), arg.substr(inner + 1, arg.find(')') - inner - 1)};
    return {fn, arg};
}

const Region* lair_of(const World& w) {
    for (auto& r : w.regions) if (r.lair) return &r;
    return nullptr;
}

std::vector<std::string> neighbours(const World& w, const std::string& id) {
    std::vector<std::string> out;
    const Faction* f = find_faction(w, id);
    const Region* lair = id == "lair" ? lair_of(w) : nullptr;
    for (auto& g : w.factions) {
        if (g.id == id) continue;
        bool next = false;
        if (f) next = adjacent(w, *f, g);
        else if (lair) for (auto& rid : g.regions) next = next || rid == lair->id || contains(lair->neighbours, rid);
        if (next) out.push_back(g.id);
    }
    return out;
}

double score_of(const Ctx& x, const std::string& kind, const std::string& id, const std::string& by) {
    const World& w = x.w;
    if (kind == "faction") {
        const Faction* f = find_faction(w, id);
        if (!f) return 0;
        if (by == "wealth") return f->wealth;
        if (by == "power") return power(*f, x.p.c);
        if (by == "dread") return dread_eff(*f);
        if (by == "awe") return awe_eff(*f);
        if (by == "piety") return f->culture.traits.piety;
        return f->born;
    }
    if (kind == "person") {
        const Person* p = find_person(w, id);
        if (!p) return 0;
        if (by == "renown") { auto it = x.c.renown.find(id); return it == x.c.renown.end() ? 0 : it->second; }
        if (by == "cunning") return p->traits.cunning;
        if (by == "piety") return p->traits.pious;
        if (by == "age") return w.year - p->born;
        if (by == "power") return p->slayer.power;
        if (by == "wealth") return p->traits.greedy;
        Axes a = axes(x.p, *p);
        if (by == "dread") return a.dread;
        if (by == "awe") return a.awe;
        return person_court(w, *p) == "lair" ? p->hostage_since : p->born;
    }
    if (kind == "region") { const Region* r = find_region(w, id); return r ? r->last_burn_year : 0; }
    if (kind == "remnant") { for (auto& r : w.remnants) if (r.race == id) return r.strength; }
    if (kind == "item") { for (auto& r : w.dragons[kPlayer].chron.relics) if (r.id == id) return r.year; }
    return 0;
}

// candidates in `choose` order: most_recent (newest first), by_score (highest first), random (keyed shuffle)
void order(const Ctx& x, const Role& r, std::vector<std::string>& ids, int waking) {
    std::string ch = r.spec.value("choose", std::string("most_recent"));
    if (ch == "random") {
        std::sort(ids.begin(), ids.end());
        Rng g = rng_for(x.w.seed, "scn.choose", x.d.id + ":" + r.name + ":" + std::to_string(waking));
        for (size_t i = ids.size(); i > 1; --i) std::swap(ids[i - 1], ids[static_cast<size_t>(g.range(0, static_cast<int>(i)))]);
        return;
    }
    std::string by = ch.rfind("by_score:", 0) == 0 ? ch.substr(9) : "recent";
    std::vector<std::pair<double, std::string>> s;
    for (auto& id : ids) s.push_back({r.kind == "act" ? std::atof(id.c_str()) : score_of(x, r.kind, id, by), id});
    std::stable_sort(s.begin(), s.end(), [](const auto& a, const auto& b) { return a.first != b.first ? a.first > b.first : a.second < b.second; });
    for (size_t i = 0; i < s.size(); ++i) ids[i] = s[i].second;
}

std::vector<std::string> faction_candidates(const Ctx& x, const json& s) {
    const World& w = x.w;
    std::vector<std::string> pool;
    if (s.contains("is")) {
        auto c = call_of(s["is"].get<std::string>());
        std::string arg = c.second.size() > 1 && c.second[0] == '$' ? c.second.substr(1, c.second.find('.') == std::string::npos ? std::string::npos : c.second.find('.') - 1) : c.second;
        std::string id = arg == "lair" || arg == "dragon" ? arg : x.id(arg);
        if (c.first == "owner_of") { std::string o = owner_of(w, id, x.kind(arg)); if (!o.empty()) pool.push_back(o); }
        else if (c.first == "neighbour_of") pool = neighbours(w, id);
        else if (c.first == "strongest_army:neighbours") {
            std::string best;
            double bp = -1;
            for (auto& n : neighbours(w, id)) if (const Faction* f = find_faction(w, n)) if (power(*f, x.p.c) > bp) { bp = power(*f, x.p.c); best = n; }
            if (!best.empty()) pool.push_back(best);
        } else if (c.first == "worst_relations") {
            std::string best;
            double bv = -2;
            for (auto& f : w.factions) if (dread_eff(f) - awe_eff(f) > bv) { bv = dread_eff(f) - awe_eff(f); best = f.id; }
            if (!best.empty()) pool.push_back(best);
        } else if (c.first == "has_tag") { for (auto& f : w.factions) if (tag_match(f.tags, c.second)) pool.push_back(f.id); }
    } else for (auto& f : w.factions) pool.push_back(f.id);
    std::vector<std::string> out;
    for (auto& id : pool) {
        const Faction* f = find_faction(w, id);
        if (!f || contains(out, id)) continue;
        if (s.contains("remembers")) { const Act* a = x.act(ref(s["remembers"])); if (!a || !remembers(w, x.p, *f, std::atoi(x.id(ref(s["remembers"])).c_str()))) continue; }
        if (s.contains("era_min") && f->era < s["era_min"].get<int>()) continue;
        if (s.contains("era_max") && f->era > s["era_max"].get<int>()) continue;
        if (s.contains("race") && f->race != s["race"].get<std::string>()) continue;
        if (!in_range(s, "dread", dread_eff(*f)) || !in_range(s, "awe", awe_eff(*f)) || !in_range(s, "cohesion", f->cohesion)) continue;
        if (!in_range(s, "piety", f->culture.traits.piety) || !in_range(s, "openness", f->culture.traits.openness)) continue;
        if (!flag_is(s, "at_war", !f->at_war_with.empty())) continue;
        bool tags = true;
        for (auto& t : s.value("has_tag", json::array())) tags = tags && tag_match(f->tags, t.get<std::string>());
        if (!tags) continue;
        Value prophecy = faction_value(x, *f, "knows_prophecy"), slayers = faction_value(x, *f, "has_slayers"), temple = faction_value(x, *f, "has_temple");
        Value library = faction_value(x, *f, "has_library"), stage = faction_value(x, *f, "slayer_stage"), tier = faction_value(x, *f, "memory_tier");
        if (!flag_is(s, "knows_prophecy", prophecy.num > 0) || !flag_is(s, "has_slayers", slayers.num > 0)) continue;
        if (!flag_is(s, "has_temple", temple.num > 0) || !flag_is(s, "has_library", library.num > 0)) continue;
        if (s.contains("slayer_stage") && stage.str != s["slayer_stage"].get<std::string>()) continue;
        if (s.contains("memory_tier_min") && memory_rank(tier.str) < memory_rank(s["memory_tier_min"].get<std::string>())) continue;
        out.push_back(id);
    }
    return out;
}

std::vector<std::string> region_candidates(const Ctx& x, const json& s) {
    const World& w = x.w;
    std::vector<std::string> out;
    for (auto& r : w.regions) {
        if (r.lair) continue;
        if (s.contains("owner")) {
            std::string o = s["owner"].get<std::string>();
            if (o == "none" ? !r.owner.empty() : r.owner != x.id(ref(s["owner"]))) continue;
        }
        if (s.contains("terrain") && !contains(s["terrain"].get<std::vector<std::string>>(), r.terrain)) continue;
        bool tags = true;
        for (auto& t : s.value("tags", json::array())) tags = tags && tag_match(r.tags, t.get<std::string>());
        if (!tags || !in_range(s, "burn", r.burn)) continue;
        if (s.contains("has_structure") && !has_tag(r.tags, s["has_structure"].get<std::string>())) continue;
        if (s.contains("adjacent_to")) {
            std::string a = x.id(ref(s["adjacent_to"]));
            bool next = contains(r.neighbours, a);
            if (const Faction* f = find_faction(w, a)) for (auto& rid : f->regions) next = next || contains(r.neighbours, rid);
            if (!next) continue;
        }
        out.push_back(r.id);
    }
    return out;
}

double tier_stock_of(const Tiers& t, const std::string& tier) {
    return tier == "monumental" ? t.monumental : tier == "written" ? t.written : t.oral;
}

bool knows_dragon(const World& w, const Person& p) {
    return find_known(w, p.id) || !p.mem.empty() || !p.permanent.empty() || p.memory_of_dragon > 0;
}

bool memory_has(const Person& p, const std::string& line) {
    for (auto& m : p.mem) if (m.ev == line) return true;
    for (auto& m : p.permanent) if (m.ev == line) return true;
    return false;
}

std::vector<std::string> person_candidates(const Ctx& x, const Role& r) {
    const World& w = x.w;
    const json& s = r.spec;
    std::vector<std::string> out;
    std::string court = s.contains("court") ? s["court"].get<std::string>() : "";
    if (!court.empty() && court != "lair") court = x.id(ref(s["court"]));
    if (s.contains("court") && court.empty()) return out;
    for (auto& p : w.people) {
        if (!court.empty() && person_court(w, p) != court) continue;
        if (s.contains("role") && !person_role(w, p, s["role"].get<std::string>())) continue;
        if (s.contains("tag") && !person_tag(w, p, s["tag"].get<std::string>())) continue;
        if (s.contains("trait")) { std::string t = s["trait"].get<std::string>(); if (!has_trait(p, t) && p.virtue != t && p.vice != t) continue; }
        if (!flag_is(s, "knows_dragon", knows_dragon(w, p))) continue;
        if (s.contains("memory_has") && !memory_has(p, s["memory_has"].get<std::string>())) continue;
        if (!flag_is(s, "is_lie", person_is_lie(w, p))) continue;
        auto it = x.c.renown.find(p.id);
        double renown = std::min(1.0, (it == x.c.renown.end() ? 0 : it->second) / std::max(1.0, x.p.scenarios.n.renown_events));
        if (!in_range(s, "renown", renown) || !in_range(s, "cunning", p.traits.cunning) || !in_range(s, "age", w.year - p.born)) continue;
        if (r.sex != "any" && p.sex != r.sex) continue;
        bool taken = false;   // one person, one role
        for (auto& b : x.bind) if (b.second == p.id) taken = true;
        if (taken) continue;
        out.push_back(p.id);
    }
    return out;
}

std::vector<std::string> candidates(const Ctx& x, const Role& r, const std::function<bool(int)>& cite_ok) {
    std::vector<std::string> out;
    const json& s = r.spec;
    if (r.kind == "act") {
        for (auto& t : types_of(s)) {
            auto it = x.c.acts_by_type.find(t);
            if (it == x.c.acts_by_type.end()) continue;
            for (int i : it->second) if (act_matches(x, s, i) && cite_ok(i)) out.push_back(std::to_string(i));
        }
    } else if (r.kind == "faction") out = faction_candidates(x, s);
    else if (r.kind == "region") out = region_candidates(x, s);
    else if (r.kind == "person") out = person_candidates(x, r);
    else if (r.kind == "remnant") {
        for (auto& m : x.w.remnants)
            if ((!s.contains("race") || m.race == s["race"].get<std::string>()) && in_range(s, "strength", m.strength) && m.strength > 0) out.push_back(m.race);
    } else if (r.kind == "item") {
        std::string in = s.value("in", std::string("hoard"));
        if (in == "hoard")
            for (auto& rl : x.w.dragons[kPlayer].chron.relics) {
                if (s.contains("agenda") && rl.agenda != s["agenda"].get<std::string>()) continue;
                if (s.contains("kind") && s["kind"].get<std::string>() != "artifact" && rl.kind != s["kind"].get<std::string>()) continue;
                out.push_back(rl.id);
            }
    }
    return out;
}

struct Search {
    Ctx& x;
    const std::function<bool(int)>& cite_ok;
    int waking;
    std::vector<std::vector<size_t>> req_at, forbid_at;   // checks that become decidable at each role index
    int steps = 0;

    bool check(size_t i) {
        PredEnv env = env_of(x);
        for (size_t k : req_at[i]) if (!eval(x.d.require[k], env)) return false;
        for (size_t k : forbid_at[i]) {
            const Forbid& f = x.d.forbid[k];
            if (!f.is_act) { if (eval(f.pred, env)) return false; continue; }
            for (auto& t : types_of(f.act)) {
                auto it = x.c.acts_by_type.find(t);
                if (it != x.c.acts_by_type.end()) for (int a : it->second) if (act_matches(x, f.act, a)) return false;
            }
        }
        return true;
    }

    bool go(size_t i) {
        if (i == x.d.roles.size()) return true;
        if (++steps > 4000) return false;
        const Role& r = x.d.roles[i];
        std::vector<std::string> ids = candidates(x, r, cite_ok);
        order(x, r, ids, waking);
        if (ids.size() > static_cast<size_t>(x.p.scenarios.n.candidates_per_role)) ids.resize(static_cast<size_t>(x.p.scenarios.n.candidates_per_role));
        for (auto& id : ids) {
            x.bind[r.name] = id;
            if (check(i) && go(i + 1)) return true;
        }
        x.bind.erase(r.name);
        if (r.optional && check(i) && go(i + 1)) return true;
        return false;
    }
};

size_t decided_at(const Def& d, const std::vector<std::string>& names) {
    size_t at = 0;
    for (auto& n : names)
        for (size_t i = 0; i < d.roles.size(); ++i) if (d.roles[i].name == n) at = std::max(at, i);
    return at;
}

}

bool remembers(const World& w, const Params& p, const Faction& f, int act) {
    (void)w;
    for (auto& m : f.memory) {
        if (m.act != act) continue;
        double stock = std::max(m.axis == "awe" ? 0.0 : tier_stock_of(f.dread, m.tier), m.axis == "dread" ? 0.0 : tier_stock_of(f.awe, m.tier));
        if (stock >= p.scenarios.n.remember_stock) return true;
    }
    return false;
}

bool act_matches(const Ctx& x, const json& pat, int i) {
    const World& w = x.w;
    const Act& a = w.acts[static_cast<size_t>(i)];
    auto ts = types_of(pat);
    if (!ts.empty() && !contains(ts, a.type)) return false;
    if (pat.contains("structure") && a.item != pat["structure"].get<std::string>()) return false;
    for (auto& k : pat.value("payload_has", json::array())) {
        std::string key = k.get<std::string>();
        auto pv = a.payload.find(key);
        if ((pv == a.payload.end() || pv->second <= 0) && !a.args.count(key)) return false;
    }
    if (!in_range(pat, "magnitude", a.magnitude)) return false;
    if (!in_range(pat, "age_wakings", act_age_wakings(x.c, i))) return false;
    if (!in_range(pat, "age_years", w.year - a.year)) return false;
    if (pat.contains("source") && (a.source.empty() ? "raid" : a.source) != pat["source"].get<std::string>()) return false;
    if (pat.contains("answers") && a.answers != pat["answers"].get<std::string>()) return false;
    if (pat.contains("target")) {
        std::string role = ref(pat["target"]), id = x.id(role);
        if (id.empty() || (act_court(w, a) != id && a.item != id && a.region != id)) return false;
    }
    for (const char* k : {"after", "before"}) {
        if (!pat.contains(k)) continue;
        std::string id = x.id(ref(pat[k]));
        if (id.empty()) return false;
        int other = std::atoi(id.c_str());
        if (std::string(k) == "after" ? i <= other : i >= other) return false;
    }
    return true;
}

bool bind_roles(Ctx& x, const std::function<bool(int)>& cite_ok, int waking) {
    Search s{x, cite_ok, waking, {}, {}};
    size_t n = std::max<size_t>(1, x.d.roles.size());
    s.req_at.assign(n, {});
    s.forbid_at.assign(n, {});
    for (size_t k = 0; k < x.d.require.size(); ++k) s.req_at[decided_at(x.d, pred_roles(x.d.require[k]))].push_back(k);
    for (size_t k = 0; k < x.d.forbid.size(); ++k) {
        std::vector<std::string> names;
        if (x.d.forbid[k].is_act) {
            for (const char* key : {"target", "after", "before"}) if (x.d.forbid[k].act.contains(key)) names.push_back(ref(x.d.forbid[k].act[key]));
        } else names = pred_roles(x.d.forbid[k].pred);
        s.forbid_at[decided_at(x.d, names)].push_back(k);
    }
    x.bind.clear();
    if (x.d.roles.empty()) return s.check(0);
    return s.go(0);
}

std::string bind_why(Ctx& x) {
    x.bind.clear();
    for (auto& r : x.d.roles) {
        std::vector<std::string> ids = candidates(x, r, [](int) { return true; });
        if (ids.empty()) { if (r.optional) continue; return "role " + r.name + ": no candidate"; }
        order(x, r, ids, 0);
        x.bind[r.name] = ids.front();
    }
    PredEnv env = env_of(x);
    for (auto& q : x.d.require) if (!eval(q, env)) return "require: " + q.text;
    return "forbid, or a combination of roles";
}

}
