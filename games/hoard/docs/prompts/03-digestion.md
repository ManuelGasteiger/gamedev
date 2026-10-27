# 03 · Digestion consumes the hoard

Model: Sonnet 5.5 · Effort: medium · Depends on: 01 (uses the sweep) · Branch: `item/03-digestion`

Corrected 2026-10-27 to the tree: item 02 is merged (dread and awe, research spending wealth, rules version 4); the sweep is `tools/sweep.py`, the bands are `data/bands.json`; text keys live in `data/text/en/narrator.json`. Tests (Manu, 2026-10-27): `digestion_consumes` only.

## Goal

Decision 2026-10-23: sleeping on the hoard consumes it, partially. Each sleep converts a share of every hoard category into growth and that share is gone; the gold bed keeps a floor so a hatchling's first moult still pays. The aim is that raids stay necessary all game and that no single sleep bucket dominates. This is a data-and-one-rule item, tuned against the sweep.

## Read first

- `docs/core-loop.md` section 3 (growth axes, digestion on the phase clock, bed rate, phases).
- `docs/ages-sim.md` sections 6 (digestion v0), 15.6 (hunger), 17.4 (phases and tempo).
- `docs/research-book-advanced-game-design.md` sections 3.4 (the dragon has no braking engine) and 3.8 items 1 and 5.
- `reports/01-baseline-v04.md`, the sweep table and the bucket-win table; `reports/02-dread-awe-memory.md` for the numbers after item 02.
- `hoard.cpp` and the constants it reads.

## Scope

In:

1. **Consumption.** In `hoard.cpp`, the digested fraction of books, art, artifacts, embers, food and royals leaves the hoard (today it converts but stays). Gold: the bed still gives Might at `bed_rate × sqrt(gold)` per moult window, and in addition `gold_consumed = gold_burn × max(0, gold − floor)` per moult window slept, start `gold_burn 0.15`, `floor = gold_floor_base × phase_mult[phase]` (start 50 × [1, 2, 4, 8, 16]). Rests consume a tenth, as digestion does.
2. **Forecast and bet.** `forecast` returns the hoard after the sleep per category and the growth it buys, so the sleep screen can show both sides (item 13). Add `hoard_after` and `growth_gained` to the forecast result; the page already reads `growth_gained` when present.
3. **Hunger interplay.** Consumed food counts as nourishment as before. Check that a long sleep on a small hoard still wakes by hunger (15.6) and that consumption does not double-count food.
4. **Tuning** against the sweep with these targets, in order of priority: (a) `raid_then_1000` is the best growth policy in fewer than 40 % of seeds; (b) each of the seven sleep buckets is the best choice (survival or growth) at ≥ 10 % of played wakings, and none at more than 40 %; (c) played deaths stay within 5 % to 25 %; (d) the played dragon still reaches phase 2 by about year 50 and phase 3 before year 800. Tune only `gold_burn`, `gold_floor_base`, `phase_mult`, the half-lives and `bed_rate`. If (a) and (b) cannot both be met with these levers, report the best trade-off and the lever that would be needed.

Out: new growth axes, talents, Might upkeep (rejected in favour of consumption), anything in the page beyond the forecast fields.

## Design notes

- **The bed must not pay forever.** The paper model (`docs/pacing-model.md`, finding 1) shows that if the gold floor keeps yielding Might per moult window, a 1000-year sleep dominates every other policy regardless of consumption. So Might comes from the gold consumed: `Might += gold_pts × consumed_gold` per step (start `gold_pts` so that seed 42's first moult on 180 gold still gives about +3 Might), and `bed_rate × sqrt(gold)` is removed or kept at a tenth of today's value. The floor is what the dragon keeps; it grows nothing. Decided 2026-10-24 (`decisions.md`); implement it behind a constant (`bed_mode: consumed | window`) with `consumed` as the default so the sweep can show the difference once.
- The gold floor exists so that the first moult on a small hoard still grows Might; check seed 42 hatchling: 180 gold, 10-year moult, Might +3 as in the paper playtest.
- Royals: a digested royal is "the heir grows up in the lair" (17.2); consumption here means the lineage effect fires once and the royal leaves the hoard count, which already happens through persons. Make sure the person record is the source of truth and the hoard count is derived.
- Determinism: consumption is arithmetic on existing state; goldens will change in value, not in event order. Record them in the separate golden commit.

## Acceptance

- Sweep rerun: the four targets above, reported as a table with before and after.
- `forecast` on seed 42 at the first waking shows `hoard_after.gold` below the current gold and above the floor.
- Hunger wakes still occur in `raze_all` without food (15.8 band holds).
- Bands: all `v0.4` bands that passed after item 02 still pass.

## Tests (confirmed by the owner)

1. `digestion_consumes`: a hoard of 1000 gold, 10 books, phase 2, one moult window: gold after is `1000 − 0.15 × (1000 − 100)` within rounding; books after is the undigested remainder from the half-life; Might and Lore rose by the documented amounts.

## Report

Standard report, plus the sweep and bucket-win tables before and after, and the three constants that mattered most.
