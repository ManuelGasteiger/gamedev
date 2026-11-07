# 08 · Slayer roster

Model: Sonnet 5.5 · Effort: medium · Depends on: 02, 07 · Branch: `item/08-slayer-roster`

## Goal

Slayer orders become rosters of named people with rank, power, traits, scars and a memory of the dragon, in the Shadow of Mordor pattern, on top of the existing slayer ladder (forming, ready, attempt) and the slayer order card. The briefing names the nemesis and his scars, unknown traits show as `?` until revealed, and the `fight` act (item 09) resolves against a named slayer.

## Notes for this tree (2026-11-07)

- Rules version is 10 (`sim/ages/record.h`). Items 08 and 09 run in parallel; in your `golden:` commit set `kRulesVersion` to 11. The goldens are regenerated once more when the two are merged. Golden recipe: `reports/14-story-sifting.md`, plus the replays and the save check (see `reports/06b-rare-visitors.md`).
- The slayer ladder is in `rules/slayers.cpp` (`slayer_clock`, the stages `slayers_forming` and `slayers_ready`, `slayer_strength`, `disperse_slayers`). Since 06b a band marches only on a lair its court knows, and only while the slayer-band arrival is off its 600-year cross-court cooldown. Played deaths are 5 per 100 runs. Keep those gates and say in the report how the roster changes the death rate.
- People since item 07: `Person` in `state.h` has `traits[]` from the 12-trait list, `acquired[]` and `mem[]` with derived dread, awe and debt. The memory rules are in `rules/person_memory.cpp`, risers in `rules/people_rise.cpp`. Heroes exist as role `hero`; 04b left their number unchanged on purpose. Use existing people as slayers where you can, and say how slayers count against the cap of 8 named people per court.
- Since 04b the player sees only people the dragon has met or learned of (`sim/ages/person_seen.cpp`). A slayer named in a briefing's slayer line is learned of: add that route there.
- `fight` lands in item 09. You provide `resolve_fight` (scope 7) in its own file, with a header item 09 can include. The hooks already have fight answers (`hooks.cpp`, `hook_resolve.cpp`, the `fight_*` keys in `data.h`); leave them unless `resolve_fight` should replace their damage path, and say which in the report.
- New rolls draw from `rng_for(seed, tag, id)` (`sim/ages/rng.h`), never `world.rng`, so no existing draw moves. This replaces the design note below.
- Player text goes in `data/text/<lang>/*.json` with an `en` and a `de` line for every key.
- Tests: add none. List the proposed tests in the report; Manu decides.
- Before you finish: `ctest --test-dir build -L fast`; the wasm build (`git lfs pull --include "games/hoard/assets/gltf/**"` first) and `node web/smoke.mjs --golden 1-60`; `node web/l10n-check.cjs --lang pseudo,de`; `node web/game-check.cjs`; `node web/check.cjs`. Push the branch; no pull request.

## Read first

- `docs/research-social-sim.md` section 3 (the patent's record shape, promotion rules, scars, what Monolith cut) and its "For Hoard: slayer roster".
- `docs/core-loop.md` section 4 (defences, wounds, death; the slayer warning ladder) and section 8 (adaptation).
- `docs/design-references.md` sections 3 and 6 (fair permadeath; the nemesis roster proposal; the dragonbane bar as a later candidate).
- `docs/event-catalogue.md` "The slayer order", "The hero's challenge" hook in `core-loop.md`.
- `docs/act-api.md` 2.1 `fight`, 2.4 `reveal(slayer_plan)`, 2.5 `haunt`.
- The slayer code in `rules/slayers.cpp`, the cards for `slayer_order`, `data/defences.json`, the hero logic (17.2).

## Scope

In:

1. **Order** entity: `{id, court, founded, grievance_ref (the act or event), ranks: {grandmaster: person?, masters[≤2], slayers[≤6], hunters: int}, power_total, traits_known_to_dragon[]}`. Founded when the existing `slayers` tag is gained and a `grief` memory of severity ≥ 3 about the dragon survives at strength ≥ 0.3 in that court.
2. **Slayer person** fields (on the existing person record from item 07): `rank`, `power 1..4 → cap 12`, `traits`: one counter from {fireward (Flame), plate (Might), runeward (Arcana), waxed_ears (Dominion), forewarned (Lore)}, one hate trigger from {smoke, banners, gold, the dragon's roar}, one fear from {fire, heights, the dark, the dead}; at least one weakness at every power level; `scars[]` matching the cause of a survived encounter (burn, claw, tail, deaf); `known: {trait: bool}` per trait for what the dragon has learned.
3. **Order turn** each decade in `rules/slayers.cpp` (after the slayer-order card): each named slayer rolls a mission weighted by power relative to the dragon's threat: train (+0 power, +1 hunter), hunt a lesser beast (+1 power at 0.3), seek a ward or weapon (gains an artifact at 0.1 if world magic allows), march (only at stage `ready`). Surviving an encounter with the dragon: a scar and +1 power. Killing a dragon ally (envoy, consort, vassal army) promotes. A fallen master is replaced by the `opportunist` trait holder or the highest power. A harmed slayer's rival gains +1. Fifty years of peace: −1 power unless `zealot`. The grandmaster's death passes 50 % of negative memory strength to the successor.
4. **Relationships** capped at three kinds: rival, mentor, kin. Rivals compete for promotion; a mentor's death adds a `grief` memory to the pupil.
5. **Ladder integration**: `forming` when the order exists with ≥ 1 named slayer; `ready` when `power_total ≥ threat × ready_ratio` (start 0.8); the attempt uses the existing success formula with `× (1 + 0.1 × grandmaster.power)` and `× counter_mult` if the grandmaster's counter matches the dragon's highest axis. Adaptation (core-loop §8) applies to `power_total` growth.
6. **Telegraph**: the briefing's slayer lines name the grandmaster, his scars, his known traits, `?` for unknown, band power, and the estimate of the attempt year. Traits become known through: a `fight` (all of the fought slayer's traits), a hostage from that court (one trait), a `reveal(slayer_plan)`, a source (`take books`) about the order, or Dragon-sight.
7. **`fight` resolution** (the act lands in item 09; this item provides `resolve_fight(World&, PersonId slayer) → {dragon_wounds, slayer_outcome: dead | fled_scarred, artifact?, traits_revealed[]}`): wounds = `base × power × counter_mult × (1 − fear_match)` where `fear_match` is 1 if the dragon's act family triggers the slayer's fear; the slayer dies at `dragon_power_ratio > 1.5`, else flees with a scar and a `humiliated` memory; a listed artifact drops on death.

Out: the act plumbing (item 09), the valley-wide dragonbane bar (later candidate), orc-to-orc style relationships beyond the three kinds.

## Design notes

- The patent gave no trait counts; keep the lists short (5 counters, 4 triggers, 4 fears) and extend only through data.
- A slayer order survives the court's collapse (passes to a successor) and spreads to allies as today.
- Determinism: mission rolls use `world.rng`; iteration over slayers by id. (Superseded: use `rng_for`, see the notes above.)

## Acceptance

- Under `played`, 300 seeds: orders form around year 400 as today, each with a named grandmaster by stage `ready`; the death recap names the slayer when one struck last.
- Every slaying while asleep is preceded by a briefing line naming the grandmaster (the fair-death metric stays at 0 unwarned deaths).
- Over the same runs: a slayer survives at least one encounter and returns promoted and scarred in ≥ 30 % of seeds where an attempt happened.
- Goldens: order turns add events; separate golden commit.

## Proposed tests (confirm with the owner first)

1. `slayer_promotion`: a scripted attempt that fails leaves the grandmaster alive with a scar and +1 power; the briefing names him.
2. `slayer_weakness`: raising a slayer to power 12 never removes the last weakness.

## Report

Standard report, plus a table of grandmasters for ten seeds (name, scars, known traits at the dragon's last waking, outcome).
