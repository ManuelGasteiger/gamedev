# 03 · Digestion: progress

Branch `item/03-digestion`, from main after item 02 (rules 3). The brief came in by cherry-picking `dbb62b8` (docs only) from `item/02b-defend-awe`.

## Done

- `hoard.cpp`: `bed_mode consumed` eats the gold above `gold_floor_base × floor_phase_mult[phase]` at `gold_burn` per long window slept (`excess × (1 - burn)^windows`) and gives `gold_pts` Might per gold eaten; `window` keeps the old sqrt bed. Constants in `data/constants.json` under `dragon`.
- Non-gold categories already left the hoard in the tree (`digest` sets `stock = remaining`), so scope item 1 is gold only.
- Forecast: `hoard_after`, `growth_gained`, `healed` (exact, not blurred).
- Test `digestion_consumes`. `golden` fails until the golden commit.
- `tools/sweep.py`: top growth policy per seed, median year of phase 2 and 3.

## Next

- Tuning rounds (scratch variants via `variant.py`, 30 seeds): a = 50/0.15/0.08 (brief start), b = 10/0.15/0.1.
- Hunger check in raze_all, seed 42 forecast check.
- Final 100-seed sweep, interest bands, report, then the golden commit, wasm and page checks, push.

## Finding so far

Hatchlings hold 40 to 60 gold, so a floor of 50 leaves nothing to eat and Might stalls (seed 42: Might 1.1 at year 70, phase 1; before: 46 at year 150, phase 2 at year 40). The brief's "180 gold" is not what genesis hatchlings hold.
