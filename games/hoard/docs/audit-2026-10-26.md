# Hoard: code audit against the docs (step 0)

Status 2026-10-26. The first look at the code in `D:\gamedev` since the design and prompt work of 2026-10-22 to 25. Read-only: nothing in the tree was changed. Built and run in a cloud copy of `games/hoard` (without `assets/`, `art/`, `Hoard/`, `out/`). This is the audit `implementation-plan.md` step 0 asked for; its findings change the step-0 list and five prompts (section 11).

## 1. Version control

- `D:\gamedev` is a git repository with **no commits**. Nothing in it is versioned yet, including the v0.4 sim, the sandbox and the prototype build. No GitHub repository named `hoard` or `gamedev` exists on the account.
- `games/hoard/out/` holds three tarballs from the chats (`src.tgz` 2026-10-16, `hoard-v04-update.tgz` and `hoard-v04-savekey.tgz` 2026-10-19). They are the only history there is. `out/` is ignored, so they would not survive a clone.

## 2. Build and tests

| check | result |
|---|---|
| GCC 13, Release | builds clean |
| Clang, Release | builds clean, no warnings in the grep |
| `ctest` (6 tests) | 6/6 pass; 118 s total, `policies` 49 s, the four `fast` tests 0.7 s |
| chronicle, seed 7, 3000 years | byte-identical between GCC and Clang |
| `node web/smoke.mjs` with the committed `web/ages.wasm` (2026-10-19) | both goldens match native: idle `da26ca7f0a56d198`, raze `0648494cdfcb5bf9` |
| MSVC | never built: Visual Studio 2022 and 2026 are installed without any C++ toolset (`setup-report.txt`, still true) |
| wasi-sdk on the PC | not checked (`D:\tools` is outside the connected folder) |
| `web/game-check.cjs` (Playwright, software GL) | 90/90 pass, no console errors |

## 3. Speed of the sim

The preamble and items 05 and 14 assume "about 10 ms for 3000 simulated years". The README says 0.3 s, and a run here took 0.46 to 1.0 s on a loaded 2-core box (seed 5: 0.52 s plain, 1.04 s with `--genesis`, 0.46 s `played`). The real figure is 30 to 100 times the documented one. Consequences: item 14's acceptor budget ("30 % of 10 ms") has to be restated against a measured baseline; item 01's sweep (6 policies × 300 seeds) and its bucket analysis (7 forecasts per waking) need `--jobs` and a time estimate; item 17's perf gate should record the real number first.

## 4. Determinism exposure (item 17's size)

- Banned math: `std::pow` in 6 places (`hoard.cpp`, `talents.cpp`, `rules/memory.cpp` ×2, `rules/remnants.cpp`, plus `0.5^x` decay forms), `std::sqrt` in 3 (allowed). No `exp`, `log`, `rand`, `<random>`, `unordered_*`, `chrono` or `time` in `sim/`. No `float` anywhere.
- CMake sets warnings only; no `-ffp-contract=off`, no `/fp:strict`. The GCC/Clang match today is luck plus no FMA at the default x86-64 target.
- One golden regeneration for the own `pow`, as item 17 expects.

## 5. Inputs outside the act log (the replay question)

`World.acts` is written by `apply_acts` and by `demand`. The other nine state-changing API verbs leave no act: `sleep` (its length), `pick_talent`, `accept_offer`, `decline_offer`, `answer_hook`, `kill_person`, `cast`, `terraform`, and `load_world`. `sponsor_remnant` goes through `apply_acts`. So a replay from `(seed, data, World.acts)` cannot reproduce a played run today. Item 17's `.hrp` must record every verb call, and item 09 makes `sleep`, `pick_talent`, hook and offer answers into logged acts. The page also restores runs through `load_world` (three call sites), which item 17's save header replaces.

## 6. State the prompts assume

| prompt says | tree has | verdict |
|---|---|---|
| 02: `Faction.fear` and `demand_fear` | `fear` (10 uses), `demand_fear` (7) | matches |
| 02, 05: no tiers, no monuments, no historian | none of them exist | matches |
| 03: bed gives `bed_rate × sqrt(gold)` per window | exactly that in `hoard.cpp` | matches; the 2026-10-24 decision is still to do |
| 07: four person traits to keep | `PersonTraits` struct, `memory_of_dragon` scalar | matches |
| 12: `Culture` struct to replace regimes | `Culture` is a race template (lifespan, growth, temperament, traits); no vector, no creed | matches |
| preamble rule 8: `dragons[]` | `Dragon dragon` singleton | to do, first item that touches `World` |
| 01: the burn-everything policy | `data/policies/raze_all.json` | name it in 01 |
| 17: `long_1000` policy in the replay acceptance | does not exist; 01 creates `long_sleeper` | one name needed |
| 16, 18: `src/dragon/flight.js` and the prototype modules | not in the repo, see section 7 | blocker for 16 and 18 |

## 7. The flight prototype

- `prototype/index.html` is the minified single-file build (673 KB). The modular source (`hoard-src.zip`, about 30 modules, `src/config.js`, Playwright tests) was sent as a file attachment in the chat "Dragon game concept development" (2026-10-10) and never reached the repo. The published "Hoard" artifact is the same single page. Download the zip from that chat into `prototype/src/` before item 16.
- The frame loop uses variable `dt` clamped to 0.05 s; only a test hook (`window.__step`) steps at 1/60. There is no fixed step. Item 16 must introduce the 60 Hz step and item 18's golden trace (Node replay of `flight.js` at 60 Hz) depends on it.
- The flight constants are in the build as `{vStall:14, vMax:85, thrustCruise:7.5, thrustBoost:18, drag:.0062, turnRate:1.35, ...}`, matching `CLAUDE.md`.
- The valley sandbox (`web/hoard-game.html`) is the tile-based tactical shell; its flight is camera-relative WASD over tiles, so item 16 is a new page as planned.

## 8. The Unreal project

- `games/hoard/Hoard/` (not `unreal/Hoard/`), Blank, **no `Source/` folder**: a Blueprint-only project. `Content/` is empty apart from developer folders. Default map is the engine's OpenWorld template.
- `DefaultEngine.ini` enables Lumen GI and reflections, hardware ray tracing, Virtual Shadow Maps, Substrate and distance fields, with SM6 as the D3D12 target. The PC's GTX 980 (4 GB, Maxwell) is below Epic's Lumen minimum (`tool-setup.md`). Item 18's Lean profile has to switch these off; nothing in the project is worth keeping, so 18 starts a fresh C++ project in `unreal/Hoard/`.
- Plugins enabled: Modeling Tools, Model Context Protocol, All Toolsets.

## 9. Measured numbers (v0.4 on main, 300 seeds × 3000 years, generated worlds)

Run here with `tools/interest.py --genesis`, policies idle, raze_all, played (GCC build, about 45 minutes on 8 jobs).

| policy | outcome entropy | distinct outcomes | dragon slain | unwarned | hooks per waking | answered | agency |
|---|---|---|---|---|---|---|---|
| idle | 4.82 bits | 65 | 0 of 300 | 0 | 2.05 | 0 % | 0.00 |
| raze_all | 4.88 | 66 | 292 (263 by wounds, 29 slain asleep; median year 1290) | 0 | 1.82 | 45 % | 0.90 |
| played | 4.03 | 47 | 64 (21 %; 53 slain asleep, 11 by wounds; median year 2230) | 0 | 1.78 | 62 % | 0.90 |

- The v0.4 numbers in `core-loop.md` §9 reproduce: played deaths 21 % (band 5 to 25 %), idle 0 %, burn-everything 97 %, no unwarned death, nine starting templates with none above 16 %, undead rising on their own in 19 % of idle runs.
- **Played outcome entropy is 4.03 bits, under the 4.5 target**, and the played dragon's valleys converge: orcs are the largest race in the four commonest played outcomes (174 of 300 seeds), against humans in idle. Burning the settled courts hands the valley to the orc remnants. Item 01 should report this as a finding; it is a candidate for item 02 (dread keeping orcs in check too) or a band.
- Hooks: 17 kinds under played, libraries the commonest (24 %), 3,104 hook fights.
- The interest tool's "same first three event kinds in 100 % of seeds" flag fires on `world_began, court_founded, court_founded`, which are structural events. The flag should skip them (item 01).

`web/game-check.cjs`: 90 of 90 checks pass on the committed `hoard-game.html`, no console errors (headless Chromium, software GL).

## 10. Docs, strings, assets

- `games/hoard/docs/` has 9 files; the project has about 80. Missing in the repo: `decisions.md`, `act-api.md` and its schema, `glossary.md`, `raid-design.md`, `story-and-endings.md`, `first-hour.md`, every `research-*`, `scenarios/`, `patterns/`, `prompts/`. Item 17 copies them.
- Text: `data/text/en.json` holds 668 templates (events 210, causes 207, reactions 76, words 88, misc 32, roles, motives, pretexts, asks). About 69 English string literals sit in `sim/*.cpp` (18 in `spells.cpp`, 8 in `briefing.cpp`), and refusal reasons are built in English (`"needs knowledge " + num(...)`). Item 19 moves them; item 09's reason keys should land first or together.
- Assets: `assets/` 942 MB, of which `_unused/` about 330 MB (deletable per `assets/README.md`); `art/` 286 MB; `Hoard/` 244 MB of caches (ignored). The MattBas dragon (CC BY-SA 4.0) is in both `assets/gltf/creatures/dragon-placeholder` and `art/dragon-placeholder`. `art/redragon/redragon.blend` has no recorded licence.
- GitHub LFS on a free account includes 10 GiB storage and 10 GiB bandwidth a month [1]. About 0.9 GB of assets after deleting `_unused/` fits the storage, but every cloud agent that clones with LFS pulls it again: ten clones a month use the whole bandwidth. Sim and tooling agents must clone with `GIT_LFS_SKIP_SMUDGE=1`.

## 11. Corrections to the prompts before their agents start

1. **Preamble**: the repo is `hoard` with `sim/` at the root (not `hoard-sim/`); replace "about 10 ms" with the measured time from item 17's perf job.
2. **17**: the runner binary is `ages` and the goldens live in `sim/tests/golden/` (rename to `hoard-sim` and move to `tests/golden/` as part of the layout step, or keep the names; pick one in the item); the replay acceptance names `raze_all` instead of `long_1000` until item 01 adds it; the input audit already has its answer (section 5); move `Hoard/` to `unreal/Hoard/` or delete it, since 18 starts fresh.
3. **01**: the burn-everything policy is `raze_all`; call the 1000-year policy `long_1000` in both 01 and 17; give the sweep a time budget with `--jobs` from section 3.
4. **14**: restate the acceptor cost limit as a share of the measured step time.
5. **16 and 18**: the prototype source path is `prototype/src/` after the zip is restored; 16 adds a fixed 60 Hz step; 18 sets the Lean renderer settings explicitly, since the current ini turns Lumen and ray tracing on.
6. **19**: include the English refusal reasons built in `sim/` (move to reason keys together with item 09's `gates.json`).
7. **15**: the sandbox check has 90 checks, not 66.
8. **decisions.md**: the Open line on the demo cut still says "hatching to first moult plus two wakings"; `first-hour.md` §8 and Manu's correction say waking 7 or 8 (45 to 80 minutes).

## Sources

1. GitHub Docs, "Git LFS billing". https://docs.github.com/en/billing/concepts/product-billing/git-lfs
