# 04 · Endings, prophecy, legends: progress

Branch `item/04-endings-prophecy` from origin/main 7d8c372. Rules version 7 on main.

## Done

- state.h, trigger language, data plumbing, serialise/briefing: separate commits.
- Registry `data/endings.json`, `rules/endings.cpp`, exit verb and epilogue, exit offer rows (rite gone).
- Prophecy at genesis, spread, three keyed cards; legends `always` kinds.
- Profile (five files), weaving, notes, Myth entry; runner `--profile`, `--bot`; tools `endings_rates.py`, `endings_runs.py`.
- Tests `ending_continuity`, `myth_legend_roundtrip` pass. `golden` fails until the golden commit.
- wasm `exit`, `profile`, profile on load/reset; page keeps the profile (`web/game/profile.js`), death screen Myth line.
- Baseline interest on main: `scratchpad/main/interest_before.txt` (not in the repo).

## Next

- 300-seed rates (running), pacing bots, interest after.
- Golden commit (rules 8): delete `tests/golden/ages_v0*.hash` and `sift_bank_v1.hash`, run `build/test_golden` twice, move the `.txt.new` chronicle texts in place, regenerate `state_hashes.txt` (`seq 1 300 | xargs -P 2 -I{} build/hoard-sim --data data --golden {} | sort -n`), replays (`--seed N --years 3000 --genesis --policy played --record tests/replays/played-N.hrp`), `tests/saves/v1-seed4.expect` (nightly command), then wasm + pages.
- Checks, report.

## Blocked

- nothing
