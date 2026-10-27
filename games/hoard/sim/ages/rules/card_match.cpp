#include "card_ctx.h"
#include "../memory_tiers.h"
#include <cctype>
#include "../talents.h"
#include <cstdlib>
#include "../world_util.h"

// the trigger language of data/events: every condition yields the variable bindings under which it holds; the matcher
// tries them in order (newest event, first tag in sorted order) and backtracks when a later condition fails

namespace ages {

std::string var_of(const CardCtx& ctx, const std::string& name) {
    std::string n = !name.empty() && name[0] == '$' ? name.substr(1) : name;
    if (n == "world" || n == "dragon" || n == "lair") return n;
    auto it = ctx.vars.find(n);
    return it == ctx.vars.end() ? "" : it->second;
}

std::string subst(const CardCtx& ctx, const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size();) {
        if (s[i] != '$') { out += s[i++]; continue; }
        size_t j = i + 1;
        while (j < s.size() && (std::isalnum(static_cast<unsigned char>(s[j])) || s[j] == '_')) ++j;
        out += var_of(ctx, s.substr(i + 1, j - i - 1));
        i = j;
    }
    return out;
}

void add_refs(CardCtx& ctx, const std::vector<int>& refs) {
    for (int r : refs) if (r > 0 && std::find(ctx.refs.begin(), ctx.refs.end(), r) == ctx.refs.end()) ctx.refs.push_back(r);
}

const Tags* tags_of(const World& w, const std::string& id) {
    if (id == "world") return &w.tags;
    if (id == "dragon") return &w.dragons[kPlayer].tags;
    if (const Faction* f = find_faction(w, id)) return &f->tags;
    if (const Region* r = find_region(w, id)) return &r->tags;
    if (const Person* x = find_person(w, id)) return &x->tags;
    return nullptr;
}
Tags* tags_of(World& w, const std::string& id) { return const_cast<Tags*>(tags_of(static_cast<const World&>(w), id)); }

const TimedTag* timed_tag(const World& w, const std::string& target, const std::string& tag) {
    for (auto& t : w.timed_tags) if (t.target == target && t.tag == tag) return &t;
    return nullptr;
}

static bool kind_matches(const std::string& pattern, const std::string& kind) {
    if (pattern.size() > 1 && pattern.back() == '*') return kind.compare(0, pattern.size() - 1, pattern, 0, pattern.size() - 1) == 0;
    return pattern == kind;
}

int newest_event(const World& w, const std::string& kind, const std::string& subject) {
    for (size_t i = w.chronicle.size(); i > 0; --i) {
        const Event& e = w.chronicle[i - 1];
        if (e.id > 0 && kind_matches(kind, e.kind) && (subject.empty() || e.subject == subject)) return e.id;
    }
    return 0;
}

namespace {

using Ctxs = std::vector<CardCtx>;

int year_of(const World& w, int id) { const Event* e = find_event(w, id); return e ? e->year : 0; }

// "kind@var": the newest such event about that variable (about the condition's own target without @)
int lookup_ref(const World& w, const CardCtx& ctx, const std::string& ref, const std::string& target) {
    if (ref.empty()) return 0;
    size_t at = ref.find('@');
    std::string who = at == std::string::npos ? target : var_of(ctx, ref.substr(at + 1));
    return newest_event(w, ref.substr(0, at), who);
}

double burn_memory(const Faction& f) {
    double top = 0;
    for (auto& m : f.memory) if (m.type == "burn" && m.axis == "dread") top = std::max(top, m.weight);
    return top;
}

// v0.4 world (S2): rim regions a newcomer could take: ownerless, or held by a court of several regions below `weak` cohesion
double rim_open(const World& w, const Params& p, double weak) {
    double n = 0;
    for (auto& id : p.genesis.rim) {
        const Region* r = find_region(w, id);
        if (!r || r->lair) continue;
        const Faction* f = find_faction(w, r->owner);
        if (r->owner.empty() || (f && f->regions.size() > 1 && f->cohesion < weak)) n += 1;
    }
    return n;
}

double capital_land(const World& w, const Faction& f) {
    double top = 0;
    for (auto& rid : f.regions) if (const Region* r = find_region(w, rid)) top = std::max(top, r->land);
    return top;
}

bool faction_num(const World& w, const Params& p, const CardCtx& ctx, const Faction& f, const std::string& k, double& v) {
    const Constants& c = p.c;
    const Traits& t = f.culture.traits;
    if (k == "wealth") v = f.wealth;
    else if (k == "cohesion") v = f.cohesion;
    else if (k == "era") v = f.era;
    else if (k == "dread") v = dread_eff(f);
    else if (k == "awe") v = awe_eff(f);
    else if (k == "dread_written") v = f.dread.written;
    else if (k == "awe_years") v = f.awe_since < 0 ? 0 : w.year - f.awe_since;
    else if (k == "monuments") v = static_cast<double>(f.monuments.size());
    else if (k == "faith") v = f.faith;
    else if (k == "lair_known") v = f.lair_known >= 0 ? 1 : 0;
    else if (k == "lair_exact") v = f.lair_known >= 0 && f.lair_exact ? 1 : 0;
    else if (k == "anti_dragon") v = f.anti_dragon;
    else if (k == "power") v = power(f, c);
    else if (k == "threat_ratio") v = power(f, c) / std::max(1e-9, threat(w.dragons[kPlayer], c));
    else if (k == "regions") v = static_cast<double>(f.regions.size());
    else if (k == "realm") v = static_cast<double>(realm_regions(w, f));
    else if (k == "authority") v = t.authority;
    else if (k == "openness") v = t.openness;
    else if (k == "militarism") v = t.militarism;
    else if (k == "piety") v = t.piety;
    else if (k == "age") v = w.year - f.born;
    else if (k == "vassal") v = is_vassal(f) ? 1 : 0;
    else if (k == "wars") v = static_cast<double>(f.at_war_with.size());
    else if (k == "burn_memory") v = burn_memory(f);
    else if (k == "capital_land") v = capital_land(w, f);
    else if (k == "quiet_years") {
        int last = std::max(f.born, w.genesis.hatched_year);
        if (ctx.last_act) {
            auto it = ctx.last_act->find(f.id);
            if (it != ctx.last_act->end()) last = it->second;
        }
        v = w.year - last;
    } else return false;
    return true;
}

bool num_field(const World& w, const Params& p, const CardCtx& ctx, const std::string& id, const std::string& k, double& v) {
    if (id.empty()) return false;
    if (const Faction* f = find_faction(w, id)) return faction_num(w, p, ctx, *f, k, v);
    if (id == "dragon") {
        const Dragon& d = w.dragons[kPlayer];
        if (k == "size") v = d.size;
        else if (k == "knowledge") v = d.knowledge;
        else if (k == "magic") v = d.magic;
        else if (k == "phase") v = d.phase;
        else if (k == "gold") v = d.hoard.gold;
        else if (k == "hunger") v = d.hunger;
        else if (k == "threat") v = threat(d, p.c);
        else if (k == "asleep_years") v = w.year - w.sleep_started;
        else if (k == "hostages") v = static_cast<double>(d.hostages.size());
        else if (k.rfind("effect:", 0) == 0) v = effect(w, p, k.substr(7));   // v0.4: a talent effect, e.g. effect:thief_guard
        else return false;
        return true;
    }
    if (id == "world") {
        if (k == "year") v = w.year;
        else if (k == "magic") v = w.magic;   // v0.4 world (S2)
        else if (k.rfind("rim_open:", 0) == 0) v = rim_open(w, p, std::atof(k.c_str() + 9));
        else if (k.rfind("courts:", 0) == 0 || k.rfind("minority:", 0) == 0) {
            bool courts = k[0] == 'c';
            std::string race = subst(ctx, k.substr(courts ? 7 : 9));
            v = 0;
            for (auto& f : w.factions) {
                bool own = dominant_race(f) == race;
                auto it = f.pop.find(race);
                if (courts) v += own ? 1 : 0;
                else if (!own && it != f.pop.end()) v = std::max(v, it->second);
            }
        }
        else if (k == "quiet_years") v = w.year - ctx.any_act;
        else if (k == "factions") v = static_cast<double>(w.factions.size());
        else if (k == "max_dread" || k == "max_awe" || k == "max_faith") {
            v = 0;
            for (auto& f : w.factions) v = std::max(v, k == "max_dread" ? dread_eff(f) : (k == "max_awe" ? awe_eff(f) : f.faith));
        } else if (k.rfind("remnant:", 0) == 0) {
            v = 0;
            for (auto& r : w.remnants) if (r.race == subst(ctx, k.substr(8))) v = r.strength;
        } else if (k.rfind("regions:", 0) == 0) {
            v = 0;
            for (auto& f : w.factions) if (dominant_race(f) == subst(ctx, k.substr(8))) v += static_cast<double>(f.regions.size());
        } else return false;
        return true;
    }
    if (const Region* r = find_region(w, id)) {
        if (k == "land") v = r->land;
        else if (k == "eff_land") v = r->eff_land;
        else if (k == "burn") v = r->burn;
        else if (k == "owned") v = r->owner.empty() ? 0 : 1;
        else if (k == "empty_years") v = r->empty_years;   // v0.4 world (S2)
        else return false;
        return true;
    }
    if (k == "alive" && is_person_id(w, id)) { v = find_person(w, id) ? 1 : 0; return true; }
    if (const Person* x = find_person(w, id)) {
        if (k == "age") v = w.year - x->born;
        else if (k == "bold") v = x->traits.bold;
        else if (k == "pious") v = x->traits.pious;
        else if (k == "greedy") v = x->traits.greedy;
        else if (k == "cunning") v = x->traits.cunning;
        else return false;
        return true;
    }
    return false;
}

bool str_field(const World& w, const std::string& id, const std::string& k, std::string& v) {
    if (const Faction* f = find_faction(w, id)) {
        if (k == "race") v = dominant_race(*f);
        else if (k == "founding_race") v = f->race;
        else if (k == "ruler_class") v = f->ruler.cls;
        else if (k == "overlord") v = overlord_of(*f);
        else return false;
        return true;
    }
    if (const Region* r = find_region(w, id)) {
        if (k == "terrain") v = r->terrain;
        else if (k == "owner") v = r->owner;
        else return false;
        return true;
    }
    if (const Person* x = find_person(w, id)) {
        if (k == "role") v = x->role;
        else if (k == "race") v = x->race;
        else if (k == "faction") v = x->faction;
        else return false;
        return true;
    }
    return false;
}

bool compare_num(const std::string& op, double a, double b) {
    if (op == "lt") return a < b;
    if (op == "lte") return a <= b;
    if (op == "gt") return a > b;
    if (op == "gte") return a >= b;
    if (op == "eq") return a == b;
    if (op == "ne") return a != b;
    return false;
}

// "of": "other.power" scales the comparison value by that field (strings compare to it directly)
bool field_holds(const World& w, const Params& p, const CardPred& q, const CardCtx& ctx) {
    std::string id = var_of(ctx, q.on);
    std::string ofid, ofk;
    if (!q.of.empty()) { size_t dot = q.of.find('.'); ofid = var_of(ctx, q.of.substr(0, dot)); ofk = q.of.substr(dot + 1); }
    std::string sv;
    if (str_field(w, id, q.name, sv)) {
        std::string want = subst(ctx, q.str);
        if (!q.of.empty() && !str_field(w, ofid, ofk, want)) return false;
        if (q.op == "in") return contains(q.in, sv);
        if (q.op == "ne") return sv != want;
        return sv == want;
    }
    double v = 0;
    if (!num_field(w, p, ctx, id, q.name, v)) return false;
    double ref = q.value;
    if (!q.of.empty()) {
        double base = 0;
        if (!num_field(w, p, ctx, ofid, ofk, base)) return false;
        ref *= base;
    }
    return compare_num(q.op, v, ref);
}

Ctxs tag_matches(const World& w, const CardPred& q, const CardCtx& ctx) {
    Ctxs out;
    std::string id = var_of(ctx, q.on);
    const Tags* tags = tags_of(w, id);
    if (!tags) return out;
    std::string pat = subst(ctx, q.name);
    bool wild = pat.size() > 1 && pat.back() == '*';
    std::string prefix = wild ? pat.substr(0, pat.size() - 1) : pat;
    for (auto& t : *tags) {
        if (wild ? t.rfind(prefix, 0) != 0 : t != pat) continue;
        CardCtx c = ctx;
        if (wild) c.vars[q.bind.empty() ? "other" : q.bind] = t.substr(prefix.size());
        const TimedTag* tt = timed_tag(w, id, t);
        int ref = lookup_ref(w, c, q.ref, id);
        int from = tt ? tt->from : (ref ? year_of(w, ref) : -1000000);
        if (q.min_age > 0 && (tt || ref ? w.year - from < q.min_age : true)) continue;
        add_refs(c, {tt ? tt->event : 0, ref});
        out.push_back(c);
    }
    return out;
}

std::string party(const Event& e, const std::string& from, const std::string& self) {
    if (from == "subject") return e.subject;
    if (from == "object") return e.object;
    if (from == "dragon") return "dragon";
    if (from == "counterpart") return e.subject == self ? e.object : e.subject;
    if (from == "tag") for (auto& t : e.tags) if (t != self && t != "prehistory") return t;
    return "";
}

bool self_in(const Event& e, const std::string& as, const std::string& self) {
    if (as.empty()) return true;
    if (as == "subject") return e.subject == self;
    if (as == "object") return e.object == self;
    if (as == "either") return e.subject == self || e.object == self;
    if (as == "tag") return e.tags.count(self) > 0;
    return false;
}

// one binding per distinct (other, region, person), the newest event first; min_count counts events per binding
Ctxs event_matches(const World& w, const CardPred& q, const CardCtx& ctx) {
    Ctxs out;
    std::string self = var_of(ctx, q.on);
    std::string tag = subst(ctx, q.event_tag);
    std::map<std::string, size_t> seen;
    std::vector<int> count;
    int horizon = q.this_sleep ? w.sleep_started + 1 : w.year - q.within;   // v0.4 hooks (S3): this sleep's events only
    for (size_t i = w.chronicle.size(); i > 0; --i) {
        const Event& e = w.chronicle[i - 1];
        if (e.year < horizon) break;
        if (e.id <= 0 || e.year > w.year - q.min_age) continue;
        bool kind = false;
        for (auto& k : q.kinds) if (kind_matches(k, e.kind)) kind = true;
        if (!kind || !self_in(e, q.self_as, self) || (!tag.empty() && !e.tags.count(tag)) || e.magnitude < q.min_magnitude) continue;
        std::string other = party(e, q.other_from, self), region = party(e, q.region_from, self), person = party(e, q.person_from, self);
        if (!q.other_from.empty() && other.empty()) continue;
        std::string key = other + "|" + region + "|" + person;
        auto it = seen.find(key);
        if (it != seen.end()) { add_refs(out[it->second], {e.id}); count[it->second] += 1; continue; }
        CardCtx c = ctx;
        if (!q.other_from.empty()) c.vars[q.bind.empty() ? "other" : q.bind] = other;
        if (!q.region_from.empty()) c.vars["region"] = region;
        if (!q.person_from.empty()) c.vars["person"] = person;
        c.refs.push_back(e.id);
        seen[key] = out.size();
        out.push_back(c);
        count.push_back(1);
    }
    Ctxs keep;
    for (size_t i = 0; i < out.size(); ++i) if (count[i] >= q.min_count) keep.push_back(out[i]);
    return keep;
}

bool region_borders(const World& w, const std::string& region, const std::string& other) {
    const Region* r = find_region(w, region);
    if (!r) return false;
    for (auto& n : r->neighbours) {
        const Region* nr = find_region(w, n);
        if (!nr) continue;
        if (other == "lair" ? nr->lair : nr->owner == other) return true;
    }
    return false;
}

bool borders(const World& w, const std::string& a, const std::string& b) {
    if (a.empty() || b.empty() || a == b) return false;
    if (find_region(w, a)) return region_borders(w, a, b);
    const Faction* fa = find_faction(w, a);
    if (!fa) return false;
    if (b == "lair") { for (auto& rid : fa->regions) if (region_borders(w, rid, "lair")) return true; return false; }
    const Faction* fb = find_faction(w, b);
    return fb && adjacent(w, *fa, *fb);
}

Ctxs person_matches(const World& w, const CardPred& q, const CardCtx& ctx) {
    Ctxs out;
    std::string fid = var_of(ctx, q.on);
    std::string tag = subst(ctx, q.tag);
    for (auto& x : w.people) {
        if (x.faction != fid || (!q.name.empty() && x.role != q.name) || (!tag.empty() && !has_tag(x.tags, tag))) continue;
        CardCtx c = ctx;
        c.vars[q.bind.empty() ? "person" : q.bind] = x.id;
        out.push_back(c);
    }
    return out;
}

Ctxs find_matches(const World& w, const Params& p, const CardPred& q, const CardCtx& ctx) {
    Ctxs out;
    std::string not_this = var_of(ctx, q.on);
    std::string tag = subst(ctx, q.tag);
    std::string near = var_of(ctx, q.adjacent_to);
    for (auto& f : w.factions) {
        if (f.id == not_this || (!tag.empty() && !has_tag(f.tags, tag))) continue;
        if (!q.race.empty() && dominant_race(f) != q.race) continue;
        if (!near.empty() && !borders(w, near, f.id)) continue;
        if (q.name == "ally" && !near.empty()) {
            const Faction* n = find_faction(w, near);
            if (!n || !(allied(*n, f) || affinity(p, *n, f) > p.c.affinity_alliance)) continue;
        }
        CardCtx c = ctx;
        c.vars[q.bind.empty() ? "other" : q.bind] = f.id;
        out.push_back(c);
    }
    return out;
}

Ctxs candidates(const World& w, const Params& p, const CardPred& q, const CardCtx& ctx);

// conditions that only hold or fail (no binding): the context passes through unchanged
Ctxs pass(const CardCtx& ctx, bool holds) { return holds ? Ctxs{ctx} : Ctxs{}; }

Ctxs negate_or_pass(const CardPred& q, const CardCtx& ctx, Ctxs found) {
    if (q.absent) return pass(ctx, found.empty());
    if (found.empty() && q.optional) return Ctxs{ctx};
    return found;
}

Ctxs candidates(const World& w, const Params& p, const CardPred& q, const CardCtx& ctx) {
    if (q.type == "any") {
        Ctxs out;
        for (auto& a : q.any) for (auto& c : candidates(w, p, a, ctx)) out.push_back(c);
        return out;
    }
    if (q.type == "tag") return negate_or_pass(q, ctx, tag_matches(w, q, ctx));
    if (q.type == "event") return negate_or_pass(q, ctx, event_matches(w, q, ctx));
    if (q.type == "person") return negate_or_pass(q, ctx, person_matches(w, q, ctx));
    if (q.type == "find") return negate_or_pass(q, ctx, find_matches(w, p, q, ctx));
    if (q.type == "field") {
        Ctxs out = pass(ctx, field_holds(w, p, q, ctx));
        if (!out.empty() && !q.ref.empty()) add_refs(out[0], {lookup_ref(w, ctx, q.ref, var_of(ctx, q.on))});
        return out;
    }
    if (q.type == "adjacent") return pass(ctx, borders(w, var_of(ctx, q.on), var_of(ctx, q.name)) != q.absent);
    if (q.type == "at_war") {
        const Faction* f = find_faction(w, var_of(ctx, q.on));
        return pass(ctx, (f && at_war(*f, var_of(ctx, q.name))) != q.absent);
    }
    if (q.type == "lineage") {
        std::string id = var_of(ctx, q.name);
        bool held = contains(w.dragons[kPlayer].lineages, id);
        for (auto& h : w.dragons[kPlayer].hostages) if (const Person* x = find_person(w, h)) if (x->faction == id) held = true;
        return pass(ctx, held != q.absent);
    }
    if (q.type == "var") {
        std::string v = var_of(ctx, q.name), want = subst(ctx, q.str);
        return pass(ctx, q.op == "ne" ? v != want : v == want);
    }
    if (q.type == "let") {
        std::string v;
        if (!str_field(w, var_of(ctx, q.on), q.str, v)) return {};
        CardCtx c = ctx;
        c.vars[q.name] = v;
        return {c};
    }
    return {};
}

bool match_from(const World& w, const Params& p, const std::vector<CardPred>& preds, size_t i, CardCtx& ctx) {
    if (i == preds.size()) return true;
    for (auto& c : candidates(w, p, preds[i], ctx)) {
        CardCtx next = c;
        if (match_from(w, p, preds, i + 1, next)) { ctx = next; return true; }
    }
    return false;
}

}

bool card_holds(const World& w, const Params& p, const std::vector<CardPred>& preds, CardCtx& ctx) {
    return match_from(w, p, preds, 0, ctx);
}

// v0.4 hooks (S3)
bool card_num_field(const World& w, const Params& p, const CardCtx& ctx, const std::string& id, const std::string& key, double& v) {
    return num_field(w, p, ctx, id, key, v);
}

}
