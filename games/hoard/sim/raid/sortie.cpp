#include <cmath>
#include "report.h"
#include "ages/unrecorded.h"
#include "ages/world_util.h"
#include "ages/wounds.h"

// item 15: begin_sortie takes the snapshot, end_sortie diffs the report against it and applies the acts
namespace ages {

namespace {

double region_yield(const Region& r) { return r.eff_land * (1.0 - r.burn); }

const char* const kLoot[] = {"gold", "books", "artifacts", "art", "food"};

std::string army_key(const SortieArmy& a) { return a.attacker + "|" + a.defender + "|" + a.region; }

void snapshot_armies(const World& w, const Params& p, SortieState& s) {
    auto members = [](const War& x, bool a) { std::vector<std::string> m = a ? x.allies_a : x.allies_b; m.insert(m.begin(), a ? x.a : x.b); return m; };
    for (auto& x : w.wars)
        for (int side = 0; side < 2; ++side)
            for (auto& att : members(x, side == 0)) {
                const Faction* fa = find_faction(w, att);
                if (!fa) continue;
                for (auto& def : members(x, side != 0)) {
                    const Faction* fd = find_faction(w, def);
                    if (!fd) continue;
                    std::string region;
                    for (auto& rid : fd->regions) {
                        const Region* r = find_region(w, rid);
                        if (!r) continue;
                        for (auto& n : r->neighbours) if (contains(fa->regions, n)) { region = rid; break; }
                        if (!region.empty()) break;
                    }
                    if (region.empty()) continue;
                    SortieArmy a{att, def, region, power(*fa, p.c) * p.raid_diff.army_per_power};
                    bool seen = false;
                    for (auto& o : s.armies) if (army_key(o) == army_key(a)) seen = true;
                    if (!seen) s.armies.push_back(a);
                }
            }
}

}

SortieState take_snapshot(const World& w, const Params& p, int number) {
    const RaidDiffBook& b = p.raid_diff;
    SortieState s;
    s.open = true;
    s.number = number;
    s.year = w.year;
    for (auto& f : w.factions) {
        SortieCourt c;
        c.id = f.id;
        c.wealth = f.wealth;
        for (auto& kv : b.holdings) c.holds[kv.first] = f.wealth * kv.second;
        s.courts.push_back(c);
    }
    for (auto& r : w.regions) {
        const Faction* f = find_faction(w, r.owner);
        if (!f) continue;
        double total = 0;
        for (auto& rid : f->regions) if (const Region* rr = find_region(w, rid)) total += region_yield(*rr);
        double share = total > 0 ? region_yield(r) / total : 1.0 / static_cast<double>(std::max<size_t>(1, f->regions.size()));
        double part = f->wealth * share;
        double each = power(*f, p.c) / static_cast<double>(std::max<size_t>(1, f->regions.size()));
        SortieSettlement x;
        x.region = r.id;
        x.owner = f->id;
        x.value = part * b.value_per_wealth;
        x.population = part * b.population_per_wealth;
        x.garrison = each * b.garrison_per_power;
        x.soldiers = each * b.soldiers_per_power;
        for (auto& st : b.structures) {
            auto frac = b.structure_share.find(st);
            if (frac != b.structure_share.end() && has_tag(r.tags, st)) x.structures[st] = x.value * frac->second;
        }
        s.settlements.push_back(x);
    }
    snapshot_armies(w, p, s);
    return s;
}

bool begin_sortie_unrecorded(World& w, const Params& p, int number, std::string& reason) {
    if (dragon_dead(w)) { reason = "dragon_slain"; return false; }
    if (w.sortie.open) { reason = "sortie.already_open"; return false; }
    w.sortie = take_snapshot(w, p, number);
    return true;
}

SortieResult end_sortie_unrecorded(World& w, const Params& p, const RaidReport& rep) {
    SortieResult out;
    out.shell = rep.shell;
    if (dragon_dead(w)) { out.reason = "dragon_slain"; return out; }
    if (!w.sortie.open) { out.reason = "sortie.not_open"; return out; }
    DiffResult d = raid_diff(w, p, rep);
    out.warnings = d.warnings;
    if (rep.sortie != 0 && rep.sortie != w.sortie.number) out.warnings.push_back("raid.sortie_mismatch|n=" + std::to_string(w.sortie.number));
    out.derived = d.acts;
    Hoard before = w.dragons[kPlayer].hoard;
    out.first_act = w.acts.size();
    out.lines = apply_acts_unrecorded(w, p, d.acts, &out.refused);
    out.last_act = w.acts.size();
    if (rep.lair.has_dropped) {   // what reached the lair is the hoard's gain; the takes credited what the court lost
        Hoard& h = w.dragons[kPlayer].hoard;
        double* now[] = {&h.gold, &h.books, &h.artifacts, &h.art, &h.food};
        const double* was[] = {&before.gold, &before.books, &before.artifacts, &before.art, &before.food};
        for (size_t i = 0; i < 5; ++i) {
            auto it = rep.lair.dropped.find(kLoot[i]);
            if (it == rep.lair.dropped.end()) continue;
            double credited = *now[i] - *was[i];
            if (credited > it->second + 1e-9) out.warnings.push_back(std::string("raid.loot_lost|what=") + kLoot[i]);
            *now[i] += it->second - credited;
        }
    }
    w.sortie = SortieState();
    out.ok = true;
    return out;
}

}
