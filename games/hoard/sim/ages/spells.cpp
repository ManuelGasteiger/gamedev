#include <cmath>
#include "memory_tiers.h"
#include "unrecorded.h"
#include <cstdio>
#include "ages.h"
#include "rules/rules.h"
#include "talents.h"
#include "world_util.h"

// section 8 verbs that resolve to a modifier or a spawn: ward, curse, bless, incite, raise_undead (corrupt stays disabled),
// plus terraform. Gates and costs come from constants.json ("spells" and "terraform").

namespace ages {

static std::string num(double v) { char b[32]; std::snprintf(b, sizeof b, "%g", v); return b; }

static double& axis(Dragon& d, const std::string& gate) {
    if (gate == "knowledge") return d.knowledge;
    if (gate == "faith_pool") return d.faith_pool;
    return d.magic;
}

static bool gate_open(const Dragon& d, const SpellDef& s, std::string& reason) {
    double need = std::max(s.gate_min, s.cost);
    if (s.gate == "knowledge_or_magic") {
        if (d.knowledge >= need || d.magic >= need) return true;
        reason = "needs_knowledge_or_magic|n=" + num(need);
        return false;
    }
    Dragon copy = d;
    if (axis(copy, s.gate) >= need) return true;
    reason = "needs_axis|axis=" + s.gate + "|n=" + num(need);
    return false;
}

static void pay(Dragon& d, const SpellDef& s) {
    if (s.cost <= 0) return;
    if (s.gate == "knowledge_or_magic") {
        double& a = d.magic >= s.cost ? d.magic : d.knowledge;
        a = std::max(0.0, a - s.cost);
        return;
    }
    double& a = axis(d, s.gate);
    a = std::max(0.0, a - s.cost);
}

static Modifier make_modifier(World& w, const std::string& source, const std::string& target, const ModifierDef& m) {
    add_modifier(w, source, target, m.var, m.op, m.value, m.decades);
    return w.modifiers.back();
}

static void start_war_between(World& w, Faction& a, Faction& b, const std::string& reason) {
    if (!at_war(a, b.id)) a.at_war_with.push_back(b.id);
    if (!at_war(b, a.id)) b.at_war_with.push_back(a.id);
    War war;
    war.a = a.id;
    war.b = b.id;
    war.started = w.year;
    w.wars.push_back(war);
    emit(w, "war_started", a.id, b.id, 0, {reason});
}

static CastResult fail(CastResult r, World& w, const std::string& reason) {
    r.cast = false;
    r.reason = reason;
    emit(w, "spell_failed", "dragon", r.spell, 0, {reason_code(reason)});
    return r;
}

static bool undead_site(const Params& p, const Region& r, const SpellDef& s) {
    if (!r.owner.empty() || r.lair) return false;
    if (has_tag(r.tags, "wasteland") || r.burn >= s.site_min_burn) return true;
    auto rt = p.races.find(s.spawn_race);
    return rt != p.races.end() && home_terrain(rt->second.culture, r);
}

static CastResult cast_modifier(World& w, const SpellDef& s, CastResult r) {
    std::string target = s.target == "region" ? r.region : r.target;
    if (s.target == "region" && !find_region(w, target)) return fail(r, w, "no_region");
    if (s.target == "faction" && !find_faction(w, target)) return fail(r, w, "no_faction");
    pay(w.dragons[kPlayer], s);
    r.modifier = make_modifier(w, "dragon." + s.id, target, s.modifier);
    r.has_modifier = true;
    r.cast = true;
    emit(w, "spell_cast", s.id, target, s.cost, {s.modifier.var});
    return r;
}

static CastResult cast_incite(World& w, const Params& p, const SpellDef& s, CastResult r) {
    Faction* orc = find_faction(w, r.target);
    if (!orc || dominant_race(*orc) != "orc") return fail(r, w, "not_orc");
    std::string victim = r.against;
    if (victim.empty() && !r.region.empty())
        if (const Region* reg = find_region(w, r.region)) victim = reg->owner;
    Faction* v = find_faction(w, victim);
    if (!v || v->id == orc->id) return fail(r, w, "no_victim");
    if (!adjacent(w, *orc, *v)) return fail(r, w, "not_adjacent");
    if (at_war(*orc, v->id)) return fail(r, w, "already_at_war");
    pay(w.dragons[kPlayer], s);
    start_war_between(w, *orc, *v, "incited");
    if (s.modifier.present) { r.modifier = make_modifier(w, "dragon.incite", orc->id, s.modifier); r.has_modifier = true; }
    r.war_with = v->id;
    r.cast = true;
    emit(w, "spell_cast", s.id, orc->id, 0, {v->id});
    (void)p;
    return r;
}

static CastResult cast_raise_undead(World& w, const Params& p, const SpellDef& s, CastResult r) {
    Region* site = find_region(w, r.region);
    if (!site) return fail(r, w, "no_region");
    if (!undead_site(p, *site, s)) return fail(r, w, "bad_site");
    if (!p.races.count(s.spawn_race)) return fail(r, w, "no_race|race=" + s.spawn_race);
    std::string victim = r.against.empty() ? r.target : r.against;
    if (victim.empty()) {
        double best = -1;
        for (auto& n : site->neighbours)
            if (const Region* nr = find_region(w, n))
                if (const Faction* o = find_faction(w, nr->owner))
                    if (power(*o, p.c) > best) { best = power(*o, p.c); victim = o->id; }
    }
    double wealth = s.spawn_wealth_per_magic * w.dragons[kPlayer].magic;
    pay(w.dragons[kPlayer], s);
    Faction host = found_faction(w, p, s.spawn_race, site->id, wealth, s.spawn_cohesion, s.spawn_era);
    host.tags.insert("undead");
    host.tags.insert("raised_by_dragon");
    site->owner = host.id;
    w.factions.push_back(host);
    sort_by_id(w);
    Faction* h = find_faction(w, host.id);
    emit(w, "undead_raised", h->id, site->id, wealth);
    if (Faction* v = find_faction(w, victim)) {
        if (v->id != h->id) { start_war_between(w, *h, *v, "raised_against"); r.war_with = v->id; }
    }
    r.spawned_faction = h->id;
    r.cast = true;
    emit(w, "spell_cast", s.id, h->id, s.cost, {site->id});
    return r;
}

// v0.4 talents: cheaper spells and undead, longer wards and teaching
static SpellDef with_talents(const World& w, const Params& p, SpellDef s) {
    s.cost *= effect(w, p, "spell_cost_mult");
    if (s.id == "raise_undead") s.cost *= effect(w, p, "undead_cost_mult");
    double longer = s.id == "ward" ? effect(w, p, "ward_duration_mult") : s.id == "teach" ? effect(w, p, "teach_mult") : 1;
    s.modifier.decades = static_cast<int>(std::lround(s.modifier.decades * longer));
    return s;
}

CastResult cast_spell_unrecorded(World& w, const Params& p, const std::string& spell, const std::string& target,
                      const std::string& region, const std::string& against) {
    CastResult r;
    r.spell = spell;
    r.target = target;
    r.region = region;
    r.against = against;
    auto it = p.spells.find(spell);
    if (it == p.spells.end()) return fail(r, w, "unknown_spell");
    SpellDef s = with_talents(w, p, it->second);
    if (!s.enabled) return fail(r, w, "disabled");
    std::string why;
    if (!gate_open(w.dragons[kPlayer], s, why)) return fail(r, w, why);
    if (spell == "incite") return cast_incite(w, p, s, r);
    if (spell == "raise_undead") return cast_raise_undead(w, p, s, r);
    if (!s.modifier.present) return fail(r, w, "no_effect");
    r = cast_modifier(w, s, r);
    if (r.cast && spell == "curse" && effect(w, p, "hex") > 0)
        if (Faction* f = find_faction(w, r.target)) f->anti_dragon *= p.c.hex_anti_dragon_mult;
    return r;
}

TerraformResult terraform_unrecorded(World& w, const Params& p, const std::string& region, const std::string& kind) {
    TerraformResult r;
    const TerraformDef& t = p.terraform;
    auto k = t.kinds.find(kind);
    auto done = [&](const std::string& reason) { r.reason = reason; emit(w, "spell_failed", "dragon", "terraform", 0, {reason_code(reason)}); return r; };
    if (k == t.kinds.end()) return done("unknown_terraform");
    Region* reg = find_region(w, region);
    if (!reg) return done("no_region");
    if (reg->lair) return done("lair_region");
    if (w.dragons[kPlayer].size < t.gate_size) return done("needs_size|n=" + num(t.gate_size));
    if (has_tag(reg->tags, k->second.tag)) return done("already_tag|tag=" + k->second.tag);
    r.cost = w.dragons[kPlayer].size * t.cost_size_fraction;
    w.dragons[kPlayer].size -= r.cost;
    reg->tags.insert(k->second.tag);
    if (k->second.tag == "wasteland") {
        reg->hot_decades = 0;
        reg->cold_decades = 0;
        for (auto& f : w.factions) raze_monuments(f, reg->id, "");
    }
    r.tag = k->second.tag;
    if (k->second.modifier.present) {
        r.modifier = make_modifier(w, "dragon.terraform." + kind, reg->id, k->second.modifier);
        r.has_modifier = true;
    }
    r.done = true;
    emit(w, "terraformed", reg->id, kind, r.cost, {k->second.tag});
    return r;
}

}
