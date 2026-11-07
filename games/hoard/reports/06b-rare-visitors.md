# Item 06b: rare visitors, slow lair knowledge

Branch `item/06b-rare-visitors` from origin/main 4659c29. Follow-up of item 06 after the owner's play: "visitors should be more rare and special. And the location of the lair should take a while to figure out for a court." No to the current prophet, thief and slayer frequency; no to a show at a capital telling a court where the lair is. Nothing paid.

## What changed

- `sim/ages/state.h`, `serialise.cpp`: `Faction.lair_known` and `lair_exact` replaced by two stocks, `lair_oral` and `lair_written` (0 to 1).
- `sim/ages/arrival_gates.cpp`: `learn_lair(w, p, f, evidence, scale)` raises the stocks by evidence kind; `lair_level` (0 unknown, 1 rough, 2 exact); `note_lair_sight` now only reads flights home and released hostages; `fade_lair_knowledge` replaces `refresh_lair_knowledge`. `lair_on_record` reads the stocks.
- `sim/ages/step.cpp`: `fade_lair_knowledge` runs each step before `rule_arrivals`.
- `sim/ages/memory_tiers.{h,cpp}`: `halflife_of` exported (lair knowledge fades at the court's oral half-life); `burn_library` also burns `lair_written`.
- `sim/ages/arrivals.{h,cpp}`: `arrival_resting` (per-court cooldown, world cooldown per kind, one visitor of a kind at the ledge, the kind seen at the last waking) and `arrival_noted`; `envoy_returned` takes `Params`.
- `sim/ages/rules/arrivals.cpp`: uses `arrival_resting` / `arrival_noted`.
- `sim/ages/rules/slayers.cpp`: a band readies only when its court knows the lair at least roughly and the `slayer_band` row is not resting (`band_may_march`); a failed slaying teaches the court the way in (`returned`).
- `sim/ages/rules/card_match.cpp`: card fields `lair_known` / `lair_exact` read `lair_level` (the treasure hunters card).
- `sim/ages/data.h`, `arrival_data.cpp`, `data/arrivals.json`: `lair` block (rough, exact, written_share, flight_falloff, evidence, caps, no_flight), `world_cooldown_years` per kind, `ledge_no_repeat`, `ledge_one_per_kind`, `fired_keep_years`; `oral_floor` and `lair_memory_years` removed; `sight_regions` 2 to 3; cooldowns raised.
- `sim/runner/analyse.cpp`, `tools/interest.py`: wakings log carries each court's lair level; the tool prints arrivals per 1000 years, same kind on consecutive wakings, counts per run in the first 40 wakings, and years to rough and exact knowledge.
- `sim/tests/arrival_gating.cpp`, `arrival_cooldown.cpp`: expectations adjusted (below).
- `web/game-check.cjs`: `seeLair()` sets `lair_oral = lair_written = 1`.
- `README.md`: arrivals and lair knowledge paragraphs.

## Lair knowledge: every path, before and after

Before (main) `lair_known`/`lair_exact` were set by:

| Path | Before | Now |
| --- | --- | --- |
| `note_lair_sight`: a `show` at the court's capital | rough | removed (owner: no) |
| `note_lair_sight`: any witnessed act within 2 regions of the lair | rough, at once | kept as `flight`: 0.12 per waking at distance 0, x0.7 per region out to 3 regions, capped at 0.5 (below exact 0.7); the closest sighting per court and waking counts once. Courts with land within 3 regions also see the flight home. `show` and `receive` show no flight. Justification: the dragon flying home is real evidence, but a sighting never shows the entrance |
| `note_lair_sight`: `release` of a hostage | exact | kept as `hostage` 0.6. Justification: the person lived in the lair |
| `arrivals_depart`: a waiting visitor walks home at the sleep (not thieves or slayers) | exact | kept as `returned` 0.5. Justification: pilgrims or envoys who came back |
| `receive_outcome` (not `eat`) | exact | kept as `returned` 0.5, same reason |
| `envoy_returned` (offer accepted or declined) | exact | kept as `returned` 0.5, same reason |
| `arrival_ignored`: a thief gets away | exact | kept as `thief` 0.5. Justification: he stood in the hoard and came back |
| failed slaying (new) | none | `returned` 0.5: the survivors bring the way home |

Each gain is `gain x (1 - stock)`, so knowledge builds: two returned visitors take a court from nothing past exact (0.5, 0.75), one gives rough. A court that keeps records (`keeps_records`: sage, historian, lettered ruler, library) writes 0.6 of each gain into `lair_written`. The oral stock fades at the court's oral awe half-life (60 y at human memory, scaled by race; a sage doubles it), the written at `written_halflife` (250 y); a burned library cuts the written stock like dread and awe. A court that saw a few flights and then nothing forgets within a lifetime or two; a court with a library keeps it for centuries.

Not built, because the sim has nothing to read: a captured or interrogated dragon servant (no servant entity; `kobold_servants` is a talent effect only) and a source item that records the lair (`HeldSource` is the dragon's, not a court's). The written stock stands in for "a library that records it".

## Numbers

`tools/interest.py --policies played --genesis --seeds 1-100 --years 3000 --jobs 2`, main 4659c29 against this branch.

Arrivals per waking after year 200:

| Kind | Before | After |
| --- | --- | --- |
| envoy | 0.310 | 0.013 |
| prophet | 0.750 | 0.075 |
| slayer band | 0.423 | 0.060 |
| thief | 0.574 | 0.005 |
| pilgrim | 0.035 | 0.006 |
| refugee | 0.026 | 0.013 |
| kin | 0.009 | 0.002 |
| total | 2.13 | 0.17 |

Per 1000 years: before envoy 4.61, kin 0.13, pilgrim 0.52, prophet 11.18, refugee 0.38, slayer band 6.30, thief 8.56; after envoy 0.18, kin 0.03, pilgrim 0.09, prophet 1.04, refugee 0.18, slayer band 0.83, thief 0.07.

Per run in the first 40 wakings (mean, runs average 36 wakings): before envoy 11.8, prophet 23.1, slayer band 10.6, thief 21.7, pilgrim 1.7, refugee 1.3, kin 0.35; after envoy 0.55, prophet 3.0, slayer band 1.9, thief 0.25, pilgrim 0.36, refugee 0.8, kin 0.08.

Same kind on consecutive wakings (share of waking pairs): before prophet 0.279, slayer band 0.123, thief 0.118, envoy 0.091; after 0 for every kind.

Years until a court knows the lair (from the hatching or the court's founding, first waking at the level, courts that ever reach it):

| | Before | After |
| --- | --- | --- |
| rough: courts reaching it | 4032 of 7262 (56 %) | 586 of 7622 (8 %) |
| rough: median years | 30 | 270 |
| exact: courts reaching it | 790 (11 %) | 61 (0.8 %) |
| exact: median years | 170 | 1080 |

Other played measures: slain 11 to 5 seeds of 100 (wounds 2, slaying 3; unwarned 0), outcome entropy 3.77 to 3.63 bits, hooks 1.80 to 1.66 per waking, coverage 98.6 to 97.9 %. No run without an arrival.

Target numbers (mine; the owner gave none): over a run of 30 to 50 wakings (decision 2026-10-23) each kind a few times, which I read as 1 to 4, and never the same kind on consecutive wakings: prophet 2 to 3, slayer band 2 to 3, envoy 2 to 4, thief 1 to 2, pilgrim 1 to 2, refugee and kin up to 1; about 0.2 to 0.3 visitors per waking. Derived from the run length and the owner's "rare and special": with 8 kinds, 2 to 4 each is 10 to 20 visitors in 40 wakings. Prophet, slayer band and refugee land in the band; envoy (0.55), thief (0.25) and pilgrim (0.36) are below it, because all three now need lair knowledge (envoys a written record or exact knowledge, thieves exact), which is the slow part the owner asked for. Raising them means faster lair knowledge or looser gates; I did not trade one request against the other.

How the rates came down: cooldowns per (court, kind, cause) 100 to 300 years; new world cooldowns per kind across all courts (envoy 400, pilgrim, scholar, kin, refugee 500, thief and slayer band 600, prophet 700); one visitor of a kind at the ledge at a time; a kind seen at the last waking does not come in the sleep after it (`ledge_no_repeat`). The slow lair knowledge does the rest for envoys, thieves, pilgrims and kin.

Checks: `ctest -L fast` 24/24 after the golden commit (only `golden` failed before it). wasm build ok; `node web/smoke.mjs --golden 1-60` wasm equals native on 60 seeds and both chronicle hashes; `node web/l10n-check.cjs --lang pseudo,de` passed; `node web/game-check.cjs` 89/89, no console errors (main had 90: `v02.tasks_late` is now skipped, see Deviations).

## Tests: expectation changes

- `arrival_gating`: the setup set `lair_known = year`, `lair_exact = false`; those fields are gone, so it sets `lair_oral` halfway between `lair.rough` and `lair.exact` (rough, not exact). Assertions unchanged.
- `arrival_cooldown`: the setup sets `lair_oral = 1` instead of the two removed fields. The test checked no repeat at +20, +60, +99 and a repeat at +101 years; the cooldown is now 300 per court and 400 for envoys anywhere, so it checks no repeat at +20, +60, +99 and `cd - 1`, and a repeat at `cd + 1`, with `cd = max(cooldown_years, world_cooldown_years)` read from the data. Message text "within 100 years" became "within the cooldown".

## Decisions taken

- Rough from 0.3, exact from 0.7 of the larger stock; evidence gains flight 0.12 (cap 0.5), returned 0.5, hostage 0.6, thief 0.5; written share 0.6; flight seen to 3 regions at x0.7 per region. All mine, in `data/arrivals.json`.
- "Never the same kind on consecutive wakings" read as: a kind waiting at waking N does not arrive in the sleep after N (it may come again one sleep later). Configurable with `ledge_no_repeat`.
- One visitor of a kind at the ledge at a time, from any court (`ledge_one_per_kind`). The old rule only stopped the same court twice.
- The slayer clock now respects the arrival row: a band readies only on a known lair and while `slayer_band` is not resting. Without it the clock kept forming bands that attacked whether or not they could arrive, and the arrival row was only a mirror. Warnings and attacks stay together (unwarned deaths 0). This lowers deaths under played from 11 to 5 per 100 runs.
- `lair_on_record` (envoys) holds for a written lair stock at rough, for written memory of the dragon plus rough knowledge, or for exact knowledge.
- Lair knowledge fades at the court's oral awe half-life (not dread's, which a talent can shorten).
- `fired_keep_years` 300 keeps the per-court cooldown keys; world keys (`*|kind`) and ledge keys are never pruned (one per kind).

## Deviations

- No servant capture or interrogation and no lair-recording source item: no sim state to read (above).
- Arrivals per waking (0.17) are below item 06's acceptance band (0.5 to 1.5); the owner's words replace that band.
- `web/game-check.cjs` changed, forced by the rarer envoys (no assertion weakened, one conditional check now skipped):
  - `seeLair()` writes the new fields and also deletes `arrival_fired`, so the envoy cooldowns of centuries do not outlast the lab.
  - d (protect offer from a court at war): waits up to 40 centuries instead of 12; with one envoy at the ledge at a time it found the protect offer in year 3590.
  - v02.envoys: waits for one envoy in person instead of two. Two envoys can no longer wait at once (`ledge_one_per_kind` plus the world cooldown), so the lab eats the one that came and `v02.tasks_late` (accept one of two) is skipped; accepting an offer stays checked by d.accept and v02.tasks.
  - c.era read the settlement meshes of `hill_hold` alone. The history now reaches that point about 35 centuries later, when hill_hold no longer changes era; the check now reads, at the end of the sleep that showed the era, the regions of every court whose era rose (or that was founded with an era above the first) and asks that one of them changed its meshes.

## Goldens

One `golden:` commit, `kRulesVersion` 8 to 9: `state_hashes.txt` (300 seeds), `ages_v0_1_raze.hash`, `sift_bank_v1.hash`, the three chronicle texts (the tracked `seed-N.txt.new` leftovers removed), `tests/replays/played-1..3.hrp` re-recorded (verify: 36, 39, 29 checkpoints), `tests/saves/v1-seed4.expect`, and the wasm and pages. `ages_v0.hash` (fixed start, idle) did not change: no arrivals there. Chronicle diff: the histories run equal until the first arrival that main had and the branch lacks (seed 1 year 700 a thief, seed 2 year 510 an envoy, seed 3 year 630 an envoy), then diverge. Arrivals in the three texts: seed 1 183 to 10, seed 2 97 to 5, seed 3 110 to 5; slayings 3 to 3, 0 to 0, 7 to 1.

## Open questions

- Envoys, thieves and pilgrims come under once per run. Accept, or should returned visitors and libraries teach faster so envoys reach 2 to 4 per run?
- With bands limited by the arrival row, slayer deaths halved. Is that the danger level you want, or should the slayer band's world cooldown come down (600 to 300)?
- Prophets are now the most common visitor (3 per run). Item 04's prophecy gate is still the stand-in (`awe_eff` at `cult_awe`).

## How to verify in five minutes

- `cmake --build build -j2 && ctest --test-dir build -L fast`
- `python3 tools/interest.py --bin build/hoard-sim --data data --seeds 1-20 --years 3000 --policies played --genesis --jobs 2` and read the `arrivals` and `lair known` lines.
- `./build/hoard-sim --data data --seed 1 --genesis --policy played --years 3000 --out out/s1` and count `arrival` events in `out/s1/chronicle.json`.
