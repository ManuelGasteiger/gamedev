#include "memory_tiers.h"
#include <algorithm>
#include <cmath>
#include <set>
#include "core/detmath.h"
#include "talents.h"
#include "world_util.h"

namespace ages {

namespace {

double& axis_of(Tiers& t, const char* tier) {
    if (tier[0] == 'w') return t.written;
    if (tier[0] == 'm') return t.monumental;
    return t.oral;
}

void add_stock(double& x, double d) { x = clamp01(x + d); }

Monument* monument_for(Faction& f, const std::string& region) {
    if (f.monuments.empty()) return nullptr;
    for (auto& m : f.monuments) if (m.region == region) return &m;
    return &f.monuments.front();
}

void record(Faction& f, const Deposit& d, const char* axis, const char* tier, double strength, int year) {
    if (strength == 0) return;
    MemoryEntry e;
    e.year = year;
    e.type = d.type;
    e.magnitude = d.magnitude;
    e.axis = axis;
    e.tier = tier;
    e.strength = strength;
    e.act = d.act;
    e.tags = d.tags;
    f.memory.push_back(e);
}

double halflife_of(const World& w, const Params& p, const Faction& f, const std::string& axis, const std::string& tier) {
    const MemoryConstants& m = p.c.mem;
    if (tier == "written") return m.written_halflife;
    if (tier == "monumental") return m.monumental_halflife;
    double race = f.eff.memory_halflife > 0 ? f.eff.memory_halflife : f.culture.memory_halflife;
    if (race <= 0) return 0;
    double hl = m.oral_halflife * race / m.oral_reference;
    if (axis == "dread") hl /= effect(w, p, "oral_dread_decay_mult");
    return hl;
}

double decay_factor(const World& w, double hl) { return hl > 0 ? det::half_life(w.dt, hl) : 1.0; }

}

double tiers_eff(const Tiers& t) { return clamp01(1.0 - (1.0 - t.oral) * (1.0 - t.written) * (1.0 - t.monumental)); }

std::string memory_tier(const Faction& f, const MemoryConstants& m) {
    auto at = [&](double a, double b) { return a >= m.tier_shown || b >= m.tier_shown; };
    if (at(f.dread.monumental, f.awe.monumental)) return "monumental";
    if (at(f.dread.written, f.awe.written)) return "written";
    if (at(f.dread.oral, f.awe.oral)) return "oral";
    return "";
}

bool keeps_records(const World& w, const Faction& f) {
    for (auto& x : w.people) if (x.faction == f.id && (x.role == "sage" || x.role == "historian")) return true;
    for (auto& rid : f.regions)
        if (const Region* r = find_region(w, rid)) if (has_tag(r->tags, "library")) return true;
    return false;
}

void refresh_monumental(Faction& f) {
    double d = 0, a = 0;
    for (auto& m : f.monuments) { d += m.dread; a += m.awe; }
    f.dread.monumental = clamp01(d);
    f.awe.monumental = clamp01(a);
}

void deposit(World& w, const Params& p, Faction& f, const Deposit& d) {
    const MemoryConstants& m = p.c.mem;
    double dd = d.dread * (1.0 - dread_eff(f));
    double da = d.awe * (1.0 - awe_eff(f));
    if (dd == 0 && da == 0) return;
    const char* top = "oral";
    add_stock(f.dread.oral, dd);
    add_stock(f.awe.oral, da);
    if (keeps_records(w, f)) {
        double k = d.tier_up ? 1.0 : m.written_deposit;
        add_stock(f.dread.written, k * dd);
        add_stock(f.awe.written, k * da);
        top = "written";
    }
    if (Monument* mon = monument_for(f, d.region)) {
        double k = d.tier_up ? m.written_deposit : m.monumental_deposit;
        double md = k * dd, ma = k * da;
        if (mon->bias == "dragon_temple" && md > 0) { ma += m.temple_convert * md; md *= 1.0 - m.temple_convert; }
        if (mon->bias == "memorial" && ma > 0) { md += m.temple_convert * ma; ma *= 1.0 - m.temple_convert; }
        add_stock(mon->dread, md);
        add_stock(mon->awe, ma);
        refresh_monumental(f);
        top = "monumental";
    }
    record(f, d, "dread", top, dd, w.year);
    record(f, d, "awe", top, da, w.year);
}

void set_eff(Tiers& t, double want) {
    double rest = (1.0 - t.written) * (1.0 - t.monumental);
    t.oral = rest > 0 ? clamp01(1.0 - (1.0 - clamp01(want)) / rest) : 0;
}

int recent_acts(const Faction& f, const std::string& type, int year, double years) {
    std::set<int> seen;
    for (auto& e : f.memory)
        if (e.act >= 0 && e.type == type && e.axis != "faith" && year - e.year < years) seen.insert(e.act);
    return static_cast<int>(seen.size());
}

void decay_tiers(World& w, const Params& p, Faction& f) {
    const MemoryConstants& m = p.c.mem;
    double oral_d = decay_factor(w, halflife_of(w, p, f, "dread", "oral"));
    double oral_a = decay_factor(w, halflife_of(w, p, f, "awe", "oral"));
    double written = decay_factor(w, m.written_halflife), mon = decay_factor(w, m.monumental_halflife);
    f.dread.oral *= oral_d;
    f.awe.oral *= oral_a;
    f.dread.written *= written;
    f.awe.written *= written;
    for (auto& x : f.monuments) { x.dread *= mon; x.awe *= mon; }
    refresh_monumental(f);
    for (auto& e : f.memory) {
        if (e.axis == "faith") continue;
        if (e.tier == "written") e.weight *= written;
        else if (e.tier == "monumental") e.weight *= mon;
        else e.weight *= e.axis == "dread" ? oral_d : oral_a;
    }
    f.memory.erase(std::remove_if(f.memory.begin(), f.memory.end(),
                                  [&](const MemoryEntry& e) { return e.axis != "faith" && std::fabs(e.strength * e.weight) < m.entry_drop; }),
                   f.memory.end());
}

void burn_library(Faction& f, const MemoryConstants& m) {
    f.dread.written *= m.library_burn;
    f.awe.written *= m.library_burn;
    for (auto& e : f.memory) if (e.tier == "written") e.weight *= m.library_burn;
}

void raze_monuments(Faction& f, const std::string& region, const std::string& bias_any) {
    f.monuments.erase(std::remove_if(f.monuments.begin(), f.monuments.end(),
                                     [&](const Monument& x) { return x.region == region && (bias_any.empty() || x.bias == bias_any); }),
                      f.monuments.end());
    refresh_monumental(f);
}

double oath_stability(const Faction& f, const MemoryConstants& m) { return 1.0 - m.stability_dread * dread_eff(f) * (1.0 - awe_eff(f)); }
double slayer_pressure(const Faction& f, const MemoryConstants& m) { return dread_eff(f) * (1.0 - m.pressure_awe * awe_eff(f)); }

}
