#include <algorithm>
#include <cstdio>
#include <set>
#include "core/detmath.h"
#include "ages/world_util.h"
#include "bind.h"
#include "scn.h"

namespace scn {

using namespace ages;

namespace {

std::string theme(const std::string& tag) { return tag.rfind("theme:", 0) == 0 ? tag : ""; }

// faction:$role tags resolved to the bound court, theme tags as they are
std::vector<std::string> quota_keys(const Def& d, const std::map<std::string, std::string>& roles) {
    std::vector<std::string> out;
    for (auto& t : d.tags) {
        if (!theme(t).empty()) out.push_back(t);
        else if (t.rfind("faction:$", 0) == 0) { auto it = roles.find(t.substr(9)); if (it != roles.end()) out.push_back("faction:" + it->second); }
    }
    return out;
}

bool monumental(const World& w, int act) {
    for (auto& f : w.factions) for (auto& m : f.memory) if (m.act == act && m.tier == "monumental") return true;
    return false;
}

int tier_rank(const std::string& stakes) { return stakes == "must" ? 2 : stakes == "major" ? 1 : 0; }

struct Cand {
    const Def* d;
    std::map<std::string, std::string> roles;
    std::vector<int> cites;
    double score = 0;
};

bool fits(const Cand& c, const std::string& slot) {
    const std::string& k = c.d->kind;
    if (slot == "must") return c.d->stakes == "must";
    if (c.d->stakes == "must") return false;
    if (slot == "consequence") return k == "consequence";
    if (slot == "audience") return k == "audience";
    return k == "world" || k == "lair" || k == "omen" || k == "consequence";
}

}

int current_waking(const World& w) {
    int n = 0;
    for (auto& a : w.acts) if (a.type == "sleep") ++n;
    return n;
}

std::vector<Pick> select(const World& w, const Params& p, Diag* diag) {
    std::vector<Pick> out;
    const Bank& bank = p.scenarios;
    if (bank.defs.empty()) return out;
    const Numbers& n = bank.n;
    Cache cache = make_cache(w, p);
    int W = cache.waking;
    std::string era = n.eras.empty() ? "" : n.eras[static_cast<size_t>(std::clamp(cache.strongest_era, 0, static_cast<int>(n.eras.size()) - 1))];
    std::map<int, int> cited_run;
    std::map<std::string, int> last_shown, last_year, shown_count;
    std::map<std::string, int> theme_run;
    int total = 0;
    std::vector<std::string> recent_themes;
    bool consequence_recent = false;
    for (auto& s : w.scn.shown) {
        for (int a : s.cites) cited_run[a] += 1;
        last_shown[s.scenario] = s.waking;
        last_year[s.scenario] = s.year;
        shown_count[s.scenario] += 1;
        total += 1;
        if (const Def* d = bank.find(s.scenario)) {
            for (auto& t : d->tags) if (!theme(t).empty()) {
                theme_run[t] += 1;
                if (W - s.waking <= n.theme_wakings) recent_themes.push_back(t);
            }
            if (d->kind == "consequence" && W - s.waking < n.pity_wakings) consequence_recent = true;
        }
    }
    auto cite_ok = [&](int a) { auto it = cited_run.find(a); return it == cited_run.end() || it->second < (monumental(w, a) ? n.cites_monumental : n.cites_per_run); };
    bool acted = false;
    for (size_t i = 0; i < w.acts.size(); ++i) {
        const Act& a = w.acts[i];
        if (a.witnesses.empty() || a.magnitude < n.pity_magnitude) continue;
        int age = act_age_wakings(cache, static_cast<int>(i));
        if (age >= 0 && age < n.pity_wakings) acted = true;
    }
    bool pity = acted && !consequence_recent;

    std::vector<Cand> cands;
    for (auto& d : bank.defs) {
        if (!d.eras.empty() && !contains(d.eras, era)) continue;
        auto ls = last_shown.find(d.id);
        if (ls != last_shown.end()) {
            if (d.once != "never") continue;
            if (W - ls->second < d.cd_wakings || w.year - last_year[d.id] < d.cd_years) continue;
        }
        if (!d.group.empty() && w.scn.closed.count(d.group)) continue;
        bool suppressed = false;
        for (auto& t : d.tags) { auto it = w.scn.suppress.find(t); if (it != w.scn.suppress.end() && it->second > w.year) suppressed = true; }
        if (suppressed) continue;
        Ctx x{w, p, d, cache, {}};
        if (!bind_roles(x, cite_ok, W)) continue;
        if (d.has_arrival) {
            const Faction* f = find_faction(w, x.id(d.arrival_from));
            bool open = f != nullptr;
            for (auto& g : d.needs) open = open && gate_open(w, p, bank, g, *f);
            if (!open) continue;
        }
        Cand c{&d, x.bind, {}, 0};
        for (auto& r : d.roles) if (r.kind == "act" && x.bound(r.name)) c.cites.push_back(std::atoi(x.id(r.name).c_str()));
        double s = n.spec_w * static_cast<double>(d.roles.size() + d.require.size() + d.forbid.size());
        PredEnv env = env_of(x);
        for (auto& sl : d.salience) if (eval(sl.pred, env)) s += sl.w;
        for (int a : c.cites) s += n.cite_w * w.acts[static_cast<size_t>(a)].magnitude * det::half_life(act_age_wakings(cache, a), n.cite_half);
        if (ls != last_shown.end()) s -= n.recent_w * det::half_life(W - ls->second, n.recent_half);
        for (auto& t : d.tags) if (!theme(t).empty()) {
            s -= n.theme_w * static_cast<double>(std::count(recent_themes.begin(), recent_themes.end(), t));
            if (total * n.theme_share >= 1 && theme_run[t] > total * n.theme_share) s -= n.theme_share_w;
            auto sk = w.scn.seek.find(t);
            if (sk != w.scn.seek.end() && sk->second == W) s += n.seek_w;
        }
        auto sb = w.scn.seen_before.find(d.id);
        if (sb != w.scn.seen_before.end()) s -= n.seen_w * std::min(n.seen_cap, sb->second);
        if (d.kind == "audience") {
            auto ao = w.scn.arrival_open.find(d.id);
            if (ao != w.scn.arrival_open.end()) s += n.arrival_pity_w * std::max(0, W - ao->second - n.arrival_pity_after);
        }
        c.score = s;
        cands.push_back(c);
    }
    if (diag) {
        for (auto& c : cands) { diag->eligible.push_back(c.d->id); diag->scores[c.d->id] = c.score; if (c.d->kind == "consequence") diag->consequence_depth += 1; }
        diag->acted_last = acted;
        diag->pity = pity;
    }

    std::set<std::string> used_keys;
    std::set<int> cited_now;
    std::set<const Def*> taken;
    char hx[17];
    std::snprintf(hx, sizeof hx, "%016llx", static_cast<unsigned long long>(bank.hash));
    for (const char* slot : {"must", "consequence", "audience", "open"}) {
        bool force = pity && std::string(slot) == "consequence";
        std::vector<const Cand*> pool;
        for (auto& c : cands) {
            if (taken.count(c.d) || !fits(c, slot)) continue;
            bool clash = false;
            for (int a : c.cites) clash = clash || (n.cites_per_waking <= 1 && cited_now.count(a));
            if (!force) for (auto& k : quota_keys(*c.d, c.roles)) clash = clash || used_keys.count(k);
            if (!clash) pool.push_back(&c);
        }
        if (pool.empty()) continue;
        int tier = 0;
        for (auto* c : pool) tier = std::max(tier, tier_rank(c->d->stakes));
        double best = -1e18;
        for (auto* c : pool) if (tier_rank(c->d->stakes) == tier) best = std::max(best, c->score);
        std::vector<const Cand*> window;
        double weights = 0;
        for (auto* c : pool) if (tier_rank(c->d->stakes) == tier && c->score >= best - n.window) { window.push_back(c); weights += c->d->weight; }
        Rng g = rng_for(w.seed, "scn.pick", std::to_string(W) + ":" + hx + ":" + slot);
        double r = g.uniform() * weights;
        const Cand* pick = window.back();
        for (auto* c : window) { if (r < c->d->weight) { pick = c; break; } r -= c->d->weight; }
        taken.insert(pick->d);
        for (int a : pick->cites) cited_now.insert(a);
        for (auto& k : quota_keys(*pick->d, pick->roles)) used_keys.insert(k);
        out.push_back({pick->d, pick->roles, pick->cites, slot, pick->score});
    }
    return out;
}

void wake(World& w, const Params& p) {
    if (!w.scn.on || p.scenarios.defs.empty() || has_tag(w.tags, "dragon_slain") || !w.ended.empty()) return;
    Diag diag;
    std::vector<Pick> picks = select(w, p, &diag);
    int W = current_waking(w);
    std::set<std::string> eligible(diag.eligible.begin(), diag.eligible.end());
    for (auto& d : p.scenarios.defs) {
        if (d.kind != "audience") continue;
        if (!eligible.count(d.id)) w.scn.arrival_open.erase(d.id);
        else if (!w.scn.arrival_open.count(d.id)) w.scn.arrival_open[d.id] = W;
    }
    for (auto& pk : picks) {
        ScnShown s;
        s.scenario = pk.def->id;
        s.slot = pk.slot;
        s.waking = W;
        s.year = w.year;
        s.roles = pk.roles;
        s.cites = pk.cites;
        w.scn.shown.push_back(s);
        w.scn.arrival_open.erase(pk.def->id);
        for (auto& t : pk.def->seek) w.scn.seek[t] = W + pk.def->seek_after;
    }
    for (auto it = w.scn.seek.begin(); it != w.scn.seek.end();) it = it->second < W ? w.scn.seek.erase(it) : std::next(it);
    w.scn.diag_eligible = diag.eligible;
    w.scn.diag_depth = diag.consequence_depth;
    w.scn.diag_acted = diag.acted_last;
}

void enable(World& w, const Profile* pr) {
    w.scn.on = true;
    if (!pr || !pr->seen.is_object() || !pr->seen.contains("scenarios")) return;
    for (auto it = pr->seen["scenarios"].begin(); it != pr->seen["scenarios"].end(); ++it)
        if (it.value().is_number_integer()) w.scn.seen_before[it.key()] = it.value().get<int>();
}

void note_seen(Profile& pr, const World& w) {
    if (!pr.seen.is_object()) pr.seen = nlohmann::json::object();
    nlohmann::json& s = pr.seen["scenarios"];
    if (!s.is_object()) s = nlohmann::json::object();
    for (auto& x : w.scn.shown) s[x.scenario] = s.value(x.scenario, 0) + 1;
}

}
