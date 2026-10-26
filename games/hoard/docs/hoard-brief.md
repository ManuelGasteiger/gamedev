# Hoard – from prototype to production

Status 2026-10-10, prototype version 9. The browser prototype lives in the "Hoard" artifact (published from Claude) and in `hoard-src.zip` (three.js, no engine). This brief maps what the prototype proved onto the Stefan-style pipeline (Claude Code or Codex + Unity + Blender + generation APIs). The fire has its own document, `fire-mechanic.md`.

## Concept (one line)

You are the last dragon. Burn the valley, carry its gold home, sleep on the heap, wake stronger. Multiplayer later, dragons stay rare.

## Core loop, as built

raze → loot → carry → nest → sleep/dream → boon → the valley rebuilds richer → repeat

The sleep is the save point and the progression gate. Keep that.

## What the prototype settled

- Flight: cruise ~35 m/s, surge ~54, stall 14, turn 1.35 rad/s. A laden dragon climbs only with surge or a gentle angle. This reads as "gold is heavy" without a tutorial.
- Fire: four phases (inhale 0.26 s, column at 150 m/s with a fireball front, splash on connect, detached tail on release), range 64 m, direct hit 80 hp/s. What lands becomes a ground simulation: 2 m cells with fuel and heat, wind-biased spread, char; buildings catch from their footprint and set their ground alight in turn. One short breath starts a fire that takes 15 to 20 of 56 buildings in a minute; ten autopilot passes raze about 40. Stone burns at half rate. No aim assist. Details and the engine assessment in `fire-mechanic.md`.
- Wings: rigged after bat kinematics (shoulder, elbow, wrist, four two-segment fingers, smooth membrane patches rooted along the flank, propatagium). Downstroke fully extended and swept forward, upstroke flexes elbow and wrist so the span shrinks, hand pitches down on the way down and supinates on the way up, membrane billows against the stroke. Flap amplitude 0.7 rad around +0.14, 1.5 Hz cruise, 2.1 Hz surge.
- Loot: gold spills as piles on collapse; swoop pickup radius 14 m below 15 m altitude. Carry cap 300, +150 per boon.
- Defense: archers (3 dmg, 3 to 4.6 s, range 170, ceiling 110) are pressure; ballistae (8 dmg, 7 to 10 s, range 240, ceiling 170) punish lingering. The keyboard-only autopilot survives a straight approach with ~90 hp and dies if it circles the keep.
- Economy: village buildings 8 to 48 gold, church/keep chests 90/140; hold 30 to 180 plus chests 110/260; +25 % per day. 3,000 gold is the demo's "legend" end.
- Day cycle: dusk, dawn, noon, moon. Each sleep advances one. Village rebuilds 60 % of ruins, hold 45 %.
- Boons: lungs, wings, claws, scales, hot blood. Pick 1 of 3 per night.

## Systems that carry over as design (engine-agnostic)

| System | Prototype file | Keep |
|---|---|---|
| Flight model and modes | `src/dragon/flight.js` | yes, port numbers 1:1 first |
| Fire grid: fuel, heat, wind spread, char, building coupling | `src/fire/grid.js`, `fuel.js`, `system.js` | yes, port 1:1, see `fire-mechanic.md` |
| Breath phases, splash, direct damage | `src/systems/breath.js`, `src/fire/splash.js` | yes |
| Ignition, ruin, rebuild | `src/world/settlement.js`, `building.js` | yes |
| Loot, carry weight, deposit stream | `src/systems/loot.js`, `game.js` | yes |
| Hoard growth and milestones | `src/systems/hoard.js` | yes |
| Day presets and sky blending | `src/config.js` DAYS, `world/sky.js` | yes |
| Boons | `src/game.js` BOONS | yes, expand |
| Crowd flee | `src/systems/villagers.js` | yes, replace with navmesh agents |
| Defenders and lead aiming | `src/systems/archers.js` | yes |
| World state as plain data (razed, looted, hoard) | `game.js` snapshot | yes, this is what keeps multiplayer possible |

## Assets, and which route makes them

| Asset | Prototype | Production route |
|---|---|---|
| Dragon | procedural mesh, code animation | concept exploration → strict-front sheet (Nano Banana Pro) → Tripo P2 mesh → Blender rig with wings, tail, jaw, fingers → clip set: idle, walk, takeoff, fly, glide, land, bite, breath, sleep, hoard-nuzzle → FBX to Unity. Stefan's animation workflow doc is this exact job for a non-humanoid. |
| Village kit | merged primitives, 3 states | modular kit (walls, roofs, props) generated per piece via Tripo through AssetHub, damage states authored in Blender, baked down (`materials-to-game`) |
| Hold kit | same | same, stone set |
| Loot, gold, trophies | instanced coins, mound | small batch generation, instanced in engine |
| Trees, clouds, birds | instanced primitives | engine assets or generated low-poly batch |
| Fire, smoke, embers | flame cards from the grid, HDR bloom, sprites | engine VFX (`game-vfx-workflow`), verified with OFF/ON captures; the grid stays as the source of truth |
| Audio | WebAudio synthesis | ElevenLabs SFX through fal, music later |
| Motion references | none | Seedance clips: dragon landing, breathing fire, curling up on gold |

## Engine

Unity. C#, Stefan's game skills target it, AnkleBreaker MCP lets the agent drive the editor, FBX/GLB handoff from Blender is clean.

## Milestones

1. Port: terrain, one village, flight, fire, loot, hoard, sleep. Procedural stand-ins for every asset. Same numbers as the prototype. Target: the prototype's 5-minute loop in Unity.
2. Dragon: the full generation and rig pipeline for one hero asset. This is the longest single item.
3. Kit: village building set with damage states, VFX pass, audio pass.
4. Hold and defenders. Dream layer as a real system (memories of razed places, lore, upgrades).
5. Vertical slice review, then decide multiplayer architecture.

## Open design questions

- Dream layer: upgrade tree only, or also where other dragons exist?
- Loot carry: physical claws (visible gold) vs. abstract capacity. The prototype is abstract.
- Settlement response over days: more archers, rebuilt stone, a hero knight?
- Scale: dragon 16 m long, wingspan 18 m, houses 6×7 m. Felt right in the prototype.
- Fire balance: one breath can take a third of the village on its own. Keep the spread as the power fantasy and raise the cost elsewhere (defenders, stone, rain days), or dial `SPREAD` down. Decide in the vertical slice.
