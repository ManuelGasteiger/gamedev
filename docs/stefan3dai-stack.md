# Stefan 3D AI (Stefan Vaskevich) – tool stack and workflow

Source: Patreon Stefan3DAI, X @Stefan_3D_AI, YouTube "Stefan 3D AI" (~190k subs), GitHub witnesstodark/mr-mak-workspace, the free skill pack Mr-Mak-Skills-Free.zip (v0.4.14) and his "AI Animation Workflow" doc. Compiled 2026-10-10.

## Who he is
3D/AI content creator. Sponsored heavily by Tripo (32 videos), also Hitem3d, Hyper3D/Rodin, 3D AI Studio, Modddif, Hi3D and others. Runs top3d.ai (tool directory) and learn3d.ai (courses). Open-sourced his whole workspace in Sept 2026.

## The core idea
He does not use a single "AI game maker". He runs a coding agent (Claude Code with Opus 5.5, or Codex CLI with GPT-6) inside one repo that holds project context, knowledge and workflows, and gives that agent MCP connections to Blender, Unity and generation APIs. His own know-how is packed into agent skills. Showcase: "World of Oldcraft", a WoW-style game built in a 36h autonomous Claude Max run, ~8B tokens, ~2,175 USD. Earlier project: steampunk cat platformer in UE5 in 3 days.

## Stack

### Agents
- Claude Code (Opus 5.5, Claude Max) for the long autonomous runs
- Codex CLI (GPT-6) for Blender animation work (he says he switched to GPT-6 for that in Sept 2026)

### Engine and DCC
- Unity (current main project) via AnkleBreaker Unity MCP: github.com/AnkleBreaker-Studio/unity-mcp-server
- Unreal Engine 5 (earlier platformer project)
- Blender with the native Blender MCP (blender.org/lab/mcp-server): scene inspection, Python execution, viewport screenshots
- Mixamo and AccuRIG for quick rigging in older videos; now rigging/animation done by the agent in Blender

### Generation providers
- Higgsfield: CLI (higgsfield.ai/cli) and Blender plugin. His primary generation route and current sponsor. Covers image, video, 3D.
- fal.ai MCP (mcp.fal.ai/mcp, or luminarylane/fal-mcp-server) as the alternative, with his own Python queue helper
- Tripo (P2 in Tripo Studio, tripo3d/h3.1 image-to-3d on fal) for meshes
- Hyper3D Rodin Gen-2 / 2.5, Hitem3d, TRELLIS.2 as other mesh options
- Modddif for AI texturing (older videos), fal-ai/patina/material for PBR material maps

### Models per task (from the skill pack prompts)
- Strict-front A-pose character sheet and part extraction: Nano Banana Pro (fallback Nano Banana 2), 2K, 1:1. Never GPT-Image here.
- Multi-view character sheet: GPT-Image-2.5 (fal openai/gpt-image-2.5/flare/*), fallback GPT-Image-2, then Seedream V5 Lite
- Turntable / 360 video: Seedance 2.0 orbit mode, fallback Veo, Kling on request
- Motion reference video: Seedance 2.5
- Mesh: Tripo P2 / h3.1
- Materials: Patina (fal)

### Support tools
- Python + uv, FFmpeg, whisper.cpp dictation (Wispr Local)
- Mr. Mak Workspace: his Tauri/React desktop app wrapping Codex/Claude Code with project cards, reports and the skills

## Pipeline (asset to game)
1. Concept exploration (vary silhouette/proportions/palette), pick one, then faithful edits only (image-reference-workflow)
2. Character sheet: strict-front A-pose from the approved concept, extract isolated parts (body, paired items, accessories, hair), optional side/back views and turntable (character-sheet-pipeline)
3. Mesh from the sheet (Tripo etc.), or procedural Three.js rebuild (img2threejs) for web
4. Materials and bake-down: high-poly to game low-poly, UV, bake, channel checks (materials-to-game)
5. Motion reference video from a character image (Seedance), fixed camera, readable foot contacts (motion-reference-workflow, video-watch)
6. Rig, skin, animate in Blender with the agent, export runtime skeleton + clips, reimport check (blender-game-animation)
7. Integrate in engine: clip mapping, events, VFX, UI, audio, level (game-* skills)
8. Verify in the actual engine with OFF/ON captures (gameplay-visual-review)

## The 20 skills in the free pack
3d-production-routing, blender-game-animation, character-sheet-pipeline, fal-ai-generation, feature-handoff, game-animation-integration, game-audio-workflow, game-level-design, game-ui-workflow, game-vfx-workflow, gameplay-visual-review, higgsfield-workflow, image-reference-workflow, img2threejs (Apache-2.0, third party), materials-to-game, motion-reference-workflow, plan, video-watch, voice-dictation-setup, workspace-authoring.

Pack is MIT, agent-agnostic (.claude/skills and .agents/skills copies). Contains no assets, accounts or MCP configs. Each skill is a process document, not an automation.

In this repo: 17 of them are in `.claude/skills/`. Left out: img2threejs (web only), voice-dictation-setup, workspace-authoring (tied to his desktop app). The game-* skills are engine-agnostic; none of them names Unity or Unreal.

## Principles that recur in every skill
- Separate concept approval, mesh approval, export verification and engine integration
- Save job receipts (endpoint, request id, inputs) before anything else; never resubmit a paid job on timeout
- Review at full size; keep rejected takes with reasons
- Edits change one named feature of an accepted image, never a redesign from text
- A successful API call or nice render is not a game asset

## Links
- Patreon: patreon.com/Stefan3DAI (free post with Windows build + files: /posts/world-of-windows-170793929)
- Workspace repo: github.com/witnesstodark/mr-mak-workspace (copy in `reference/`)
- Animation workflow doc: drive.google.com/file/d/11xZHJSJs2NRkvs0fd6zW8cf4xI-mWtBQ/view
- Videos: youtu.be/doR2RhsneRA (36h run), youtu.be/mNjBBO1gWFI (week one), youtu.be/T4jV7O8SMGE (UE5 platformer), youtu.be/cJH4UgCWrT8 (Kaiju Tamagotchi, see `kaiju-tamagotchi-notes.md`)
- img2threejs: github.com/img2threejs/img2threejs

## ZER0-G (github.com/witnesstodark/zer0-g) – the "small complete game" pipeline

Copy in `reference/zer0-g`. Anti-gravity racer (F-Zero X tribute) built by Stefan + his agent for Project 0 lot #383. Web game: three.js r186 in a sandboxed web worker, ~10k lines of plain JS modules, no build step, no engine. 26 "rounds" of agent work as of 2026-10-07.

Asset routes:
- Concepts: Nano Banana Pro via fal.ai, one prompt JSON per batch (sources/concepts/*_prompts.json), every prompt starts with "Original design ... not based on any existing game", 3/4 view, white background, no text/logos
- Meshes: Tripo P2 via AssetHub CLI (meshGen.tripo_p2_preview, faceLimit 10000, pbr, textureQuality detailed = 155 credits, 4K maps). Resumable submit script with fixed operation ids so a retry never pays twice (sources/machines/run_p2d.py)
- Mesh prep: headless Blender script (tools/machine_prep.py): orient nose, scale to 0.9 m, decimate to ~7k tris, keep Tripo normals, shrink maps to 1024/512, derive emissive glow map from saturated bright seams; then gltf-transform quantise
- Tracks: not generated, designed procedurally (tools/track_design.py): turtle path of pieces (straight, banked turn, helix, loop, corkscrew, pipe, drum) with box/clearance/curvature checks, output JSON centreline + plan image
- Music: ElevenLabs Music v2.5 and Sonilo via fal; SFX + announcer: ElevenLabs via fal; one input JSON per sound, audio_pack.py levels and packs
- Characters (AssetHub V4 Character Assembly): concept → part plan → part images → per-part meshes → Blender assembly, ~1h, 1,000 to 2,600 credits per character, often ends needs_review. Forced Tripo P2 keeps parts at ~5k tris; defaults can hit millions
- Testing: headless physics sim (tools/sim.mjs) for tuning, Playwright scripts for screenshots and 2-player online tests, a p0 API mock harness for local runs
- Lesson recorded in the code: first decimation round to 3k tris / 512 px crumpled panels and smeared seams; round 2 regenerated all 18 at 7k / 1024

Skills used: fal-ai-generation, assethub-production, 3d-production-routing (same family as the Mr. Mak pack).
