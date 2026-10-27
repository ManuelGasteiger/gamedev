#include <algorithm>
#include "candidate.h"
#include "sift.h"
#include "sift_util.h"
#include "core/detmath.h"
#include "ages/world_util.h"

// research-story-sifting.md sections 3 and 4: candidates, the headline in Facade's order (top priority tier, score,
// arc fit, a seeded draw among the top three), tier 5 outside the budget, the ages block for long sleeps, callbacks,
// then a greedy fill above the floor. Never padded beyond min_lines.

namespace ages {

int line_budget(int years) {
    int k = 0;
    long long y2 = static_cast<long long>(std::max(1, years)) * std::max(1, years), ten = 10;
    while (k < 6 && y2 >= ten) { ++k; ten *= 10; }
    return std::min(9, 3 + k);
}

namespace {

struct Picker {
    const World& w;
    const Params& p;
    const SiftConstants& k;
    int since, years, waking;
    std::vector<Candidate> cands;
    std::vector<ThreadLine> out;

    Picker(const World& world, const Params& params, int since_year)
        : w(world), p(params), k(params.sift.c), since(since_year), years(std::max(1, world.year - since_year)), waking(world.sift.waking + 1) {}

    double phase() const { return std::min(1.0, std::max(0.0, (waking - 1) / std::max(1.0, k.run_phase_wakings - 1.0))); }
    double diversity() const { return k.diversity_start + (k.diversity_end - k.diversity_start) * phase(); }

    int first_id = 0;   // the first event of the sleep: what came after its sleep_begin

    void mark_start() {
        size_t i = 0;
        while (i < w.chronicle.size() && !(w.chronicle[i].kind == "sleep_begin" && w.chronicle[i].year >= since)) ++i;
        first_id = 1 << 30;
        for (; i < w.chronicle.size(); ++i) if (w.chronicle[i].id > 0) { first_id = w.chronicle[i].id; break; }
        if (i >= w.chronicle.size()) first_id = w.next_event_id;
    }

    bool in_sleep(int id) const {
        const Event* e = sift_event(w, id);
        if (!e) return false;
        if (e->id < kCompoundBase) return e->id >= first_id;
        return !e->cause.refs.empty() && in_sleep(e->cause.refs.back());
    }

    void finish(Candidate& c) {
        for (int id : c.events) if (in_sleep(id)) c.fresh.push_back(id);
        c.court = court_of(w, c.protagonist);
        if (c.court.empty()) c.court = c.protagonist;
        Features f = features(w, p, c);
        c.dragon = f.dragon;
        c.reversals = f.reversal_count;
        c.base = weighted(k, f);
        double s = c.base;
        if (c.kind == "callback" && c.thread && (c.thread->state == "closed" || c.thread->state == "cut")) s += k.shown_end_bonus;
        if (c.tier >= 5) s += k.tier5_bonus;
        if (c.pattern) {
            int n = 0;
            for (size_t i = w.sift.headlines.size(); i > 0 && n < k.recent_headline_wakings; --i, ++n)
                if (w.sift.headlines[i - 1] == c.pattern->id) { s -= k.recent_headline_penalty; break; }
            if (c.pattern->tags.count("prophecy") || c.pattern->tags.count("ending")) s += k.run_bonus_max * phase();
        }
        c.score = s;
        cands.push_back(c);
    }

    void threads() {
        for (auto& t : w.sift.threads) {
            if (t.state == "latent" || t.state == "dropped") continue;
            const SiftPattern* pat = find_pattern(p, t.pattern);
            if (!pat) continue;
            Candidate c;
            c.thread = &t;
            c.pattern = pat;
            c.events = t.events;
            c.protagonist = protagonist(*pat, t);
            c.stakes = pat->stakes;
            c.year = t.state == "open" ? t.advanced : t.ended;
            if (!t.shown.empty()) {
                const SiftShown& last = t.shown.back();
                if (last.state == t.state && last.stage == static_cast<int>(t.events.size())) continue;
                c.kind = "callback";
            } else if (t.events.empty() || !in_sleep(t.events.back())) continue;
            int top = 0;
            for (int id : t.events) if (const Event* e = sift_event(w, id)) top = std::max(top, template_tier(w, p, *e));
            c.tier = t.state == "closed" ? pat->tier : std::min(pat->tier, std::max(2, top));
            if (t.state == "cut" && t.shown.empty()) continue;   // a thread that came to nothing unseen is not news
            finish(c);
        }
    }

    void raw() {
        std::set<int> covered;
        for (auto& c : cands) covered.insert(c.events.begin(), c.events.end());
        std::map<std::string, Candidate> arcs;
        std::vector<Candidate> singles;
        for (auto& e : w.chronicle) {
            if (e.id < first_id || e.id <= 0 || covered.count(e.id)) continue;
            int tier = template_tier(w, p, e);
            if (tier < 2) continue;
            if (!e.thread.empty()) {
                Candidate& c = arcs[e.thread];
                c.events.push_back(e.id);
                if (tier >= c.tier) { c.tier = tier; c.protagonist = e.subject; c.year = e.year; }
                continue;
            }
            Candidate c;
            c.events.push_back(e.id);
            c.tier = tier;
            c.protagonist = e.subject;
            c.year = e.year;
            singles.push_back(c);
        }
        for (auto& kv : arcs) singles.push_back(kv.second);
        for (auto& c : singles) {
            Features f = features(w, p, c);
            c.stakes = std::max(f.dragon, f.stakes);
            finish(c);
        }
    }

    double fit(const Candidate& c) const {
        double target = w.story_breather ? k.t_major : k.t_breather;
        return det::exp(-k.arc_k * std::fabs(target - c.stakes));
    }

    // the most reportable event this candidate stands on
    int lead(const Candidate& c) const {
        int best = 0;
        double top = -1;
        for (int id : c.fresh.empty() ? c.events : c.fresh)
            if (const Event* e = sift_event(w, id)) { double r = reportable(w, p, *e); if (r > top) { top = r; best = id; } }
        return best;
    }

    std::string key_of(const Candidate& c) const {
        if (!c.pattern) {
            const Event* e = sift_event(w, lead(c));
            return e ? e->text_key : "";
        }
        const SiftThread& t = *c.thread;
        std::string base = "pat." + c.pattern->id;
        if (c.kind == "callback") {
            if (t.state == "closed") return base + ".cb.closed" + (t.outcome.empty() ? "" : "." + t.outcome);
            if (t.state == "cut") return base + ".cb.cut";
            return base + ".cb.advanced";
        }
        if (t.state == "closed") return base + ".line" + (t.outcome.empty() ? "" : "." + t.outcome);
        if (t.state == "cut") return base + ".cut";
        return base + ".open";
    }

    ThreadLine line_of(const Candidate& c, const std::string& kind) const {
        ThreadLine l;
        l.kind = kind;
        l.tier = c.tier;
        l.score = c.score;
        l.text_key = key_of(c);
        if (c.thread) {
            l.thread = c.thread->id;
            l.event = c.thread->emitted ? c.thread->emitted : lead(c);
            for (auto& kv : c.thread->cast) l.params["role:" + kv.first.substr(1)] = kv.second;
            for (size_t i = 0; i < c.thread->events.size() && i < c.pattern->clauses.size(); ++i)
                l.params["ev:" + c.pattern->clauses[i].as] = std::to_string(c.thread->events[i]);
            if (!c.thread->outcome.empty()) l.params["outcome"] = c.thread->outcome;
            if (c.kind == "callback" && !c.thread->shown.empty()) {
                const SiftShown& s = c.thread->shown.back();
                l.params["ago"] = std::to_string(waking - s.waking);
                l.params["shown_year"] = std::to_string(s.year);
            }
            int nth = 1;
            if (c.thread->emitted)
                for (auto& e : w.sift.compounds) if (e.id < c.thread->emitted && e.kind == c.pattern->emit_kind && e.subject == c.protagonist) ++nth;
            l.params["nth"] = std::to_string(nth);
            for (auto& kv : c.thread->cast)
                if (find_faction(w, kv.second) && l.sources.size() < 2) l.sources.push_back("src:" + std::to_string(c.thread->id) + ":" + kv.second);
        } else {
            l.event = lead(c);
        }
        l.because = because_chain(w, p, l.event, k.because_hops);
        // IPOCL: a line about a person needs a motive among its causes
        if (const Event* e = sift_event(w, l.event))
            if (e->kind.rfind("person_", 0) == 0) {
                bool motive = false;
                for (int id : l.because) if (const Event* x = sift_event(w, id)) if (k.motivating.count(x->kind)) motive = true;
                if (!motive) l.because.clear();
            }
        return l;
    }

    // ages: cut at a new hegemon or a rise of the valley's era, named after it; at most max_ages
    void ages() {
        struct Age { int from; std::string key, who; };
        std::vector<Age> list;
        int era = 0;
        for (auto& e : w.chronicle) {
            if (e.kind == "era_advance" && e.id > 0 && e.id < first_id) era = std::max(era, static_cast<int>(e.magnitude));
            if (e.id < first_id || e.id <= 0) continue;
            if (e.kind == "hegemony") list.push_back({e.year, "pat.age.hegemony", e.subject});
            else if (e.kind == "era_advance" && static_cast<int>(e.magnitude) > era) {
                era = static_cast<int>(e.magnitude);
                list.push_back({e.year, "pat.age.era", std::to_string(era)});
            }
        }
        if (list.empty()) return;
        while (static_cast<int>(list.size()) > k.max_ages) {   // drop the shortest
            size_t worst = 0;
            int len = 1 << 30;
            for (size_t i = 0; i < list.size(); ++i) {
                int end = i + 1 < list.size() ? list[i + 1].from : w.year;
                if (end - list[i].from < len) { len = end - list[i].from; worst = i; }
            }
            list.erase(list.begin() + static_cast<std::ptrdiff_t>(worst));
        }
        for (size_t i = 0; i < list.size(); ++i) {
            ThreadLine l;
            l.kind = "age";
            l.text_key = list[i].key;
            l.params["from"] = std::to_string(list[i].from);
            l.params["to"] = std::to_string(i + 1 < list.size() ? list[i + 1].from : w.year);
            l.params[list[i].key == "pat.age.era" ? "era" : "role:court"] = list[i].who;
            out.push_back(l);
        }
    }

    std::vector<ThreadLine> pick(int budget) {
        mark_start();
        threads();
        raw();
        std::vector<bool> used(cands.size(), false);
        // 1. headline
        std::string last = w.sift.headlines.empty() ? "" : w.sift.headlines.back();
        int top_tier = 3;
        for (auto& c : cands) if (c.tier >= 3 && !(c.pattern && c.pattern->id == last)) top_tier = std::max(top_tier, c.tier);
        std::vector<size_t> pool;
        for (size_t i = 0; i < cands.size(); ++i)
            if (cands[i].tier == top_tier && !(cands[i].pattern && cands[i].pattern->id == last)) pool.push_back(i);
        std::sort(pool.begin(), pool.end(), [&](size_t a, size_t b) {
            double x = cands[a].score * fit(cands[a]), y = cands[b].score * fit(cands[b]);
            return x != y ? x > y : a < b;
        });
        if (pool.size() > static_cast<size_t>(k.headline_pool)) pool.resize(static_cast<size_t>(k.headline_pool));
        if (!pool.empty()) {
            double total = 0;
            for (size_t i : pool) total += std::max(1e-9, cands[i].score * fit(cands[i]));
            Rng r = rng_for(w.seed, "sift_headline", std::to_string(waking) + ":" + std::to_string(p.sift.hash));
            double roll = r.uniform() * total;
            size_t pickd = pool.back();
            for (size_t i : pool) {
                double v = std::max(1e-9, cands[i].score * fit(cands[i]));
                if (roll < v) { pickd = i; break; }
                roll -= v;
            }
            used[pickd] = true;
            out.push_back(line_of(cands[pickd], "headline"));
        }
        // 2. tier 5, outside the budget
        for (size_t i = 0; i < cands.size(); ++i)
            if (!used[i] && cands[i].tier >= 5) { used[i] = true; out.push_back(line_of(cands[i], "line")); }
        int left = budget;
        // 3. ages for long sleeps
        if (years >= 300) {
            size_t before = out.size();
            ages();
            left -= static_cast<int>(out.size() - before);
        }
        std::map<std::string, int> per_court, per_pattern;
        std::set<int> printed;   // events a picked line already tells
        for (auto& l : out) {
            printed.insert(l.event);
            for (size_t i = 0; i < cands.size(); ++i) if (used[i]) printed.insert(cands[i].fresh.begin(), cands[i].fresh.end());
        }
        auto repeats = [&](const Candidate& c) {
            for (int id : c.fresh) if (printed.count(id)) return true;
            return false;
        };
        auto take = [&](size_t i, const std::string& kind) {
            used[i] = true;
            ThreadLine l = line_of(cands[i], kind);
            out.push_back(l);
            per_court[cands[i].court] += 1;
            if (cands[i].pattern) per_pattern[cands[i].pattern->id] += 1;
            left -= 1;
            printed.insert(cands[i].fresh.begin(), cands[i].fresh.end());
            printed.insert(l.event);
            // under 30 years a thread still under way may take a second line, event by event
            if (years < 30 && cands[i].thread && cands[i].thread->state == "open" && cands[i].kind == "line" && cands[i].fresh.size() >= 2 && left > 0 &&
                !printed.count(cands[i].fresh[cands[i].fresh.size() - 2]) ) {
                ThreadLine more = l;
                more.kind = "line";
                more.event = cands[i].fresh[cands[i].fresh.size() - 2];
                more.text_key = sift_event(w, more.event) ? sift_event(w, more.event)->text_key : "";
                more.because = because_chain(w, p, more.event, k.because_hops);
                out.push_back(more);
                left -= 1;
            }
        };
        auto adjusted = [&](size_t i) {
            double s = cands[i].score - diversity() * per_court[cands[i].court];
            if (cands[i].pattern) s -= k.same_pattern_penalty * per_pattern[cands[i].pattern->id];
            return s;
        };
        auto best_of = [&](auto pred) {
            size_t best = cands.size();
            for (size_t i = 0; i < cands.size(); ++i)
                if (!used[i] && pred(cands[i]) && !(cands[i].kind == "line" && repeats(cands[i])) && (best == cands.size() || adjusted(i) > adjusted(best))) best = i;
            return best;
        };
        // 4. tier 4 always gets a line
        for (size_t i; left > 0 && (i = best_of([](const Candidate& c) { return c.tier == 4; })) < cands.size();) take(i, "line");
        // 5. callbacks, the ends of shown threads first
        int callbacks = 0;
        for (size_t i; left > 0 && callbacks < k.max_callbacks &&
                       (i = best_of([](const Candidate& c) { return c.kind == "callback" && (c.thread->state == "closed" || c.thread->state == "cut"); })) < cands.size();) {
            take(i, "callback");
            ++callbacks;
        }
        for (size_t i; left > 0 && callbacks < k.max_callbacks &&
                       (i = best_of([&](const Candidate& c) { return c.kind == "callback" && adjusted(static_cast<size_t>(&c - cands.data())) >= k.floor; })) < cands.size();) {
            take(i, "callback");
            ++callbacks;
        }
        // 6. greedy fill above the floor
        for (size_t i; left > 0 && (i = best_of([&](const Candidate& c) { return c.kind == "line" && c.tier >= 2 && adjusted(static_cast<size_t>(&c - cands.data())) >= k.floor; })) < cands.size();)
            take(i, "line");
        // 7. at least min_lines, from the raw events below the floor
        int shown = 0;
        for (auto& l : out) if (l.kind != "headline" && l.tier < 5) ++shown;
        fallbacks = 0;
        for (size_t i; shown < k.min_lines && (i = best_of([](const Candidate& c) { return c.kind == "line" && c.tier >= 2; })) < cands.size(); ++shown, ++fallbacks)
            take(i, "line");
        std::stable_sort(out.begin() + (out.empty() || out[0].kind != "headline" ? 0 : 1), out.end(), [&](const ThreadLine& a, const ThreadLine& b) {
            auto rank = [](const ThreadLine& l) { return l.tier >= 5 && l.kind != "age" ? 0 : (l.kind == "age" ? 1 : 2); };
            if (rank(a) != rank(b)) return rank(a) < rank(b);
            const Event* x = sift_event(w, a.event);
            const Event* y = sift_event(w, b.event);
            int ya = x ? x->year : std::atoi(a.params.count("from") ? a.params.at("from").c_str() : "0");
            int yb = y ? y->year : std::atoi(b.params.count("from") ? b.params.at("from").c_str() : "0");
            return ya < yb;
        });
        return out;
    }

    int fallbacks = 0;
};

}

std::vector<ThreadLine> briefing_lines(const World& w, const Params& p, int since_year, int budget_override) {
    if (p.sift.patterns.empty() && !w.sift.on) return {};
    Picker pk(w, p, since_year);
    return pk.pick(budget_override >= 0 ? budget_override : line_budget(pk.years));
}

void sift_wake(World& w, const Params& p, int since_year) {
    if (p.sift.patterns.empty()) return;
    sift_events(w, p);
    Picker pk(w, p, since_year);
    int budget = line_budget(pk.years);
    std::vector<ThreadLine> lines = pk.pick(budget);
    SiftDiag d;
    d.candidates = static_cast<int>(pk.cands.size());
    d.fallbacks = pk.fallbacks;
    d.budget = budget;
    double sum = 0;
    for (auto& c : pk.cands) { sum += c.score; if (c.tier >= 5) d.tier5 += 1; }
    d.mean_candidate = pk.cands.empty() ? 0 : sum / static_cast<double>(pk.cands.size());
    double shown_sum = 0;
    int shown_n = 0;
    std::set<int> picked;
    for (auto& l : lines) {
        if (l.kind == "age") continue;
        shown_sum += l.score;
        ++shown_n;
        if (l.tier >= 5) d.tier5_shown += 1;
        if (l.thread) picked.insert(l.thread);
    }
    d.mean_shown = shown_n ? shown_sum / shown_n : 0;
    if (pk.cands.size() >= 2) {
        size_t top = 0;
        for (size_t i = 1; i < pk.cands.size(); ++i) if (pk.cands[i].score > pk.cands[top].score) top = i;
        Rng r = rng_for(w.seed, "sift_pair", std::to_string(pk.waking));
        size_t other = static_cast<size_t>(r.range(0, static_cast<int>(pk.cands.size()) - 1));
        if (other >= top) ++other;
        d.pair = {pk.line_of(pk.cands[top], "line"), pk.line_of(pk.cands[other], "line")};
    }
    w.sift.waking += 1;
    std::string head;
    for (auto& l : lines) {
        if (l.kind != "headline") continue;
        for (auto& c : pk.cands) {
            if (!((c.thread && c.thread->id == l.thread && l.thread) || (!c.thread && !l.thread && std::find(c.events.begin(), c.events.end(), l.event) != c.events.end()))) continue;
            if (c.pattern) head = c.pattern->id;
            d.headline_dragon = c.dragon;
            d.headline_reversals = c.reversals;
            break;
        }
    }
    d.headline_pattern = head;
    for (auto& l : lines) {
        std::string who;
        for (auto& c : pk.cands) if (c.thread && c.thread->id == l.thread && l.thread) who = c.protagonist;
        if (!l.thread) if (const Event* e = sift_event(w, l.event)) who = e->subject;
        if (!who.empty() && w.sift.named.count(who)) d.names_earlier = true;
    }
    for (auto& l : lines) {
        if (!l.thread) { if (const Event* e = sift_event(w, l.event)) if (!e->subject.empty()) w.sift.named.insert(e->subject); continue; }
        for (auto& t : w.sift.threads)
            if (t.id == l.thread && (t.shown.empty() || t.shown.back().waking != w.sift.waking + 0 || t.shown.back().year != w.year)) {
                t.shown.push_back({w.sift.waking, static_cast<int>(t.events.size()), w.year, t.state});
                if (const SiftPattern* pat = find_pattern(p, t.pattern)) { std::string who = protagonist(*pat, t); if (!who.empty()) w.sift.named.insert(who); }
            }
    }
    w.sift.headlines.push_back(head);
    // ended threads leave once their end was told or when they ended unseen
    w.sift.threads.erase(std::remove_if(w.sift.threads.begin(), w.sift.threads.end(), [](const SiftThread& t) {
                             if (t.state != "closed" && t.state != "cut") return false;
                             return t.shown.empty() || t.shown.back().state == t.state;
                         }),
                         w.sift.threads.end());
    w.sift.lines = lines;
    w.sift.lines_since = since_year;
    w.sift.lines_year = w.year;
    w.sift.diag = d;
}

}
