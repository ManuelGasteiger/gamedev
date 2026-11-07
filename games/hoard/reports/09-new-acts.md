# 09 · New acts of the v1.1 API

Branch `item/09-new-acts` from main 1478254, one branch (no 09a/09b split). Item 08 (`item/08-slayer-roster`) is merged in for `resolve_fight`: first at 6a71ed4 (code), then at 2fe68cd (its game-check fix and report; its goldens were replaced by this branch's). `fight` is wired, not refused. Nothing paid, no external calls.

## 1. What changed

Added:
- `data/gates.json`: per act of the schema the target kinds, required payload values, gates (growth points per axis, any one talent, phase), pool cost, `secret`, `once`, `via`; keep roles, build, reveal(lie), sponsor gates.
- `data/policies/schemer.json`, `data/policies/priest.json`.
- `sim/ages/act_data.{h,cpp}` (gates.json, constants.json `acts`), `act_gates.{h,cpp}` (rule and gate check, pool, secrecy), `act_log.{h,cpp}` (schema shape in and out, `normalise_act`, `act_log_json`), `act_kinds.h`.
- `sim/ages/acts_routed.cpp` (acts that call the older verbs), `acts_force.cpp`, `acts_speech.cpp`, `acts_arcana.cpp`, `acts_lair.cpp`.
- `sim/ages/aging_json.{h,cpp}`: the world-aging region fields in saves.
- `sim/ages/policy_pick.{h,cpp}`: policy targets (persons, regions, second courts) and the leftovers (hooks ignored, offers sworn, exit taken).
- `sim/runner/act_script.{h,cpp}`: `hoard-sim --act-script FILE`.
- `tools/act_check.py`, `tools/act_scenarios.py`.

Changed:
- `state.h`: `Act` gains `source`, `answers`, `target_kind`, `args` (typed payload strings); `Region` gains `last_burn_year`, `destroyed_share_at_last_burn`, `ruin_year`, `abandoned_year`, `resettled_year`.
- `acts.cpp`: one `ActRun` per call; `apply_acts` normalises, checks, routes or resolves each act; refused acts are returned, not logged; `apply_logged_acts` for acts a verb already checked; act events carry the act index (`cause.params.act`) and a narrated variant (`ev.act_<type>_<kind>`).
- `ages.h`, `recorded.cpp`, `unrecorded.h`: `apply_acts(..., std::vector<ActRefusal>*)`.
- Verbs that now log their act: `sleep.cpp`, `talents.cpp` (pick_talent), `spells.cpp` (cast as the spell's act, terraform), `ages.cpp` (accept_offer as swear, demand clauses), `chron_keep.cpp`, `chron_voice.cpp`, `exit.cpp`, `arrival_acts.cpp` (source, outcome), `hook_policy.cpp` (hook answers carry `source: hook`, `answers`), `offers.cpp`, `chron_cf.cpp` (leaves verb-logged acts out of its replay).
- `ages.cpp` demand: `no_harm_to`, `hostage`, `disband`, `tear_down`, `pilgrimage`; `oaths.cpp` their clause checks; `arrival_gates.cpp` gate `sworn_pilgrimage` and `data/arrivals.json` row `sworn_pilgrim`.
- `moves.{h,cpp}`: `move_refusal` (the precondition without the roll).
- `rules/memory.cpp`: faith floor for `consecrated` and `priest_kept` courts. `rules/migration.cpp`, `collapse.cpp`, `fading.cpp`: abandoned and resettled years, `farmland`.
- `data/reactions.json` (owner rows for the new acts, gain `corrupt`), `data/talents.json` (Mortal Guise), `data/constants.json` (`acts`), `data/importance.json`, `data/text/en/narrator.json` (event lines, reaction lines, causes), `data/text/{en,de}/ui.json` (reason keys and words).
- `policy.cpp`, `policy_json.cpp`, `data.h`: policy act `args`, `person`, `target_kind`, `source`; policy flags `ignore_hooks`, `swear_offers`, `take_exit`.
- Plumbing of gates.json: `data_io.cpp`, `data.cpp`, `record.cpp` (data hash), `sim/wasm/api.cpp` (also `refused` on `apply_acts`, command `act_gates`), `web/build.sh`, `web/build.ps1`, `web/smoke.mjs`. Runner writes `act_log.json`.
- README: file list, item 09 section, constants table.

## 2. Numbers

Acceptance:

| measure | result |
|---|---|
| `tools/act_check.py`, played + schemer + priest, 300 seeds each, 3000 years | 485,393 acts in 900 logs, 0 invalid |
| malformed act through the API | refused with a reason (`act.malformed\|field=payload.subject`, `act.bad_value\|...`, `act.unknown\|type=`), not logged |
| schema acts appearing at least once in those runs | 40 of 40 (table in section 9) |
| `tools/act_scenarios.py`, 15 scenarios, 45 options, lab world seed 7 at year 1190 | 14 expressible; scenario 13 is not: `terraform(region, mountain)` refused `unknown_terraform` |
| played unchanged by this item | chronicle hash equal to main on seeds 1 to 60 before the 08 merge, equal to the 08 tip on seeds 1 to 60 after it |

`tools/interest.py`, idle and played, 300 generated seeds, 3000 years, `--jobs 2`. Before: main 1478254. After: this branch, which includes item 08; every played difference below comes from item 08 (see the last acceptance row).

| played | before | after |
|---|---|---|
| outcome entropy (bits) | 3.78 | 3.89 |
| distinct outcomes | 36 | 39 |
| reversals per 1000 y | 7.47 | 6.89 |
| causal depth | 0.131 | 0.122 |
| pacing CV | 0.372 | 0.400 |
| persistence | 0.383 | 0.455 |
| agency (Jaccard vs idle) | 0.905 | 0.916 |
| events per run | 6663 | 7334 |
| dragon slain (runs) | 14 | 26 (wounds 15, slaying 11) |
| hooks per waking, coverage, answered | 1.66, 98 %, 65 % | 1.77, 99 %, 60 % |
| majors per 10 wakings | 49.1 | 111.4 |
| briefings naming a person from an earlier one | 23 % | 28 % |
| arrivals per waking after year 200 | 0.10 | 0.09 |
| tracked people with a dragon memory per court | 2.09 | 2.75 |

The schemer and priest policies are new and have no "before". No baseline band moved because of item 09.

## 3. Decisions taken

- An act's `id` is its index in `World.acts` (monotonic per world, not stored). `source` empty reads as `raid`. The older fields stay (structure and person in `item`, secondary court in `against`); the typed payload's strings go to `args`. New fields are written only when set.
- Acts arrive in the schema shape or the flat one. `normalise_act` resolves a region target to its owner and a person target to its court, and pairs the old fields with the payload keys (receive outcome, demand clause, exit kind, take item, burn structure, show kind from the magnitude, miracle kind).
- A refused act is not logged. `apply_acts` keeps going with the next act; the wasm call stays `ok` and adds `refused: true`, the first `reason` and `refusals`. A refused routed cast still emits `spell_failed` as the `cast` verb always did.
- Acts that existed before keep their gates (miracle pays faith in `acts.cpp`, spells use `constants.json spells`, demand `demand_knowledge`). The new acts pay their pool in the sim on acceptance, as spells do.
- "Once per age" for guise is once per waking.
- Mortal Guise is a third option on the arcana 25-point node, not at 40: the tree's talent list has no 40-point node, and authoring one moves where arcana ranks start (they begin after the last authored node), which changes played.
- Event names stay `act_<type>` (the tree's), with the act index in `cause.params.act`; verbs that logged before keep their own events (`terraformed`, `talent_taken`, `sleep_begin`, `oath_sworn`, `ending`).
- Narration lines go in `data/text/en/narrator.json` (English only, item 19's decision); reason keys in `ui.json` in en and de.
- reveal: `plot` exposes the court's oldest open secret and applies the act behind it (the corrupt or teach on that person) to the court at weight 1; `heir_location` removes the `pretender:` tags; `slayer_plan` marks the order known (`reveal_order`) and gives the told court `forewarned`; `lie` removes a lying offer, tags the liar `lie_exposed` and the told court `deceived`; the betrayed court gets the oath `betrayal` effect when it is under oath. `hidden_region` removes a `hidden` tag, which nothing in the tree sets yet.
- broker: marriage and alliance write `married:` and `ally:` tags (no marriage card exists to merge lines); peace ends the war `brokered_by_dragon`; trade is a growth x1.1 modifier on both; vassalage makes the weaker the vassal.
- judge: on a person tagged false_dragon, usurper, necromancer or tyrant (or the court's `<tag>:<id>`) the judge row; otherwise the `kill.person` gain. Guilty kills; spare pardons.
- curse research and blight replace the spell's lineage modifier with their own; corrupt on a court stays refused (`disabled`), as the spell is today.
- haunt: memory rows per message (runner-set); a prophet's cult is a court tag `cult_seed` until item 12; abdication and desertion use `fall_person`.
- keep: a visitor or a willing person (guise court) comes to the lair as a consent hostage; the consort founds a `dragon_blood`, `half_dragon` hero at their court after `consort_line_years` (100); priest keeps the court's faith floor; champion and ward are tags only.
- build writes `built:<what>` on the dragon; the unlocks are not wired.
- sponsor hero or claimant spawns a hero tagged `sworn_to_dragon` or `claimant` (the pretender card reads `claimant`).
- demand: hostage hands over the heir; disband ends the order (item 08's roster cleared, the grandmaster flees to the strongest neighbour with `grudge`); tear_down removes the named structure tag; pilgrimage sends `sworn_pilgrim` visitors every 100 years; no_harm_to is broken by a war or a region taken from the protected court.
- fight: the dragon's act family is that of its last act this waking that has one.
- ignore: an open hook gets its ignore branch at once; the world gets `ignored:<hook kind>` or `ignored:<scenario id>`.
- pick_talent with skip or reroll is refused (`pick.unsupported`): the sim has neither.
- sleep and exit can be sent through `apply_acts`.

## 4. Deviations from the brief

- The new acts are flat files in `sim/ages/` (`acts_*.cpp`), not `sim/ages/acts/`, so `web/build.sh` and `build.ps1` need no new folder.
- `keep_person` stays a verb for saves and replays; `keep(historian)` calls it and it now logs a `keep` act.
- The six moves were not API verbs in the tree; teach and corrupt on a person call `resolve_move`, the other moves have no act in the schema.
- Witnesses: owner and neighbours as before; "a court with a sage or envoy present" is not modelled (people have no position).
- granary: wealth -10 % only; "plague chance x1.5 for 2 decades" needs a plague modifier the modifier rule does not read.
- `tools/act_check.py` is not run by ctest: no tests were added (see section 8).
- The played act log now holds `sleep`, `pick_talent`, hook `source` and `answers`; `played.json` is unchanged and its chronicle hash matched main on seeds 1 to 60 before the item 08 merge.

## 5. Goldens

Changed, `kRulesVersion` 11 (item 08 had also set 11 on its branch; the notes expect one more regeneration when both land). Commits: `golden: rules 11 ...` (state_hashes 300 seeds, the three chronicle texts, `ages_v0_1_raze.hash`, `sift_bank_v1.hash`, the three replays, re-recorded and replayed with 42, 33 and 30 checkpoints, wasm and both pages), `golden: save expectation ...` (`tests/saves/v1-seed4.expect`), `golden: game page re-embedded ...` (the first page build had an empty dragon model after the disk filled). `ages_v0.hash` unchanged.

Cause, read from the diffs: the chronicles change only through item 08. On played, this branch's chronicle hash equals main on seeds 1 to 60 before the 08 merge and equals the 08 tip (6a71ed4) on seeds 1 to 60 after it; the raze golden equals the 08 tip too, and git merged the three chronicle texts of the later 08 tip without conflict. The state hashes change through item 09 as well: `World.acts` now holds `sleep`, `pick_talent` and the other verbs' acts, acts carry `source`, `answers`, `args`, and regions carry the world-aging fields. The first chronicle change (seed 1, year 1070) is 08's slayer order.

## 6. Open questions

1. Scenario 13 (the mountain): `terraform(mountain)` is refused, `constants.json terraform.kinds` has only wasteland and lake, and terraform refuses the lair region, which the Mountain ending needs. Add `lava` and `mountain` rows and allow the lair for `mountain`?
2. Scenario 5: `incite` only works on an orc horde (`spells.cpp`, `not_orc`). The lab world's neighbour was orc, so it passed; with a human neighbour it is refused. Should incite take any court?
3. Mortal Guise sits on the arcana 25 node (third option) instead of 40. Keep, or author the 40 node with Thermal Sense (moves arcana ranks)?
4. Unwired consequences, each a tag today: `champion` (fight the next slayer band first, needs the slayer attempt path), `build` unlocks (library digestion, vault, altar, gate), `crusade_eligible` after burn(temple), `cult_seed` (item 12), `hidden` regions (nothing sets the tag). Which should land where?
5. granary: plague chance x1.5 needs a plague modifier read by the shock rule. Add one?
6. Witnesses with a sage or envoy present need people with a place. Leave until a later item?
7. Old acts keep their old gates (miracle, spells, demand). Should they move into `gates.json`?
8. A refused routed cast still writes `spell_failed`, as the `cast` verb always has. Keep, or make refusals write nothing?
9. `pick_talent` skip and reroll are refused: the sim has neither (`core-loop.md` has "skip pays +5 health; one reroll per moult").
10. The narrator lines for the new acts are English only, like the rest of `narrator.json`.

## 7. How to verify in five minutes

```
cmake --build build -j2 && ctest --test-dir build -L fast
python3 tools/act_scenarios.py --bin build/hoard-sim --data data          # 15 scenarios, one refused option (13 terraform mountain)
python3 tools/act_check.py --bin build/hoard-sim --data data --seeds 1-10 --policies played,schemer,priest   # 0 invalid
./build/hoard-sim --data data --seed 3 --years 3000 --genesis --policy schemer --out out/s3 && head -c 600 out/s3/act_log.json
```

## 8. Proposed tests (not added; for Manu to decide)

1. `act_gates`: `guise` without the talent is refused with `gate.talent`; with it, accepted and logged once per waking.
2. `burn_structure_library`: the library burn applies the three documented effects (written x0.1, neighbours' era_rate modifier, sources gone).
3. `secret_acts_no_witness`: `corrupt` on a person writes no court memory; a later `reveal` writes it.
Also possible: `tools/act_check.py` on a few policy logs as a ctest (the brief's scope 1 says the tests run it).

## 9. Act coverage

Act counts in the act logs, 300 generated seeds x 3000 years per policy (`tools/act_check.py --seeds 1-300 --policies played,schemer,priest`). The counts include acts the verbs log (sleep, pick_talent, swear, demand, the casts); refused acts are not in them.

| act | played | schemer | priest | all |
|---|---|---|---|---|
| burn | 47837 | 41286 | 0 | 89123 |
| kill | 6661 | 5096 | 0 | 11757 |
| crush | 0 | 30019 | 0 | 30019 |
| fight | 0 | 10192 | 0 | 10192 |
| destroy_altar | 0 | 10468 | 0 | 10468 |
| take | 14907 | 13324 | 12910 | 41141 |
| take_hostage | 51 | 8983 | 0 | 9034 |
| hunt | 0 | 0 | 3453 | 3453 |
| receive | 43 | 113 | 2570 | 2726 |
| defend | 11015 | 8838 | 8229 | 28082 |
| give | 11057 | 0 | 8332 | 19389 |
| release | 0 | 4736 | 0 | 4736 |
| bless | 0 | 0 | 6435 | 6435 |
| ward | 0 | 0 | 7782 | 7782 |
| show | 0 | 0 | 8332 | 8332 |
| demand | 0 | 32765 | 1345 | 34110 |
| swear | 0 | 279 | 0 | 279 |
| teach | 0 | 12241 | 8012 | 20253 |
| incite | 0 | 7141 | 0 | 7141 |
| broker | 0 | 11704 | 0 | 11704 |
| reveal | 0 | 6511 | 0 | 6511 |
| judge | 0 | 8382 | 0 | 8382 |
| accept_offering | 0 | 0 | 8332 | 8332 |
| consecrate | 0 | 0 | 727 | 727 |
| miracle | 0 | 0 | 8332 | 8332 |
| curse | 0 | 2101 | 0 | 2101 |
| corrupt | 0 | 4518 | 0 | 4518 |
| raise_undead | 0 | 203 | 0 | 203 |
| haunt | 0 | 4171 | 0 | 4171 |
| commune | 0 | 0 | 5110 | 5110 |
| keep | 0 | 4779 | 531 | 5310 |
| dismiss | 0 | 1534 | 0 | 1534 |
| build | 0 | 0 | 599 | 599 |
| terraform | 0 | 0 | 218 | 218 |
| sponsor | 0 | 8131 | 11536 | 19667 |
| guise | 0 | 557 | 0 | 557 |
| sleep | 11920 | 10436 | 8233 | 30589 |
| pick_talent | 5411 | 5452 | 4252 | 15115 |
| ignore | 0 | 7162 | 0 | 7162 |
| exit | 0 | 0 | 99 | 99 |

## 10. Reason keys

`code|key=value`; text in `data/text/{en,de}/ui.json` as `reason.<code>` (76 keys in each language). New in item 09:

| code | when |
|---|---|
| `act.unknown\|type=` | the type is not in gates.json |
| `act.malformed\|field=` | target, secondary or a required payload key missing |
| `act.bad_value\|field=\|value=` | a payload value or target kind outside the schema's list |
| `act.no_target` | the court, person or region does not exist |
| `gate.axis\|axis=\|n=` | growth points short |
| `gate.talent\|talent=` | none of the gate's talents taken |
| `gate.phase\|n=` | phase short |
| `gate.pool\|pool=\|n=` | magic, knowledge or faith pool short |
| `gate.once` | guise again this waking |
| `fight.not_slayer` | fight on a person who is not on an order's roster |
| `broker.not_at_war`, `broker.already` | peace without a war; the pact exists |
| `reveal.nothing\|secret=` | nothing of that kind to tell |
| `judge.in_lair` | the person is a hostage |
| `consecrate.already`, `consecrate.no_temple` | |
| `keep.not_in_lair`, `keep.already` | |
| `build.already` | |
| `sponsor.no_remnant` | |
| `ignore.no_hook` | |
| `pick.unsupported` | skip or reroll |
| `swear.no_offer`, `swear.refused` | |
| `demand.no_ward`, `demand.no_order`, `demand.no_structure` | no_harm_to without a third court, disband without an order, tear_down without a structure |

Reasons the routed verbs return as before (and now with text in both languages): `no_arrival`, `no_outcome`, `not_wild`, `no_region`, `run_ended`, `unknown_ending`, `ending_not_ready`, `no_relic`, `silent`, `no_answer`, `already_accepted`, `no_role`, `not_in_lair`, `unlettered`, `already_kept`, `unknown_spell`, `disabled`, `needs_axis`, `needs_knowledge`, `needs_knowledge_or_magic`, `not_orc`, `no_victim`, `not_adjacent`, `already_at_war`, `bad_site`, `unknown_terraform`, `lair_region`, `needs_size`, `already_tag`, `unknown_demand`, `already_sworn`, `no_faction`, `unknown_talent`, `needs_phase`, `node_taken`, `node_not_reached`, `no_person`, `no_court`, `dragon_slain`, and item 07's `move_no_access`, `move_no_subject`, `move_unknown`, `move_no_person`.
