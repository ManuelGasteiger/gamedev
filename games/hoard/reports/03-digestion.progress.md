# 03 · Digestion: progress

Branch `item/03-digestion`, from main after item 02 (rules 3). The brief came in by cherry-picking `dbb62b8` (docs only) from `item/02b-defend-awe`.

## Done

- `hoard.cpp`: `bed_mode consumed` eats the gold above `gold_floor_base × floor_phase_mult[phase]` at `gold_burn` per long window slept (`excess × (1 - burn)^windows`) and gives `gold_pts` Might per gold eaten; `window` keeps the old sqrt bed. Constants in `data/constants.json` under `dragon`.
- Non-gold categories already left the hoard in the tree (`digest` sets `stock = remaining`), so scope item 1 is gold only.
- Forecast: `hoard_after`, `growth_gained`, `healed` (exact, not blurred).
- Test `digestion_consumes`. `golden` fails until the golden commit.
- `tools/sweep.py`: top growth policy per seed, median year of phase 2 and 3, tied growth buckets, resumable runs (`<out>/cache`).
- Tuning on 30 seeds (floor / burn / pts): a 50/0.15/0.08 (played deaths 63 %, Might starved), b 10/0.15/0.1 (67 %), d 10/0.15/0.5 (phase 3 at 650), c 10/0.15/1.0 (deaths 7 %, phase 2 at 40, phase 3 at 390, raid_then_1000 top in 0 %). c is committed. At equilibrium the gold eaten equals the gold raided, so `gold_pts` sets the Might scale; floor and burn set how large the hoard stays.
- Finding: hatchlings hold 40 to 60 gold, so the brief's floor of 50 leaves nothing to eat. The "180 gold" of the brief is not what genesis hatchlings hold.

## Next

- Final 100-seed sweep: `python3 tools/sweep.py --seeds 100 --years 3000 --jobs 2 --policy-seeds rest_only=10 --bucket-seeds 100 --perturb-seeds 100 --out <scratch>/sw-final`. Background jobs are cut at 2 h and the machine is loaded; rerun the same command until `summary.json` exists (finished runs are cached).
- Then: interest bands, `ctest -R policies` (raze_all hunger), forecast check on seed 42 through the wasm, report, golden commit, wasm and page checks, push.
