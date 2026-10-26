# 02 · Dread and awe, memory tiers

Model: Opus 5.5 (or Fable 5.1) · Effort: high · Depends on: 01 · Branch: `item/02-dread-awe-memory`

Corrected 2026-10-26 to the tree: narrator text keys live in `data/text/en/narrator.json` (item 19), importances in `data/importance.json`; D5 taken (valleys that remember dragons start as data, here and in 12).

## Goal

Replace the single `fear` stat with two independent meters per court, dread and awe, each held as stocks in three memory tiers (oral, written, monumental) with their own half-lives. Wire every rule that read `fear` to the right meter, add the unlock ladder, make research spend wealth, and let a burned library erase what a court knew. This is the foundation items 04 to 13 read.

## Read first

- `docs/act-api.md` section 6 (the update rule, the per-act gain table, memory tiers, unlocks). These are the start values; tune, don't invent.
- `docs/research-faith-and-culture.md` sections 3 and 5 (where the numbers come from and why).
- `docs/research-social-sim.md` section 1 "For Hoard" (memory entries with source and strength; the same idea at person level, which item 07 builds; keep the court-level shape compatible: `{act_id, year, tier, strength, axis}`).
- `docs/ages-sim.md` sections 4.7 (memory rule), 5 (reactions table), 7 (sleep), 17.2 and 18.4 (sages and priests as keepers of memory).
- `docs/story-and-endings.md` section 4.
- `reports/01-baseline-v04.md`.

## Scope

In:

1. **State.** `Faction.fear` becomes `Faction.dread` and `Faction.awe`, each `{oral, written, monumental}` floats 0..1 plus derived `dread_eff`, `awe_eff` computed as `1 − Π(1 − x_k)`. `Faction.memory[]` entries gain `tier` and `axis` (`dread`, `awe`, `faith`). Add `Faction.monuments[]`: `{region, act_id, bias: dragon_temple | memorial | neutral, built}`.
2. **Update per act** in `acts.cpp` from the reaction rows: `dD = g_D · m · w · (1 − D_eff)`, `dA = g_A · m · rep · w · (1 − A_eff)`, `rep = max(0.25, 0.5^n)` over same-type acts witnessed in the last 30 years, `show` awe × `(1 − D_eff)`. Witness weights: owner 1.0, neighbour 0.5, a court with a sage or envoy present at the target 0.5. Deposits: oral at full strength; written at 0.6 when the court has a `library` region tag, a sage, or a historian; monumental at 0.4 when a monument exists in the witnessing court. Gains table in `data/reactions.json`, one row per act (and sub-kind), replacing the fear column. Keep the relationship-dependent rows (oath active, faith > 0.5, power ratio) as multipliers on the base gains, not separate numbers.
3. **Decay** in `rules/memory.cpp`: per tier half-life from constants (start: oral 30 y, written 250 y, monumental 1000 y). Race `memory_halflife` scales the oral tier only (elves never forget orally; dwarves 500). Sage or priest present: oral decay × `sage_memory_mult` as today. `burn` with structure `library`: written stocks × 0.1 for that court. A monument destroyed (`burn` structure `temple` or `altar`, or the region razed to wasteland) removes its monumental share.
4. **Unlocks** as derived flags read by existing rules (`rules/dragon_rolls.cpp`, `oaths.cpp`, cards): pilgrims at `A ≥ 0.15`; cults may found and honest envoys at `A ≥ 0.30`; `consecrate` and faith floor 0.3 at `A ≥ 0.50`; demands accepted at `D ≥ 0.30` (replaces `demand_fear`); slayer formation at `D ≥ 0.50` with pressure `D · (1 − 0.7·A)`; garrison flight at `D ≥ 0.70` with defence × `(1 − D)`. Oath stability `1 − 0.8·D·(1 − A)` replaces the fear-faded breach rule for `no_walls` and `no_settle`.
5. **Research spends wealth.** `anti_dragon` progress per decade = `base × era_mult × dread_factor × wealth_share`, with `dread_factor = 1 + 2·D·(1 − 0.7·A)`, and the court's wealth loses `research_spend × progress` the same decade. The product dread × wealth is the arms race in one line (`research-book-advanced-game-design.md` 3.7); keep both factors visible in constants.
6. **Fear mapping.** Every remaining read of `fear` is replaced: threat, slayer and tribute logic read `dread_eff`; faith regrowth, pilgrims, offers and oaths read `awe_eff`; the briefing and the wasm `world` export expose both, plus the tiers, plus `memory_tier` (highest tier with stock ≥ 0.1).
7. **Monuments.** A `dragon_temple` monument arises from `consecrate` (item 09) or from a card when awe ≥ 0.5 holds for 50 years; an anti-dragon `memorial` arises from the slayer-order card or a sack the dragon caused. Until item 09 lands, add the card-driven version so monuments exist in the sim. A monument deposits to monumental and biases: a temple converts half of newly deposited dread into awe, a memorial the reverse. Each monument records the region where the act happened (or the capital when the region is gone) so the shells can place it (`docs/world-aging.md` §2 and §3).
8. **Talents.** The fear talents (Terror, Dread Flame, Long Memory, Glamour, Dragon-king) take their dread, awe and tier terms from `docs/talents-v2.md`; `data/talents.json` is rewritten to that list in this item for the five axes' existing nodes (the flight talents' numbers are read by item 16, so include their data fields now with the v0.4 flight values as defaults).

Out: person-level memory (item 07), cult fervor and tenets (item 12), the page (item 13), `consecrate` as an act (item 09; this item adds only the monument object and the card).

## Design notes

- **Mortal memory must not get shorter than v0.4's.** The paper model (`docs/pacing-model.md`, finding 2) shows that a 30-year oral half-life erases dread between 100-year sleeps, and with the written tier settling around 0.3 to 0.45 no slayer order ever formed under moult-length sleeps. Check slayer founding under `played` first (v0.4: around year 400; accept 300 to 600). If it collapses, tune in this order: (1) a slayer order may also form from a recorded grievance, a written dread stock ≥ 0.25 (item 08 says the same from the person side), (2) oral half-life 60 to 100 years scaled by race (`memory_halflife` keeps its meaning), (3) the 0.5 threshold. Do not touch the research formula to fix it.
- Serialization and `load_world` must read old saves: a save with `fear` loads as `dread.oral = fear`, `awe.oral = faith × 0.5`, written and monumental 0.
- `forecast` returns projected `dread_eff` and `awe_eff` per court and the slaying odds under the new pressure formula.
- The reaction table's text keys (`reactions` in `data/text/en/narrator.json`) gain rows for awe-positive outcomes (`defend.saved`, `give.gift`, `show.awe`) so narrated lines exist for the new axis.
- Expect goldens to change. Record new goldens in a separate commit after reading the diff for one seed end to end; the report lists the first ten events that differ and why.

## Acceptance

- Baseline (200 × 3000, idle): every `v0.4` band in `data/bands.json` still passes.
- Played policy: deaths between 5 % and 25 % (unchanged band); slayer orders form only in courts with `dread_eff ≥ 0.5` at founding.
- A `protect_one` run shows the protected court's `awe_eff` above 0.5 by year 500 and pilgrims arriving; a `raze_all` run shows `awe_eff` under 0.1 everywhere and research spending visible as a wealth dip in the burned courts.
- Library burn test: a court at written tier with `dread_eff` 0.6 drops to its oral stock alone the decade after `burn(library)`.
- Sweep (`tools/sweep.py`): `raid_then_1000` is no longer the top growth policy in more than 50 % of seeds, or the report says so and names the lever (item 03 follows).

## Proposed tests (confirm with the owner first)

1. `dread_awe_update`: one `burn` 0.4 on a neutral court gives `dread.oral ≈ 0.14` and `awe.oral ≈ −0.04` clamped to 0; a second `burn` within 30 years gives half the awe loss (`rep`).
2. `memory_tiers`: a court with a library deposits written at 0.6 of oral; after `burn(library)` the written stock is ×0.1; after 250 years without acts the written stock is about half.
3. Extend `reactions` (existing test 3c): the strong-court row now raises `anti_dragon` and spends wealth.

## Report

Standard report, plus the dread/awe/memory_tier table for the five courts of seed 42 at years 100, 500, 1500 under `played`, and the interest tool's agency metrics before and after.
