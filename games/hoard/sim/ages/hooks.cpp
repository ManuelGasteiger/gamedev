#include "hooks.h"
#include "arrivals.h"
#include "defences.h"
#include "names.h"
#include "rules/card_ctx.h"
#include "world_util.h"

namespace ages {

namespace {

struct Candidate {
    Hook hook;
    const HookDef* def = nullptr;
    std::string self;
    double weight = 0;
    bool forced = false;
};

std::vector<std::string> selves(const World& w, const HookDef& t) {
    std::vector<std::string> out;
    if (t.scope == "world") out.push_back("world");
    else if (t.scope == "region") for (auto& r : w.regions) out.push_back(r.id);
    else for (auto& f : w.factions) out.push_back(f.id);
    return out;
}

std::string near_lair(const World& w, const Faction& f) {
    for (auto& rid : f.regions)
        if (const Region* r = find_region(w, rid))
            for (auto& n : r->neighbours) if (const Region* nr = find_region(w, n)) if (nr->lair) return rid;
    return capital_region(w, f);
}

std::string largest(const World& w, const Faction& f) {
    std::string best;
    double top = -1;
    for (auto& rid : f.regions) if (const Region* r = find_region(w, rid)) if (r->land > top) { top = r->land; best = rid; }
    return best;
}

// the court's region that borders the other side of its war, else its capital
std::string front(const World& w, const Faction& f, const std::string& other) {
    const Faction* o = find_faction(w, other);
    if (o)
        for (auto& rid : f.regions)
            if (const Region* r = find_region(w, rid))
                for (auto& n : r->neighbours) if (contains(o->regions, n)) return rid;
    return capital_region(w, f);
}

std::string where(const World& w, const HookDef& t, const CardCtx& ctx, const Faction* f) {
    if (!t.where.empty() && t.where[0] == '$') return var_of(ctx, t.where);
    if (t.where == "lair") return lair_region(w);
    if (!f) return "";
    if (t.where == "near_lair") return near_lair(w, *f);
    if (t.where == "front") return front(w, *f, var_of(ctx, "other"));
    if (t.where == "largest") return largest(w, *f);
    return capital_region(w, *f);
}

std::string cap(std::string s) { if (!s.empty() && s[0] >= 'a' && s[0] <= 'z') s[0] = static_cast<char>(s[0] - 32); return s; }

void replace_all(std::string& s, const std::string& from, const std::string& to) {
    for (size_t pos = s.find(from); pos != std::string::npos; pos = s.find(from, pos + to.size())) s.replace(pos, from.size(), to);
}

std::string fill(const World& w, std::string s, const Hook& h) {
    const std::pair<const char*, std::string> names[] = {{"faction", h.faction}, {"region", h.region}, {"person", h.person}, {"other", h.other}, {"lair", lair_region(w)}};
    for (auto& kv : names) {
        std::string n = display_name(w, kv.second);
        replace_all(s, std::string("{") + kv.first + "}", n);
        replace_all(s, std::string("{") + cap(kv.first) + "}", cap(n));
    }
    return s;
}

std::string text_of(const HookDef& t, const char* key) { auto it = t.text.find(key); return it == t.text.end() ? "" : it->second; }

Hook make_hook(const World& w, const HookDef& t, const CardCtx& ctx) {
    Hook h;
    h.kind = t.kind;
    h.source = t.id;
    std::string self = var_of(ctx, "self");
    const Faction* f = find_faction(w, self);
    if (!f && t.scope == "region") f = find_faction(w, var_of(ctx, "owner"));
    h.faction = f ? f->id : "";
    h.person = var_of(ctx, "person");
    h.other = var_of(ctx, "other");
    h.region = where(w, t, ctx, f);
    h.acts = t.acts;
    h.choices = t.choices;
    h.fight = t.fight_damage > 0 || t.fight_strength_mult > 0 || t.fight_health_share > 0;
    h.offered = w.year;
    h.refs = ctx.refs;
    h.expires_year = w.year;   // the next sleep starts in the waking's year: no time passes while the dragon is awake
    h.id = h.kind + ":" + h.region + ":" + std::to_string(w.year);
    h.title = text_of(t, "title");
    h.text = fill(w, text_of(t, "text"), h);
    h.reward_text = fill(w, text_of(t, "reward_text"), h);
    h.ignore_text = fill(w, text_of(t, "ignore_text"), h);
    return h;
}

bool on_cooldown(const World& w, const HookDef& t, const std::string& self) {
    if (t.cooldown_years <= 0) return false;
    auto it = w.hook_fired.find(t.kind + "|" + self);
    return it != w.hook_fired.end() && w.year - it->second < t.cooldown_years;
}

bool offered_last_waking(const World& w, const std::string& kind) {
    for (auto& kv : w.hook_fired)
        if (kv.second == w.sleep_started && kv.first.compare(0, kind.size() + 1, kind + "|") == 0) return true;
    return false;
}

double pick_value(const World& w, const Params& p, const HookDef& t, const CardCtx& ctx) {
    size_t dot = t.pick.find('.');
    std::string id = var_of(ctx, dot == std::string::npos ? "self" : t.pick.substr(0, dot));
    std::string key = dot == std::string::npos ? t.pick : t.pick.substr(dot + 1);
    double v = 0;
    card_num_field(w, p, ctx, id, key, v);
    return v;
}

// one candidate per template (the best by `pick`, else the first self in id order that holds), with its bindings
std::vector<Candidate> candidates(const World& w, const Params& p) {
    std::map<std::string, int> last_act;
    int any_act = 0;
    for (auto& e : w.chronicle)
        if (e.id > 0 && e.kind.rfind("act_", 0) == 0) { last_act[e.subject] = e.year; any_act = e.year; }
    std::vector<Candidate> out;
    for (auto& t : p.hooks.templates) {
        if (t.acts.empty() && t.choices.empty()) continue;
        bool have = false;
        double best = 0;
        Candidate keep;
        for (auto& self : selves(w, t)) {
            if (on_cooldown(w, t, self)) continue;
            if (!t.arrival.empty() && !arrival_for(w, t.arrival, self)) continue;   // item 06
            CardCtx ctx;
            ctx.last_act = &last_act;
            ctx.any_act = any_act;
            ctx.vars["self"] = self;
            ctx.vars["year"] = std::to_string(w.year);
            if (const Region* r = find_region(w, self)) { ctx.vars["region"] = self; ctx.vars["owner"] = r->owner; }
            if (!card_holds(w, p, t.when, ctx)) continue;
            std::string person = var_of(ctx, "person");
            if (!person.empty() && on_cooldown(w, t, person)) continue;
            Candidate c;
            c.hook = make_hook(w, t, ctx);
            if (c.hook.region.empty()) continue;
            c.def = &t;
            c.self = self;
            c.weight = t.priority * (offered_last_waking(w, t.kind) ? p.hooks.repeat_mult : 1.0);
            c.forced = !t.arrival.empty();
            if (!t.force_if.empty()) { CardCtx probe = ctx; c.forced = c.forced || card_holds(w, p, t.force_if, probe); }
            double v = t.pick.empty() ? 0 : pick_value(w, p, t, ctx);
            if (have && !(v > best)) continue;
            have = true;
            best = v;
            keep = c;
            if (t.pick.empty()) break;
        }
        if (have) out.push_back(keep);
    }
    return out;
}

bool clashes(const World& w, const std::vector<Candidate>& chosen, const Candidate& c) {
    for (auto& x : chosen) {
        if (x.hook.kind == c.hook.kind) return true;
        if (x.hook.region == c.hook.region && c.hook.region != lair_region(w)) return true;
    }
    return false;
}

// a weighted draw from the pool, the clashing entries dropped first
bool draw(World& w, std::vector<Candidate>& pool, std::vector<Candidate>& chosen) {
    pool.erase(std::remove_if(pool.begin(), pool.end(), [&](const Candidate& c) { return clashes(w, chosen, c); }), pool.end());
    double total = 0;
    for (auto& c : pool) total += c.weight;
    if (total <= 0) return false;
    double roll = w.rng.uniform() * total;
    for (size_t i = 0; i < pool.size(); ++i) {
        if (roll >= pool[i].weight && i + 1 < pool.size()) { roll -= pool[i].weight; continue; }
        chosen.push_back(pool[i]);
        pool.erase(pool.begin() + static_cast<std::ptrdiff_t>(i));
        return true;
    }
    return false;
}

}

std::string lair_region(const World& w) {
    for (auto& r : w.regions) if (r.lair) return r.id;
    return "";
}

const HookDef* hook_def(const Params& p, const Hook& h) {
    for (auto& t : p.hooks.templates) if (t.id == h.source) return &t;
    for (auto& t : p.hooks.templates) if (t.kind == h.kind) return &t;
    return nullptr;
}

// one to max_hooks situations: forced ones (a premonition's slayers, the cult's pilgrims) first, then a weighted draw
// by priority from what the world and this sleep offer, a fallback only when nothing else calls
std::vector<Hook> generate_hooks(World& w, const Params& p) {
    const HookBook& b = p.hooks;
    w.hooks.clear();
    if (b.templates.empty()) return {};
    std::vector<Candidate> all = candidates(w, p), pool, fallbacks, chosen;
    bool second = w.rng.chance(b.second);
    bool third = w.rng.chance(b.third);
    int n = 1 + (second ? 1 : 0) + (third ? 1 : 0);
    n = std::min(n, b.max_hooks);
    for (auto& c : all) {
        if (c.forced && static_cast<int>(chosen.size()) < b.max_hooks && !clashes(w, chosen, c)) chosen.push_back(c);
        else if (c.def->fallback) fallbacks.push_back(c);
        else pool.push_back(c);
    }
    while (static_cast<int>(chosen.size()) < n && draw(w, pool, chosen)) {}
    if (chosen.empty()) draw(w, fallbacks, chosen);
    int keep = w.year - w.sleep_started;   // the last waking's entries, and every entry a cooldown still reads
    for (auto& t : b.templates) keep = std::max(keep, t.cooldown_years);
    for (auto it = w.hook_fired.begin(); it != w.hook_fired.end();) it = w.year - it->second > keep ? w.hook_fired.erase(it) : std::next(it);
    for (auto& c : chosen) {
        w.hook_fired[c.hook.kind + "|" + c.self] = w.year;
        if (!c.hook.person.empty()) w.hook_fired[c.hook.kind + "|" + c.hook.person] = w.year;
        w.hooks.push_back(c.hook);
    }
    return w.hooks;
}

void expire_hooks(World& w, const Params& p) {
    std::vector<Hook> open;
    open.swap(w.hooks);
    for (auto& h : open) {
        apply_hook_ignore(w, p, h);
        emit(w, "hook_ignored", h.faction, h.region, 0, {h.kind});
    }
}

}
