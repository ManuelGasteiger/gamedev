# 01 · Interest tool, sleep sweep, agency metrics: report

Branch `item/01-interest-baseline`, on main after item 17. Numbers: `reports/01-baseline-v04.md`.

## 1. What changed

Sim (read-only hooks for analysis, nothing hashed):
- `Policy`: `sleep_open` and `avoid_rings` fields, scenario callbacks for analysis; hooks carry their refs at runtime (not saved).
- Runner: `--snapshots` (dragon and court stocks per century), `--wakings` (per waking: listed wounds, hooks with refs, towers, ring acts), `--buckets` (sleep copies of the world at every waking), `--perturb burn|sleep`, `--replay ... --out` writes the core outputs. The analysis code is in `sim/runner/analyse.{h,cpp}`.
- Policies: `rest_only`, `moult_only`, `long_1000`, `raid_then_1000`, `newcomer`.

Tools:
- `tools/sweep.py`: the sweep, buckets, lethality, perturbation, first hour, replays per seed (`out/sweep/replays/`), `--recorded run.hrp`, `--policy-seeds`, `--selftest`.
- `tools/interest.py`: agency chain metrics, `--bands`, `--sweep summary.json`; the first-three-events check skips the founding instant.
- `data/bands.json`: 8 v0.4 bands from core-loop 7 and 8, 6 proposals from research book 4.3 and first-hour.
- `docs/prompts/01-interest-baseline.md`: the brief with the audit corrections.

Goldens unchanged (300 state hashes, chronicle hashes and texts). `ctest -L fast` passes. No new ctest tests.

## 2. Findings, ranked

1. **Waking often beats sleeping long by an order of magnitude.** `rest_only` (the phase's short window every time) gains might 180 and flame 776 per century in the first 500 years against 18 and 3 under `played`, answers 149 hooks in the first 150 years against 7, and lost no dragon in 10 seeds. The bucket table points the other way (3000 years wins growth in 42 % of states) only because it scores total points per bucket. Under these rules the sleep bet has one right answer for growth: wake as often as the window allows. I have not traced which rule pays per waking; item 03 (digestion) is where growth per year gets defined, so it should start from this number.
2. **One act flips the ending in two of three seeds.** Burning 0.1 more at the first burn, or sleeping one bucket longer once, changes the ending class (alive or dead, killer race, largest race, hegemon) in 64 % of 100 seeds; the proposal band is 5 to 30 %. At the same time played reaches only 47 distinct outcomes with entropy 4.03 bits (the v0.4 band is 4.5, the one failed v0.4 band) and orcs are the largest race in 206 of 300 played worlds. Small inputs move the result, but among few results: the world funnels.
3. **A hatchling that burns everything dies at once.** `raze_all` loses the dragon at year 0 after the hatching in 35 of 300 seeds and at year 100 in 17 more, all to wounds. The proposal band (no death before year 80 under any policy) fails as a warning, as you decided. Played and newcomer never die before 80. Lethality (listed wounds of a waking's acts over health) was measured under played and long_1000 only, where it stays under 1 outside era 2 to 3 phase 4 to 5; I have not traced what kills raze_all at year 0.

Also measured: hooks reaching a dragon act within 3 hops 13.5 %, events 7.0 %, waking headlines 66.1 %. Played deaths 21.3 % (band 5 to 25 %), 0 unwarned.

## 3. Decisions taken

- Bucket growth follows the brief (most points over the bucket). Points per year would need per-bucket points stored; left for item 03 since finding 1 already shows the rate gap from the sweep itself.
- Survival "decisive" states are the 53.5 % where buckets do not all tie; the band uses the decisive row.
- `rest_only` capped at 10 seeds (`--policy-seeds`), since one seed takes over 3 minutes.
- Hook refs are kept at runtime only, so saves and hashes did not move.

## 4. Deviations from the brief

- `rest_only` on 10 seeds instead of 300 (time).
- Buckets on 100 seeds (`--bucket-seeds`), as the brief allows.

## 5. How to verify in five minutes

```
cd games/hoard
cmake -S . -B build && cmake --build build -j && ctest --test-dir build -L fast
python3 tools/sweep.py --selftest
python3 tools/interest.py --bin build/hoard-sim --data data --seeds 1-20 --years 3000 --policy played --genesis --bands
```
