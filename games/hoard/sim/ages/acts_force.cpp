#include "act_kinds.h"
#include "memory_tiers.h"
#include "defences.h"
#include "resolve_fight.h"
#include "world_util.h"

// item 09: burn with a structure, crush, destroy_altar, take of a named item, fight

namespace ages {

namespace {

const ActNumbers& nums(const Params& p) { return p.act_book.n; }

std::vector<Region*> burned_regions(World& w, const Faction& f, const Act& a) {
    std::vector<Region*> out;
    if (Region* r = find_region(w, a.region)) out.push_back(r);
    else for (auto& id : f.regions) if (Region* r = find_region(w, id)) out.push_back(r);
    return out;
}

void drop_tag_prefix(Tags& t, const std::string& prefix) {
    for (auto it = t.begin(); it != t.end();) it = it->rfind(prefix, 0) == 0 ? t.erase(it) : std::next(it);
}

void altar_down(World& w, Faction& f, const std::string& region) {
    drop_tag_prefix(f.tags, "cult");
    raze_monuments(f, region, "");
    if (Region* r = find_region(w, region)) { r->tags.erase("altar"); r->tags.erase("dragon_temple"); }
}

// world-aging section 3: when and how hard the place last burned; a place burned through is a ruin with a year
void age_burned(World& w, const Params& p, const Faction& f, const Act& a, double m) {
    std::vector<Region*> rs = burned_regions(w, f, a);
    double share = rs.size() > 1 ? m / static_cast<double>(rs.size()) : m;
    for (Region* r : rs) {
        r->last_burn_year = w.year;
        r->destroyed_share_at_last_burn = share;
        if (r->burn >= nums(p).ruin_burn && r->ruin_year < 0) {
            r->tags.insert("ruin");
            r->ruin_year = w.year;
        }
    }
}

void burn_structure(World& w, const Params& p, Faction& f, const Act& a) {
    const ActNumbers& n = nums(p);
    const std::string& s = a.item;
    std::vector<Region*> rs = burned_regions(w, f, a);
    if (s == "library") {
        for (auto& g : w.factions)
            if (g.id != f.id && adjacent(w, f, g)) add_modifier(w, "dragon.burn_library", g.id, "era_rate", "mul", n.library_era_rate, n.library_decades);
    } else if (s == "temple") {
        f.faith = clamp01(f.faith + n.temple_faith);
        drop_tag_prefix(f.tags, "cult");
        for (auto& g : w.factions)
            if (g.id != f.id && adjacent(w, f, g) && g.faith > p.c.reaction_faith_high) g.tags.insert("crusade_eligible");
        for (Region* r : rs) r->tags.erase("temple");
    } else if (s == "altar") {
        altar_down(w, f, a.region);
    } else if (s == "foundry") {
        f.era_progress = std::max(0.0, f.era_progress + n.foundry_era_progress);
        f.anti_dragon = std::max(0.0, f.anti_dragon + n.foundry_anti_dragon);
        for (Region* r : rs) r->tags.erase("foundry");
    } else if (s == "walls") {
        for (Region* r : rs) r->tags.erase("walls");
    } else if (s == "granary") {
        f.wealth *= 1.0 - n.granary_wealth;
        for (Region* r : rs) r->tags.erase("granary");
    } else if (s == "tower") {
        for (Region* r : rs) r->tags.insert("defences_wrecked");   // rebuild_defences clears it when the dragon sleeps
    }
}

}

std::string force_refusal(const World& w, const Params& p, const Act& a) {
    (void)p;
    if (a.type == "fight") {
        const Person* x = find_person(w, a.item);
        if (!x) return "act.no_target";
        return x->slayer.rank.empty() ? "fight.not_slayer" : "";   // item 08 rosters the slayers resolve_fight takes
    }
    if (a.type == "crush" && !find_faction(w, a.target)) return "act.no_target";
    return "";
}

bool force_effects(World& w, const Params& p, Act& a, double m, int act_index) {
    (void)act_index;
    Faction* f = find_faction(w, a.target);
    if (a.type == "burn") {
        if (!f) return true;
        if (!a.item.empty()) burn_structure(w, p, *f, a);
        age_burned(w, p, *f, a, m);
        return true;
    }
    if (a.type == "destroy_altar") {
        if (f) altar_down(w, *f, a.region);
        return true;
    }
    if (a.type == "crush") {
        if (!f) return true;
        if (Region* r = find_region(w, a.region.empty() ? capital_region(w, *f) : a.region)) r->tags.insert("defences_wrecked");
        return true;
    }
    if (a.type == "take") {
        if (!a.item.empty()) w.dragons[kPlayer].hoard.artifacts += nums(p).take_item_artifacts;
        return true;
    }
    if (a.type == "fight") {   // item 08's duel; the family is that of the dragon's last act this waking that has one
        std::string family;
        for (size_t i = w.acts.size(); i > 0 && family.empty(); --i) {
            if (w.acts[i - 1].year < w.year) break;
            family = act_family(p, w.acts[i - 1].type);
        }
        FightResult r = resolve_fight(w, p, a.item, family);
        a.args["outcome"] = r.slayer_outcome;
        if (r.artifact) a.args["artifact"] = "1";
        return true;
    }
    return false;
}

}
