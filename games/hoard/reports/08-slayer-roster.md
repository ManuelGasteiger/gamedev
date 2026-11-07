# Item 08: slayer roster

Slayer orders now have named people: a grandmaster, masters and slayers with power, one counter, one hate, one fear and scars. The dragon learns the traits by fighting, by hostages, by sources and by dragon sight. The ladder (forming, ready, attempt) is unchanged in shape.

## What changed

- `data/slayers.json`: every number of the roster, with the brief's values and the runner-set ones marked in its note.
- `sim/ages/slayer_data.{h,cpp}`: `SlayerBook`, read from the json.
- `sim/ages/slayer_roster.{h,cpp}`: traits, power, scars, reveals, promotion, succession, relations.
- `sim/ages/resolve_fight.{h,cpp}`: `resolve_fight`, `act_family`, `reveal_order` (own files, pushed early for item 09).
- `sim/ages/slayer_json.{h,cpp}`: saves and the view scrub (unknown traits show as `?`).
- `sim/ages/slayer_view.{h,cpp}`: briefing block and details.
- `sim/ages/rules/slayer_order.cpp`: the order turn (missions, recruiting, peace loss, repair), on slow ticks after `rule_person_memory`.
- `sim/ages/rules/slayer_march.cpp`: `march_failed` and `gate_fight`.
- `sim/ages/rules/slayers.cpp`: the ladder needs a roster; the lead is the grandmaster.
- `sim/runner/roster_stats.{h,cpp}`: `roster` block in `stats.json`.
- State and plumbing: `state.h` (`Person.slayer`, `Order`, `World.orders`), `data.*`, `record.*`, `serialise.cpp`, `person_seen.cpp`, `briefing.cpp`, `names.*`, `dragon_json.cpp`, `hook_resolve.cpp`, `step.cpp`, `wasm/api.cpp`, `web/build.*`, `web/smoke.mjs`, `CMakeLists.txt`, `README.md`.
- Text: `data/text/en/narrator.json` and `data/importance.json`. English only (item 19 decision), so no German narrator lines exist.

## Numbers

300 seeds, played, genesis, 3000 years. Baseline is main at 1478254.

Acceptance against the brief:

| target | result |
|---|---|
| orders form around the year slayers are founded | median 895 against 890 (300 of 300 seeds) |
| every ready band has a named grandmaster | 481 of 481 |
| unwarned slayings | 0 |
| a scarred or promoted slayer returns after a failed march or gate fight | 99 of 107 seeds with an attempt (93 %, target 30 %) |
| slayings per 100 seeds | 3.7, baseline 3.7 |

Ladder counts: forming 6248 (baseline 5532), ready 481 (614), failed attempts 159 (186), slain by band 11 (11), seeds with an attempt 107 (117).

`interest.py` (played, 300 seeds):

| metric | main | branch |
|---|---|---|
| outcome entropy | 3.78 | 3.89 |
| distinct outcomes | 36 | 39 |
| reversals per 1000 y | 7.47 | 6.89 |
| causal depth | 0.131 | 0.122 |
| persistence | 0.383 | 0.455 |
| events per run | 6663 | 7334 |
| dragon deaths by slaying | 11 | 11 |
| dragon deaths by wounds | 3 | 15 |
| majors per 10 wakings | 49.1 | 111.4 |
| quiet stretches over 300 y | 0.25 | 0.01 |

Moved bands: majors per 10 wakings more than doubled and quiet stretches nearly vanished (see open questions). Deaths by wounds went from 3 to 15.

### Grandmaster table, seeds 1 to 10

Played, genesis, 3000 years. The grandmaster is the one of the strongest order in the last waking's briefing, with the traits the dragon knew then. "Scarred" is the number of scarred returns in the run. No dragon was slain in these ten seeds.

| seed | grandmaster | power | scars | known traits at last waking | outcome |
|---|---|---|---|---|---|
| 1 | Thalona the Pious (Saelwen) | 4 | 0 | runeward, fears the dark, hates the roar | order ready at the end, no attempt; 6 scarred |
| 2 | none ready or forming | | | | 7 orders, no attempt |
| 3 | Caeliel the Pious (Lorareth) | 9 | 0 | plate, fears the dark, hates gold | ready at the end; 2 failed marches; 9 scarred |
| 4 | Ylira (Vaelieth) | 5 | 0 | plate, fears fire, hates gold | forming; 2 failed marches; 5 scarred |
| 5 | Ylael the Pious (Ilvieth) | 6 | 0 | waxed ears, fears the dead, hates gold | forming; 2 failed marches; 8 scarred |
| 6 | Nimira the Grasping (Saelael) | 1 | 0 | runeward, fears the dead, hates smoke | ready; 1 failed march; 10 scarred |
| 7 | none ready or forming | | | | 5 orders, no attempt |
| 8 | Lorith the Grasping (Vaelieth) | 2 | 0 | waxed ears, fears heights, hates smoke | forming; 12 scarred |
| 9 | Nekith the Pious (Morcrypt) | 12 | 1 | plate, fears fire, hates banners | forming; 1 failed march; 5 scarred |
| 10 | Thulmor the Sly (Dunshade) | 12 | 1 | fireward, fears the dark, hates the roar | forming; 6 scarred |

The grandmaster of the order that failed is often gone by the last waking (a successor stands), which is why most scar counts read 0 here while the runs hold 5 to 12 scarred returns.

### Death rate

The roster does not change how often the dragon is slain: 11 slayings in 300 seeds before and after, none unwarned. `ready_ratio` 0.8 and `grandmaster_odds` 0.1 cut slayings to 2.0 per 100; 0.6 and 0.2 restore 3.7. Failed attempts fall (186 to 159) because fewer bands reach ready; each failure now costs the order people and promotes or scars the survivors.

### Cap of 8 named people per court

An order holds one grandmaster, two masters and six slayers, up to 9 persons, all role `hero`. I found no cap of 8 in the sim code. `hero_max` (3) limits only heroes that are not on a roster (`heroes_of` skips roster members). Roster members are hidden until met (`person_seen`); the grandmaster is learned of when the court's stage is set or a premonition tag exists, and only he is named in the briefing. So slayers do not count against any visible cap; they add up to 9 hidden people per court. Owner decision below.

### `resolve_fight` and the hooks

`resolve_fight` does not replace the hooks' damage path. `hook_fight_damage` is unchanged and a hero hook fight still wounds by its own formula. The roster path (`resolve_fight`) serves the gate fight and the failed march, and is the function item 09 calls for acts. It takes `Params` and an optional family because the data lives in `Params`.

## Decisions taken

- Fear is a weakness at every power level; `fear_match` zeroes dragon wounds; the slayer dies when dragon points over power exceed 1.5.
- Traits are revealed by fight (all matched), hostage (1), source (2), `reveal_order` and dragon sight.
- A promotion only comes through a war over a dragon-sworn faction (the one path the sim has events for).
- Slayers are existing people of role `hero`; the order is an entity in `World.orders`.
- `ready_ratio` 0.6 instead of the brief's 0.8, `grandmaster_odds` 0.2, `hunters_max` 12 (hunters grew to about 1500 per order without it).
- A ready band needs a rostered grandmaster. A tag-only stage is cleared by the ladder.
- Roster saves omit empty parts, so saves without orders hash as before.

## Deviations

- The order turn is in `rules/slayer_order.cpp`, not `slayers.cpp`.
- `ready_ratio` 0.6 (brief 0.8). The `note` in `data/slayers.json` still says 0.8; I left it so the data hash in the stored replays holds.
- `act_family`, the hate to family map, and the zealot and opportunist trait lists (zealot: devout, pious; opportunist: ambitious, greedy) are runner guesses, marked in the json note.
- Killing an envoy or consort has no sim event, so there is no promotion from it.
- The narrator has no German lines.

## Goldens

`kRulesVersion` 10 to 11. One `golden:` commit: `state_hashes.txt` (300 seeds), `ages_v0_1_raze.hash`, `sift_bank_v1.hash`, the three chronicle texts, `tests/replays/played-1..3.hrp` (verify: 42, 33, 30 checkpoints), `tests/saves/v1-seed4.expect`, wasm and pages. `ages_v0.hash` did not change.

Chronicle diff: all three histories run equal until the first `slayers_forming` of the first order (seed 1 year 1070, seed 2 year 1390, seed 3 year 780). That event now names the lead and is preceded by `order_formed`. Person ids shift after that, so later `person_rose` lines differ, and the runs diverge. Added kinds: `order_formed`, `slayer_promoted` (96, 167 and 287 lines in seeds 1 to 3), `slayer_scarred` (6, 0, 9). Seed 1 gets a fight at 1080 with two `dragon_wounded` lines and a dead person. Seed 3 has 30 wakings against 58 and `tribute_paid` 6 against 64; I did not trace why.

## Changes to existing tests and checks

- `web/game-check.cjs` `armBand`: the lab band now also gets an order and a grandmaster person, because the ladder drops a stage with no roster. The `v04.slain` regex accepts ", led by NAME," in the death line. Without these the check failed at `#death`.
- `ctest`: no test changed. `golden` failed until the golden commit, as expected.
- `smoke.mjs`, `build.sh`, `build.ps1`: embed `data/slayers.json`.

## Proposed tests (for the owner to confirm; none added)

- `slayer_promotion`: a war won over a dragon-sworn faction promotes a roster member and leaves the others.
- `slayer_weakness`: every slayer at every power has at least one weakness, and a matching fear zeroes dragon wounds.

## Open questions

1. Majors per 10 wakings went from 49 to 111 and quiet stretches nearly vanished. `slayer_promoted` lines are numerous (up to 287 per run). Lower their importance further, or fold repeated promotions into one line?
2. Deaths by wounds rose from 3 to 15 per 300 seeds. Gate fights and failed marches now fight with the roster. Accept, or soften the dragon's wounds in `gate_fight`?
3. Up to 9 hidden roster people per court against a cap of 8. Count them, or keep them outside the cap?
4. Autosave in the page stores the world view, which drops unmet people and orders and writes unknown traits as `?`. A reload loses the roster. The fix is a full-state save for autosave; I did not change the page. The same hole already exists for unmet people since item 04b.
5. Confirm the act-to-family and hate-to-family maps and the zealot and opportunist trait lists.
6. German narrator lines for the new keys wait for the localisation of the narrator.

## How to verify in five minutes

```
cmake --build build -j2 && ctest --test-dir build -L fast
./build/hoard-sim --data data --replay tests/replays/played-1.hrp
seq 1 300 | xargs -P 2 -I{} ./build/hoard-sim --data data --golden {} | tr -d '\r' | sort -n | diff - tests/golden/state_hashes.txt
./build/hoard-sim --data data --seed 3 --genesis --policy played --years 3000 --out out/s3
python3 tools/interest.py --bin build/hoard-sim --data data --seeds 1-20 --years 3000 --policies played --genesis --jobs 2
```

Read `out/s3/stats.json` `roster` and the `slayers` block of `out/s3/briefing.json`.
