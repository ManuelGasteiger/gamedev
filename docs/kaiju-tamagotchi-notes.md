# Kaiju Tamagotchi (Stefan 3D AI)

Video: "I left Claude my laptop for 47 hours. It built a destructible city", YouTube `cJH4UgCWrT8`, 2026-10-03, 18 min. Files on Patreon: patreon.com/posts/kaiju-tamagotchi-171142359. The build kit from there is unpacked in `reference/kaiju-tamagotchi/` (no keys in it, only `env.example`). Video notes come from the description and the auto-generated transcript, not from frames.

## What it is

A Unity game built in one autonomous run by Claude Opus 5.5 (Claude Max x20) with Blender, Higgsfield as the main generation route, Tripo P2 for characters and fal for sound effects. You raise a baby kaiju by eating things on a beach, grow through stages, and from stage 5 to 6 wreck a New York-like city: cars fly, buildings collapse, helicopters and the military attack, a reactor is the final target. Destruction persists across the map. Achievements and a grade at the end, day and night toggle, minimap, a cheat key to grow. He told it not to kill people; the city is empty of victims.

## Numbers

- 47 h wall clock, about 38 h of work. One out-of-memory stop in Unity overnight needed a manual restart.
- 5.28 B tokens, 97 % cached, about 2,500 USD at API prices.
- Generation: about 300 USD Higgsfield, 20 USD Tripo, 10 USD fal. About 300 3D jobs, 367 images, 414 ElevenLabs jobs (about 5 USD).
- 2,000+ screenshots the agent took to review its own work.
- Budget caps he gave the agent: 500 USD Higgsfield, 50 USD each for Tripo and fal.

## How he briefed it

- References alone did not work in his previous run (World of Oldcraft). This time the brief says which tool to use for which job, for example Tripo P2 for characters generated in parts with clean UVs, not the high-poly model.
- Everything that broke last time got an explicit approach in the brief.
- Concepts included but loose, so the direction is right and the agent does not overspend on detail.
- An empty Unity project and a folder with all images were prepared before the run.
- He kept the run open on a notebook; the agent logged his complaints and fixed them with before/after records.

## What did not work, in his words

Rough animations, flat-looking NPC models, part-assembled characters that move oddly, some building collapses not smooth, and the kaiju's fire breath "doesn't look really good" and destroys nothing.

## For Hoard

- Size-gated destruction (a bigger stage breaks bigger buildings) is the same lever as our building hp and stone burning at half rate. Persistent destruction matches our raze-then-rebuild-overnight state.
- His weakest point is our core mechanic. A one-shot autonomous run does not reach film-like fire; the fire stays hand-directed with captures, as in the production plan.
- Worth copying: the brief format (tool per job, budget per provider, concept folder), the complaint log, the self-review screenshots.
## The build kit

What the agent got: a 65 KB spec (`Docs/TASK.md`) with 25 acceptance items, priorities and a 30-hour plan with a hard gate at hour 20 (core loop and destruction done, no new systems after), a 130-line `CLAUDE.md` of working rules, 36 approved concept images with a README, five skills (three written for this run), Python helpers and Claude Code hooks. Built for Unity 6000.5.9f1 URP on an RTX 3060 laptop.

Pieces worth taking:

- **Destruction spec (TASK §8).** Nothing is a physics object until it has to be. Intact buildings are 1 to 4 static colliders. The first real hit "breaks open" a building: pre-fractured convex pieces from pools (fractured at edit time, never generated meshes at runtime) and a support graph. A floor gives way below about 35 % of its supports and everything above falls as one body (progressive collapse). Pieces sleep, freeze and merge into a rubble mesh that stays. Anything under about 1 % of the creature's length breaks as VFX only. A budget manager caps moving pieces, new bodies per tick and plumes, and downgrades new breaks to VFX when full; a "max chaos" fixture has to hold 60 FPS. Same thinking fits Hoard's burning roofs and collapses; map it to Unreal's Chaos tools when we get there.
- **Why his fire was weak.** URP with no real-time GI: "emissive windows, fires and bloom carry the night", flames are particles with a scrolling core. That is the gap our plan closes with Lumen, which in turn needs a GPU this machine does not have (see `tool-setup.md`).
- **Long-run harness.** State in one small `Docs/state/STATE.md` that a hook prints after every start, resume and compaction, so the agent never re-reads the docs; auto-compact at 500k tokens; a `keep_working` stop hook; helper lanes that return 15 lines and keep details in files; one blocking wait (`wait_for.py`) instead of polling; a ledger per paid job with hard stop lines (Tripo 50, Higgsfield 500, fal 50 USD).
- **Verification.** A scripted "rampage bot" plays every stage and writes `checks.json` with screenshots at fixed checkpoints; a fresh-context reviewer scores captures against the approved concepts and lists the three worst defects; acceptance items only pass with an evidence file.
- **`Docs/PIPELINE_LESSONS.md`** collects his lessons from earlier runs: greybox first, never edit the harness mid-run, screenshots alone miss bugs, log provenance per asset, report implemented / agent-verified / owner-confirmed separately.
