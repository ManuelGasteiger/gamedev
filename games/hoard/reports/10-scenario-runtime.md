# 10 · Scenario runtime, validator, dry run

Branch `item/10-scenario-runtime` from main d1fe9c1. The runtime is namespace `scn` in `sim/scenarios/`; `ages::run_scenario` (the policy runner) is untouched. Nothing paid, no external calls.

## 1. What changed

Added:
- `sim/scenarios/bank.{h,cpp}`: loader of `data/scenarios/*.json` and `vocab.json`; fails on an unknown role, option, field, tag, gate or chooser; bank hash in the data hash.
- `sim/scenarios/predicate.{h,cpp}`: spec section 4 grammar (clauses joined by ` and `, `not`, `bound`, `has_tag`), evaluator; set fields (`person.trait`, `dragon.talents`) compare by membership.
- `sim/scenarios/fields.{h,cpp}`: every vocab field of faction, region, person, item, remnant, act, dragon, lair, world; tag rules; act waking index; the vocab gates.
- `sim/scenarios/bind.{h,cpp}`: role binders with backtracking, act patterns, `is` expressions, `remembers`, `choose`, optional roles, `bind_why`.
- `sim/scenarios/select.cpp`: selection per waking (spec section 6), `wake`, `enable`, `note_seen`.
- `sim/scenarios/present.cpp`, `text_ctx.{h,cpp}`: the briefing's `scenarios[]` block, option act resolution.
- `sim/scenarios/answer.cpp`: `answer_scenario` and outcomes. `sim/scenarios/lab.cpp`: the forced-state world overlay. `sim/scenarios/scn_json.cpp`: state in saves.
- `sim/runner/scn_cmd.{h,cpp}`: `--scenario-bot`, `--scenario-log`, `--scenario-why`, `--lab`.
- `data/bots/scenario_answers.json`, `data/policies/random.json`.
- `tools/scenario_dryrun.py`, `tools/scenario_lab.py`.

Changed:
- `data/scenarios/` (moved from `docs/scenarios/`: the three seed files and `vocab.json`; `vocab.json` gains `gate_rows`). The German draft stays in `docs/scenarios/`.
- `state.h` (`ScnShown`, `ScnState`, `World.scn`), `data.h/.cpp`, `data_io.cpp`, `record.cpp` (data hash, `apply_input` case), `recorded.cpp` (wrapper), `serialise.cpp` (state, only while on), `sleep.cpp` (`scn::wake` after `sift_wake`), `briefing.cpp` (block while on), `person_seen.cpp` (named by a shown scenario).
- `sim/runner/main.cpp`: `--scenarios`, `--min-sleep`, the three scenario flags; replay header `scenarios: true`.
- `sim/wasm/api.cpp`: `load`/`reset` flag `scenarios`, command `answer_scenario`, data `scenarios` and `scenario_vocab` (8 lines). `web/build.sh`, `web/build.ps1`: sources and data (`build.ps1` also lacked `sim/sift`, added).
- `data/constants.json` "scenarios", `data/text/{en,de}/ui.json` (six reason keys, five size words).
- `tools/scenario_check.py` (`--lab`: steps 3 and 4; `--usage`), `tools/interest.py` (`--scenarios`), `CMakeLists.txt` (sources, `text_lint_seeds` glob), `tools/text/review.py`, `tools/text/scenario_strings.py`, `tools/bank/translate.py` (paths), README.
- Seeds: section 9.

## 2. Numbers

Bank off, the runtime does not run: `interest.py` on this branch equals item 09's "after" row exactly (played, 300 generated seeds: entropy 3.89, distinct 39, reversals 6.89, causal 0.122, pacing 0.400, persistence 0.455, agency 0.916, 7334 events, 26 slain), so main and the branch with the bank off are one row. Bank on, played answering by `data/bots/scenario_answers.json` "played" (idle stays bank off as the agency baseline):

| played, 300 seeds, 3000 y | bank off | bank on |
|---|---|---|
| outcome entropy (bits) | 3.89 | 3.96 |
| distinct outcomes | 39 | 43 |
| reversals per 1000 y | 6.89 | 6.87 |
| causal depth | 0.122 | 0.122 |
| pacing CV | 0.400 | 0.401 |
| persistence | 0.455 | 0.461 |
| agency (Jaccard vs idle) | 0.916 | 0.917 |
| events per run | 7334 | 7433 |
| dragon slain (runs) | 26 | 25 |
| scenarios per waking | | 0.13 |
| cited_player_act_share | | 59.6 % |
| slot fill: must, consequence, audience, open | | 0, 7.7, 3.8, 1.4 % |
| top theme share | | 54 % (war) |
| answered | | 99 % |

Acceptance:

| target | result |
|---|---|
| all 30 seeds pass `scenario_check.py` | steps 1, 2 and text lint 30 of 30; with `--lab` (steps 3, 4) 27 of 30. Fail: `door_in_the_temple` (portal_progress is always 0: no portal rite in the sim), `thief_with_the_cup` (no items held by courts), `the_mountain` (terraform option `unknown_terraform`, item 09 question 1). |
| each seed shown >= 3 times in 1800 runs | 24 of 30. Dead: the three above, `princess_returns` (1), `scribe_among_hostages` (0), `the_one_you_spared` (0), `the_walls_went_up` (0). Section 6. |
| none above the hog threshold | not met: over 50 % of runs `the_gold_you_took` 65 %, `the_city_you_saved` 60 %, `prophet_with_the_prophecy` 51 %. Over 2 % of showings: 14 (the mean share of 30 scenarios is 3.3 %, so this rule only works on a full bank). |
| played: `cited_player_act_share` >= 60 % | 59.6 % |
| played: consequence slot >= 40 % of wakings after the first act | 7.7 %. 14 consequences, and played only burns, takes, kills, defends, gives. |
| `answer_scenario` round-trips | yes: seed 42 schemer: 19 scenario acts in `act_log.json`, each `source: scenario` with `answers` = the scenario id, every shown entry `answered`; `--record` then `--replay`: 45 checkpoints, 23 recorded answers. |
| goldens unchanged with the bank off | yes: `state_hashes.txt` 300 of 300, `ctest -L fast` 24 of 24. |

Dry run (`tools/scenario_dryrun.py`), 300 generated seeds x 6 bots x 3000 years: 1800 runs, 55,404 wakings, 10,226 shown (0.18 per waking), consequence share 47 %. Eligible % is of the wakings in the scenario's eras; wake is the median waking when shown.

| scenario | kind | elig % | shown | runs | run % | show % | wake |
|---|---|---|---|---|---|---|---|
| burned_library_scribes | consequence | 0.10 | 47 | 47 | 2.6 | 0.46 | 8 |
| door_in_the_temple | world | 0.00 | 0 | 0 | 0.0 | 0.00 | - |
| dwarves_ask_for_fire | audience | 0.10 | 43 | 43 | 2.4 | 0.42 | 14 |
| envoy_who_lies | audience | 0.01 | 5 | 5 | 0.3 | 0.05 | 5 |
| false_dragon | world | 0.12 | 51 | 51 | 2.8 | 0.50 | 24 |
| hero_at_dawn | consequence | 0.37 | 201 | 201 | 11.2 | 1.97 | 17 |
| inquisition_opens | world | 0.21 | 80 | 80 | 4.4 | 0.78 | 17 |
| kin_of_a_hostage | audience | 1.84 | 664 | 437 | 24.3 | 6.49 | 7 |
| lich_crown_speaks | lair | 0.06 | 34 | 34 | 1.9 | 0.33 | 24 |
| pilgrims_in_winter | audience | 3.23 | 1397 | 435 | 24.2 | 13.66 | 14 |
| princess_returns | consequence | 0.00 | 1 | 1 | 0.1 | 0.01 | 13 |
| prophet_with_the_prophecy | audience | 2.10 | 926 | 926 | 51.4 | 9.06 | 9 |
| quiet_valley | omen | 1.56 | 812 | 304 | 16.9 | 7.94 | 18 |
| refugee_column | audience | 0.44 | 180 | 165 | 9.2 | 1.76 | 9 |
| scribe_among_hostages | consequence | 0.00 | 0 | 0 | 0.0 | 0.00 | - |
| slayer_at_the_ledge | audience | 0.58 | 312 | 312 | 17.3 | 3.05 | 20 |
| the_army_you_broke | consequence | 0.61 | 321 | 321 | 17.8 | 3.14 | 14 |
| the_city_you_saved | consequence | 2.00 | 1073 | 1073 | 59.6 | 10.49 | 5 |
| the_consort | lair | 0.10 | 51 | 51 | 2.8 | 0.50 | 34 |
| the_dead_you_raised | consequence | 0.11 | 62 | 62 | 3.4 | 0.61 | 11 |
| the_gold_you_took | consequence | 2.31 | 1172 | 1172 | 65.1 | 11.46 | 4 |
| the_heir_who_grew_up_here | consequence | 0.68 | 378 | 378 | 21.0 | 3.70 | 4 |
| the_mountain | lair | 0.66 | 363 | 363 | 20.2 | 3.55 | 30 |
| the_oath_they_remember | consequence | 0.60 | 331 | 331 | 18.4 | 3.24 | 18 |
| the_one_you_spared | consequence | 0.00 | 0 | 0 | 0.0 | 0.00 | - |
| the_pupils | consequence | 1.38 | 661 | 661 | 36.7 | 6.46 | 6 |
| the_road_of_your_shadow | consequence | 1.16 | 564 | 378 | 21.0 | 5.52 | 9 |
| the_walls_went_up | consequence | 0.00 | 0 | 0 | 0.0 | 0.00 | - |
| thief_with_the_cup | world | 0.00 | 0 | 0 | 0.0 | 0.00 | - |
| two_crowns | world | 1.37 | 497 | 497 | 27.6 | 4.86 | 10 |

Bank metrics: depth after an act median 0, 5th percentile 0 (targets 6 and 2 assume the full bank). Top theme share 41 % (faith; target 15 %). Themes never shown: ascension, cult, elves, exile, humans, plague, portal, thief, trade. Repeatables over 3 per run: kin_of_a_hostage, pilgrims_in_winter, quiet_valley, the_road_of_your_shadow. Too loose: none. Too tight (under 1 % of their era's wakings): 21.

Vocabulary usage (`scenario_check.py --usage`): themes 28 of 34 (unused ascension, elves, exile, humans, plague, trade); endings 4 of 11; races 1 of 5 (dwarf); gates 12 of 14 (unused gold_rumour, has_library); nouns 20 of 23 (unused historian, liar, reader); choosers 10 of 10; top paths dragon.lore 24, faction.dread 15, faction.awe 14, faction.cohesion 11; 29 option act types (ignore 30, give 10, demand 7, receive 7); act roles burn 3, ignore 3, take_hostage 2, ten others 1.

Cost: a run with the bank and a bot takes about 1.3 s more (schemer seed 3: 3.5 s against 2.2 s).

## 3. Decisions taken

- Namespace `scn`; the policy runner stays `ages::run_scenario`.
- The runtime runs only while `World.scn.on` (runner `--scenarios`, wasm `scenarios: true`); `World.scn` is saved and hashed only then. The bank is always loaded and in the data hash (only the replay header's data warning notices).
- Selection runs in `sleep()` at wake, after the sifter, and writes `shown`; the briefing reads it. The waking index is the number of `sleep` acts; `age_wakings` 0 is the waking before the latest sleep.
- `owner_of` an act: the court it fell on. `remembers`: a court memory entry for that act whose tier stock (by the entry's axis) is >= `remember_stock` 0.1.
- Tags: a court has a tag when it or one of its regions carries it; a person when they carry it or their court carries `<tag>:<person id>` (false_dragon, inquisition); `x` matches `x:<anything>`. Person tag `slayer` = on a roster, `hostage` = in the lair.
- Person roles beyond the sim's five: `hostage`, `envoy` (at the ledge, else the offer's envoy, else heir, ruler, sage, priest, hero as `envoy_for`), `historian`, `consort`, `champion`, `ward` (`kept:<role>`).
- `renown`: chronicle lines naming the person, 10 for 1. `knows_dragon`: in the ledger or holding a memory of the dragon.
- Items: relics in the hoard; `kind: artifact` is any relic; `in: court` binds nothing.
- `world.ignored_streak`: wakings in a row with at least one `ignore`. `world.chronicle_level` as `line_level` (2/3 with a historian, 1 with a source). Because-lines show from level 1. `world.year` counts from the hatching. `portal_progress` and `cult_fervor` read 0, `stamina` 1.
- Vocab gates map to arrival gate rows (`vocab.json gate_rows`); `dares` is the envoy's blocked_when row negated, `faith_share_min` and `awe_min` the pilgrim's rows; `motive` also holds while the court's envoy waits with an offer.
- Option acts: `$self` = the scenario, an act role as target = its court, payload `who`/`about`/`item`/`region` refs = `{kind, id}`, `$slider` = given magnitude or 0.5. Swear on a court targets its offer; bless and ward on a court name its first region; receive of a named person names their arrival. An option failing the act's field check is dropped (`dropped`); a failed gate or act gate greys it with the reason.
- Theme share applies once `total x 0.15 >= 1`. Pity skips the quotas for the consequence slot.
- Every `answer_scenario` is recorded, refused or not: a refused routed act can still write `spell_failed`, and replays diverged before this.
- Profile `seen.scenarios` (id -> count), frozen into `World.scn.seen_before` at run start.
- Bots: burner `raze_all`, sleeper-hoarder `rest_only` with `--min-sleep 100` (rest_only sleeps one year at a time: 3000 wakings, minutes per run), protector `protect_one`, `priest`, `schemer`, random `data/policies/random.json` (the five in turn, random answers). Bots never answer with `sleep` or `exit`.
- `lab_world` is runner-only (`--lab`), so it has no recorded wrapper.

## 4. Deviations from the brief

- Step 3 runs only with `--lab`, so `text_lint_seeds` keeps its old scope (door_in_the_temple fails step 3).
- No `card_match` reuse: one evaluator over the vocab fields.
- The 2 %-of-showings hog rule and the depth target are reported, not enforced.

## 5. Goldens

Unchanged; `kRulesVersion` stays 11.

Checks run at the end: `ctest -L fast` 24 of 24 (incl. `text_lint_seeds`, `golden`, `replay_verify`); `hoard-sim --golden 1-300` equal to `state_hashes.txt`; wasm build, `node web/smoke.mjs --golden 1-60` (both chronicle goldens MATCH, 60 state hashes equal); `node web/l10n-check.cjs --lang pseudo,de` passed; `node web/game-check.cjs` 90 of 90; `node web/check.cjs` PLAYWRIGHT OK. Through the wasm API: `load {scenarios: true}`, a shown scenario in `briefing.scenarios`, `answer_scenario` accepted, a second answer refused with `scn.answered` worded "already answered", the report replay holds the answer and `scenarios: true`. `web/ages.wasm` and both pages are rebuilt and committed; `web/sandbox-check.png` is not.

## 6. Open questions

1. Seeds needing a sim feature or a rewrite: `door_in_the_temple` (no portal rite), `thief_with_the_cup` (no court items), `the_walls_went_up` (nothing builds walls), `the_mountain` option (terraform mountain).
2. Too tight for the bots: `the_one_you_spared` (only scenario options spare; schemer judges guilty), `princess_returns` (1 in 1800), `scribe_among_hostages` (castles court with a library that remembers a recent hostage). Loosen or keep rare?
3. Hogs over 50 % of runs: the_gold_you_took, the_city_you_saved, prophet_with_the_prophecy. Lower weight, tighten, or wait for the bigger bank?
4. Repeatables over 3 per run: allow, or cap `once: never` per run?
5. Played: cited share 59.6 % and consequence fill 7.7 %; both need consequences for burn, take, kill, defend, give (item 11).
6. `incite` only on orcs (item 09 question 2) shows open and is refused in inquisition_opens and the_gold_you_took unless the rival is orc.
7. Confirm the bot preference lists and the sleeper's `--min-sleep 100`.
8. Map defences to court tags (`has_tag cannons` never holds)?

## 7. How to verify in five minutes

```
cmake --build build -j2 && ctest --test-dir build -L fast
python3 tools/scenario_check.py docs/scenario.schema.json docs/act-api.schema.json data/scenarios/vocab.json data/scenarios/seed-*.json --lab --bin build/hoard-sim --usage
./build/hoard-sim --data data --seed 3 --years 3000 --genesis --policy schemer --scenario-bot schemer --scenario-log /tmp/s.jsonl --record /tmp/s.hrp --out /tmp/s3
./build/hoard-sim --data data --replay /tmp/s.hrp
python3 tools/scenario_dryrun.py --seeds 1-10 --out /tmp/dry
```

Look at `scn` in `world_final.json`, the `source: scenario` acts in `act_log.json`, `scenarios` in `briefing.json`.

## 8. Proposed tests (not added)

1. `scenario_select_determinism`: same world, log and bank twice give the same shown list.
2. `scenario_cites`: a `burn(library)` two wakings ago makes `burned_library_scribes` eligible with that act in `cites`; with the court's memory below `remember_stock`, not.
3. Golden `scenarios_seed_v1.hash` (seed 42, played, bank on, played bot).
Also possible: `scenario_check.py --lab` as a ctest; a replay of a recorded bank-on run.

## 9. Seed changes

Logic only; no text changed, so `data/text/*/scenarios.json` are unchanged.

| seed | change | runtime showed |
|---|---|---|
| hero_at_dawn | hero `memory_has: sworn_to_memory` -> `tag: slayer`; salience `has_tag(hero, sworn_to)` w 3 | no such memory line (heroes get tag `sworn_to:<mark>`); fight takes roster members only |
| the_one_you_spared | spared `memory_has: spared_by_dragon` -> `tag: pardoned`; broker secondary `$court` -> new optional role `rival`, gate `and bound(rival)` | judge(spare) writes `pardoned`; broker with itself is `act.no_target` |
| princess_returns | agent `memory_has: schooled:intrigue` -> require `has_tag(agent, schooled:intrigue)` | a tag, not a memory line |
| scribe_among_hostages | keep gate `and has_tag(hostage, lettered)` | keep(historian) refuses an unlettered hostage |
| slayer_at_the_ledge | reveal `about: $slayer` -> `$court` | reveal(slayer_plan) reads the order's court |
| envoy_who_lies | reveal `about: $envoy` -> `$court` | reveal(lie) reads the liar's court |
| the_consort | option dismiss act -> `release $willing` | dismiss needs the person in the lair |
| the_gold_you_took | option defend: `swear` -> `defend $court` against `$rival` | swear needs an envoy's offer; none comes here |
| the_walls_went_up | demand payload `structure: walls` | `demand.no_structure` |
| the_mountain | require might 400 -> 150, salience 600 -> 250; terraform target -> new optional region role `ground`, gate `dragon.might >= 250 and bound(ground)` | might stayed under 330 in 40 measured runs; terraform takes a region |
| the_army_you_broke | act role: dropped `payload_has: [who]`, magnitude [0.4, 1] -> [0.3, 1] | kills carry no `who` key and hook kills are 0.3 |

## 10. Changes to existing tests and checks

- ctest: none changed; `text_lint_seeds` glob now `data/scenarios/`.
- `scenario_check.py`: new `--lab`, `--data`, `--usage`; default behaviour unchanged.
- `interest.py`: new `--scenarios`; default output unchanged.
- `web/build.ps1` lacked `sim/sift` since item 14; it now compiles `sim/sift` and `sim/scenarios`.

## 11. Hardest spec rules

1. `remembers` "at a tier that has not faded": entries keep their tier, not their own stock, so the check reads the court's stock at that tier.
2. Pity "forces the best eligible consequence": the slot is filled anyway when something is eligible, so forcing means skipping the quotas.
3. Audiences: `arrival.needs` are gates, but the sim's visitor may not have come, so an audience can show while `receive` is refused (`no_arrival`).
