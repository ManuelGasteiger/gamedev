# 03 · Digestion consumes the hoard: report

Branch `item/03-digestion`, from main after item 02 (rules 3). The brief file was missing from that tree; I cherry-picked the docs-only commit `dbb62b8` from `item/02b-defend-awe` to get it (and the research docs it lists). Nothing else of 02b is in this branch, so `kRulesVersion` goes 3 to 4 here and will collide with 02b's own bump when the lead merges.

## 1. What changed

- `sim/ages/hoard.cpp`: `bed_mode consumed` (default). Per step the gold above `gold_floor_base × floor_phase_mult[phase]` shrinks by `(1 - gold_burn)^(slept / long window)`, and Might rises by `gold_pts` per gold eaten (times `bed_mult`). `slept` is the existing bed time: `bed_overtime` past the first long window, doubled in a moult, a tenth for a rest. `bed_mode window` keeps the old `bed_rate × sqrt(gold)` bed and eats nothing. Gold is not nourishment.
- `data/constants.json` (`dragon`): `bed_mode`, `gold_burn` 0.15, `gold_floor_base` 10, `floor_phase_mult` [1, 2, 4, 8, 16], `gold_pts` 1.0. Loaded in `dragon_data.cpp`, checked there (5 multipliers, mode is one of two), declared in `data.h`.
- `sim/ages/forecast.cpp`, `serialise.cpp`, `ages.h`: forecast returns `hoard_after`, `growth_gained` and `healed`. With `healed` present the page uses the forecast's numbers instead of its own estimate (`web/game/bet.js` reads both or neither). They are not blurred (decision below).
- `sim/tests/digestion_consumes.cpp` and its line in `CMakeLists.txt`: 1000 gold, 10 books, phase 2, one long window. Gold ends at `1000 - 0.15 × (1000 - floor)`, books at the half-life remainder, Might at `gold_pts × eaten`, Lore at the digested books × 0.3.
- `tools/sweep.py`: top growth policy per seed, median year of phase 2 and 3, growth buckets with buckets within 2 % of the best counted as tied, resumable runs (`<out>/cache`), a guard for `--perturb-seeds 0`.
- README: file list and constants line.
- Golden commit: `kRulesVersion` 4, state hashes, both chronicle hashes and the three chronicle texts, replays, `v1-seed4.expect`, wasm, both HTML pages. The leftover `seed-N.txt.new` files that were tracked are gone.

## 2. Numbers

Sweep: `tools/sweep.py --seeds 100 --years 3000 --jobs 2 --policy-seeds rest_only=10 --bucket-seeds 100 --perturb-seeds 100` (6049 s on a loaded machine). Interest tool, 300 seeds played genesis. Before is the sweep after item 02.

Targets:

| target | before | after | result |
|---|---|---|---|
| (a) raid_then_1000 top growth policy | 0 % | 0 % | met (it was already met) |
| (b) every one of seven buckets best at ≥ 10 %, none > 40 %: survival, decisive | 28.0 / 20.9 / 14.5 / 9.1 / 8.4 / 9.1 / 9.9 % | 28.1 / 21.1 / 14.4 / 9.3 / 8.6 / 9.2 / 9.3 % | not met: 3 of 7 at ≥ 10 %, top 28 % |
| (b) growth | 0 / 1.9 / 2.5 / 4.2 / 17.5 / 32.8 / 41.0 % | 0 / 2.0 / 2.5 / 4.1 / 15.6 / 35.2 / 40.6 % (with 2 % ties: 18.3 / 33.3 / 39.8 for 300 / 1000 / 3000) | not met: 3 of 7 at ≥ 10 %, top 40.6 % (39.8 % with ties) |
| (c) played deaths 5 to 25 % | 9 % | 9 % in the sweep, 10.3 % in the 300-seed interest run (8 wounds, 23 slaying) | met |
| (d) phase 2 by about year 50, phase 3 before 800 (played, median) | not recorded | 40 and 390 | met |

Top growth policy per seed: raze_all 28, long_1000 25, played 24, moult_only 23, raid_then_1000 0 before; played 41, moult_only 30, raze_all 18, long_1000 11, raid_then_1000 0, newcomer 0 after. The raiding policies now lead and the 1000-year sleeper falls from 25 to 11.

Other sweep rows, after (before in brackets where given in the 01 and 02 reports): deaths moult_only 8 % (6), long_1000 12 % (3), played 9 % (9), raze_all 79 % (74), newcomer 0, raid_then_1000 0. Phase at year 3000 for played: p3 10, p4 81. Might at year 100: played 39.2 (41.8 before), moult_only 38.0 (40.0), long_1000 7.5 (7.9), newcomer 13.9 (15.6). Lethality under played: no cell above 1 except era 2 phase 4 (0.6 % of 814). Perturbation flips 40 % (burn), 42 % (sleep), either 63 % (40, 43, 64 % before). No death before year 80 except raze_all (as before).

rest_only (10 seeds) now ends at phase 1 with 2 deaths by slaying; it ate almost no gold at a tenth of a window per sleep. Before it had no deaths.

Bands (`interest.py --bands`, 300 seeds): played entropy 3.815 (fail, as in v0.4 and after 02: 3.869), idle entropy 4.885, played death share 0.103, hook coverage 0.985, unwarned deaths 0, first-three share 0.080. All bands that passed after 02 pass. `ctest -L fast` passes (14 tests); `ctest -R "policies|baseline"` passes, so the raze_all hunger wake (15.8) holds; food is counted once, through `digest`.

Seed 42, first waking, classic data load: 40 gold, floor 10. Forecast for 10 years: `hoard_after.gold` 35.5, Might +4.5; for 100 years 32.0 and +8.0; for 1 year 39.5 and +0.48. The brief's "180 gold" does not occur: genesis hatchlings hold 40 to 60.

Tuning rounds (30 seeds, floor / burn / pts):

| round | played deaths | played phase 2 / 3 (median year) | note |
|---|---|---|---|
| a 50 / 0.15 / 0.08 (brief start) | 63 % | 65 / 660 | Might starved: 1.8 per century against 18 before |
| b 10 / 0.15 / 0.1 | 67 % | 60 / 630 | same cause |
| c 10 / 0.15 / 1.0 | 7 % | 40 / 390 | kept |
| d 10 / 0.15 / 0.5 | 10 % | 40 / 510 | Might at year 100 24.9 against 41 before |

The three constants that mattered: `gold_pts` (the whole Might scale: at equilibrium the gold eaten equals the gold raided, so Might per century is `gold_pts` times the raid income), `gold_floor_base` (50 left nothing to eat in a 40 to 60 gold hoard), and `bed_mode` (the window mode lets the 1000-year sleeper win, finding 1 of the paper model). `gold_burn` stayed at 0.15.

Page checks on the rebuilt page: `web/game-check.cjs` 90 of 90, `web/l10n-check.cjs --lang pseudo,de` passed. One existing check changed: `v04.sleep_bet` required the "(estimate)" label on the sleep screen; the forecast now sends the numbers, so the label is gone and the check no longer asks for it. (The first page build embedded Git LFS pointers for the models, because the clone had none; I pulled the assets and rebuilt before committing.)

## 3. Decisions taken

- Floor 10 and `gold_pts` 1.0 instead of the brief's 50 and a value giving +3 Might on 180 gold. Reason above. `phase_mult`, half-lives and `bed_rate` untouched (`bed_rate` only matters in window mode).
- The eaten fraction uses the exponential form so that one window eats exactly `gold_burn` of the excess whatever the step length.
- Forecast `hoard_after`, `growth_gained` and `healed` are exact. The blur exists for the world's uncertainty, and the lair is the dragon's own; a blur of 0.5 / (1 + knowledge) could also put `hoard_after.gold` outside floor and current gold.
- Growth buckets within 2 % of the best count as tied (an extra row; the strict row is kept for comparison). The tool picks the best bucket by exact maximum, and a hoard that is used up leaves the 300, 1000 and 3000 buckets a rounding apart (seed 3: 25.2, 31.3, 31.6).
- Resumable sweep cache, because background jobs are cut at two hours and one 100-seed run took 6049 s on the shared machine.

## 4. Deviations from the brief

- Scope item 1 says non-gold categories convert but stay in the hoard. In this tree `digest` already sets `stock = remaining`, so they leave already; only gold changed.
- Start values changed as in section 3.
- Target (b) is not met, see section 6.
- The page's own estimate in `web/game/bet.js` still uses `bed_rate × sqrt(gold)`. It is only a fallback now (the forecast sends the numbers), and I left it alone as out of scope.
- `kRulesVersion` 4 collides with 02b.

## 5. Goldens

Changed: all state hashes, both chronicle hashes, three chronicle texts, replays, save expectation (rules 4). Seed 1's chronicle first differs at 660, the waking after the hatching: a wound of 12.871 is 12.7466, because Might (health and size) now comes from gold eaten. Over the run: 106 fewer burns, 94 fewer collapses, 23 fewer atrophy and 18 fewer starving events, one more dragon death (`dragon_slain` 0 to 1). Seed 2 first differs at 520, a different talent pick, then more acts overall (+42 burns). Seed 3 at 610 (wound 12.9292 to 12.8616), then +168 burns. The wasm agrees with native on seeds 1 to 60 (`node web/smoke.mjs --golden 1-60`).

## 6. Open questions for the owner

1. Target (b) cannot be met with these levers. Survival is the odds of a slayer band being ready during the sleep, and the short buckets win it because the band has less time; digestion does not touch that. The levers that would: a slayer clock that does not favour short sleeps, or a survival bonus for the long sleep (for example the lair warding of Sleeper's Ward as a base effect). Growth is scored as total points over the bucket, so 1, 10 and 30 years cannot win; per-year points or points relative to the bucket's best would let them. Which do you want?
2. Is `gold_pts` 1.0 the right Might scale? It reproduces the old pace (phase 2 at 40, phase 3 at 390) but makes Might proportional to raid income by construction. A hoard bigger than the floor by much is never a goal now, since the dragon eats it.
3. rest_only now stays in phase 1 and loses 2 of 10 seeds. If rests should pay, `gold_burn` could scale with the rest window instead of the long one.
4. 02b also bumps `kRulesVersion` to 4; the lead regenerates the goldens at the merge anyway.

## 7. How to verify in five minutes

```
cd games/hoard
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j2 && ctest --test-dir build -L fast
./build/test_digestion_consumes
python3 tools/interest.py --bin build/hoard-sim --data data --seeds 1-60 --years 3000 --policy played --genesis --bands --jobs 2
node web/smoke.mjs --golden 1-60
```

The 100-seed sweep is the command in section 2; rerun it until `summary.json` exists (finished runs are cached).
