# 17 · Repository, determinism, hashes, replays, CI: report

Branch `item/17-repo-determinism-ci`, on top of `item/00-docs-fixes`. CI run on the last commit: hoard-ci green on GCC, Clang, MSVC and wasm, compare passing, about 6 minutes.

## 1. What changed

Layout (one commit):
- `tests/golden/` (the two chronicle hashes moved from `sim/tests/golden/`), `tests/saves/`, `tests/replays/`, `rt/README.md`, `assets/LICENSES.md`, `reports/`.
- Runner renamed `ages` → `hoard-sim`; README, `tools/interest.py`, `tools/ages_stats.py`, `web/smoke.mjs` follow.
- `Hoard/` removed (Blueprint-only, empty content; item 18 starts fresh in `unreal/Hoard/`).
- `.gitattributes`: LF for `games/hoard/data/**`, `*.json`, `*.md`.

Determinism:
- `CMakeLists.txt`: `hoard_sim_flags` (`-ffp-contract=off -fno-fast-math`, MSVC `/fp:strict`) on the library, runner and every test; configure fails on a fast-math flag.
- `sim/core/detmath.{h,cpp}`: `det::exp`, `det::log`, `det::pow`, `det::half_life`; the six `std::pow` calls replaced.
- `tools/check_banned.sh` (test `banned_ops`); it found one real hazard, two `rng.chance` calls in one expression in `hooks.cpp` (order unspecified by C++), now two statements.
- `sim/ages/rng.h`: `rng_for(seed, tag, id)`.
- `web/build.sh`, `build.ps1`: the same flags for wasm, `sim/core/` compiled, `--wasm` compiles only, the build id from git.

Hashing:
- `sim/core/hash.h` (`Hasher`: values, −0 as +0, non-finite counted), `sim/ages/state_hash.{h,cpp}` (one part per top-level field of the saved world; the save and the hash cannot drift apart).
- Runner: `--trace t.jsonl` (state hash per sleep), `--state-hash`, `--golden A-B`; `tools/first_div.py`.
- wasm API: `state_hash`, `golden`; `web/smoke.mjs --golden A-B --out f`.
- `tests/golden/state_hashes.txt`: 300 seeds, generated worlds, played, 3000 years (its own `golden:` commit).

Saves and replays:
- `sim/ages/serialise.cpp`: a loaded world now continues exactly as the original (see section 6, bug found).
- `sim/ages/record.{h,cpp}`, `recorded.cpp`, `unrecorded.h`: every outermost call of the ten state-changing verbs is recorded; `apply_input` and `replay_lines` replay them; `kRulesVersion = 1`.
- `sim/ages/save.{h,cpp}`: header (`format_version`, `rules_version`, build, data hash, seed, year, acts, state hash) plus the world; refuses newer formats and hash mismatches; empty migration table.
- Runner: `--record`, `--replay` (exit 3 at the first bad checkpoint), `--save` (temp, rename, `.bak`), `--load-save`.
- wasm: the session records from `load`/`reset`, `report` returns replay, save and build; `events_since` no longer writes importance into the world.
- Page: Game sheet, "Report a problem" (`web/game/report.js`): base64 blob, preview, clipboard, nothing sent.
- `World.dragons` (vector, player at `kPlayer`), own commit, no hash moved.

CI: `.github/workflows/hoard-ci.yml` (push and PR), `hoard-nightly.yml`, `hoard-web.yml`.

Tests added (the list you confirmed): `banned_ops`, `detmath_bounds`, `rng_sequence`, `hash_canonical`, `save_roundtrip`, `replay_verify`. All `fast`; `ctest -L fast` takes about 5 s.

## 2. Numbers

- Sim time: 0.74 s per 3000-year generated world under `played` (300 seeds in 115 s on 2 cores). The 10 ms in the old docs was v0, before people and cards.
- Platforms: GCC 13, Clang 18, MSVC (`windows-2022`, `/fp:strict`) and wasm32 (wasi-sdk 25, Node 22) agree on seeds 1 to 60 in CI; locally GCC and Clang agree on 1 to 20, wasm and GCC on 1 to 10.
- `detmath_bounds` (10,000 inputs against glibc): exp 2.2e-16, log 2.7e-16 relative, pow 2.0e-14 and half_life 3.4e-14 (both dominated by the argument's own rounding).
- Interest tool, 300 seeds × 3000 years, generated worlds, before (main, audit §9) and after: identical at the tool's precision for all three policies (played: entropy 4.03 bits, 47 distinct outcomes, 64 slain, 0 unwarned, 1.78 hooks per waking, 62 % answered; idle and raze_all likewise unchanged). The `pow` change moves last bits, not outcomes.

## 3. Decisions taken

- The state hash walks the saved world's JSON tree (types, sorted keys, float bits) instead of a hand-written `fields()` per struct. It hashes values, never text, and whatever a save holds is hashed by construction.
- Trace and replay checkpoints are per sleep, not per decade (per decade with `--sleep 10`).
- `.hrp` files are plain JSON lines, not gzipped (no zlib in the tree; a 3000-year played run is 80 to 140 KB). Magnitudes are not quantised: nlohmann writes doubles in shortest round-trip form, so a replay reads back the exact bits. Quantisation stays with `RaidReport` (item 15), where shell floats enter.
- Recording is a wrapper around each verb (`recorded.cpp`); only the outermost call is an input, so nested calls (a hook answer that applies an act) are not recorded twice. A world copy (the forecast) never records.
- `demand` is recorded even when refused, because the refusal hardens the court.
- `banned_ops` runs on Linux only (it needs `grep -P`); MSVC is covered by the same code.
- The perf gate has no baseline yet: the first nightly prints the instruction count to copy into `tests/perf.json`.
- The web workflow runs Chromium only; Firefox and WebKit later, since the sandbox needs WebGL.

## 4. Deviations from the brief

- No `hoard-sim dump`; the trace names the part, and `--out` writes the full world.
- `sim/save/migrate.cpp` is a table inside `save.cpp` until a migration exists.
- The chronicle text goldens of the brief (three seeds as text with `chronicle_diff.py`) are not written: the two existing chronicle hashes plus the 300 state hashes cover it, and `first_div.py` finds the part. Say if you want them.

## 5. Goldens

- The two chronicle hashes did not move with the own `pow`: they hash text rounded to six decimals, which hides last-bit changes (the research's E9 warning, confirmed). Played generated worlds did change in the last bits, which the new state hashes pin.
- `tests/golden/state_hashes.txt` is new (one `golden:` commit). The `dragons` and save changes moved no hash.
- Negative controls: a build with `-march=haswell -ffp-contract=fast` moves all 20 of 20 state hashes checked, and `first_div.py` puts the first divergence at the first checkpoint (after genesis), parts `chronicle, factions`. The compare job's diff fails on an altered platform file and on an altered golden (same `diff -u` as the dry run in the research; run here by hand on altered copies: both caught, not yet run inside CI).

## 6. Found on the way

- **Save/load bug (fixed)**: a world loaded from `world_to_json` continued differently from the original in 18 of 20 played seeds. `faction.eff` (read by `apply_acts` for faith gain) and `region.eff_land` (read by the defences) were reset on load. The page's autosave hit this on every reload. Now 0 of 120 diverge (played, raze_all, protect_one; 40 seeds each), and `save_roundtrip` checks it.
- Correction (2026-10-26): `region.eff_land` is recomputed every decade step from `land`, home terrain and `land` modifiers (`rules/modifiers.cpp`), so terraforms and cataclysms do reach the defences. The earlier note here was wrong; nothing to fix.

## 7. Open questions

1. Chronicle text goldens: added on 2026-10-26 (`tests/golden/chronicle/seed-{1,2,3}.txt`, checked by the `golden` test, `tools/chronicle_diff.py`).
2. The page records from load; a restored autosave appears as a `load_world` input with the full world (about 200 KB). Fine for bug reports, or cap it?

## 8. How to verify in five minutes

```
cd games/hoard
cmake -S . -B build && cmake --build build -j && ctest --test-dir build -L fast
./build/hoard-sim --data data --golden 1-5 | diff - <(head -5 tests/golden/state_hashes.txt)
./build/hoard-sim --data data --replay tests/replays/played-2.hrp     # replay ok: 36 checkpoints
tools/check_banned.sh && echo clean
```
In the sandbox page: Game, Report a problem, then decode the blob as in the README and `--replay` it.
