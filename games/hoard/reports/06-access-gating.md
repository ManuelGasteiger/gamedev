# Item 06: access gating for visitors

Branch `item/06-access-gating`, from 3836e41. Nothing paid, no external calls.

## What changed

- `data/arrivals.json`: eight kinds (envoy, pilgrim, thief, slayer band, scholar, kin, refugee, prophet) with `requires`, `blocked_when`, `cooldown_years`, `specificity`, `person_role`, `text_key` and, for pilgrim, scholar, kin, refugee and prophet, the `answers` the verb `receive` accepts.
- `rules/arrivals.cpp`, after oaths. Per court per tick the most specific kind whose gates hold, none blocked, off cooldown, comes. One visitor per court per decade (the slayer band is exempt) and no second visitor of a kind that already waits for the court. The rule also runs once at the waking, so the offers match the state the dragon wakes into. Arrivals carry `because`, the gate keys that let them through.
- Lair knowledge and routes (`arrival_gates.cpp`): `Faction.lair_known` and `lair_exact`, `route_open`, learned from witnessed acts near the lair, a show at a capital, a released hostage, or a visitor who walked home. Knowledge fades with the oral tier unless a written record keeps it, and lasts at least `lair_memory_years` (100).
- Verbs `receive {id, outcome}` (receive, eat, turn_away) and `hunt {region}`: recorded wrappers in `recorded.cpp`, `apply_input` cases, wasm commands. `receive` on a kind with a hook goes through the hook, so the hook cards keep working.
- Hooks thief, slayers and pilgrims read the arrival (`arrival` key in `data/hooks.json`); pilgrims gain a turn_away choice. Their old `when` and `force_if` triggers are gone; rewards of pilgrims moved to `answers` plus the `receive` rows of `data/reactions.json`.
- Briefing block `ledge` (one line per waiting visitor with its because-line), wake page section "at the ledge".
- Narrator lines and importance rows for arrivals, `ledge.<kind>` and `because.<gate>` misc keys; ui reason lines `reason.no_arrival`, `reason.no_outcome`, `reason.not_wild`, `wake.ledge` in en and de.
- Tests: `arrival_gating`, `arrival_cooldown` (both fast).
- `tools/interest.py` prints arrivals per waking by kind and the ten most common because-lines; the runner's wakings log carries the arrivals.

## Numbers

Acceptance:

| Item | Result |
| --- | --- |
| Idle, 300 seeds x 3000 years, `--genesis`: arrivals | 0 (met) |
| Envoys only at courts with written memory or exact knowledge | by gate `lair_on_record` (met) |
| Pilgrims only where awe_eff >= 0.15 | by gate `awe_eff` (met) |
| Because-line non-empty and naming a clause | every arrival carries at least its gate keys; checked in `arrival_gating` (met) |
| Arrivals per waking after year 200 under played | 1.36 (12959 in 9535 wakings), every run has some (met, band 0.5 to 1.5) |
| v0.4 bands that passed after item 02 | all still pass; `played.outcome_entropy_bits` 3.948 fails as it did before (3.869 at the baseline run) |

Arrivals per waking by kind, played, 300 genesis seeds, years 200 to 3000:

| Kind | Per waking |
| --- | --- |
| envoy | 0.319 |
| slayer band | 0.392 |
| thief | 0.394 |
| prophet | 0.191 |
| pilgrim | 0.041 |
| refugee | 0.018 |
| kin | 0.005 |
| scholar | 0 |

Ten most common because-lines, played:

1. gold_rumour + era + knows_lair_exact (3756)
2. slayer_order + dread_eff + band_ready (3733)
3. prophecy_or_cult + awe_eff + piety (1825)
4. lair_on_record + cohesion + route_open + motive.fear_of_conquest (928)
5. lair_on_record + cohesion + route_open + motive.survival (805)
6. lair_on_record + cohesion + route_open + motive.catch_up (548)
7. faith_share + knows_lair + awe_eff + route_open (391)
8. lair_on_record + cohesion + route_open + motive.protect_the_line (179)
9. lost_region + awe_or_oath (172)
10. lair_on_record + cohesion + route_open + motive.legitimacy (170)

Played, other measures: death share 0.097, unwarned deaths 0, hook coverage 0.992, first-three top share 0.08.

Checks: `ctest -L fast` 15/15, `web/game-check.cjs` 90/90 with no console errors, `web/l10n-check.cjs` passed, `node web/smoke.mjs --golden 1-60` matches the new goldens.

## Decisions

- The slayer band keeps the slayer clock; the arrival only reflects it (`band_ready` reads the clock and the `slayers_coming` tag). No bolt-thrower requirement (item 08 owns the roster).
- A killed envoy blocks that court's envoys for `envoy_kill_generations` x 30 years. The 30 is my assumption; the docs give no generation length.
- The cooldown key is (court, kind, cause). For envoys the cause is the offer kind of the motive.
- `faith_share` reads `Faction.faith` until item 12 (cults) exists. `prophecy_or_cult` reads the court tag `prophecy` or awe_eff at `cult_awe` until item 04. `taught` reads an active teach modifier until item 09 logs teach as an act.
- Values not in the docs, set by me and listed in the note of `data/arrivals.json`: `oral_floor` 0.05, `lure_gold` 60 (the old thief hook's number), `hunt_food` 10, `slopes_hunt_mult` 1.5, `lair_memory_years` 100, `rumour_years` 150, `refugee_years` 20, `sight_regions` 2, the specificity numbers, and the answers of scholar, kin and prophet.

## Deviations

- The envoy needs `route_open` (brief item 4) in addition to the table's gates.
- Pilgrim hook rewards were moved out of `data/hooks.json` into the arrival answers.
- The rule also runs at the waking; the brief names only the per-decade evaluation.
- `lair_memory_years` is my addition. Without it a show over a capital was forgotten within a sleep and the court never sent an envoy to the next waking.
- Scholar arrives 0 times in played: `taught` has nothing to read until item 09.
- Changed existing check `web/game-check.cjs`, forced: an envoy now climbs only to a lair its court has seen, so the lab (a) lets every court see the lair (`seeLair`, through `loadWorld`) before the sleeps that wait for envoys, (b) sleeps on for up to ten centuries until an oath breaks, and (c) gives the orc lab up to eight centuries. Before this the history shifted and these checks found no event in their windows. No assertion was weakened.
- Process: I stopped runs with bare `pkill -f` patterns several times; one of those very likely killed the lead's game-check browser run. I now stop only my own processes by PID.

## Goldens

Two `golden:` commits on the branch (the second after lair memory and the waking check changed the world stream), the first bumping `kRulesVersion` 3 to 4. Each regenerates `tests/golden/state_hashes.txt`, the chronicle hashes and texts, `tests/replays/played-1..3.hrp`, `web/ages.wasm`, `web/hoard-game.html` and `web/ages-sandbox.html`. The arrival lines are added to the chronicle, and from the first arrival on the histories diverge. On merging with item 14 the lead regenerated everything once more under rules 6.

## German lines, unreviewed

`reason.no_arrival`, `reason.no_outcome`, `reason.not_wild`, `wake.ledge` in `data/text/de/ui.json`. The narrator text is English only.

## Open questions

- Prophet visits are 0.19 per waking, more than the brief seems to expect; item 04 will replace the stand-in gate.
- Slayer bands and thieves make up about 58 % of the arrivals. Do you want a lower share?
- Should a show at a capital give exact knowledge? Now it does not, so a court that only saw the dragon sends no envoy until it has a written record or a visitor came home.

## How to verify

- `ctest --test-dir build -L fast`
- `python3 tools/interest.py --bin build/hoard-sim --data data --seeds 1-300 --years 3000 --policy played --genesis --bands --jobs 2`
- `node web/game-check.cjs` and `node web/l10n-check.cjs`
- Idle arrivals: run `--policy idle --genesis` and count `arrival` events in `chronicle.json`.
