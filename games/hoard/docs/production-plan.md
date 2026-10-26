# Hoard – production plan

Status 2026-10-10. The browser prototype (version 9) is a working vertical slice: flight, a four-phase breath, a ground fire simulation, loot, hoard, sleep, boons, two settlements. This plan answers three questions: what to build the real game in, how to get a dragon that holds up, and how to get the fire to the level of film. Story and the long sleep come later and nothing here blocks them.

## 1. Engine

### What the game needs that the prototype cannot give

The identity of the game is cinematic fire over a valley seen from the air, at night as much as by day. Film fire has four properties the prototype lacks: volume (self-shadowing fire and smoke with depth), light (the fire lights the village, the smoke, the dragon), motion (fluid turbulence, not scrolling noise on cards), and sound mixed for distance. The engine choice is mostly the question of which of these come built in.

### Options

**Stay in the browser (three.js, moving to WebGPU).** WebGPU is Baseline since January 2026 and on by default in Chrome, Edge, Firefox and Safari 26, and three.js switches renderers with one line, so compute shaders for a 3D gas simulation are possible in principle. Everything else would be ours to write: volumetric rendering, lit smoke, decals, GI from fire, an audio mixer, a rig and animation pipeline, a destruction system. No asset ecosystem of note. Verdict: the right place for the design sandbox and a shareable web demo, the wrong place for the product.

**Unity 6 (HDRP, C#).** Your language, Stefan's skills, an official MCP server (open beta since May 2026, Unity 6 or later, needs Unity Cloud and the AI tools beta), the Asset Store. Fire: VFX Graph with six-way lit flipbooks (HDRP, Unity 2022.2 or later), Zibra Smoke & Fire for real-time volumetric fire and smoke as a paid plugin. Missing: dynamic global illumination from emissive fire. HDRP has screen-space GI and baked probe volumes; a village lit by its own flames at night is approximated with point lights, as in the prototype. Pricing: Personal is free under a revenue cap, Pro and Enterprise per seat; the runtime fee is gone since September 2024. Unity 6.3 is the current LTS.

**Unreal Engine 5.7 (C++ and Blueprints).** Niagara Fluids is shippable as of 5.7: a 64³ 3D fire simulation costs about 2.4 ms on PS5-class hardware, 128³ smoke 5.5 ms, with a realistic total fluid budget of 3 to 4 ms at 60 Hz, which means one hero simulation near the camera and cheaper fire everywhere else. Lumen turns emissive fire into light on the world. Heterogeneous volumes play baked VDB simulations for set pieces; ZibraVDB does the same with a free tier up to 256³ voxels. Control Rig for procedural animation layers, MetaSounds for procedural audio, Chaos for destruction, Fab for assets. License: free until a product passes 1 million USD lifetime gross revenue, 5 % royalty above that, zero on the Epic Games Store. Costs on your side: C++ compile cycles, Blueprints that are binary and therefore poor material for agents, less mature MCP tooling (community servers exist), bigger projects and builds.

**Something else.** Godot has no fluid or volumetric story and weak 3D tooling for this scope. Babylon.js and PlayCanvas share the browser ceiling. A custom engine is a different hobby.

### Recommendation

Unreal Engine 5.7, decided by a two-weekend bake-off rather than by this document.

The reasoning: the three hardest fire properties (volume, light, motion) are native in Unreal and plugins or workarounds in Unity, and the one that matters most for this game's night scenes, light from fire on the world, has no good Unity answer today. Your C# advantage is real but it buys speed on gameplay code, which is the smaller part of this project; the fire and the dragon are editor and content work in either engine.

The bake-off: build the same shot in both engines, the connect on the Thornbury square from the chase camera, 20 seconds from release to a burning square. Unreal: Niagara Fluids fire at the impact, Niagara flames spawned from the grid, Lumen on. Unity: VFX Graph six-way flipbooks from EmberGen, Zibra Smoke & Fire trial at the impact, HDRP SSGI on. Capture both at 1440p, side by side, and judge three things: does the impact read as liquid fire, is the square lit by the fire at night, and how many hours did each take. Also note how it felt to work with Claude Code in each (C++ with Live Coding against C# with the Unity MCP). If Unity is within reach on the first two and clearly faster on the third, Unity is a legitimate choice and the rest of this plan ports with small changes.

Working rules if it is Unreal: gameplay systems in C++, written by agents from written specs and tested with Automation tests; Blueprints only as thin glue and data; Niagara, materials and Control Rig authored by hand in the editor from written recipes, verified with the OFF/ON capture habit from Stefan's workflow and an automated screenshot pass (Movie Render Queue or a functional test map) that replaces `test/review.mjs`.

## 2. The dragon

Three routes, used in sequence.

**Week one: buy a rig.** A rigged, animated dragon or wyvern from Fab or the Asset Store gets the flight controller and the breath pipeline working on day one. Example: the WDallgraphics "Wyvern" on Fab ships rigged with 25 animations including TakeOff, Fly, Glide, Hover, Landing, Roar and Attack_FireBreath, about 10k polygons. Many listings are old (the 3dFoin "Dragon" on the Asset Store dates from 2017), so check the update date, the animation list and the license before buying. This asset is a stand-in. Nothing about it should leak into the design.

**Months two to four: the hero dragon.** The pipeline from the stack document, applied to a non-humanoid:

1. Concept: silhouette and proportions first (16 m long, 18 m span, four legs, a long neck with a throat that can glow). Strict front, side and top sheets.
2. Mesh: generate from the sheets (Tripo or Meshy), then retopologize and clean in Blender. The generated mesh is a sculpt reference, not the final topology; wings and the jaw need edge loops placed by hand or the deformation fails.
3. Rig in Blender with Auto-Rig Pro: quadruped limbs placed manually (the Smart placement is for bipeds), the Wings limb, a Tail limb, a Spline IK neck, finger chains for the wing fingers, jaw and eye bones. Universal export type, FBX to Unreal. Rigify is the free alternative with the same shape of work. Tripo's rig service (v2.5, February 2026) supports quadruped and avian types and can produce a quick skinned stand-in, but a creature with both four legs and wings is outside its categories and will need manual correction.
4. Clip set, authored in Blender or bought as motion and retargeted: idle, walk, takeoff, flap, glide, bank left and right, dive, flare and land, breath (neck recoil, jaw, throat), roar, sleep, curl onto the hoard, death.
5. In Unreal: Control Rig layers on top of the clips. Head aim toward the breath direction, wing flex from airspeed and bank, tail drag from turn rate, foot IK on landing, a secondary bone chain or cloth on the membranes. This is the part the prototype faked with code and the part that makes the dragon feel alive.
6. Verify with the review habit: the same five angles across the flap cycle, a landing, a breath, as images, every time the rig changes.

**If the rig fights back.** Commission the rig and skin weights only (not the model, not the animation) from a creature rigger. Rigging a winged quadruped well is a specialist skill, and it is a one-time cost that the rest of the project stands on.

## 3. The fire

The prototype's ground simulation stays as the source of truth. Everything visual is spawned from it.

**Layer 0, simulation.** Port `fire/grid.js`, `fire/fuel.js` and `fire/splash.js` to C++: 2 m cells, fuel from the landscape layer and from building footprints, heat with wind, char. Expose the grid as a render target so Niagara can read it on the GPU. This is a week of work and it is engine-independent.

**Layer 1, the column.** Niagara: a sprite and ribbon jet for the body of the column with the travelling front, plus one Niagara Fluids 3D gas simulation (64³) parented to the mouth for the near-camera volume. The simulation is what makes it look like liquid fire. Only one runs at a time, it is the player's own breath, and it is replaced by the cheaper jet beyond 60 m or when the camera is far.

**Layer 2, the connect.** The splash drops become Niagara particles that pour heat into the grid, exactly as now. The fireball of the connect is a six-way lit flipbook explosion rendered in EmberGen (one afternoon of simulation gives a dozen variants), a shock ring, dust, embers. On water, steam from the same family.

**Layer 3, seas of fire.** GPU particles spawned from the grid render target, one or two per burning cell, rendered as six-way lit flipbook flames and smoke columns from EmberGen, so the smoke takes light from the sun, the moon and the fire. Emissive drives Lumen: the burning village lights itself, the smoke and the dragon's belly. Heat distortion as a material on the column and over strong cells. Char through a runtime virtual texture or decals written from the grid. Roofs burn as a second grid layer on the building footprints, with debris and collapse through Chaos or the swap-state ruins the prototype uses.

**Layer 4, set pieces.** The waking, the first breath into the great hall, the hold's gate: baked simulations from EmberGen or Houdini played as VDB sequences through heterogeneous volumes or ZibraVDB. Expensive per frame, used for seconds at a time.

**Sound.** Wwise, free under a 250,000 USD production budget. Layers for the breath: intake, ignition thump, jet loop, impact, the burning bed scaled by the grid's heat near the listener. Sources: recorded libraries plus generated effects (ElevenLabs SFX through fal, as in the stack document). MetaSounds for the procedural parts (the bed's crackle density follows the grid). Distance, occlusion by terrain and buildings, a low-pass when the dragon's own breath is between you and the fire.

**Verification.** The prototype's two sheets become automated captures: the dragon review and the fire review, run on a functional test map after every change, kept in the repository as the record of what the fire looked like when.

## 4. Order of work

Solo, evenings and weekends, with agents doing the code and you doing the editor work and the judgement.

1. Bake-off (two weekends). The connect shot in both engines. Decide the engine.
2. Skeleton (four to six weeks). Landscape with the prototype's valley shape, flight controller with the prototype's numbers (cruise 35 m/s, surge 54, stall 14, turn 1.35 rad/s), chase camera, the bought dragon, the grid port, placeholder Niagara fire, a blocked-out Thornbury. Goal: the prototype's five-minute loop running in the engine.
3. Fire (six to eight weeks). Layers 1 to 3, first sound pass, roofs and collapse. This is the milestone the game is sold on, so it comes before the hero dragon.
4. Dragon (six to ten weeks, overlapping). Hero asset through the pipeline above, Control Rig layers, the clip set.
5. World and loop (eight weeks). Village kit with damage states, Hammerdeep, defenders, loot and carry weight, the hoard and the dream, the day cycle.
6. Vertical slice review. Decide multiplayer architecture, the long sleep, the story layer. A Steam page and a demo build belong here, not earlier.

## 5. Costs and licenses

- Unreal Engine: free until 1 million USD lifetime gross per product, then 5 %, none on the Epic Games Store.
- Unity, if chosen: Personal free under the revenue cap, Pro per seat. The AI tools and MCP are in open beta and need a subscription.
- Wwise: free under a 250,000 USD production budget.
- EmberGen: paid, indie tier. The single best purchase for the fire.
- ZibraVDB: free tier up to 256³ voxels, paid above. Zibra Smoke & Fire is Unity only until the Unreal port, listed on their roadmap for 2027.
- Auto-Rig Pro: inexpensive one-time purchase. A commissioned rig: a few hundred to a few thousand depending on who and how far.
- Fab or Asset Store dragon stand-in: tens of dollars.

## 6. What the browser prototype is for from here

It stays the design sandbox. Economy, spread rates, boons, the shape of the valley and the loop are cheaper to try there in an evening than in any engine, and the numbers carry over 1:1. Keep it at version 9 unless a design question needs it.
