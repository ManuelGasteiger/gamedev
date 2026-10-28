# 04 · Endings, prophecy, legends across runs

Branch `item/04-endings-prophecy` from origin/main 7d8c372. Nothing paid, no external calls.

## 1. What changed

Added:
- `data/endings.json`: 11 entries, 3 enabled (mortality, myth, ascension), 8 sketches with one condition each; the prophecy templates; the legend weaving; Myth deeds. `todo` fields carry the `TODO(item12)` markers.
- `data/modifiers.json`: the difficulty ladder, data only, with the parameter each rank would change.
- `data/text/{en,de}/endings.json`: ending names, offers, epilogues, a met and a missing line per condition, prophecy lines (two variants each), number words, Myth lines. `endings` is now a string part of the text bundle (`sim/text/bundle.cpp`).
- `data/policies/steer_mortality.json`, `steer_ascension.json`: the pacing bots' steering.
- `sim/ages/endings_def.h`, `endings_data.cpp` (registry and loader), `endings.h` (timers, conditions, exit), `rules/endings.cpp` (the decade rule, after the cards), `exit.cpp` (exit, epilogue, exit offer rows), `endings_json.{h,cpp}` (save fields, view, briefing block, prophecy and Myth views).
- `sim/ages/prophecy.{h,cpp}`: writing the prophecy during the pre-history, spreading it.
- `sim/ages/profile.{h,cpp}` (the five files as JSON), `legend_weave.cpp`, `profile_notes.cpp` (atoms, bestiary, glossary, the Myth's legend entry, closest conditions).
- `sim/runner/bot.{h,cpp}`: amnesiac, carry and oracle bots (`--bot`).
- `web/game/profile.js`: the page keeps the profile in localStorage.
- `tools/endings_rates.py`, `tools/endings_runs.py`.
- Tests `ending_continuity`, `myth_legend_roundtrip` (both fast).

Changed:
- `state.h`: `World.endings`, `World.prophecy`, `World.ended` (own commit).
- Trigger language (`cards.h`, `card_data.cpp`, `card_match.cpp`, `cards.cpp`): `count`, `within_wakings`, fields `temples`, `temple_years`, `monumental`, `reign`, `legends`, `growth:<axis>`, `acts:<a|b>`, card option `keyed` (own commit).
- `data.h`, `data.cpp`, `data_io.cpp`, `record.cpp`: `endings.json` loaded and hashed; profile file IO; `offer_exit_era`/`offer_exit_faith` deleted (own commit).
- `serialise.cpp`, `briefing.cpp`: endings in save and view, briefing blocks `endings` and `prophecy` (own commit).
- `offers.cpp`, `data/offers.json`, `ages.cpp`: the `rite`/`exit` proposal is gone; ready endings come as `exit` offer rows, accepting one runs the exit.
- `recorded.cpp`, `record.cpp`: `exit` wrapper and `apply_input` case. `sleep.cpp`, `policy.cpp`: no sleep after an exit.
- `rules/slayers.cpp`, `rules/arrivals.cpp`, `rules/cards.cpp`: no slayers, visitors or dragon cards in the epilogue (one guard each).
- `acts.cpp`, `spells.cpp`: one call each to spread the prophecy.
- `genesis_world.cpp`, `genesis.h`: `make_genesis` with a profile; legends woven and the prophecy written in the pre-history loop.
- `legends.cpp`, `genesis.json`, `genesis_data`: `always` kinds (the prophecy, woven legends) on the first screen; the riddle comes with the legends.
- `data/events/prophecy.json`: `prophecy_cult`, `prophecy_slayers`, `prophecy_schism`. Narrator lines and importance for the new kinds; refusal reasons in `ui.json` en and de.
- Runner: `--profile`, `--bot`, `stats.json` `endings`; replay headers carry woven legends.
- wasm: `endings` data, `profile` on `load`/`reset`, commands `exit` and `profile`, `myth` on a fatal sleep. Page: profile kept across runs, a Myth line on the death screen. `web/build.sh`, `web/smoke.mjs` embed `endings.json`.
- `README.md`: item 04 section.

## 2. Numbers

Endings over 300 played seeds, generated worlds, 3000 years (`tools/endings_rates.py`). The played policy never takes an exit.

| ending | ready | condition: ever met, longest hold (median of runs that met it), best count |
|---|---|---|
| Mortality | 26.0 % | voice 88.7 %, 210 y; audience 26.7 %, 70 y |
| The Myth | 75.3 % (qualifying deaths 20/300) | reign 99.0 %, 2500 y; monumental 83.3 %, 120 y, best 7 courts; legends 100 %, 2960 y |
| Ascension | 0 % | peoples 0 %, best 1 of 4 courts; temples 0 %, best 1; craft 7.7 %, 2520 y; voice 0 %, best 1; mild 0 %, best 1; strong 0 %, best 1; dominion 10.3 %, 1030 y |

Ascension blocker: the cult courts. No run ever had more than one court with awe ≥ 0.6 and faith ≥ 0.4, so conditions 1, 2, 4, 5 and 6 (all built on those courts) never hold; best value 1 of 4. Craft and Dominion are met in some runs. Thresholds not loosened.

Prophecy cards, share of the 300 runs firing within 1000 years of the hatching: `prophecy_cult` 77.7 %, `prophecy_slayers` 72.0 %, `prophecy_schism` 72.7 %; at least one of the three 88.7 % (acceptance ≥ 30 %). `prophecy_spread` 95.3 %. Courts knowing the prophecy at the end: median 9.

Seed 42: the prophecy is written in year 440 into Cordale's library (known by Cordale alone at the hatching) and is on the first screen:

> When the fire is loved by four houses of three kinds, and every house that loves it has built it a hall of stone, and the old arts wake again, and one voice speaks for three peoples, then the dragon shall rise and be a god.

Inter-run pacing (`tools/endings_runs.py`, 15 chains of 12 runs per bot; every bot takes any exit it is offered):

| bot | ending | N=1 | 2 | 3 | 4 | 6 | 8 | 10 | 12 |
|---|---|---|---|---|---|---|---|---|---|
| amnesiac | Mortality | 0.27 | 0.40 | 0.40 | 0.47 | 0.60 | 0.67 | 0.80 | 0.80 |
| amnesiac | Myth | 0 | 0.13 | 0.13 | 0.20 | 0.40 | 0.53 | 0.53 | 0.53 |
| amnesiac | any | 0.27 | 0.53 | 0.53 | 0.67 | 0.93 | 0.93 | 0.93 | 0.93 |
| carry | Mortality | 0.40 | 0.67 | 0.80 | 0.87 | 1.00 | 1.00 | 1.00 | 1.00 |
| oracle:mortality | Mortality | 0.93 | 1.00 | 1.00 | 1.00 | 1.00 | 1.00 | 1.00 | 1.00 |
| oracle:ascension | Mortality | 0.40 | 0.67 | 0.80 | 0.87 | 1.00 | 1.00 | 1.00 | 1.00 |

Ascension is reached by no bot; carry and oracle reach the Myth never (they leave by Mortality first). Knowledge leverage at N = 1: Mortality 3.50 (target ≤ 1.2, missed), Ascension n/a (0 / 0, target ≥ 3). Carry steers toward Ascension from run 1, because the hatching prophecy already gives it four Ascension rumors, so carry equals oracle:ascension here. Against the proposed targets: a first real ending by run 2 holds for every bot at about 0.5 or better; a second kind by run 4 only for amnesiac (Myth 0.20); Ascension by run 8 to 10 not reached.

Interest tool before (main) and after (branch), 300 seeds, generated worlds: see the table in section 2b.

## 3. Decisions taken

- Mortality's stand-in voice is a living `sage` or `priest` of the court (no `witch` or `god_voice` tag exists). "Received or brokered within 2 wakings" is an `act_receive` or `act_broker` event naming the court since the second-newest `wake`; `act_broker` never occurs until item 09.
- Conditions are evaluated separately unless `with_previous`; then they are matched together with the conditions before them so they bind the same court (Mortality's audience).
- A condition that was met counts as held from the tick it was first met; a gap is the years since it was last met, and a gap longer than `lapse_years` resets it (field names `held_since` and `last_met` instead of `last_lapse`). Ready needs the condition met now. Lapse years for Mortality and Myth are 0 (the brief gives none).
- Ascension placeholders: cult share and fervor read `faith ≥ 0.4` (`≥ 0.5` in two); a consecrated temple reads a `dragon_temple` monument; the high priest reads a living priest in a cult court that has buried a priest before; persecution is not checked; "4 consecrate acts" reads `give`, `miracle` or `consecrate` acts, since `consecrate` does not exist yet. All marked in `endings.json` only.
- Myth: "its name appears in ≥ 3 legends" reads as three distinct dragon acts some court keeps in written or monumental memory (`legends` field). "Two courts hold it at monumental tier" is `memory_tier` monumental.
- The legend's tier is the tier most courts held the dragon at when it died; a woven legend tag lasts 30 decades for oral, 100 for written, for ever for monumental. Weaving rolls on `rng_for(seed, "legend", id)`; the most extreme verdicts are tried first. Each woven legend adds exactly one world tag `legend:<id>` and one to three pre-history events tagged `src:legend`.
- Legend years −900 to −200 are relative to the hatching, clamped to at least the first decade.
- The prophecy is written 100 to 300 years before the hatching into the most lettered court: keeps records first, then era, then written memory. Its known tag is inherited by successors.
- Spreading: "a take books from that court" spreads it to the act's witnesses; "teach(letters) by that court to another" has no court-to-court act in the sim (diffusion emits no event, teach is the dragon's spell), so a `teach` cast on a knowing court spreads it to its neighbours.
- The three cards roll on a keyed stream (`keyed: true`), so no world draw moves; the schism branch is chosen by awe against dread, not a roll. `prophecy_slayers` adds anti_dragon +0.2 once and a tag; it does not set the `slayers` tag.
- The prophecy and woven legends have importance 0.45, below the storyteller's major (0.5), so writing a book does not reset the storyteller's quiet clock; they are always on the first screen through `legends.always`.
- The epilogue is 50 idle decades for every exit; Mortality also adds the dragon as a sage of that court (tag `was_dragon`). Exits are recorded inputs.
- The played policy and the golden runs never take an exit. Bots take any exit offered.
- Rumor routes implemented: `death_report` (one to three closest conditions), `prophecy` (a line encodes it), `legend` (a woven legend ended this way), `event` (a briefing told an event of the listed kinds). Every enabled condition has three routes. The level-3 counterfactual and the "source mentioning another dragon" routes wait for item 05.
- Bestiary entries are slayer orders and defences met, with the waking, years since the hatching and the court's era. `seen.json` is passed through untouched (item 10 owns it).

## 4. Deviations

- The pacing bots' steering is two data policies plus receiving every visitor; there is no per-condition planner. Leverage is far from the targets for that reason as much as for the endings' numbers.
- The page shows a Myth line on the death screen and keeps the profile; it has no exit button or epilogue view yet (item 13). Everything it needs is in `world.endings_view`, `world.prophecy_view`, `world.ending`, `world.myth`, the briefing's `endings` and `prophecy`, and the `exit` command.
- Mortality's offer is an offer row, not an audience arrival.

## 5. Goldens

Rules 7 to 8. Commits: chronicles, chronicle hashes, replays and save; the 300 state hashes; the page. The fixed-start goldens (`ages_v0_1_raze`) move because the endings rule emits `ending_ready` and `ending_lapsed` and the `rite` proposal is gone from the envoys; `ages_v0` did not move. Generated worlds: the first difference is the `prophecy_written` event in the pre-history (seed 1 at 380, seed 2 at 370, seed 3 at 300); after it the courts' new sages and priests take up the prophecy as the court's oldest mark (keepers), and after the hatching the three cards fire (seed 1: cult 680, slayers 740 and 770, Mortality ready 830, schism 840). Over the run seed 1 has 235 fewer burns and seed 2 359 more (counts from the first golden run, before the importance change; the shape of the diff was the same after it).

## 6. Open questions

1. Ascension never gets ready: no played run has more than one court with awe ≥ 0.6 and faith ≥ 0.4. Is that item 12's job (cults raising faith), or should the steering bot be smarter first?
2. Mortality is easy for a player who knows it (oracle 0.93 at run 1, leverage 3.5 against a target of at most 1.2). Make it harder (longer hold, awe 0.5) or accept that knowing helps?
3. Should a bot, and the page, offer an exit the player is not aiming at? Now Mortality ends every run that reaches it, which hides the Myth and Ascension for carry and oracle.
4. The German lines in `endings.json` are a draft for review.

## 7. How to verify

```
cmake --build build -j2 && ctest --test-dir build -L fast
python3 tools/endings_rates.py --bin build/hoard-sim --data data --seeds 1-300 --jobs 2
./build/hoard-sim --data data --seed 42 --years 3000 --genesis --policy played --out out/b42   # out/b42/legends.json: prophecy; briefing.json: endings
./build/hoard-sim --data data --seed 1 --years 3000 --genesis --policy played --bot oracle:mortality --profile out/p --out out/m1   # ends by Mortality
node web/smoke.mjs --golden 1-60
```
