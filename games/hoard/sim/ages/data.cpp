#include "data.h"
#include "json.h"
#include "memory_data.h"
#include "world_util.h"

namespace ages {

using nlohmann::json;

static bool parse_json(const std::string& text, const char* name, json& out, std::string& err) {
    out = json::parse(text, nullptr, false);
    if (out.is_discarded()) { err = std::string("invalid json in ") + name; return false; }
    return true;
}

template <class T>
static void get(const json& j, const char* key, T& out) {
    auto it = j.find(key);
    if (it != j.end()) out = it->get<T>();
}

static void get_tags(const json& j, const char* key, Tags& out) {
    auto it = j.find(key);
    if (it == j.end()) return;
    for (auto& t : *it) out.insert(t.get<std::string>());
}

static void load_constants(const json& j, Constants& c) {
    if (j.contains("memory")) memory_constants_from_json(j["memory"], c.mem);
    get(j, "step_years", c.step_years);
    get(j, "land_wealth_scale", c.land_wealth_scale);
    get(j, "wealth_upkeep", c.wealth_upkeep);
    get(j, "power_cap", c.power_cap);
    get(j, "cohesion_decay_floor", c.cohesion_decay_floor);
    get(j, "era_mult", c.era_mult);
    get(j, "era_threshold", c.era_threshold);
    get(j, "era_diffusion", c.era_diffusion);
    get(j, "era_diffusion_gap", c.era_diffusion_gap);
    get(j, "anti_dragon_min_era", c.anti_dragon_min_era);
    get(j, "slayers_at", c.slayers_at);
    get(j, "war_siege_anti_dragon", c.war_siege_anti_dragon);
    get(j, "war_penalty", c.war_penalty);
    get(j, "war_chance", c.war_chance);
    get(j, "war_power_ratio", c.war_power_ratio);
    get(j, "war_target_cohesion", c.war_target_cohesion);
    get(j, "war_take_chance", c.war_take_chance);
    get(j, "war_loser_wealth_loss", c.war_loser_wealth_loss);
    get(j, "war_end_regions", c.war_end_regions);
    get(j, "war_max_decades", c.war_max_decades);
    get(j, "orc_war_cohesion", c.orc_war_cohesion);
    get(j, "orc_plunder", c.orc_plunder);
    get(j, "collapse_cohesion", c.collapse_cohesion);
    get(j, "successor_cohesion", c.successor_cohesion);
    get(j, "succession_bonus_mortal", c.succession_bonus_mortal);
    get(j, "succession_bonus_longlived", c.succession_bonus_longlived);
    get(j, "immortal_decay_mult", c.immortal_decay_mult);
    get(j, "immortal_era_rate_mult", c.immortal_era_rate_mult);
    get(j, "warlord_death_cohesion", c.warlord_death_cohesion);
    get(j, "warlord_lifespan", c.warlord_lifespan);
    get(j, "succession_jitter", c.succession_jitter);
    get(j, "memory_drop", c.memory_drop);
    get(j, "betrayal_decay_mult", c.betrayal_decay_mult);
    get(j, "betrayal_block_weight", c.betrayal_block_weight);
    get(j, "tribute_faith", c.tribute_faith);
    get(j, "tribute_faith_no_oath", c.tribute_faith_no_oath);
    get(j, "tribute_rate", c.tribute_rate);
    get(j, "tribute_faith_pool", c.tribute_faith_pool);
    get(j, "slay_attempt_rate", c.slay_attempt_rate);
    get(j, "slay_success_div", c.slay_success_div);
    get(j, "slay_fail_wealth_loss", c.slay_fail_wealth_loss);
    get(j, "slay_fail_dread", c.slay_fail_dread);
    get(j, "elf_withdraw_ratio", c.elf_withdraw_ratio);
    get(j, "dug_in_invasion_mult", c.dug_in_invasion_mult);
    get(j, "orc_spawn_burn", c.orc_spawn_burn);
    get(j, "orc_spawn_chance", c.orc_spawn_chance);
    get(j, "orc_spawn_weak_cohesion", c.orc_spawn_weak_cohesion);
    get(j, "orc_spawn_weak_power", c.orc_spawn_weak_power);
    get(j, "orc_spawn_wealth", c.orc_spawn_wealth);
    get(j, "orc_spawn_cohesion", c.orc_spawn_cohesion);
    get(j, "abandon_burn", c.abandon_burn);
    get(j, "settle_chance", c.settle_chance);
    get(j, "settle_max_burn", c.settle_max_burn);
    get(j, "settle_min_cohesion", c.settle_min_cohesion);
    get(j, "burn_recovery", c.burn_recovery);
    get(j, "burn_recovery_wasteland", c.burn_recovery_wasteland);
    get(j, "wasteland_burn", c.wasteland_burn);
    get(j, "wasteland_decades", c.wasteland_decades);
    get(j, "wasteland_clear_burn", c.wasteland_clear_burn);
    get(j, "wasteland_clear_decades", c.wasteland_clear_decades);
    get(j, "oath_tribute_min_faith", c.oath_tribute_min_faith);
    get(j, "oath_walls_era", c.oath_walls_era);
    get(j, "oath_fade_memory", c.oath_fade_memory);
    get(j, "oath_breach_rate", c.oath_breach_rate);
    get(j, "act_kill_cohesion", c.act_kill_cohesion);
    get(j, "act_defend_attacker_loss", c.act_defend_attacker_loss);
    get(j, "act_accept_offering_faith_pool", c.act_accept_offering_faith_pool);
    get(j, "act_give_era_share", c.act_give_era_share);
    get(j, "act_kill_soldier_wealth", c.act_kill_soldier_wealth);
    get(j, "act_give_wealth_floor", c.act_give_wealth_floor);
    get(j, "act_miracle_faith_cost", c.act_miracle_faith_cost);
    get(j, "reaction_faith_high", c.reaction_faith_high);
    get(j, "reaction_weak_ratio", c.reaction_weak_ratio);
    get(j, "losing_window_decades", c.losing_window_decades);
    get(j, "hoard_halflife", c.hoard_halflife);
    get(j, "hoard_per_unit", c.hoard_per_unit);
    get(j, "threat_base", c.threat_base);
    get(j, "threat_k_size", c.threat_k_size);
    get(j, "threat_k_magic", c.threat_k_magic);
    get(j, "threat_scale", c.threat_scale);
    get(j, "rage_burn_mult", c.rage_burn_mult);
    get(j, "rage_show_as_burn", c.rage_show_as_burn);
    get(j, "forgotten_show_as_burn", c.forgotten_show_as_burn);
    get(j, "offer_faith", c.offer_faith);
    get(j, "offer_rival_cohesion", c.offer_rival_cohesion);
    get(j, "offer_exit_era", c.offer_exit_era);
    get(j, "offer_exit_faith", c.offer_exit_faith);
    get(j, "offer_expiry_years", c.offer_expiry_years);
    get(j, "offer_tribute_defence", c.offer_tribute_defence);
    get(j, "offer_tribute_worship", c.offer_tribute_worship);
    get(j, "offer_tribute_rival", c.offer_tribute_rival);
    get(j, "demand_knowledge", c.demand_knowledge);
    get(j, "demand_tribute_rate", c.demand_tribute_rate);
    get(j, "demand_refused_resolve", c.demand_refused_resolve);
    get(j, "home_land_mult", c.home_land_mult);
    get(j, "vassal_tribute", c.vassal_tribute);
    get(j, "vassal_independence_chance", c.vassal_independence_chance);
    get(j, "vassal_overlord_cohesion", c.vassal_overlord_cohesion);
    get(j, "vassal_wealth_weight", c.vassal_wealth_weight);
    get(j, "vassal_absorb", c.vassal_absorb);
    get(j, "conquest_pop_shift", c.conquest_pop_shift);
    get(j, "refugee_share", c.refugee_share);
    get(j, "remnant_start_strength", c.remnant_start_strength);
    get(j, "remnant_halflife", c.remnant_halflife);
    get(j, "remnant_min_strength", c.remnant_min_strength);
    get(j, "remnant_refound_chance", c.remnant_refound_chance);
    get(j, "remnant_refound_wealth", c.remnant_refound_wealth);
    get(j, "remnant_refound_cohesion", c.remnant_refound_cohesion);
    get(j, "remnant_sponsor_knowledge", c.remnant_sponsor_knowledge);
    get(j, "trait_jitter", c.trait_jitter);
    get(j, "trait_mult_base", c.trait_mult_base);
    get(j, "trait_authority_decay", c.trait_authority_decay);
    get(j, "trait_authority_high", c.trait_authority_high);
    get(j, "trait_succession_halved", c.trait_succession_halved);
    get(j, "drift_militarism_war", c.drift_militarism_war);
    get(j, "drift_militarism_peace", c.drift_militarism_peace);
    get(j, "drift_peace_decades", c.drift_peace_decades);
    get(j, "drift_burned_authority", c.drift_burned_authority);
    get(j, "drift_burned_piety", c.drift_burned_piety);
    get(j, "drift_openness_border", c.drift_openness_border);
    get(j, "affinity_alliance", c.affinity_alliance);
    get(j, "coalition_power_ratio", c.coalition_power_ratio);
    get(j, "coalition_region_share", c.coalition_region_share);
    get(j, "coalition_affinity", c.coalition_affinity);
    get(j, "coalition_join_chance", c.coalition_join_chance);
    get(j, "alliance_join_chance", c.alliance_join_chance);
    get(j, "hegemony_share", c.hegemony_share);
    get(j, "fading_human_share", c.fading_human_share);
    get(j, "fading_decades", c.fading_decades);
    get(j, "fading_end_share", c.fading_end_share);
    get(j, "fading_growth_mult", c.fading_growth_mult);
    get(j, "fading_decay_mult", c.fading_decay_mult);
    get(j, "fading_depart_chance", c.fading_depart_chance);
    get(j, "fading_remnant_gain", c.fading_remnant_gain);
    get(j, "hunger_need", c.hunger_need);
    get(j, "hunger_need_per_size", c.hunger_need_per_size);
    get(j, "hunger_rate", c.hunger_rate);
    get(j, "hunger_atrophy_at", c.hunger_atrophy_at);
    get(j, "hunger_atrophy", c.hunger_atrophy);
    get(j, "hunger_wake_at", c.hunger_wake_at);
    get(j, "hunger_kill_feed", c.hunger_kill_feed);
    get(j, "hunger_food_feed", c.hunger_food_feed);
    get(j, "forecast_noise", c.forecast_noise);
    get(j, "great_ruler_chance", c.great_ruler_chance);
    get(j, "golden_age_growth_mult", c.golden_age_growth_mult);
    get(j, "golden_age_cohesion", c.golden_age_cohesion);
    get(j, "tyrant_authority", c.tyrant_authority);
    get(j, "tyrant_militarism", c.tyrant_militarism);
    get(j, "tyrant_death_cohesion", c.tyrant_death_cohesion);
    get(j, "succession_crisis_chance", c.succession_crisis_chance);
    get(j, "succession_crisis_cohesion", c.succession_crisis_cohesion);

    if (j.contains("people")) {
        const json& v = j["people"];
        get(v, "accession_jitter", c.accession_jitter);
        get(v, "epithet_trait", c.epithet_trait);
        get(v, "hero_chance", c.hero_chance);
        get(v, "hero_max", c.hero_max);
        get(v, "hero_slay_bold_base", c.hero_slay_bold_base);
        get(v, "hero_unite_cohesion", c.hero_unite_cohesion);
        get(v, "hero_usurp_bold", c.hero_usurp_bold);
        get(v, "hero_usurp_cohesion", c.hero_usurp_cohesion);
        get(v, "hero_usurp_chance", c.hero_usurp_chance);
        get(v, "sage_min_era", c.sage_min_era);
        get(v, "sage_openness", c.sage_openness);
        get(v, "priest_piety", c.priest_piety);
        get(v, "sage_memory_mult", c.sage_memory_mult);
        get(v, "keeper_min_weight", c.keeper_min_weight);
        get(v, "keeper_named_years", c.keeper_named_years);
        get(v, "hostage_raise_years", c.hostage_raise_years);
        get(v, "betrothal_chance", c.betrothal_chance);
        get(v, "tyrant_greedy", c.tyrant_greedy);
        get(v, "golden_pious", c.golden_pious);
        get(v, "known_harm", c.known_harm);
        get(v, "known_help", c.known_help);
        get(v, "known_show", c.known_show);
        get(v, "known_accept", c.known_accept);
        get(v, "known_decline", c.known_decline);
        get(v, "known_deceived", c.known_deceived);
        get(v, "known_release", c.known_release);
        get(v, "known_kill_envoy", c.known_kill_envoy);
    }
    if (j.contains("offers")) {
        const json& v = j["offers"];
        get(v, "lie_base", c.offer_lie_base);
        get(v, "lie_authority", c.offer_lie_authority);
        get(v, "lie_cunning", c.offer_lie_cunning);
        get(v, "truth_knowledge", c.offer_truth_knowledge);
        get(v, "act_years", c.offer_act_years);
        get(v, "honest_faith_mult", c.offer_honest_faith_mult);
        get(v, "consent_faith", c.offer_consent_faith);
        get(v, "elope_bold", c.offer_elope_bold);
        get(v, "ward_piety", c.offer_ward_piety);
        get(v, "teach_openness", c.offer_teach_openness);
        get(v, "envoy_kill_magnitude", c.envoy_kill_magnitude);
        get(v, "envoy_kill_dread", c.envoy_kill_dread);
        get(v, "bless_faith", c.offer_bless_faith);
        get(v, "affinity_rival", c.offer_affinity_rival);
        get(v, "elope_affinity", c.offer_elope_affinity);
        get(v, "judge_bold", c.offer_judge_bold);
    }
    if (j.contains("phases")) {
        const json& v = j["phases"];
        get(v, "thresholds", c.phase_thresholds);
        get(v, "short_sleep", c.phase_short_sleep);
        get(v, "long_sleep", c.phase_long_sleep);
        get(v, "hunger_mult", c.hunger_phase_mult);
        get(v, "moult_digest_mult", c.moult_digest_mult);
        get(v, "moult_stiff_sleeps", c.moult_stiff_sleeps);
    }
    if (j.contains("briefing")) {
        const json& v = j["briefing"];
        get(v, "group_years", c.briefing_group_years);
        get(v, "rich_dread", c.briefing_rich_dread);
        get(v, "slayers_forming", c.briefing_slayers_forming);
        get(v, "rich_count", c.briefing_rich_count);
        get(v, "importance_default", c.importance_default);
        get(v, "importance_min", c.importance_min);
        get(v, "importance_known", c.importance_known);
        get(v, "importance_oath", c.importance_oath);
        get(v, "importance_hostage", c.importance_hostage);
    }
    get(j, "elf_withdraw_affinity", c.elf_withdraw_affinity);
    get(j, "elf_withdraw_dread", c.elf_withdraw_dread);
    get(j, "elf_withdraw_chance", c.elf_withdraw_chance);
    get(j, "tribute_food", c.tribute_food);
    get(j, "hegemony_end_share", c.hegemony_end_share);
    get(j, "cause_window_years", c.cause_window_years);
}

static void load_names(const json& j, Names& n) {
    auto table = [](const json& v, std::map<std::string, NameTable>& out) {
        for (auto it = v.begin(); it != v.end(); ++it) {
            NameTable t;
            get(it.value(), "a", t.a);
            get(it.value(), "b", t.b);
            out[it.key()] = t;
        }
    };
    if (j.contains("factions")) table(j["factions"], n.factions);
    if (j.contains("people")) table(j["people"], n.people);
    if (j.contains("epithets")) get(j, "epithets", n.epithets);
}

static void load_offers(const json& j, std::vector<OfferDef>& out) {
    for (auto& v : j) {
        OfferDef d;
        get(v, "kind", d.kind);
        get(v, "motive", d.motive);
        get(v, "pretexts", d.pretexts);
        get(v, "can_lie", d.can_lie);
        get(v, "pred", d.pred);
        get(v, "gives", d.gives);
        get(v, "price_rate", d.price_rate);
        get(v, "envoy", d.envoy);
        get(v, "clauses", d.clauses);
        get(v, "expires_years", d.expires_years);
        get(v, "enabled", d.enabled);
        get_tags(v, "tags", d.tags);
        out.push_back(d);
    }
}

static void load_texts(const json& j, Texts& t) {
    if (j.contains("events"))
        for (auto it = j["events"].begin(); it != j["events"].end(); ++it) {
            TextTemplate tt;
            const json& v = it.value();
            if (v.is_string()) tt.text = v.get<std::string>();
            else {
                get(v, "text", tt.text);
                get(v, "brief", tt.brief);
            }
            t.events[it.key()] = tt;
        }
    get(j, "causes", t.causes);
    get(j, "motives", t.motives);
    get(j, "pretexts", t.pretexts);
    get(j, "asks", t.asks);
    get(j, "roles", t.roles);
    get(j, "words", t.words);
    get(j, "reactions", t.reactions);
    if (j.contains("misc"))
        for (auto it = j["misc"].begin(); it != j["misc"].end(); ++it) {
            if (it.value().is_string()) t.misc[it.key()] = it.value().get<std::string>();
            else if (it.key() == "era_names") t.era_names = it.value().get<std::vector<std::string>>();
            else if (it.key() == "phase_names") t.phase_names = it.value().get<std::vector<std::string>>();
            else if (it.key() == "numbers") t.numbers = it.value().get<std::vector<std::string>>();
        }
}

static void load_oath_effects(const json& j, std::map<std::string, OathEffect>& out) {
    for (auto it = j.begin(); it != j.end(); ++it) {
        OathEffect e;
        const json& v = it.value();
        if (v.contains("faith_set")) { e.has_faith_set = true; e.faith_set = v["faith_set"].get<double>(); }
        get(v, "dread_add", e.dread_add);
        get_tags(v, "tags", e.tags);
        if (v.contains("modifier")) {
            const json& m = v["modifier"];
            e.has_modifier = true;
            get(m, "var", e.mod_var);
            get(m, "op", e.mod_op);
            get(m, "value", e.mod_value);
            get(m, "decades", e.mod_decades);
        }
        out[it.key()] = e;
    }
}

static void load_modifier_def(const json& j, ModifierDef& m) {
    m.present = true;
    get(j, "var", m.var);
    get(j, "op", m.op);
    get(j, "value", m.value);
    get(j, "decades", m.decades);
}

static void load_spells(const json& j, std::map<std::string, SpellDef>& out) {
    for (auto it = j.begin(); it != j.end(); ++it) {
        SpellDef s;
        s.id = it.key();
        const json& v = it.value();
        get(v, "enabled", s.enabled);
        get(v, "gate", s.gate);
        get(v, "target", s.target);
        get(v, "gate_min", s.gate_min);
        get(v, "cost", s.cost);
        if (v.contains("modifier")) load_modifier_def(v["modifier"], s.modifier);
        if (v.contains("spawn")) {
            const json& sp = v["spawn"];
            get(sp, "race", s.spawn_race);
            get(sp, "wealth_per_magic", s.spawn_wealth_per_magic);
            get(sp, "cohesion", s.spawn_cohesion);
            get(sp, "era", s.spawn_era);
            get(sp, "site_min_burn", s.site_min_burn);
        }
        out[s.id] = s;
    }
}

static void load_terraform(const json& j, TerraformDef& t) {
    get(j, "gate_size", t.gate_size);
    get(j, "cost_size_fraction", t.cost_size_fraction);
    if (j.contains("kinds"))
        for (auto it = j["kinds"].begin(); it != j["kinds"].end(); ++it) {
            TerraformKind k;
            get(it.value(), "tag", k.tag);
            if (it.value().contains("modifier")) load_modifier_def(it.value()["modifier"], k.modifier);
            t.kinds[it.key()] = k;
        }
}

bool params_from_json(const DataJson& d, Params& p, std::string& err) {
    json j;
    if (!parse_json(d.constants, "constants", j, err)) return false;
    load_constants(j, p.c);
    if (j.contains("oath_effects")) load_oath_effects(j["oath_effects"], p.oath_effects);
    if (j.contains("spells")) load_spells(j["spells"], p.spells);
    if (j.contains("terraform")) load_terraform(j["terraform"], p.terraform);
    if (p.c.era_mult.size() < 5 || p.c.era_threshold.size() < 4) { err = "constants: era tables too short"; return false; }
    if (p.c.phase_thresholds.size() < 12 || p.c.phase_short_sleep.size() < 5 || p.c.phase_long_sleep.size() < 5 || p.c.hunger_phase_mult.size() < 5) {
        err = "constants: phases tables too short";
        return false;
    }
    if (!d.names.empty()) { if (!parse_json(d.names, "names", j, err)) return false; load_names(j, p.names); }
    if (!d.offers.empty()) { if (!parse_json(d.offers, "offers", j, err)) return false; load_offers(j, p.offers); }
    if (!d.text.empty()) { if (!parse_json(d.text, "text", j, err)) return false; load_texts(j, p.texts); }
    if (!d.importance.empty()) { if (!parse_json(d.importance, "importance", j, err)) return false; get(j, "events", p.importance); }
    if (!cards_from_json(d.events, p.cards, err)) return false;
    if (!genesis_from_json(d.genesis, p.genesis, err)) return false;

    if (!parse_json(d.races, "races", j, err)) return false;
    for (auto it = j.begin(); it != j.end(); ++it) {
        Race r;
        r.id = it.key();
        const json& v = it.value();
        Culture& c = r.culture;
        get(v, "lifespan", c.lifespan);
        get(v, "growth", c.growth);
        get(v, "cohesion_decay", c.cohesion_decay);
        get(v, "era_rate", c.era_rate);
        get(v, "memory_halflife", c.memory_halflife);
        get(v, "ruler", c.ruler_class);
        get(v, "loot", c.loot);
        get(v, "alliance", c.alliance);
        get_tags(v, "tags", c.tags);
        if (v.contains("temperament")) {
            const json& t = v["temperament"];
            get(t, "war_appetite", c.temperament.war_appetite);
            get(t, "expansion", c.temperament.expansion);
            get(t, "home_terrain", c.temperament.home_terrain);
            get(t, "off_home_land", c.temperament.off_home_land);
            get(t, "settles_off_home", c.temperament.settles_off_home);
        }
        if (v.contains("traits")) {
            const json& t = v["traits"];
            get(t, "authority", c.traits.authority);
            get(t, "openness", c.traits.openness);
            get(t, "militarism", c.traits.militarism);
            get(t, "piety", c.traits.piety);
        }
        if (v.contains("affinity")) get(v, "affinity", p.race_affinity[r.id]);
        if (v.contains("remnant")) {
            get(v["remnant"], "start", r.remnant_start);
            get(v["remnant"], "floor", r.remnant_floor);
        }
        if (v.contains("person")) {
            const json& t = v["person"];
            get(t, "accession_age", r.person.accession_age);
            get(t, "lifespan", r.person.lifespan);
            get(t, "lifespan_jitter", r.person.lifespan_jitter);
            get(t, "max_age", r.person.max_age);
            get(t, "generation", r.person.generation);
        }
        p.races[r.id] = r;
    }
    std::map<std::string, std::map<std::string, double>> given = p.race_affinity;
    for (auto& a : given)
        for (auto& b : a.second)
            if (!p.race_affinity[b.first].count(a.first)) p.race_affinity[b.first][a.first] = b.second;

    if (!parse_json(d.reactions, "reactions", j, err) || !reactions_from_json(j, p, err)) return false;

    if (!parse_json(d.shocks, "shocks", j, err)) return false;
    Shocks& s = p.shocks;
    if (j.contains("plague")) {
        get(j["plague"], "chance", s.plague_chance);
        get(j["plague"], "wealth_mult", s.plague_wealth_mult);
        get(j["plague"], "cohesion_delta", s.plague_cohesion);
    }
    if (j.contains("bad_century")) {
        get(j["bad_century"], "chance", s.bad_century_chance);
        get(j["bad_century"], "land_mult", s.bad_century_land_mult);
        get(j["bad_century"], "decades", s.bad_century_decades);
    }
    if (j.contains("volcanic_winter")) {
        get(j["volcanic_winter"], "trigger_burn_total", s.volcanic_trigger_burn_total);
        get(j["volcanic_winter"], "chance", s.volcanic_chance);
        get(j["volcanic_winter"], "land_mult", s.volcanic_land_mult);
        get(j["volcanic_winter"], "decades", s.volcanic_decades);
    }
    if (!dragon_data_from_json(d, p, err)) return false;   // v0.4 (S1)
    if (!hooks_from_json(d, p, err)) return false;         // v0.4 hooks (S3)
    if (!arrivals_from_json(d, p, err)) return false;      // item 06
    return true;
}

bool world_from_data_json(const DataJson& d, const Params& p, World& w, std::string& err) {
    json j;
    if (!parse_json(d.regions, "regions", j, err)) return false;
    for (auto& v : j) {
        Region r;
        get(v, "id", r.id);
        get(v, "neighbours", r.neighbours);
        get(v, "owner", r.owner);
        get(v, "terrain", r.terrain);
        get(v, "land", r.land);
        get(v, "burn", r.burn);
        get(v, "lair", r.lair);
        get_tags(v, "tags", r.tags);
        r.eff_land = r.land;
        w.regions.push_back(r);
    }
    for (auto& r : w.regions)
        for (auto& n : r.neighbours)
            if (!find_region(w, n)) { err = "region " + r.id + " has unknown neighbour " + n; return false; }

    if (!parse_json(d.factions, "factions", j, err)) return false;
    for (auto& v : j) {
        Faction f;
        get(v, "id", f.id);
        get(v, "race", f.race);
        auto race = p.races.find(f.race);
        if (race == p.races.end()) { err = "faction " + f.id + " has unknown race " + f.race; return false; }
        f.culture = race->second.culture;
        f.pop[f.race] = 1.0;
        get(v, "wealth", f.wealth);
        get(v, "cohesion", f.cohesion);
        get(v, "era", f.era);
        if (v.contains("dread")) f.dread.oral = v["dread"].get<double>();
        get(v, "faith", f.faith);
        get_tags(v, "tags", f.tags);
        f.born = 0;
        f.era_years.assign(static_cast<size_t>(f.era) + 1, 0);
        w.factions.push_back(f);
    }
    for (auto& r : w.regions) {
        if (r.owner.empty()) continue;
        Faction* f = find_faction(w, r.owner);
        if (!f) { err = "region " + r.id + " owned by unknown faction " + r.owner; return false; }
        f->regions.push_back(r.id);
    }
    sort_by_id(w);
    for (auto& f : w.factions) {
        std::sort(f.regions.begin(), f.regions.end());
        f.max_regions = static_cast<int>(f.regions.size());
    }
    return true;
}

}
