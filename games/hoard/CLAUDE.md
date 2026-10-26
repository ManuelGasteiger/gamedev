# Hoard

You are the last dragon. Burn the valley, carry its gold home, sleep on the heap, wake stronger.

## Status

- Valley sandbox, the playable version of the whole loop (raid, lair, sleep, wake): `web/hoard-game.html`, built from `web/game.template.html` and `web/game/*.js` by `web/build.sh` (`--html` re-embeds without recompiling the wasm). The world is generated in `web/game/terrain.js`. The simulation it runs on is `sim/` (C++17, CMake) with its data in `data/`; build and test commands in `README.md`; specs in `docs/ages-sim.md` and `docs/sandbox-game.md`.
- Flight prototype v9 in `prototype/index.html`: the single-file build of the claude.ai "Hoard" artifact (three.js, minified). The modular source (`hoard-src.zip`) is not in this repo yet. Its flight and fire numbers carry over 1:1.
- Engine: Unreal Engine 5.8, project in `Hoard/` (Blank C++). The production plan in the claude.ai project is newer than `docs/production-plan.md`.
- Assets: third-party packs in `assets/` (`gltf/` for the browser, `fbx/` for Unreal, `licenses/`, `_unused/` is deletable), Blender sources in `art/`. Index and licences: `assets/README.md`.

## Read first

- `docs/production-plan.md`: engine, dragon pipeline, fire layers, order of work.
- `docs/sandbox-game.md` and `docs/ages-sim.md`: the valley sandbox and the simulation behind it.
- `docs/fire-mechanic.md`: breath phases and the fire grid with its numbers.
- `docs/hoard-brief.md`: the loop, prototype numbers, asset routes.

## Rules

- Gameplay systems in C++ from written specs in `docs/`. Blueprints only as thin glue and data.
- Niagara, materials and Control Rig are authored in the editor from written recipes. Agents write the recipe and verify with captures.
- The fire grid (2 m cells, fuel, heat, wind-biased spread, char) is the source of truth. Every fire visual is spawned from it.
- Port the prototype numbers first and tune only after the port matches: cruise 35 m/s, surge 54, stall 14, turn 1.35 rad/s; inhale 0.26 s, column 150 m/s, range 64 m. Scale: dragon 16 m long with an 18 m span, houses 6 x 7 m.
- `art/` holds Blender files and EmberGen projects; raw simulation output goes in `art/_raw/` and is not committed. `sources/` holds generation prompts and job receipts.
