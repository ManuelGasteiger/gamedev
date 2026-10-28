# 04 · Endings, prophecy, legends: progress

Branch `item/04-endings-prophecy` from origin/main 7d8c372. Rules version 7 on main.

## Plan

1. state.h: `World.endings`, `World.prophecy`, `World.ended` (own commit).
2. Data: `data/endings.json` (11 entries, 3 enabled), `data/modifiers.json`, `data/text/{en,de}/endings.json`, prophecy cards, exit offer row.
3. Loader `endings_data.cpp`, `DataJson.endings`, data hash, `web/build.sh`, wasm load.
4. Predicate extensions in `card_match.cpp`: `count`, `within_wakings`, new fields.
5. `rules/endings.cpp` after cards; briefing block; world view.
6. `exit` verb (recorded), epilogue, exit offer rows replace the `rite`/`exit` proposal.
7. Prophecy at genesis, spread by take(books)/teach, legends always shown.
8. Profile (five files), legend weaving at genesis, Myth on death, atoms; runner `--profile`, `--bot`; wasm `profile`.
9. Tests `ending_continuity`, `myth_legend_roundtrip`.
10. Measurements (300 seeds, bots), golden commit, wasm, checks, report.

## Done

- Baseline interest run on main started (scratchpad/main).

## Next

- Step 1.

## Blocked

- nothing
