# Preamble for every Hoard implementation run

You are implementing one item of a game simulation called Hoard for its designer and owner, Manu. The item brief follows this preamble. Read the preamble once, then the brief, then the files the brief lists, in that order, before writing any code.

## The project in four sentences

Hoard is a single-player strategy game: you are the only dragon in a valley of twelve regions; you raid, then sleep for years or centuries while a deterministic simulation advances the mortal factions (humans, dwarves, elves, orcs, later undead and others); you wake into the result, changed by what you slept on. The spine is an arms race: mortals grow by time, the dragon only by digesting loot; sleep length is the bet. Death is permanent and every danger is telegraphed first. The simulation is C++17 in `games/hoard/sim/` of the `gamedev` repository, data-driven from `games/hoard/data/*.json`, deterministic from (seed, data, act log), about 0.3 to 1 s for 3000 simulated years with people and cards (measured 2026-10-26; item 17's perf job records the exact figure), tuned with a headless runner and an interest tool over hundreds of seeds; a browser shell (`web/`) plays it through a wasm API.

## Where things are

- Repository: `github.com/ManuelGasteiger/gamedev`, private. The game lives in `games/hoard/`; run every command from there. The repo root holds shared skills (`.claude/skills/`), suite notes (`docs/`) and tools for all games.
- Clone without the large binaries unless the item needs assets: `GIT_LFS_SKIP_SMUDGE=1 git clone ...`. The free LFS bandwidth is 10 GiB a month.
- Design docs live in the claude.ai Project "Game Development" under `claude/`. The repo's `games/hoard/docs/` holds the subset copied so far; when a doc named below is missing there, read it from the Project (`claude/<name>`), and copy it into `docs/` with the same name if the item depends on it.
- The runner binary is `ages` (built to `build/ages`); goldens are in `sim/tests/golden/` until item 17 moves them.

## Read first, always

1. `README.md` in `games/hoard/` (file list, constants table, how to build and test).
2. `docs/ages-sim.md` sections 2 (design rules), 3 (state), 4 (the decade step), 11 (architecture), 14 to 19 (what was built and changed).
3. `docs/core-loop.md` (v0.4: growth, talents, arms race, hooks, measurements).
4. `act-api.md` and `act-api.schema.json` (the closed act vocabulary, v1.1).
5. `decisions.md` (every standing decision, one line each) and `glossary.md` (one name per thing; use these names in code).
6. `audit-2026-10-26.md` (what the code contains against the docs).
7. Whatever else the item brief lists.

The docs name files by their documented names. Verify each in the tree before editing; where the tree differs, the tree wins and your report notes the difference.

## Rules that are not negotiable

1. **Determinism.** The only randomness is `world.rng`. No `std::random`, no unordered iteration over factions, regions or people (sorted vectors by id). Same (seed, data, act log) gives the same chronicle hash on GCC, Clang, MSVC and wasm.
2. **Base rules stay untouched.** The thirteen steps of the decade step change only through modifiers read in `modifiers.cpp`. New behaviour is a new rule file under `sim/ages/rules/` plus data, inserted at a documented point in the order.
3. **Two inputs.** The sim reads the act log and the hoard. It never reads player intent. If a feature cannot be expressed through an act, a modifier, a spawn, an oath or a lair field, stop and write the question in your report instead of special-casing.
4. **Everything in data.** Every number goes in `data/constants.json` or the data file that owns it. Every user-facing string goes in `data/text/` as a template with tokens. No literals in rules.
5. **Goldens.** Golden hashes change only in a separate commit titled `golden: <why>`, after the chronicle diff has been read and the reason written in the report. A golden that changes without a listed cause is a bug in your change.
6. **Tests.** The six existing tests (determinism, baseline, reactions, oath_breach_sleep, policies, golden) must pass: `cmake --build build && ctest --test-dir build`. `ctest -L fast` runs 1, 3, 4, 6. Each brief lists proposed tests; add exactly those and no others unless the owner says so. He keeps the suite minimal and defines the list himself.
7. **No engine headers, no exceptions across the API, compiles with MSVC, GCC, Clang and the wasm32 target of wasi-sdk (`web/build.sh`; Emscripten would also work with the same `api.cpp`).**
8. **No singleton dragon in new code.** `World.dragon` is becoming `World.dragons[]` with index 0 as the player (decision review 2026-10-24, §7); new code takes a dragon index or reference, never the global. Whoever touches the struct first makes the change in its own commit.
9. **The shell never remembers.** Anything a shell shows about the past (soot, ruins, walls, monuments, figures) is read from sim state at wake (`world-aging.md`). If a visual needs a fact the sim does not store, the item adds the field to the sim, never a cache to the shell.

## How the owner likes code

- Programmer-readable, not beginner-readable. No comment essays, no file-header blocks, no restating what the code says. A one-line comment where a number or an order is surprising.
- One job per file, clean folder structure, but not a thousand files. A new rule is one file. A new act is a case in `acts.cpp` plus its reaction rows, not a subsystem.
- Names match the docs (`dread`, `awe`, `memory_tier`, `historian`), so the designer can read the code against the design.
- Prefer a data table to a branch. Prefer deleting to abstracting.

## How to work

- Branch `item/NN-slug`. Commit after each coherent step with a plain one-line message. Never squash the golden commit into another.
- Keep `reports/NN-slug.progress.md` (under `games/hoard/`) current: done, next, blocked. A replacement agent must be able to resume from it alone.
- If the brief needs a decision it does not contain, take the simplest option that keeps determinism and the base rules intact, record it under "Decisions taken" in the report, and continue. Do not stop to ask unless the decision is irreversible or changes another item's interface.
- Do not refactor outside scope. Do not touch `web/` unless the brief says so. Do not update design docs except the README's file list and constants table; the designer updates design docs from your report.
- Measure. Run `tools/interest.py` (and the sweep from item 01 once it exists) before your change on main and after on your branch, same seeds and years, and put both tables in the report. A 300-seed run of three policies takes about 45 minutes on 8 jobs; use `--jobs`.

## The report (`reports/NN-slug.md`)

1. What changed: files added, files changed, one line each.
2. Numbers: the interest tool before and after; the item's own acceptance metrics; baseline bands that moved and why.
3. Decisions taken that the brief did not contain.
4. Deviations from the brief, with reasons.
5. Goldens: changed or not; if changed, the chronicle diff summary.
6. Open questions for the designer.
7. How to verify in five minutes: commands and what to look for.

Plain prose, short. No praise of the work, no summary of the brief.
