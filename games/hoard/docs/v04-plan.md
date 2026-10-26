# v0.4 implementation plan and contract

Design: `docs/core-loop.md`. This file is the build contract the parallel work follows. Numbers below are starting values; tune them in data, never in code.

## Work packages

| package | branch | owns | depends on |
|---|---|---|---|
| S1 dragon | `dragon` | growth economy, talents, phases from points, health and wounds, defences, death, slay changes, Premonition, the oath fix | – |
| S2 world | `world` | genesis (terrain deal, names, start deck), pre-history, irreversible events, undead paths, storyteller, interest tool | – |
| S3 hooks | `hooks` | wake hooks: generation, resolution, consequences | S1 merged |
| W page | `page` | everything in `web/` for the above | contract below; real API after merges |

Shared files (`state.h`, `serialise.cpp`, `sim/wasm/api.cpp`, `data.h`, `data.cpp`, `data_io.cpp`, `constants.json`, `CMakeLists.txt`, `web/build.sh`, `web/build.ps1`, `web/smoke.mjs`): edit them only additively, in a block of your own (new struct fields at the end of the struct, new commands at the end of `dispatch`, new constants in a new top-level section of `constants.json`), so merges stay mechanical.

Rules for every package: C++17 that also compiles on MSVC, no exceptions across the API, sorted iteration and `world.rng` only (determinism), every number in data, short files with one job each, no comment essays. Exactly the six existing tests: do not add tests; keep them green; when behaviour changes on purpose, re-record both golden hashes and say so; baseline bands (test 2) hold with genesis off. Base rules change only where this plan says so.

## Data files

New files are loaded like the existing ones: a field in `DataJson`, `data_io.cpp`, the embed lists in `web/build.sh`, `web/build.ps1` and `web/smoke.mjs`. Missing files load as empty (the sim must still run without them).

- `data/talents.json` (S1): `axes` [{id, name, fed_by, rank_every, rank: {effects, look, text}}], `nodes` [3, 8, 15, 25], `node_phase` [1, 1, 1, 2], `talents` [{id, axis, node, name, text, effects: {key: number}, look: {key: value}}].
- `data/defences.json` (S1): `defences` [{id, label, min_era, min_anti_dragon, tags_any [], races_any [], ward_modifier: bool, per_act, passive, radius, burn_mult, era_scale}], `fear_damp` 0.6, `hero_bonus` 5.
- `data/genesis.json` (S2): terrain deck and rules, region name parts per terrain, start templates, pre-history range, the irreversible events' numbers, storyteller numbers.
- `data/hooks.json` (S3): hook templates (when, resolve, reward, ignore, fight, text keys).

## Effects (talents and ranks)

`dragon.effects` is the merged map of every taken talent and rank. Keys ending in `_mult` multiply (default 1), keys ending in `_add` add (default 0), every other key is a flag or level and takes the maximum (default 0). S = read by the sim, P = read by the page.

| key | side | meaning |
|---|---|---|
| armor_mult | S | all wound damage |
| arcane_armor_mult | S | bolt-thrower and cannon damage |
| health_add | S | health max |
| heal_mult | S | healing per sleep |
| slay_mult | S | success of every slaying |
| premonition | S | a slaying that is about to be tried wakes you instead (wake reason `premonition`) |
| stamina_add | P | raid stamina |
| speed_mult | P | flight speed |
| burn_mult, burn_cost_mult | P | magnitude per stamina, stamina per burn |
| fury | P | after a wound, burns ×1.3 for the rest of the age |
| kill_mult, kill_cost_mult | P | Kill army |
| crush | P | the Crush action (kill soldiers there plus a show to every neighbour) |
| witness_mult | P | share of witnesses kept |
| sight | P | defences' ranges and damage shown everywhere |
| burn_spread | S | share of a burn that spreads to the owner's other regions next to it |
| burn_linger | S | burned regions recover at half speed (region tag `clinging_fire`) |
| ember_mult | S | embers from burns |
| ward_breaker | S | wards no longer halve burns |
| molten_gold | S | take from a region with burn ≥ 0.3 gives gold ×1.5 |
| tower_breaker | S | a burn wrecks the region's defences until the next sleep (region tag `defences_wrecked`) |
| fear_mult | S | fear from the dragon's acts |
| memory_mult | S | half-life of every memory of the dragon |
| oath_steadfast | S | clauses never fail from faded fear or faith |
| spell_cost_mult | S | spell costs |
| ward_duration_mult | S | ward modifiers last longer |
| lair_ward | S | the lair region carries a permanent ward (no sieges) |
| undead_cost_mult | S | raise undead |
| forecast_mult | S | forecast noise |
| hex | S | a curse also halves the target's anti-dragon |
| demand_mult | S | knowledge needed to Demand |
| lie_sight | S | offers in the world view carry `revealed_lie` |
| digest_mult | S | non-gold growth points |
| bed_mult | S | Might from the gold bed |
| teach_mult | S | teach modifier duration |
| hostage_lore_mult | S | Lore from royals |
| tribute_mult | S | tribute |
| faith_mult | S | faith from acts |
| thief_guard | S | treasure hunters fail, no thief hook |
| hunt_mult | S | nourishment from the hunt |
| cult | S | the most faithful court keeps faith ≥ 0.5; pilgrims hook every waking |
| brood | S | dragon-raised courts keep faith ≥ 0.6; slay_mult 0.8 |
| size_mult | S | size per Might point |
| magic_per_sleep_add | S | magic added each sleep |
| lore_mult, faith_gain_mult | S | ranks |

`look` keys (page only): `plates` dark|metal, `fire` gold|white|blue, `wings` scale, `spikes`, `horns`, `runes`, `crest`, `eyes`, `veins`, `shadow`, `aura`, `hoard_glitter`, `banners`.

## World view additions (`cmd: world`)

```
dragon: {
  ...v0.3 fields,
  growth: {might, flame, arcana, lore, dominion},          // permanent points
  next_node: {might: 8, ...},                              // points of the next node (or next rank) per axis
  talents: ["thick_hide", ...],                            // taken, in order
  ranks: {might: 0, ...},
  picks: [{axis, node, points, needs_phase, open, options: ["thick_hide", "deep_lungs"]}],   // pending choices
  effects: {key: number}, look: {key: value},
  wounds, health_max,
  hoard: {...v0.3, embers}
}
regions[i]: { ...v0.3, name, defences: [{id, label, per_act, passive, radius, burn_mult, wrecked}] }   // damage after fear and hero, before armor
hooks: [{id, kind, title, text, region, faction, person, resolve: {acts: ["kill"], region, target, person},
         reward_text, ignore_text, fight, expires_year}]        // S3
genesis: {template, prehistory_years, hatched_year}             // S2
phase: {...v0.3, points, next_phase_points}
```

## Commands

- `talents` → `{ok, axes, nodes, node_phase, talents}` (the catalogue).
- `pick_talent {id}` → `{ok, events}`; fails when the node is not open or the id is not one of its options.
- `reset {seed, genesis}` and `load {..., seed, genesis}`: `genesis: true` builds the world from `genesis.json` and runs the pre-history. Default false (tests, goldens).
- `legends` → `{ok, events: [{year, text, importance}]}`: the pre-history's highlights, at most 10, chronological.
- `apply_acts` → also `{wounds: {taken, total, max}, slain, hooks_resolved: [...]}`. Harmful acts (burn, take, kill, take_hostage) at a defended region wound the dragon; act `wound` {target, region, magnitude, item: defence id} is passive damage from the page (armor applies).
- `sleep` → also `{growth_gained: {axis: points}, healed, hooks}`.

Events: `talent_taken`, `dragon_wounded` (subject the court, magnitude damage, tags the defence ids), `dragon_slain` (cause `defences` when wounds did it), `slayers_coming` (premonition), `hook_resolved`, `hook_ignored`, `race_extinct`, `people_arrived`, `cataclysm`, `age_of_magic`, `necromancer_rose`, `dead_rose`, and the storyteller's.

## S1 numbers to start from

- Digestion on the phase clock: window W = the phase's long sleep; half-life of kind k = `hl_frac[k] × W` with books 0.25, art 0.5, royals 0.5, artifacts 1.0, embers 0.3, food 0.1; a moult doubles digestion as before.
- Points: Lore 0.12 per book and 4 per royal; Arcana 0.15 per artifact; Dominion 0.12 per art and 0.05 per tribute gold; Flame 2.5 per ember; Might `0.4 × sqrt(gold) × years / W`. Pools: knowledge +1 per book, magic +1 per artifact, faith pool +0.5 per art (as before), size +0.05 per Might point.
- Embers: a burn adds its magnitude × `ember_mult`.
- Phases from total points: 25, 90, 250, 600, then a moult as before.
- Health 100 + 3 per Might point. A sleep heals `health_max × min(1, years / short window) × heal_mult`.
- Slaying success × (1 + wounds / health_max) × slay_mult. Tune so the played policy dies in 5 to 25 % of 3000-year runs and idle in under 5 %.
- Hunger: gold no longer digests; keep the policies test's hunger expectations by tuning data.
- Oath fix: a tribute clause holds while faith > `oath_tribute_min_faith` or fear ≥ `oath_fade_fear`; a court under a tribute oath pays when faith > `tribute_faith` or fear ≥ `demand_fear`.

## Order

S1 and S2 run in parallel, W starts on the contract; merge S2, then S1; S3 on the merged tree; W wires the real API; then the wasm build, smoke, game check, interest run, scripted playthroughs and fixes.
