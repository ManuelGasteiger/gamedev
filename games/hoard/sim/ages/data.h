#pragma once
#include <map>
#include <string>
#include <vector>
#include "cards.h"
#include "endings_def.h"
#include "genesis_data.h"
#include "move_data.h"
#include "../sift/bank.h"
#include "state.h"

namespace ages {

// constants.json "memory": tiers, deposits, witnesses, unlocks, research (act-api 6.1, 6.3, 6.4)
struct MemoryConstants {
    double oral_halflife = 30, written_halflife = 250, monumental_halflife = 1000;
    double oral_reference = 150;   // the race memory_halflife at which the oral tier keeps oral_halflife
    double written_deposit = 0.6, monumental_deposit = 0.4;
    double rep_years = 30, rep_floor = 0.25;
    double witness_owner = 1, witness_neighbour = 0.5;
    double library_burn = 0.1;
    double temple_convert = 0.5;
    double entry_drop = 0.002;
    double tier_shown = 0.1;
    double kept_oath_awe = 0.02;
    double pilgrims_awe = 0.15, honest_awe = 0.30, cult_awe = 0.30, temple_awe = 0.50, faith_floor = 0.3;
    int temple_years = 50;
    double demand_dread = 0.30, slayer_dread = 0.50, flee_dread = 0.70;
    double grievance_written = 0;   // > 0: a written dread stock this high may found an order too (pacing-model finding 2)
    double stability_dread = 0.8, pressure_awe = 0.7, research_dread = 2.0;
    double research_base = 0.01, research_spend = 50, research_wealth_half = 100;
};

// constants.json "chronicle" (item 05): chronicle levels, sources in the hoard, relics with a voice
struct RelicKind { std::string id, agenda; std::vector<std::string> races; };
struct ChronicleConstants {
    double lore_counterfactual = 25;   // Lore growth points for level 3
    int take_sources = 4;              // sources a full take of books carries off
    double relic_chance = 0.25;        // per take of artifacts, times its magnitude
    int relic_max = 3;
    int quiet_wakings = 3;             // commune(refuse)
    double honest_openness = 0.6;
    double cf_min_importance = 0.5;    // the counterfactual names an event at least this important
    std::vector<RelicKind> relics;
};

struct Constants {
    MemoryConstants mem;
    ChronicleConstants chron;
    int step_years = 10;
    double land_wealth_scale = 100;
    double wealth_upkeep = 0.01;
    double power_cap = 40;
    double cohesion_decay_floor = 0.25;
    std::vector<double> era_mult;
    std::vector<double> era_threshold;
    double era_diffusion = 4;
    int era_diffusion_gap = 2;

    int anti_dragon_min_era = 2;
    double slayers_at = 1.0;
    double war_siege_anti_dragon = 0.005;

    double war_penalty = 0.4;
    double war_chance = 0.15;
    double war_power_ratio = 1.5;
    double war_target_cohesion = 0.5;
    double war_take_chance = 0.3;
    double war_loser_wealth_loss = 0.1;
    int war_end_regions = 2;
    int war_max_decades = 5;
    double orc_war_cohesion = 0.4;
    double orc_plunder = 0.15;

    double collapse_cohesion = 0.2;
    double successor_cohesion = 0.6;
    double succession_bonus_mortal = 0.15;
    double succession_bonus_longlived = 0.05;
    double immortal_decay_mult = 0.1;
    double immortal_era_rate_mult = 0.5;
    double warlord_death_cohesion = 0.2;
    double warlord_lifespan = 40;
    double succession_jitter = 0.25;

    double memory_drop = 0.02;
    double betrayal_decay_mult = 0.5;
    double betrayal_block_weight = 0.1;

    double tribute_faith = 0.5;
    double tribute_faith_no_oath = 0.8;
    double tribute_rate = 0.02;
    double tribute_faith_pool = 0.1;
    double slay_attempt_rate = 0.05;
    double slay_success_div = 3;
    double slay_fail_wealth_loss = 0.3;
    double slay_fail_dread = 0.2;

    double elf_withdraw_ratio = 2.0;
    double dug_in_invasion_mult = 0.5;
    double orc_spawn_burn = 0.5;
    double orc_spawn_chance = 0.1;
    double orc_spawn_weak_cohesion = 0.5;
    double orc_spawn_weak_power = 10;
    double orc_spawn_wealth = 20;
    double orc_spawn_cohesion = 0.8;
    double abandon_burn = 0.8;
    double settle_chance = 0.1;
    double settle_max_burn = 0.5;
    double settle_min_cohesion = 0.5;

    double burn_recovery = 0.02;
    double burn_recovery_wasteland = 0.005;
    double wasteland_burn = 0.8;
    int wasteland_decades = 10;
    double wasteland_clear_burn = 0.2;
    int wasteland_clear_decades = 30;

    double oath_tribute_min_faith = 0.2;
    int oath_walls_era = 2;
    double oath_fade_memory = 0.2;    // no_walls and no_settle hold while dread or awe stays this high
    double oath_breach_rate = 0.05;   // and break by chance, this per decade at stability 0 (act-api 6.4)

    double act_kill_cohesion = 0.05;
    double act_defend_attacker_loss = 0.15;
    double act_accept_offering_faith_pool = 0.1;
    double act_give_era_share = 1.0;
    double act_kill_soldier_wealth = 0.5;
    double act_give_wealth_floor = 10;
    double act_miracle_faith_cost = 1.0;

    double reaction_faith_high = 0.5;
    double reaction_weak_ratio = 0.5;
    int losing_window_decades = 2;

    std::map<std::string, double> hoard_halflife;
    std::map<std::string, double> hoard_per_unit;
    double threat_base = 1.0;
    double threat_k_size = 0.5;
    double threat_k_magic = 2.0;
    double threat_scale = 20;

    double rage_burn_mult = 1.3;
    double rage_show_as_burn = 0.2;
    double forgotten_show_as_burn = 0.3;   // world tag dragon_forgotten: a show is read as a burn of this magnitude

    double offer_faith = 0.7;
    double offer_rival_cohesion = 0.5;
    int offer_expiry_years = 500;
    double offer_tribute_defence = 0.1;
    double offer_tribute_worship = 0.05;
    double offer_tribute_rival = 0.1;

    // demand (section 8): a knowledge-gated oath the dragon imposes on a cowed court
    double demand_knowledge = 0.5;
    double demand_tribute_rate = 0.1;
    double demand_refused_resolve = 0.05;

    // 15.1 terrain
    double home_land_mult = 1.2;

    // 15.2 vassalage
    double vassal_tribute = 0.2;
    double vassal_independence_chance = 0.2;
    double vassal_overlord_cohesion = 0.4;
    double vassal_wealth_weight = 0.5;
    std::string vassal_absorb = "same_race";

    // 15.3 population and remnants
    double conquest_pop_shift = 0.15;
    double refugee_share = 0.1;
    double remnant_start_strength = 0.3;
    double remnant_halflife = 1000;
    double remnant_min_strength = 0.1;
    double remnant_refound_chance = 0.05;
    double remnant_refound_wealth = 10;
    double remnant_refound_cohesion = 0.7;
    double remnant_sponsor_knowledge = 0;

    // 15.4 regime traits
    double trait_jitter = 0.15;
    double trait_mult_base = 0.5;
    double trait_authority_decay = 0.4;
    double trait_authority_high = 0.6;
    double trait_succession_halved = 0.5;
    double drift_militarism_war = 0.02;
    double drift_militarism_peace = -0.01;
    int drift_peace_decades = 10;
    double drift_burned_authority = 0.05;
    double drift_burned_piety = 0.05;
    double drift_openness_border = 0.01;
    double affinity_alliance = 0.5;
    double coalition_power_ratio = 2.0;
    double coalition_region_share = 0.4;
    double coalition_affinity = 0.3;
    double coalition_join_chance = 0.1;
    double alliance_join_chance = 0.3;
    double hegemony_share = 0.5;

    // 15.5 fading
    double fading_human_share = 0.6;
    int fading_decades = 10;
    double fading_end_share = 0.4;
    double fading_growth_mult = 0.5;
    double fading_decay_mult = 0.5;
    double fading_depart_chance = 0.03;
    double fading_remnant_gain = 0.05;

    // 15.6 hunger
    double hunger_need = 0.4;
    double hunger_need_per_size = 0.1;
    double hunger_rate = 0.05;
    double hunger_atrophy_at = 0.5;
    double hunger_atrophy = 0.01;
    double hunger_wake_at = 0.9;
    double hunger_kill_feed = 0.5;
    double hunger_food_feed = 0.1;
    double forecast_noise = 0.5;

    // 15.7 rare events
    double great_ruler_chance = 0.02;
    double golden_age_growth_mult = 1.5;
    double golden_age_cohesion = 0.2;
    double tyrant_authority = 0.3;
    double tyrant_militarism = 0.3;
    double tyrant_death_cohesion = -0.1;
    double succession_crisis_chance = 0.15;
    double succession_crisis_cohesion = -0.3;

    // 17.2 people
    double accession_jitter = 0.25;
    double epithet_trait = 0.75;
    double hero_chance = 0.02;
    int hero_max = 3;
    double hero_slay_bold_base = 0.5;
    double hero_unite_cohesion = 0.2;
    double hero_usurp_bold = 0.7;
    double hero_usurp_cohesion = 0.4;
    double hero_usurp_chance = 0.2;
    int sage_min_era = 2;
    double sage_openness = 0.5;
    double priest_piety = 0.5;
    double sage_memory_mult = 0.5;
    double keeper_min_weight = 0.1;   // a dragon memory this faint is not taken up by a new sage or priest
    int keeper_named_years = 200;     // the chronicle names a kept memory only when it is at least this old
    int hostage_raise_years = 20;
    double betrothal_chance = 0.3;
    double tyrant_greedy = 0.3;
    double golden_pious = 0.2;
    double known_harm = -0.3;
    double known_help = 0.3;
    double known_show = -0.1;
    double known_accept = 0.3;
    double known_decline = -0.1;
    double known_deceived = -0.5;
    double known_release = 0.5;
    double known_kill_envoy = -0.6;

    // 17.3 offers
    double offer_lie_base = 0.1;
    double offer_lie_authority = 0.3;
    double offer_lie_cunning = 0.4;
    double offer_truth_knowledge = 2.0;
    int offer_act_years = 100;
    double offer_honest_faith_mult = 1.5;
    double offer_consent_faith = 0.2;
    double offer_elope_bold = 0.6;
    double offer_ward_piety = 0.6;
    double offer_teach_openness = 0.5;
    double envoy_kill_magnitude = 0.05;
    double envoy_kill_dread = 0.3;
    double offer_bless_faith = 0.3;
    double offer_affinity_rival = 0.0;
    double offer_elope_affinity = 0.0;
    double offer_judge_bold = 0.5;

    // 17.4 phases and tempo
    std::vector<double> phase_thresholds;    // score = size + knowledge + magic; 4 entries for phases 2..5
    std::vector<int> phase_short_sleep;      // years, 5 entries
    std::vector<int> phase_long_sleep;
    std::vector<double> hunger_phase_mult;   // 5 entries
    double moult_digest_mult = 2.0;
    int moult_stiff_sleeps = 1;

    // 17.5 briefing
    int briefing_group_years = 100;
    double briefing_rich_dread = 0.2;
    double briefing_slayers_forming = 0.5;
    int briefing_rich_count = 3;
    double importance_default = 0.2;   // kinds data/importance.json does not list
    double importance_min = 0.25;      // "while you slept" keeps an event at or above this, or one on a thread that has one
    double importance_known = 0.3;     // the subject or object is a person the dragon knows
    double importance_oath = 0.2;      // ... a court under an active oath with the dragon
    double importance_hostage = 0.3;   // ... a hostage, or a court whose heir is in the lair

    // 17.7
    double elf_withdraw_affinity = 0.2;
    double elf_withdraw_dread = 0.6;
    double elf_withdraw_chance = 0.3;
    double tribute_food = 0.3;

    // 17.8
    double hegemony_end_share = 0.4;
    int cause_window_years = 100;   // how far back a rule looks for the event that explains the one it emits

    // v0.4 (S1) constants.json "dragon": digestion on the phase clock (half-life = share x the phase's long sleep),
    // growth points per digested unit, the gold bed, phases from total points, health, talent mechanics
    std::map<std::string, double> growth_halflife;   // books, art, royals, artifacts, embers, food
    std::map<std::string, double> growth_points;     // books, royals, artifacts, art, tribute, embers, food
    double bed_rate = 0.4;                           // might per sqrt(gold) per long window slept
    double bed_overtime = 1;                         // the bed's rate past the sleep's first long window
    std::string bed_mode = "window";                 // "window": Might from sqrt(gold) and nothing consumed; "consumed": Might from the gold eaten above the floor
    double gold_burn = 0.15;                         // share of the gold above the floor eaten per long window slept
    double gold_floor_base = 50;                     // the floor is this x floor_phase_mult[phase]
    std::vector<double> floor_phase_mult;            // 5 entries
    double gold_pts = 0.08;                          // Might per gold eaten
    double size_per_might = 0.05;
    std::vector<double> phase_points;                // total points that open phases 2..5; empty: the old thresholds
    double health_base = 100;
    double health_per_might = 3;
    int premonition_decades = 5;                     // slayers_coming:<court> stays on the world this long
    double molten_gold_burn = 0.3;
    double clinging_recovery_mult = 0.5;
    double hex_anti_dragon_mult = 0.5;
    double cult_faith = 0.5;
    double brood_faith = 0.6;
    // "slayers": the band's lapse, its threat points and odds against health (strength / (health x health_div)), the
    // adaptation factor (adapt_base + adapt_step x adaptation, clamped), the wounds share that makes a sleep a near death
    double slay_lapse_dread = 0.4;
    double slay_ready_rate = 0.05;   // per decade, once a waking has passed since the band formed
    double slay_health_div = 1;
    double slay_points_base = 2;
    double slay_points_hoard = 0.1;
    double slay_points_growth = 0.02;
    double adapt_gain = 1, adapt_gain_negative = 3, adapt_loss = 3;
    double adapt_base = 0.8, adapt_step = 0.1, adapt_min = 0.4, adapt_max = 1.6;
    double near_death_wounds = 0.7;
    std::vector<double> anti_dragon_marks;   // shares of the way to the next defence the chronicle reports
    double dread_research = 0;               // research per decade below anti_dragon_min_era (x dread x burn memory)
    std::map<std::string, double> start_hoard;   // the hoard the egg lies on (gold, books, artifacts, art, food)
};

struct NameTable {
    std::vector<std::string> a, b;
};

struct Names {
    std::map<std::string, NameTable> factions;
    std::map<std::string, NameTable> people;
    std::map<std::string, std::string> epithets;   // trait -> " the Bold"
};

// one row of data/offers.json: what the kind asks, who carries it, what it may hide behind
struct OfferDef {
    std::string kind;
    std::string motive;
    std::vector<std::string> pretexts;
    bool can_lie = true;
    std::string pred;        // the clause the dragon is asked for ("defend", "burn", "release", ...) or "" for nothing
    std::string gives;       // "tribute" | "ransom" | "none"
    double price_rate = 0.1;
    std::string envoy = "heir";   // sage | priest | heir | hero | ruler | self (the person the offer is about)
    std::vector<std::string> clauses;   // clause preds of the oath draft ("tribute", "no_harm", "defend")
    int expires_years = 100;
    bool enabled = true;
    Tags tags;               // copied onto the oath_sworn event (e.g. exit_not_implemented)
};

// data/text/en/narrator.json: templates per event kind, plus cause, motive and pretext texts
struct TextTemplate {
    std::string text, brief;
};

struct Texts {
    std::map<std::string, TextTemplate> events;
    std::map<std::string, std::string> causes, motives, pretexts, asks, roles, words, misc;
    std::map<std::string, std::string> reactions;   // apply_acts lines: "<act>.<relationship>[.<race>]", "extra.*", "tag.*"
    std::vector<std::string> era_names, phase_names, numbers;   // numbers: words for 0..n ("no", "one", ...)
};

// data/reactions.json (act-api section 6.2): base dread and awe gains per act or "<act>.<kind>" at m = 1 for the owner,
// and rows read through the court's relationship that multiply them (who: owner | witness) and carry v0.4's faith,
// research and tags
struct Gain {
    double dread = 0, awe = 0;
};

struct Reaction {
    std::vector<std::string> acts;
    std::string relationship;
    std::string who = "owner";
    double dread_mult = 1, awe_mult = 1;
    double faith_per_m = 0, faith_flat = 0;
    double anti_dragon_per_m = 0;
    bool has_faith_set = false;
    double faith_set = 0;
    Tags tags;
};


struct Shocks {
    double plague_chance = 0.01, plague_wealth_mult = 0.7, plague_cohesion = -0.1;
    double bad_century_chance = 0.005, bad_century_land_mult = 0.7;
    int bad_century_decades = 10;
    double volcanic_trigger_burn_total = 3.0, volcanic_chance = 1.0, volcanic_land_mult = 0.5;
    int volcanic_decades = 5;
};

struct OathEffect {
    bool has_faith_set = false;
    double faith_set = 0;
    double dread_add = 0;
    Tags tags;
    bool has_modifier = false;
    std::string mod_var, mod_op;
    double mod_value = 0;
    int mod_decades = 0;
};

struct ModifierDef {
    bool present = false;
    std::string var, op = "mul";
    double value = 1;
    int decades = 10;
};

// section 8 verbs that resolve to a modifier or a spawn; gate names a dragon axis (magic, knowledge, faith_pool,
// knowledge_or_magic), target is "region" or "faction"
struct SpellDef {
    std::string id;
    bool enabled = true;
    std::string gate = "magic";
    std::string target = "region";
    double gate_min = 0;
    double cost = 0;
    ModifierDef modifier;
    std::string spawn_race;
    double spawn_wealth_per_magic = 0;
    double spawn_cohesion = 0.8;
    int spawn_era = 0;
    double site_min_burn = 0.5;
};

struct TerraformKind {
    std::string tag;
    ModifierDef modifier;
};

struct TerraformDef {
    double gate_size = 2;
    double cost_size_fraction = 0.1;
    std::map<std::string, TerraformKind> kinds;
};

// v0.4 (S1): data/talents.json and data/defences.json
struct TalentLook {
    std::map<std::string, double> num;
    std::map<std::string, std::string> str;
};

struct TalentDef {
    std::string id, axis, name, text;
    int node = 0;                              // index into TalentBook.nodes
    std::map<std::string, double> effects;
    TalentLook look;
};

struct TalentAxis {
    std::string id, name, fed_by;
    double rank_every = 25;                    // points of the first rank after the axis' last authored node
    double rank_growth = 1;                    // each further rank costs this much more
    std::map<std::string, double> rank_effects;
    TalentLook rank_look;                      // numbers add once per rank
    std::string rank_text;
    int milestone_every = 0;                   // every this many ranks the milestone effects and look apply once more
    std::map<std::string, double> milestone_effects;
    TalentLook milestone_look;
};

struct TalentBook {
    std::map<std::string, double> flight;     // "flight": the v0.4 flight numbers the dragon.flight block starts from
    std::vector<TalentAxis> axes;
    std::vector<double> nodes;                 // points per node
    std::vector<int> node_phase;               // phase a node needs
    std::vector<TalentDef> talents;            // file order
    std::map<std::string, size_t> index;       // id -> talents[i]
};

// opens at min_era for races_any (any race when empty) when anti_dragon >= min_anti_dragon or a tag of tags_any is on
// the court (always when neither is given), or wherever a ward modifier holds the region if ward_modifier; a defence
// with a radius is ranged (arcane armor applies); a fearless one stands when the garrisons flee (dread >= flee_dread)
struct DefenceDef {
    std::string id, label;
    int min_era = 0;
    double min_anti_dragon = 0;
    std::vector<std::string> tags_any, races_any;
    bool ward_modifier = false;
    double per_act = 0, passive = 0, radius = 0, burn_mult = 1, era_scale = 0;
    bool fearless = false;   // dread does not make its crew flee
};

struct DefenceBook {
    std::vector<DefenceDef> defences;
    double hero_bonus = 5;
};

// v0.4 hooks (S3): data/hooks.json. A template's `when` is a card trigger (cards.h) evaluated per self of its scope;
// `where` names the hook's region (capital | near_lair | lair | largest | $region); reward and ignore are flat maps of
// the keys hook_resolve.cpp applies; a fight wounds the dragon through the named defence's armor
struct HookDef {
    std::string id, kind, scope = "faction";
    std::vector<CardPred> when, force_if;
    std::string where = "capital", pick;
    std::string arrival;   // item 06: an arrival kind; the template holds on the courts with one waiting, and is always shown
    std::vector<std::string> acts, choices;
    double priority = 10;
    bool fallback = false;
    int cooldown_years = 0;
    std::map<std::string, double> reward, ignore;
    std::map<std::string, std::map<std::string, double>> choice_reward;   // per choice
    std::map<std::string, std::string> text;   // title, text, reward_text, ignore_text, choice texts
    double fight_damage = 0, fight_era_scale = 0, fight_strength_mult = 0, fight_health_share = 0;   // flat x (1 + era_scale x era), plus the court's slayer strength x mult, plus health_max x share
    std::string fight_defence;
};

struct HookBook {
    std::vector<HookDef> templates;
    int max_hooks = 3;
    double second = 0.6, third = 0.15;   // chance of a second and a third hook per waking
    double repeat_mult = 0.5;            // priority of a kind offered on the previous waking
};

// item 06: data/arrivals.json. A clause names a gate (arrival_gates.cpp) and bounds its value; no bound means the gate
// must be true. `unless` names a gate that voids the clause; `event` and `faith_pool` give a blocked clause its
// side effect (the foothill shrine)
struct ArrivalGateDef {
    std::string gate, unless, event;
    bool has_gte = false, has_lte = false, has_gt = false, has_lt = false;
    double gte = 0, lte = 0, gt = 0, lt = 0;
    double faith_pool = 0;
};

struct ArrivalDef {
    std::string kind, person_role, window = "wake", text_key, cause;   // window: wake | short (leaves after the phase's short sleep)
    int specificity = 0, cooldown_years = 100;
    bool ignores_decade_limit = false;
    std::vector<ArrivalGateDef> requires_, blocked_when;
    std::map<std::string, std::map<std::string, double>> answers;       // receive outcome -> reward keys of hooks.json
};

struct ArrivalBook {
    std::vector<ArrivalDef> kinds;
    double oral_floor = 0.05, lure_gold = 60, hunt_food = 10, slopes_hunt_mult = 1.5;
    int rumour_years = 150, thief_era = 1, refugee_years = 20, envoy_kill_generations = 2, sight_regions = 2;
    int lair_memory_years = 100;
};

struct Params {
    Constants c;
    uint64_t data_hash = 0;   // of the loaded data, set by load_data_from_json (record.cpp)
    std::map<std::string, Race> races;
    std::map<std::string, std::map<std::string, double>> race_affinity;
    std::map<std::string, Gain> gains;
    std::vector<Reaction> reactions;
    Shocks shocks;
    std::map<std::string, OathEffect> oath_effects;
    std::map<std::string, SpellDef> spells;
    TerraformDef terraform;
    Names names;
    std::vector<OfferDef> offers;
    Texts texts;
    std::map<std::string, double> importance;   // data/importance.json, per event kind or template key
    std::vector<CardDef> cards;   // data/events, sorted by id
    TalentBook talents;           // v0.4 (S1)
    DefenceBook defences;         // v0.4 (S1)
    HookBook hooks;               // v0.4 hooks (S3)
    ArrivalBook arrivals;         // item 06
    MoveBook moves;               // item 07
    GenesisDef genesis;           // v0.4 world (S2): data/genesis.json
    SiftBank sift;                // item 14: data/patterns, importance.json "tiers", constants.json "sift"
    EndingsDef endings;           // item 04: data/endings.json
};

struct PolicyAct {
    std::string type, target, region, against;
    double magnitude = 0;
    std::map<std::string, double> payload_share;
};

struct Policy {
    std::vector<std::string> cycle;
    std::vector<PolicyAct> acts;
    // v0.4 (S1): talent preference (ids, first wins), sleep rhythm ("short" | "long" | years, cycled), shortest sleep
    std::vector<std::string> talents;
    std::vector<std::string> sleep_cycle;
    std::vector<std::string> sleep_open;   // played once before the cycle starts (item 01: long_1000, newcomer)
    int min_sleep = 0;
    bool avoid_rings = false;              // drop acts where a defence beyond levies would strike (newcomer)
    int act_wakings = 0;                   // acts only at the first N wakings (0: every waking; raid_then_1000)
    // v0.4 hooks (S3): the acts the policy answers hooks with (empty: none), its choice for choice hooks, the magnitude
    // and take payload of a synthesised act, and the share of health its wounds may reach with the answer's damage
    std::vector<std::string> hook_acts;
    std::string hook_choice = "receive";
    double hook_magnitude = 0.3;
    std::map<std::string, double> hook_payload;
    double hook_wound_cap = 0.4;
    int hook_reach = 0;   // regions from the lair the raid flies to answer a hook (0: any)
    int hook_max = 0;     // hooks answered per waking (0: all it can)
};

struct DataJson {
    std::string constants, races, reactions, regions, factions, shocks;
    std::string names, offers, text;   // v0.2; optional (defaults compiled in are empty, so the files are expected)
    std::string events;                // data/events/*.json joined into one array; optional
    std::string talents, defences;     // v0.4 (S1); optional
    std::string hooks;                 // v0.4 hooks (S3); optional
    std::string genesis;               // v0.4 world (S2): data/genesis.json; optional
    std::string importance;            // data/importance.json
    std::string arrivals;              // item 06: data/arrivals.json
    std::string moves;                 // item 07: data/moves.json
    std::string patterns, base_rates;  // item 14: data/patterns/*.json joined into one array, data/patterns/base_rates.json; optional
    std::string endings;               // item 04: data/endings.json; optional
    std::map<std::string, std::string> text_bundles;   // item 19: per language, sim/text/bundle_io.h; hashed, never read by the sim
};

bool params_from_json(const DataJson& d, Params& out, std::string& err);
bool world_from_data_json(const DataJson& d, const Params& p, World& out, std::string& err);
bool dragon_data_from_json(const DataJson& d, Params& out, std::string& err);   // v0.4 (S1), dragon_data.cpp
bool hooks_from_json(const DataJson& d, Params& out, std::string& err);         // v0.4 hooks (S3), hook_data.cpp
bool arrivals_from_json(const DataJson& d, Params& out, std::string& err);      // item 06, arrival_data.cpp
bool moves_from_json(const DataJson& d, Params& out, std::string& err);         // item 07, move_data.cpp
bool sift_bank_from_json(const DataJson& d, SiftBank& out, std::string& err);   // item 14, sim/sift/bank_data.cpp

// file IO, data_io.cpp (not part of the wasm build)
bool load_data_json(const std::string& dir, DataJson& out, std::string& err);
bool load_params(const std::string& dir, Params& out, std::string& err);
bool load_world(const std::string& dir, const Params& p, World& out, std::string& err);
bool load_policy(const std::string& dir, const std::string& name, Policy& out, std::string& err);
bool policy_from_json(const std::string& text, Policy& out, std::string& err);   // policy_json.cpp

}
