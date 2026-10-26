# Hoard

You are the last dragon. Burn the valley, carry its gold home, sleep on the heap, wake stronger.

## Status

- Valley sandbox, the playable version of the whole loop (raid, lair, sleep, wake): `web/hoard-game.html`, built from `web/game.template.html` and `web/game/*.js` by `web/build.sh` (`--html` re-embeds without recompiling the wasm). The world is generated in `web/game/terrain.js`. The simulation it runs on is `sim/` (C++17, CMake) with its data in `data/`; build and test commands in `README.md`; specs in `docs/ages-sim.md` and `docs/sandbox-game.md`.
- Flight prototype v9 in `prototype/index.html`: the single-file build of the claude.ai "Hoard" artifact (three.js, minified). The modular source (`hoard-src.zip`) goes to `prototype/src/` (not in the repo yet). Its flight and fire numbers carry over 1:1.
- Engine: Unreal Engine 5.8; the project starts fresh in `unreal/Hoard/` with item 18. The plan is `docs/implementation-plan.md`; decisions in `docs/decisions.md`; briefs in `docs/prompts/`.
- Assets: third-party packs in `assets/` (`gltf/` for the browser, `fbx/` for Unreal, `licenses/`, `_unused/` is deletable), Blender sources in `art/`. Index and licences: `assets/README.md`.

## Read first

- `docs/production-plan.md`: engine, dragon pipeline, fire layers, order of work.
- `docs/sandbox-game.md` and `docs/ages-sim.md`: the valley sandbox and the simulation behind it.
- `docs/fire-mechanic.md`: breath phases and the fire grid with its numbers.
- `docs/hoard-brief.md`: the loop, prototype numbers, asset routes.

## Determinism

- The sim is deterministic from (seed, data, inputs) on GCC, Clang, MSVC and wasm. CI checks it on every push (`.github/workflows/hoard-ci.yml`).
- Every target that compiles `sim/` uses `hoard_sim_flags` (no contraction, no fast-math). `tools/check_banned.sh` (the `banned_ops` test) rejects libm calls other than sqrt/fabs/floor/ceil/trunc/round/copysign, `float`, `<random>`, unordered containers, clocks, threads and two rng draws in one expression. Use `det::` from `sim/core/detmath.h`.
- Randomness only from `world.rng`, or a keyed `rng_for(seed, tag, id)` for new rules.
- Goldens: `tests/golden/state_hashes.txt` (300 seeds, `hoard-sim --golden`) and the two chronicle hashes. Platforms must agree with each other first; a deliberate change regenerates them in its own commit titled `golden: <why>`, after reading the diff, and bumps `kRulesVersion` in `sim/ages/record.h`.
- A new API verb that changes state gets a wrapper in `sim/ages/recorded.cpp` and a case in `apply_input`, or replays break.

## Work

- One branch per item (`item/NN-slug`), a commit per step, `reports/NN-slug.progress.md` while working, `reports/NN-slug.md` at the end. Build and test: `cmake -S . -B build && cmake --build build -j && ctest --test-dir build -L fast`; wasm: `WASI_SDK=... bash web/build.sh` then `node web/smoke.mjs --golden 1-60`.
- Clone without LFS (`GIT_LFS_SKIP_SMUDGE=1`) unless the item needs assets; `web/build.sh` (full) needs the assets listed in `web/assets.json`.

## Rules

- Gameplay systems in C++ from written specs in `docs/`. Blueprints only as thin glue and data.
- Niagara, materials and Control Rig are authored in the editor from written recipes. Agents write the recipe and verify with captures.
- The fire grid (2 m cells, fuel, heat, wind-biased spread, char) is the source of truth. Every fire visual is spawned from it.
- Port the prototype numbers first and tune only after the port matches: cruise 35 m/s, surge 54, stall 14, turn 1.35 rad/s; inhale 0.26 s, column 150 m/s, range 64 m. Scale: dragon 16 m long with an 18 m span, houses 6 x 7 m.
- `art/` holds Blender files and EmberGen projects; raw simulation output goes in `art/_raw/` and is not committed. `sources/` holds generation prompts and job receipts.
