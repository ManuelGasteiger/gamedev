#include "genesis.h"
#include "world_util.h"

// the terrain deal (genesis.json "deal"): the deck holds the base cards, the template's minimum and wild draws up to one
// card per region. Cluster terrains are laid first as connected runs, the rest is shuffled onto the open regions, and a
// lair-neighbour rule is mended by swapping. Every region, the lair too, gets a name from its terrain's name parts.

namespace ages {

namespace {

struct Dealer {
    World& w;
    const DealDef& d;
    std::map<std::string, std::string>& deal;

    bool by_lair(const Region& r) const {
        for (auto& n : r.neighbours) if (const Region* nr = find_region(w, n)) if (nr->lair) return true;
        return false;
    }
    int at_lair(const std::string& terrain) const {
        int n = 0;
        for (auto& r : w.regions) {
            auto it = deal.find(r.id);
            if (it != deal.end() && it->second == terrain && by_lair(r)) n += 1;
        }
        return n;
    }
    bool open(const Region& r) const { return !r.lair && !deal.count(r.id); }
    bool fits(const Region& r, const std::string& terrain) const {
        auto cap = d.lair_max.find(terrain);
        return !(by_lair(r) && cap != d.lair_max.end() && at_lair(terrain) >= cap->second);
    }
    template <class T> const T& any_of(const std::vector<T>& v) { return v[static_cast<size_t>(w.rng.range(0, static_cast<int>(v.size())))]; }

    // a connected run of n regions; a start that cannot grow to n is undone and another tried
    bool run(const std::string& terrain, int n) {
        for (int t = 0; t < d.tries; ++t) {
            std::vector<std::string> starts;
            for (auto& r : w.regions) if (open(r) && fits(r, terrain)) starts.push_back(r.id);
            if (starts.empty()) return false;
            std::vector<std::string> placed = {any_of(starts)};
            deal[placed[0]] = terrain;
            while (static_cast<int>(placed.size()) < n) {
                std::vector<std::string> next;
                for (auto& r : w.regions) {
                    if (!open(r) || !fits(r, terrain)) continue;
                    for (auto& nb : r.neighbours) if (contains(placed, nb)) { next.push_back(r.id); break; }
                }
                if (next.empty()) break;
                placed.push_back(any_of(next));
                deal[placed.back()] = terrain;
            }
            if (static_cast<int>(placed.size()) >= n) return true;
            for (auto& id : placed) deal.erase(id);
        }
        return false;
    }

    // a terrain the lair needs more of comes in by swapping with a lair neighbour holding a non-cluster terrain
    void mend_lair(const std::string& terrain, int need) {
        for (int guard = 0; guard < 12 && at_lair(terrain) < need; ++guard) {
            std::vector<std::string> near, far;
            for (auto& r : w.regions) {
                if (r.lair) continue;
                const std::string& t = deal[r.id];
                if (contains(d.cluster, t) || t == terrain) { if (!by_lair(r) && t == terrain) far.push_back(r.id); continue; }
                if (by_lair(r)) near.push_back(r.id);
            }
            if (near.empty() || far.empty()) return;
            std::string a = any_of(near), b = any_of(far);
            std::swap(deal[a], deal[b]);
        }
    }
};

std::vector<std::string> deck(Rng& rng, const DealDef& d, const StartTemplate& t, size_t regions) {
    std::map<std::string, int> count = d.base;
    for (auto& kv : t.deal) count[kv.first] = std::max(count[kv.first], kv.second);
    auto total = [&]() { int n = 0; for (auto& kv : count) n += kv.second; return static_cast<size_t>(n); };
    while (total() > regions) {
        std::string most;
        for (auto& kv : count) if (!t.deal.count(kv.first) && kv.second > 0 && (most.empty() || kv.second > count[most])) most = kv.first;
        if (most.empty()) break;
        count[most] -= 1;
    }
    while (total() < regions) {
        double sum = 0;
        for (auto& kv : d.wild) {
            auto cap = d.max.find(kv.first);
            if (cap == d.max.end() || count[kv.first] < cap->second) sum += kv.second;
        }
        if (sum <= 0) { count["vale"] += 1; continue; }
        double roll = rng.uniform() * sum;
        for (auto& kv : d.wild) {
            auto cap = d.max.find(kv.first);
            if (cap != d.max.end() && count[kv.first] >= cap->second) continue;
            if (roll < kv.second) { count[kv.first] += 1; break; }
            roll -= kv.second;
        }
    }
    std::vector<std::string> cards;
    for (auto& kv : count) for (int i = 0; i < kv.second; ++i) cards.push_back(kv.first);
    return cards;
}

double jitter(Rng& rng, double base, double amount) {
    double v = base + (rng.uniform() * 2 - 1) * amount;
    return v < 0.05 ? 0.05 : (v > 1 ? 1 : v);
}

}

std::string region_name(World& w, const GenesisDef& g, const std::string& terrain) {
    auto t = g.terrain.find(terrain);
    if (t == g.terrain.end() || t->second.name_a.empty() || t->second.name_b.empty()) return "";
    std::string name;
    for (int tries = 0; tries < 12; ++tries) {
        size_t a = static_cast<size_t>(w.rng.range(0, static_cast<int>(t->second.name_a.size())));
        size_t b = static_cast<size_t>(w.rng.range(0, static_cast<int>(t->second.name_b.size())));
        name = t->second.name_a[a] + t->second.name_b[b];
        bool taken = false;
        for (auto& r : w.regions) if (r.name == name) taken = true;
        if (!taken) break;
    }
    return name;
}

void deal_map(World& w, const Params& p, const StartTemplate& t) {
    const GenesisDef& g = p.genesis;
    std::map<std::string, std::string>& deal = w.genesis.deal;
    deal.clear();
    size_t regions = 0;
    for (auto& r : w.regions) if (!r.lair) regions += 1;
    std::vector<std::string> cards = deck(w.rng, g.deal, t, regions);
    Dealer d{w, g.deal, deal};
    for (auto& terrain : g.deal.cluster) {
        int n = static_cast<int>(std::count(cards.begin(), cards.end(), terrain));
        if (n < 2 || !d.run(terrain, n)) continue;
        cards.erase(std::remove(cards.begin(), cards.end(), terrain), cards.end());
    }
    for (size_t i = cards.size(); i > 1; --i) std::swap(cards[i - 1], cards[static_cast<size_t>(w.rng.range(0, static_cast<int>(i)))]);
    std::vector<std::string> rest;
    for (auto& r : w.regions) if (d.open(r)) rest.push_back(r.id);
    for (auto& id : rest) {
        const Region& r = *find_region(w, id);
        size_t pick = 0;
        while (pick + 1 < cards.size() && !d.fits(r, cards[pick])) ++pick;
        deal[id] = cards.empty() ? "vale" : cards[pick];
        if (!cards.empty()) cards.erase(cards.begin() + static_cast<std::ptrdiff_t>(pick));
    }
    for (auto& need : g.deal.lair_min) d.mend_lair(need.first, need.second);
    for (auto& r : w.regions) {
        r.terrain = r.lair ? "lair" : deal[r.id];
        auto td = g.terrain.find(r.terrain);
        if (td != g.terrain.end()) r.land = jitter(w.rng, td->second.land, td->second.jitter);
        r.eff_land = r.land;
    }
    for (auto& r : w.regions) r.name = region_name(w, g, r.terrain);
    deal["lair"] = "lair";
}

}
