# 15 · `RaidReport` and `raid_diff` in the core

Model: Sonnet 5.5 · Effort: medium · Depends on: 09 (the act plumbing) · Branch: `item/15-raid-diff`

## Goal

Both shells report facts; the core turns facts into acts. Add the `RaidReport` record to the wasm and library API, the `raid_diff` rule set (`data/raid_diff.json`) that maps facts to acts with magnitudes relative to what the court had at sortie start, and route the valley sandbox's action panel through the same path so there is one way into the sim. `ages-sim.md` open question 4 (where the awake-phase diff lives) is settled: here.

## Notes for this tree (2026-11-07)

- Rules version is 11 (`sim/ages/record.h`), after items 08 and 09. Item 10 runs at the same time. Goldens stay unchanged (the policies keep using `apply_acts`). If a golden has to change anyway, set `kRulesVersion` to 12 in a `golden:` commit and say why; I regenerate once when merging the two.
- `docs/raid-design.md` is now in the repo.
- **Which shell.** Two pages call `apply_acts`. The valley sandbox `web/hoard-game.html` (built from `web/game.template.html` and `web/game/*.js`, checked by `web/game-check.cjs`, now 90 checks) is the tactical shell this item routes through `end_sortie`. The debug page `web/ages-sandbox.html` (`web/sandbox.template.html`, checked by `web/check.cjs`) keeps `apply_acts`; `check.cjs` must still pass. The brief's "66-check game-check" is now 90 checks.
- **Item 09 facts you build on.** All 40 schema acts are accepted by the API with gates in `data/gates.json` and reason codes; acts carry `source`, `answers`, `secondary` and a typed payload; `burn` takes `structure`; `fight` resolves through item 08's `resolve_fight`; `receive` exists. `tools/act_check.py` validates act logs against `docs/act-api.schema.json`. Read `reports/09-new-acts.md`.
- **State and saves.** `begin_sortie` and `end_sortie` change state, so each needs a wrapper in `sim/ages/recorded.cpp` and a case in `apply_input`, or replays break. Decide where the sortie snapshot lives so that the goldens and the save format stay unchanged when no sortie is open, and say how a save taken mid-sortie behaves.
- **Seed 42.** The acceptance names "Wellmere Bridge" and "Dorun"; the world generator may name seed 42's regions differently now. Use seed 42's actual first-waking regions and say which.
- Player text goes in `data/text/<lang>/*.json` with an `en` and a `de` line for every key; the sim emits codes (`code|key=value`, `ages.h` `reason_code`). Warnings in the report result are codes too.
- The diff is pure arithmetic: no rng.
- Item 10 also touches `sim/wasm/api.cpp`, `recorded.cpp` and `CMakeLists.txt`. Keep your additions in your own files and small at those three places.
- Tests: add none. List the proposed tests in the report; Manu decides. `tools/raid_report_check.py` is a tool, not a ctest.
- `tools/act_check.py` needs `jsonschema` (`pip install --break-system-packages jsonschema`).
- The machine has 2 cores and a limited disk. Write run output under the scratchpad or `/tmp`, delete it when done, and never commit it. LFS uploads are refused: do not commit `web/sandbox-check.png` or other LFS files.
- Before you finish: `ctest --test-dir build -L fast`; the wasm build (`git lfs pull --include "games/hoard/assets/gltf/**"` first) and `node web/smoke.mjs --golden 1-60`; `node web/l10n-check.cjs --lang pseudo,de`; `node web/game-check.cjs`; `node web/check.cjs`. Push the branch; no pull request.

## Read first

- `docs/raid-design.md` sections 1 and 3 (the report shape and the diff table; the rules of the diff).
- `docs/act-api.md` sections 1, 3 and 4 (what the shell owes the sim per act; witnesses are the sim's job).
- `docs/sandbox-game.md` "Raids" table and "API notes" (how the tactical shell emits acts today).
- `acts.cpp`, `api.cpp`, the sandbox's `web/game.template.html` and `web/game/*.js` files that call `apply_acts`.

## Scope

In:

1. **`RaidReport`** struct and JSON (de)serialisation exactly as `raid-design.md` section 3: `sortie`, `settlements[]` with `destroyed` by building class, `collected` by category plus `items[]`, `killed` by kind plus `persons[]`, `taken[]`, `presence`; `armies[]`; `lair` (`dropped`, `received[]`, `fought[]`); `spells[]` and `speech[]` as acts; `wounds` by source.
2. **Sortie snapshot**: on `begin_sortie`, the core records per settlement its total building value, population, garrison and the court's wealth, army sizes and holdings per category, so the diff's shares are relative to the start of the sortie, never to the current state.
3. **`raid_diff`** (`sim/raid/diff.cpp`, rules in `data/raid_diff.json`): the table in `raid-design.md` section 3, one act per (type, region, structure) per sortie with accumulated magnitude; `burn` on a settlement also yields `kill` civilians from the report's fire toll; `show` kinds from presence (land 0.5, roar 0.3, flyover 0.1 only when nothing else happened at that settlement); `defend` classification from the armies list and the sim's war and oath state; `take_hostage` from `taken[]`; `receive` and `fight` from the lair block. Spells and speech acts pass through unchanged (they are acts already). Witnesses are derived by the sim as for any act.
4. **API**: `begin_sortie`, `end_sortie {RaidReport} → {acts[], refused[], lines[], hooks_resolved[]}` (applies the derived acts through `apply_acts` and returns what the page already reads), `drop_loot` as part of the lair block. `apply_acts` stays for the debug page, the policies and scenario answers.
5. **Tactical shell routing**: the valley sandbox's panel actions become facts: "Burn 0.4" → `destroyed.house = 0.4 × settlement value`, "Take" with the slider → `collected`, "Kill army" → `armies[].killed_share`, "Roar" → `presence.roared`, hostage → `taken[]`; the panel's immediate feedback keeps working because `end_sortie` is called after each panel action (a sortie of one act). Speech acts and spells stay as they are.
6. **Validation**: `tools/act_check.py` (item 09) validates the acts `raid_diff` emits; a `tools/raid_report_check.py` validates reports against a `docs/raid_report.schema.json` added here.

Out: the flight shell (item 16), any change to reaction rows or stamina (the shell pays stamina; the core only validates gates).

## Design notes

- Magnitude clamps to [0, 1]; a report that destroys more than the snapshot held (fire after the snapshot) is capped and logged as a warning in the report result, not refused.
- Determinism: the diff is pure arithmetic over the report and the snapshot; no rng.
- The civilian toll: `killed.civilians` comes from the shell's crowd system; the core does not invent casualties. The tactical shell has no crowd, so it reports `civilians = burn_share × population × civilian_toll_factor` (constant, start 0.2) to keep both shells' acts comparable; put that factor in `data/raid_diff.json` and say in the report result which shell produced it.

## Acceptance

- The sandbox plays as before through the new path: `game-check.cjs` passes unchanged in outcome; the act log it produces for a fixed script is identical in types and within 1 % in magnitudes to the direct `apply_acts` log.
- A hand-written `RaidReport` for seed 42's first waking (burn a settlement 0.5, take 120 gold, roar at a second settlement, drop at the lair) yields `burn 0.5`, `take` with the right share, `show roar`, `kill civilians` from the toll, and the expected dread/awe deltas.
- Goldens unchanged (the policies still use `apply_acts`).

## Proposed tests (confirm with the owner first)

1. `raid_diff_shares`: a report destroying 40 % of a settlement's house value and collecting 10 % of the court's gold yields `burn 0.4` and `take 0.1` relative to the sortie snapshot, even if the court's wealth changed during the sortie.
2. `raid_diff_one_act_per_structure`: three partial library burns in one sortie yield one `burn(structure: library)` with the summed share.

## Report

Standard report, plus the act log comparison table (direct vs routed) for the sandbox script.
