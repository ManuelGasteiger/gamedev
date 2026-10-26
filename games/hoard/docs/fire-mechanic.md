# Hoard – the fire mechanic

Status 2026-10-10, prototype version 9. This is the design the prototype now runs, what it taught, and what the real game needs.

## What the breath is

Not a projectile. Four phases, each with its own look and sound:

1. Inhale (0.26 s). Throat glow in the open jaw, rising intake of air with a growl under it. The player feels the wind-up before anything leaves the mouth.
2. Release. A thump and a blast of air. The column races out of the mouth at 150 m/s with a fireball bulging at its front. Camera: FOV kick, a jolt, then a slight side offset so the column is visible past the dragon's own body.
3. Connect. The front reaches ground, building or water. Whoomp, shock ring, dust, embers. Burning fuel splashes along the surface, mostly forward, like a hose hitting the ground. Every drop pours heat into the fire grid as it travels, so the fire is laid down in a fan, not a disc.
4. Release of the key. The column detaches from the mouth and the tail follows the front out; the last of the breath still lands. Short exhale tail in the sound.

Range 64 m, 28 % of the meter per second (3.5 s of fire), 1.6 s cooldown when emptied.

## What the fire is once it has landed

A simulation on the ground, not a particle effect:

- A sparse grid of 2 m cells, created on demand around anything hot. Each cell: fuel (grass 0.9, forest crown 1.6, wooden footprint 1.6, stone footprint 0.8, steep rock 0.1 to 0.35, cobbles 0.15, water 0), heat, burn intensity, char.
- A burning cell heats its eight neighbours (diagonals half), downwind 45 % more, upwind less. Heat decays with a 0.8 s time constant, so a lone ember cannot ignite grass; a front can. Ignition at heat 1.0. Fuel burns off in 6 to 12 s and leaves char.
- Buildings sit on cells. Burning ground under a footprint damages the building (0.8 hp/s per fully burning cell, half for stone) and ignites it at 20 hp lost; a burning building heats its own footprint, so the fire crosses from house to ground to the next house. Ruins have no fuel left and are charred. Rebuilt houses get their fuel back.
- Trees sit on cells too. A burning crown pours heat around its trunk; a forest fire runs through the grid with the wind.
- A new day puts everything out and fades the char to 45 %.

Rendering: one instanced flame card per burning cell (two for strong ones), procedural flame in the shader, billboards that tilt toward the camera when seen from above so a field of fire reads from the air. Char and ember glow are a 512² world-space texture sampled by the terrain shader, so burnt ground is a continuous stain and a burning field glows from above. Smoke is a lumpy sprite, dark and lit orange when young. Bloom on an HDR frame makes the column and the fire fields glow without blowing out the sky (the water highlight had to be clamped for that).

Cost in the prototype: 350 burning cells cost under 1 ms of JS per frame; 2,400 cards and 4,000 fire sprites are one draw call each.

## Numbers from the review harness

One 1.5 s breath into Thornbury: 80 burning cells after a second (a pool about 10 m across), the fire then creeps through the village on its own and takes 15 to 20 of 56 buildings over the next minute. The keyboard autopilot's ten passes raze about 40 of 56. That is deliberately strong. If it reads as too easy in play, the first knobs are SPREAD in `fire/grid.js` (0.62) and the footprint heating in `fire/system.js` (0.9/s).

## Fixed on the way

- The hitch on first connect. It was not the aim assist. Two things compiled shaders mid-play: the hot-window material, used for the first time when the first building caught, and the five flicker point lights toggling `visible`, which changes the light count and recompiles every lit material in the scene. Now every program compiles at boot (`src/warm.js`) and lights never leave the scene; `test/programs.mjs` verifies that nothing compiles after boot.
- Aim assist removed, together with the head turning toward it. The player aims; the ground fire forgives a near miss, because a splash next to a house still burns it.
- Review harness: `test/review.mjs` writes contact sheets of the dragon (views and flap phases) and of a breath from press to a minute later, from the chase camera and from a fixed side camera.

## Does the browser carry this?

Yes, for a prototype and probably for a small finished game, with limits:

- Fire as a 2D ground simulation plus instanced cards is cheap and reads well from the air, which is the dragon's usual view. It does not do volume: no rolling fireballs that occlude themselves, no lit smoke columns casting on the scene, no heat distortion without a second scene render. Those are the things that make Smaug's fire look like a liquid.
- three.js gives us bloom and HDR today. Heat haze and screen-space smoke lighting are another two passes, feasible but hand-written.
- Sound is synthesized. It conveys the phases but a real breath needs recorded layers (intake, ignition, sustained jet, impact, bed of burning) mixed with distance and occlusion. WebAudio can play them; nothing in the browser authors them.
- Rig and animation of the dragon itself is the bigger ceiling: procedural wings got as far as a smooth membrane on a bat skeleton. A proper rigged asset with a breath clip (neck recoil, jaw, throat sacs) is an engine-agnostic need.

What an engine buys, in order of how much it would change this mechanic:

1. Volumetric fire and smoke as particles with lighting (Unity VFX Graph or Unreal Niagara, plus Lumen or baked GI reacting to the fire light). This is the single biggest step toward "clouds and seas of fire".
2. Decals and a proper material system for char, embers and glowing roof tiles instead of a texture patched into one shader.
3. Audio middleware (FMOD or Wwise) for the layered breath and the burning bed, with distance attenuation and occlusion.
4. Post stack for free: bloom, heat distortion, exposure adaptation, motion blur on the column.

The ground simulation, the phase design, the splash and the coupling to buildings are engine-agnostic and port as code or as a design document. Numbers above can be carried over 1:1.

## Scope if we switch

Unity, as planned in the brief. For the fire specifically:

- Port `fire/grid.js`, `fire/fuel.js`, `fire/splash.js` logic to C# (a day).
- VFX Graph: column (ribbon or stretched particles with a front), impact splash (surface-following particles with the same heat-pouring hook), flame cells (GPU particles spawned from the grid texture), smoke, embers. Two to three weeks for a first pass that beats the prototype, with the verified OFF/ON capture habit from Stefan's workflow.
- Terrain char as a render-texture decal layer written from the grid.
- Breath clip on the dragon rig; camera and controller feel ported from `ui/camera.js` and `dragon/flight.js`.
- Audio: a dozen recorded or generated layers (ElevenLabs SFX through fal, as in the stack doc) behind the same four phases.

If we stay in the browser for longer, the next steps in order: heat haze pass, lit smoke (store fire light in a small 3D texture or sample the ground glow map from the smoke shader), a second grid layer for burning roofs, recorded sound layers, and a real dragon asset.
