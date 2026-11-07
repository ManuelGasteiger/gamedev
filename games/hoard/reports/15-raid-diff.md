# 15 · RaidReport and raid_diff

Branch `item/15-raid-diff` from main d1fe9c1 (rules version 11, unchanged). Nothing paid. No tests added.

## 1. What changed

Added:
- `sim/raid/snapshot.h`: `SortieState`, held in `World.sortie`.
- `sim/raid/diff_book.h`, `diff_data.cpp`, `data/raid_diff.json`: the diff's numbers.
- `sim/raid/report.h`, `report_json.cpp`: `RaidReport`, `DiffResult`, `SortieResult`, JSON in and out.
- `sim/raid/diff.cpp`: `raid_diff`.
- `sim/raid/sortie.cpp`: `take_snapshot`, `begin_sortie`, `end_sortie`.
- `sim/wasm/raid_api.h/.cpp`: wasm commands `begin_sortie` and `end_sortie`.
- `sim/runner/raid_script.h/.cpp`: `hoard-sim --raid-report FILE` (`{world, report}`).
- `docs/raid_report.schema.json`, `tools/raid_report_check.py`.
- `web/game/raid.js`: the panel's facts.

Changed (small): `state.h` (`World.sortie`), `data.h/.cpp`, `data_io.cpp`, `record.cpp` (data hash, `apply_input` cases), `recorded.cpp` (two wrappers; `sleep` clears an open sortie), `serialise.cpp` (`sortie` written only while open), `act_log.cpp` (a kill with `who` set keeps it), `api.cpp` (3 lines of dispatch, 1 data line), `CMakeLists.txt`, `build.sh`, `build.ps1`, `runner/main.cpp`, `game.template.html` and `defences.js` (panel through `sortie()`), `ui.json` en and de (12 keys), `smoke.mjs` (loads raid_diff), `game-check.cjs` (see 6), README. Rebuilt: `ages.wasm`, `ages-sandbox.html`, `hoard-game.html`.

## 2. Numbers

Checks (all on the final build):

| check | result |
|---|---|
| `ctest -L fast` | 24 of 24 pass |
| wasm build, `node web/smoke.mjs --golden 1-60` | 60 seeds equal native |
| `node web/check.cjs` (debug page, `apply_acts`) | PLAYWRIGHT OK |
| `node web/l10n-check.cjs --lang pseudo,de` | passed |
| `node web/game-check.cjs` | 90 of 90 (with the two adjustments in section 6) |
| replay of a session with sorties, one `load_world` taken mid-sortie | `replay ok`, end hash equal |

The interest tool was not run: no rule, no base step and no policy path changed (goldens equal), so every number it measures is identical by construction.

Acceptance, seed 42 at its first waking (year 541, the state `--genesis` gives; regions are Tharbarrow `elder_wood` of galwick, Applevale `west_vale` of haldale; Wellmere Bridge and Dorun do not exist in this tree's seed 42). Report: destroyed house 0.5 x value, civilians 0.5 x population x 0.2, 12 gold collected, roared at Applevale, 12 gold dropped. Galwick's wealth and gold holding at the start 63.219; population 632.19.

| act | magnitude | dread / awe delta |
|---|---|---|
| take gold, galwick | 0.1898 (12 / 63.219) | galwick dread +0.2149 in total with the burn and the kill |
| burn, galwick | 0.5 | |
| kill civilians, galwick | 0.1 (63.2 / 632.2) | |
| show roar, haldale | 0.3 | haldale awe +0.06, dread +0.03 |

Hoard gold ends at 51.5 (before plus the 12 dropped); no refusals. The brief's "120 gold" is more than any court holds in this tree (largest wealth 63): the take is capped at 1 and the result carries `raid.capped|type=take|region=elder_wood`; the hoard still takes the 120 the shell says it dropped. Units are the sim's wealth units; the flight shell must scale into them.

Direct versus routed, sandbox seed 40, one fixed panel script (`game.act` on the current build against main's page): burn, take, take, roar, hostage, give, burn.

| act | region | direct | routed |
|---|---|---|---|
| burn | deep_hold | 0.5 | 0.5 |
| take gold | deep_hold | 0.2 | 0.2 |
| take gold | deep_wood | 0.3 | 0.3 |
| show | east_marches | 0.3 | 0.3 |
| take_hostage | east_vale | 0.5 | 0.5 |
| give | deep_wood | 0.2 | 0.2 |
| burn | east_marches | 0.25 | 0.25 |
| extra, from the toll | deep_hold, east_marches | none | kill civilians 0.1 and 0.05 |

Types, targets, regions and payloads equal; largest relative magnitude difference 0. The two extra kills are the design's civilian toll (0.2 x burn share) from the tactical shell; with `civilian_toll_factor` 0 the logs are identical and game-check is 90 of 90 with no adjustment.

## 3. Decisions taken

- Snapshot lives in `World.sortie`, written to saves and hashed only while `open`. A save taken mid-sortie keeps it and loads with the sortie open; `sleep` clears an open sortie. `begin_sortie` while open is refused `sortie.already_open`; `end_sortie` without one `sortie.not_open`.
- The sim has no population, garrison or building values; the snapshot derives them (`data/raid_diff.json` "snapshot"): value = the owner's wealth x the region's yield share, population = that x 10, garrison and soldiers from the owner's power per region, army size from the attacker's power. Structures come from region tags (`library`, `temple`, ...) at a share of the value. The shell reads them from the `begin_sortie` result.
- Loot shares: collected amount over (court wealth x `holdings[category]`, 1 for all). `take` magnitude is therefore the share of wealth, as `immediate()` uses it.
- `armies[]` gets an optional `repelled`: the sandbox's Defend is a defend without a kill, Kill army a kill without a defend. With `repelled` a defend of magnitude `killed_share` or 0.5 comes out; it needs the defender at war with the attacker (else warning `raid.not_at_war`). The table's oath rule (a hit on the army of a court the dragon is sworn to also defends) is in the code behind `oath_defends`, default false.
- `lair.dropped` sets the hoard to what it was before plus the dropped amount for the categories named; the `take` credit is replaced, not added. `raid.loot_lost` when the takes credited more.
- `wounds` is passive damage keyed `<region>|<defence>` (the sandbox's passive flush), emitted as `wound` acts. Defence strikes of the derived acts still happen as for any act.
- Show: the strongest of land, roar, flyover per settlement; flyover only when nothing else touched the region.
- `speech[]` and `spells[]` are passed through unchanged, in that order after the facts. The panel's give, release, altar, miracle and crush acts go through `speech[]`; demand, cast, sponsor and terraform stay their own commands.
- Order of derived acts: takes, hostages, kills, burns, civilians, defends, shows, receives, fights, spells, speech, wounds.
- Warnings are codes with en and de text (`raid.*` in `ui.json`); the result says which shell produced the report (`shell`).
- `raid_diff.json` is in the data hash, so recorded replays from before show the "data differs" warning; no test checks it.

## 4. Deviations from the brief

- The tactical shell's `civilians` use factor 0.2 as asked; the brief's "identical in types" holds only without the toll (section 2).
- `repelled` and `oath_defends` (section 3).
- `a.defend` in game-check failed at first because the table's oath rule turned a routed Kill into a defend; fixed by the `oath_defends` flag, not by changing the check.
- `act_log.cpp`: a kill that names `who` keeps it in the schema form (garrison kills would read as army otherwise).
- Seed 42's regions differ from the brief's names.

## 5. Goldens

Unchanged, `kRulesVersion` 11. `ctest` golden and replay tests and the wasm golden pass.

## 6. Changes to existing checks

`web/game-check.cjs`, two narrow edits, both caused only by the toll (with the toll at 0 and the original check the run is 90 of 90; with the toll at 0.2, `b.actlog` and `c.era` fail until adjusted):
- `b.actlog`: the expected act list has `act_kill` after the first `act_burn`; chronicle lines 9 instead of 8, the burn line at index 8.
- `c.era`: with the toll no era advanced in 1200 years of that seed (civilian kills lower cohesion). The lab sets `civilian_toll_factor` 0 before section c through `load` and `load_world` (not `game.loadWorld`, which clears the page's age history).
`smoke.mjs` loads `raid_diff`. `check.cjs` unchanged and passing.

## 7. Open questions for the designer

1. Keep the toll at 0.2? It makes every burn read as dread and slows eras in the lab seed; `c.era` shows the size of it. At 0 the sandbox is unchanged.
2. `repelled` versus the table's oath rule (`oath_defends`): with the oath rule on, `a.defend` fails because the earlier kill already ended the war by defending.
3. Units: the brief's 120 gold exceeds every court. The flight shell needs the snapshot's units (item 16).
4. `dropped` may exceed what was taken; it is trusted. Cap it at carried loot once the flight shell tracks carry?
5. Garrison, soldiers and population are placeholders (power or wealth times a factor) until the sim models them.

## 8. How to verify in five minutes

```
cmake --build build -j2 && ctest --test-dir build -L fast
python3 tools/raid_report_check.py --help   # validates a report against docs/raid_report.schema.json
node web/smoke.mjs --golden 1-60 && node web/check.cjs && node web/game-check.cjs
```

## 9. Proposed tests (not added)

1. `raid_diff_shares`: a report destroying 40 % of a settlement's house value and collecting 10 % of the court's gold yields `burn 0.4` and `take 0.1` against the snapshot, with the court's wealth changed after `begin_sortie`.
2. `raid_diff_one_act_per_structure`: three partial library burns in one sortie yield one `burn(structure: library)` with the summed share.
3. `sortie_replay`: begin, end and a mid-sortie `load_world` replay to the same hash.
