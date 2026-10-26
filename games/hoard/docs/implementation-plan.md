# Hoard: implementation plan to 1.0

Status 2026-10-26, second version. Replaces the version of 2026-10-25, which covered phases 1 to 3 only and was written before the code could be read. This one starts from the code audit (`audit-2026-10-26.md`) and runs to release: every prompt in `prompts/`, every phase in `roadmap.md`, and the ideas from the three design chats that have no prompt yet. Each iteration names its items, the decisions it needs first, what Manu does, and the gate that ends it. An iteration ends when its gate holds; merged code alone does not end it.

Sources folded in: the chats "Dragon lair game concept" and "Dragon lair game concept tasks" (sim v0 to v0.3, the interest tool, section 17), "Dragon game design improvements" (v0.4: arms race, talents, genesis, hooks, storyteller), "Game concept and progression" (endings, chronicle, dread and awe, act API, scenarios, the production research, prompts 00 to 19), and the project docs they produced.

## 0. Where things stand

**Built and working (v0.4, 2026-10-19).** The C++17 sim with genesis, pre-history, nine starting situations, the storyteller, 40 talents on five axes, defences and wounds, permadeath with the warning ladder, hooks, offers with stated reasons and lies, named people, event cards, the briefing; the wasm API; the tactical valley sandbox (`web/hoard-game.html`) and the ages sandbox; the interest tool; the six tests. GCC and Clang build and agree; the committed wasm matches both goldens. The work the "tasks" chat feared lost (section 17, the interest tool) is in this tree.

**Designed, specified, not built.** Dread and awe with memory tiers, digestion that consumes the hoard, endings and prophecy, chronicle levels and the historian, arrivals, people with traits and moves, slayer rosters, the 40-act API v1.1, scenarios and their bank, culture and cults, story sifting, `RaidReport`, the flight shell, the Unreal spike, the text renderer with German, determinism CI and replays. All of it has a prompt (`prompts/01` to `19`).

**Designed, no prompt yet.** Ambitions, dreams that pause a sleep, eight of the eleven endings, most of the 90 event archetypes, difficulty modifiers and Second Breath, the raid's open points, everything in the engine past the spike (hero dragon, fire layers, audio, kits, engine UI, the lair as a place), telemetry, the demo build, and the release work. Sections 6 to 9 turn these into new prompts 20 to 41.

**Blockers found by the audit.**
1. Nothing is committed. `D:\gamedev` has no commits and there is no GitHub repo.
2. The flight prototype's source (`hoard-src.zip`) is not in the tree. It is an attachment in the chat "Dragon game concept development"; items 16 and 18 need it.
3. MSVC is not installed, so the Windows build has never run.
4. Eight corrections to the prompts (audit §11), the largest being the sim's real speed: about 0.3 to 1 s per 3000 years, where the preamble says 10 ms.

## 1. Rules for every iteration

From `workflows.md` §1 and §17, short: one branch per item (`item/NN-slug`); the agent reads `00-preamble.md`, then the brief; commits per step; keeps `reports/NN-slug.progress.md`; ends with `reports/NN-slug.md`. Manu reads the report, then the diff, merges, runs the interest tool on main, and writes a `decisions.md` line if anything moved. Goldens change only in a `golden:` commit. Two to four agents in flight. Opus (or Fable) for rules, determinism and multi-file judgement; Sonnet for tooling, data, pages and scripts; Haiku for lint. Sim and tooling agents clone with `GIT_LFS_SKIP_SMUDGE=1` so the free LFS bandwidth (10 GiB a month) goes to the asset and engine work.

## 2. Decisions, and the iteration that needs each

| # | decision | proposal on the table | needed before |
|---|---|---|---|
| D1 | Repository shape | **taken 2026-10-26**: one private repo `gamedev` with Hoard in `games/hoard/`; prompts corrected to it | step 0 |
| D2 | Technical defaults in `decisions.md` Open (one repo layout, `-ffp-contract=off` and `/fp:strict`, own `pow`, prebuilt sim in UE, the core renders all text, wasi-sdk) | keep | iteration 1 |
| D3 | Hatchling-safe century as a band (no bot death before year 80) | yes | iteration 2 |
| D4 | German: read the seed draft, set the per-round sample | Manu reads during iteration 2 | iteration 2 |
| D5 | Valleys that remember dragons as a start dimension | yes, as data in 02 and 12 | iteration 3 |
| D6 | Endings in v0.5 | Mortality, The Myth, Ascension | iteration 5 |
| D7 | The voice present from hatching in every seed | yes | iteration 5 |
| D8 | Four-screen UI, type scale, pad controls | yes; old page behind `?layout=single` | iteration 8 |
| D9 | Telemetry opt-in, off by default, no network in phase 1 | yes | iteration 8 |
| D10 | Raid open points: civilian toll, night raids, hostage weight, travel ceiling per era, walkable lair | civilian toll always counts; the rest decided after iteration 9's bots | iteration 9 |
| D11 | Engine, from the spike's table | Unreal if rows 2, 4, 7 hold; Unity 6 otherwise | end of iteration 10 |
| D12 | GPU: upgrade, rent, or stay on the GTX 980 | decide from the spike's Lean numbers; the Modern profile and any Lumen look need an RTX-class card | before phase 4 art |
| D13 | The name | clear Wyrmsleep, Wyrmwake, Cinderwake, Slumberhoard, Ashvale | before the Steam page (iteration 15) |
| D14 | Second Breath for a first run (on, off, asked) | a playtest question; the demo decides | iteration 18 |
| D15 | Platform and input: Steam Deck Verified as a target, controller first-class | yes | iteration 8 (UI), iteration 14 (engine UI) |
| D16 | Side-business check with the employer contract, trade registration, Steuerberater | before paying the Steam fee | iteration 10 |

Each decision lands as one line in `decisions.md` with its tripwire.

## 3. Step 0: make the code safe and reachable (one evening plus one agent session)

Done 2026-10-26: `gamedev` pushed to GitHub (one commit, binaries in LFS, about 820 MB) and attached to the session (D1 taken).

Manu, on the PC, still open:
1. Download `hoard-src.zip` from the chat "Dragon game concept development" and unpack it to `games/hoard/prototype/src/` (keep `prototype/index.html` as the reference build). Needed before iteration 9.
2. Delete `assets/_unused/` from the working tree (it stays in history; harmless at this size). Note the source and licence of `art/redragon/redragon.blend`.
3. Install the MSVC v143 toolset (VS 2022, "Game development with C++" plus "MSVC v143 build tools 14.44"), per `tool-setup.md`. CI covers MSVC; this is for local builds and the spike.

Agent session (branch `item/00-docs-fixes`):
1. Design docs stay in the Project as the canonical copy; subagents cannot read the Project, so docs are copied into `games/hoard/docs/` per item, the ones its brief lists, before the item starts. `docs/README.md` says so.
2. The prompt corrections from `audit-2026-10-26.md` §11 are applied to each prompt when its item starts (preamble and 17 done 2026-10-26).
3. `decisions.md`: D1 recorded; the stale demo-cut line fixed.
4. `audit-2026-10-26.md` committed as `reports/00-audit.md`.

Gate: every prompt matches the tree when its agent starts.

## 4. Phase 1: sim v0.5 and the tactical shell (iterations 1 to 8)

Order and dependencies are `prompts/README.md`'s. Roughly three to four months of evenings.

### Iteration 1: foundation (item 17)

Status 2026-10-26: merged (pull request 2, text goldens in 3, nightly annotations in 5). Report in `games/hoard/reports/17-repo-determinism-ci.md`. It also fixed a save/load bug (a loaded world diverged from the original).

Opus, one to two agent-weeks. Layout, `docs/`, the determinism policy in CMake, `check_banned.sh`, `detmath` and the six `pow` replacements, the state hash with per-decade trace, the save header, `.hrp` record and replay covering all ten state-changing verbs (audit §5), CI on GCC, Clang, MSVC, wasm, nightly jobs, the negative controls. Also the `dragons[]` change (preamble rule 8) as its own commit, since 17 touches `World` first.

Decisions first: D2.
Manu: nothing beyond review; MSVC runs in hosted CI.
Gate: `ci.yml` green on four platforms with `compare` passing; negative controls fail as expected; one `golden:` commit for `pow`; record and replay pass for `played`, `idle`, `raze_all`; the measured sim time written into the preamble.

### Iteration 2: measure, and freeze the text grammar (01 and 19 in parallel)

Status 2026-10-26: 01 merged (pull request 4, `reports/01-baseline-v04.md`): survival favours short sleeps (1 year wins 28.9 % of decisive states, 3000 years 8.8 %), growth favours long ones (3000 years wins 42 %), so the bet is real; no death before year 80 except `raze_all`. 19 in pull request 6 (`reports/19-text-renderer-l10n.md`): renderer, `data/text/{en,de}/`, person sex stored (rules version 2), refusal reasons as codes, 30 seeds fixed with German for all 246 strings, page chrome in en, pseudo and de. Narrator and page narration stay English until a later conversion (Manu's call). Open for Manu: D4, the German read in `reports/19-de-review.md` (all 30 seeds in both sexes).

- 01 (Sonnet): sweep with six policies, agency metrics, bucket analysis, perturbation, lethality ratio, `newcomer` policy and first-hour slice, `data/bands.json`, `reports/01-baseline-v04.md`.
- 19 (Opus, Sonnet for `translate.py`): renderer in `sim/text/`, per-language files, schema change, the 30 seeds fixed, German for the seeds, pseudo-locale check, the English literals and refusal reasons out of `sim/`.

Decisions first: D3, D4.
Manu: reads the 30 German seeds in both sexes; his corrections become the lexicon and `docs/style-de.md`. Also answers the seven questions at the end of `paper-playtest-wakings.md` (half an hour; the answers steer 05, 13 and 14).
Gate: the baseline answers "is the sleep bet real" with the bucket table; the five German test strings render equal natively and in wasm. Known before it runs (audit §9): played outcome entropy is 4.03 bits against the 4.5 target, and played valleys end orc-dominated in 174 of 300 seeds; the baseline should say whether that is the burn policy or a rule. **Review point**: if the baseline contradicts `pacing-model.md`, re-read prompts 02 and 03 before going on.

### Iteration 3: dread, awe and memory (02)

Status 2026-10-27: built, pull request 7 (`reports/02-dread-awe-memory.md`). Gate: first slayer order 320 years after hatching under `played`, every order at dread 0.5 or more; the v0.4 bands that passed still pass (played deaths 8 %, was 21 %); the library-burn test holds. Not met: `protect_one` gets the protected court's awe to 0.07 by year 500, not 0.5 (Manu to say whether the policy or the gains should change). D5 taken: remembering valleys as start data.

Opus. Everything after reads these. Includes the `talents.json` rewrite to `talents-v2.md` with the flight fields (item 16 reads them) and the remembering-valley start values if D5 is yes.

Gate: slayer founding under `played` in 300 to 600 (v0.4: about 400); every v0.4 band passes; the library-burn test holds.

### Iteration 4: digestion, visitors, sifting (03, 06, 14 in parallel; 12a may start)

- 03 (Sonnet): consumption, `bed_mode: consumed`, forecast fields; the four tuning targets.
- 06 (Sonnet): arrivals with reasons and routes, `receive`, `hunt`.
- 14 (Opus): pattern bank, acceptors, threads, headline, line budget, callbacks, source transform.
- 12a (Opus, optional here): the culture half of item 12.

Manu: the human check on 15 briefing pairs.
Gate: item 03's targets and the bucket wins; item 14's headline metrics; Manu agrees with the sifter on 75 % or more of the pairs.

### Iteration 5: the player's model of the world (04, 05, 07)

- 04 (Opus): endings registry with 11 entries and 3 enabled, the `exit` act, prophecy, legends and the five profile files, three bots, `data/modifiers.json`.
- 05 (Opus): chronicle levels as a filter over 14's chain, sources, historian, the voice (from hatching if D7), `commune`.
- 07 (Opus): person traits, memory, six moves, secrets, hostage schooling.

Decisions first: D6, D7.
Gate: endings reachable in the dry run by the three bots; the paper playtest's three wakings render from seed 42.

### Iteration 6: acts, slayers, scenarios, raid boundary (08 and 09, then 10 and 15)

- 08 (Sonnet) and 09 (Opus) in parallel; 09 also turns `sleep`, `pick_talent`, hook and offer answers into logged acts, and writes the world-aging fields.
- Then 10 (Opus): scenario runtime, validator, dry run, 20 seed scenarios; and 15 (Sonnet): `RaidReport`, `raid_diff`, the sandbox routed through it.

Gate: the 30 seed scenarios run; the sandbox's check passes unchanged in outcome through `raid_diff`; zero unwarned deaths in bot logs; every schema act appears in the 300-seed coverage table.

### Iteration 7: content (11 and 12)

- 11 (Sonnet script, Opus or Fable drafts): bank rounds 1 and 2, German per batch through 19's script.
- 12 (Opus): culture vector and cults, ascension wired to real cult fields.

Manu: the 10 % read queue per round and the 60-scenario German sample.
Gate: duplicate rate under target; each cell at 70 % of its target; German sample with no defect; ascension ready in 1 to 5 % of runs.

### Iteration 8: the page, and the phase 1 gate (13)

Sonnet. The four screens if D8 is yes, the old page behind a flag. Progressive HUD, legends page, "What the world remembers", Endings block with blanks, scenario cards, arrivals, the voice panel, the sleep bet with both sides, the death screen's "did you see it coming" tap, the `demo` build flag (sleeps capped at 100 years), "Report a problem".

Decisions first: D8, D9, D15.
Manu: recruits five outside players; runs the first-hour script (`first-hour.md` §5, `research-playtesting-telemetry.md` §1); files `.hrp` files and notes in `reports/playtests/round-1.md`.
Gate (= roadmap phase 1): a full run of 30 to 50 wakings plays in the browser to an ending or a Myth death; bands hold; the five players' findings are filed and the clear fixes made (RITE).

## 5. Phases 2 and 3: the flight shell and the engine decision (iterations 9 and 10)

### Iteration 9: flight shell v0 in the browser (16)

Opus, four to six agent-weeks plus Manu's evenings on the valley's look. Built on `prototype/src/`: the continuous valley from the sim's world, travel flight, the hot settlement, the lair as a place, `RaidReport` per sortie, one full waking, a fixed 60 Hz step (audit §7). After it merges, a small follow-up item (Sonnet) extracts `flight.js` and the fire grid's constants into `data/flight.json` and `data/fire.json` so item 18's C++ port and the browser read the same numbers.

Decisions first: D10 for the civilian toll; the rest after the bots play it.
Manu: plays seed 42 and records frame times on his GPU; tunes the valley's look; captures the first footage (the Friday video).
Gate (= roadmap phase 2): two sorties plus travel under 10 minutes on seed 42; the second wake screen cites the burn; frame times hold; acts within 1 % of the tactical shell's; `world-aging.md`'s mapping renders for a 1-year and a 100-year sleep. Tripwires: decisions 2 and 3 (valley, sorties) from the travel share and sorties per waking.

Also in this iteration: the browser build goes to the first 20 to 30 outside testers (one HTML file) with the placeholder dragon replaced by the CC0 Quaternius dragon first (`research-licensing-naming.md` §1).

### Iteration 10: the Unreal spike (18), and the paperwork

Opus plus Manu in the editor. Spike A (6 to 9 working days): plugin with the sim as a prebuilt library behind the C ABI, `Hoard.Sim.Golden`, `HoardRT` flight and fire grid in `rt/` with the golden trace against the 60 Hz `flight.js`, the valley tiles, the pawn, one sleep off the game thread, the command-line agent loop, Unreal MCP. Spike B (8 to 12 days) only if A allows: settlement from tags, the hot-settlement director, fire v0, crowd and towers, `RaidReport`, the Lean and Modern profiles. Fresh C++ project in `unreal/Hoard/`; the old `Hoard/` is deleted (audit §8).

In parallel, Manu: the employer contract's side-job clause, the WKO question on the trade, one Steuerberater hour (`research-licensing-naming.md` §6); then the Steamworks account as sole proprietor, bank, tax interview, identity, and the $100 fee (Valve asks 30 days between paying and a first release).

Decisions first: D16 before the fee.
Gate (= roadmap phase 3): the seven-row table filled on Manu's PC with the machine block; D11 written into `decisions.md` as a fact; D12 taken from rows 4 and 5. If two of rows 2, 4 and 7 fail, the Unity 6 spike starts with the same sim, data and `rt/`, and every engine item below is rewritten for Unity before it starts.

## 6. Phase 4: the vertical slice in the engine (iterations 11 to 15)

Goal from `roadmap.md` §4: one waking that looks and sounds like the product, the Steam page live at the end. Four to six months; the dragon and the fire are the long poles and run in parallel from iteration 11. New prompts are written at the start of each iteration from the research docs named; numbers below are their proposed IDs.

### Iteration 11: the engine skeleton (prompt 20) and the dragon's first pass starts (21)

- **20 Engine foundation** (Opus): turn the spike into production modules. `HoardSimUE` with the full API surface generated from the schemas (`gen_ustructs.py`), save and load through the sim's header, `.hrp` from the engine shell, `ui_tokens.json` into a DataTable, the kit table (`data/kits.json`) importer, CI building `rt/` on all native platforms, `Tools/{build,test,package}.ps1` used by every later item. From `research-ue5-integration.md` §1 and §4.
- **21 Hero dragon, asset track** (Manu, hired rigger if it fights back): concept sheets, paid-plan Tripo or Meshy mesh (clean-room rule: never from the placeholder), retopology, UVs, the 93-bone rig from `research-dragon-ue5.md` §4, scripted weights, ten correctives, the clip list of §3 hand-keyed and rotoscoped from licensed or self-shot footage, FBX export with the cube test first. Every input in `assets/ai-log.jsonl`. Decision point after two weekends on the rig: commission it.

Gate: a packaged build runs one sleep and one sortie through the full API; the dragon's mesh passes the contact sheet; the cube test and the rig with `fly` and `glide` play at 1.5 Hz.

### Iteration 12: flight and fire (prompts 22 and 23)

- **22 Flight controller, animation graph, camera** (Opus for C++, recipes for Manu): `UHoardFlightMovement` on `rt/flight`, collision sweeps and the tower rule, carry sockets, mouse aim with the neck cone and soft lock, the anim graph order of `research-dragon-ue5.md` §2 with `CR_DragonProcedural` (wing fold, camber helpers, delayed edge, tail lag, head aim), takeoff and landing curves, the camera table of §5 with its shakes, the altitude tape. Then the feel gate of §7 (three runs, side-by-side video, Manu and two viewers rate on 1 to 5).
- **23 Fire v1** (Opus for the grid side, recipes for Manu): the data path of `research-fire-ue5.md` §2 (valley texture, active list, data channel events, per-building state), the three LOD tiers, the breath components of §3 with probes and splash drops in the core, building damage states of §4, stone and wards of §5, the wind vector and ember telegraph of §6, the burn forecast, the calibration run (mean 0.25 burn per 15 stamina). EmberGen flipbooks for flames and smoke. Measured in the three test scenes against §7's budget.

Gate: the feel gate passes; the fire budget holds on the reference tier at 50 % and 100 % screen percentage.

### Iteration 13: kits, audio, the lair (prompts 24, 25, 26)

- **24 Settlement kits** (Manu with agent scripts): two races × two eras × building classes with intact, scorched, burning, gutted and ruin states (`workflows.md` §6), the kit table, `tools/contact_sheet.py`, the world-aging renderer (soot by years, fields, walls, monuments, ruins from sim fields). Quaternius kits as the base where they fit.
- **25 Audio, first pass** (Sonnet for the WebAudio patch and plumbing, Manu in Wwise): the one-day Wwise-on-5.8 test (FMOD if it fails), the breath stack (layers 1 to 5, 10, 11), connect on wood, one burning house, the fire-bed rule from `research-audio.md` §2, ducking, `sim/audio_state.cpp` and `data/audio.json`. Libraries bought with receipts; every file in `assets/LICENSES.md`.
- **26 The lair as a place** (Opus): the ledge, the pile growing with Might, loot drop, kept people at their places, figures at the ledge with their reasons, scenario cards as objects, landing into the lair panel; reused from the flight shell's design.

Gate: the eyes-closed test on the three sounds (burn size, distance and breath phase by ear); a 1-year and a 100-year sleep render the valley correctly from sim state alone.

### Iteration 14: UI and montage in the engine (prompts 27, 28)

- **27 Engine UI** (Sonnet for C++ bases and data, Manu for UMG layout from recipes): Wake (Courts and Chronicle tabs), Lair stations, Sleep, the death screen, at 1280×800 and 1920×1080, the type scale 24 to 52 dp, controller focus navigation, hold-to-confirm, glyphs; Rich Text only for card and chronicle bodies. Strings from the core's renderer only.
- **28 Montage** (Opus): the sleep montage per `world-aging.md` (event by event under 30 years, one frame per age above), the era palettes, the slow heartbeat cue for audio.

Decisions first: D15 confirmed for the engine.
Gate: one full waking plays in the engine on seed 42 from wake screen to sleep and the next wake screen; no element under 12 px at 1280×800.

### Iteration 15: the slice, and the Steam page

Manu, with agents on fixes. Polish the waking to the "show a stranger" bar; a ten-minute capture on seed 42; 60 fps on medium on his GPU (after D12). Marketing: D13 cleared and the mark filed on the lawyer's read; capsule art (hired, about $600) and a 30-second gameplay trailer (own edit first); hook and short description from `research-release-marketing.md` §4; the five readiness conditions ticked; Content Survey and AI disclosure from the AI log; the page submitted 7 business days ahead; a mailing list open since the first public clip; the weekly three-hour routine running.

Gate (= roadmap phase 4): the capture, the frame rate, the page live under the cleared name, the hero dragon settled before the page (Valve's caution on changing art direction).

## 7. Phase 5: content and systems complete, alpha (iterations 16 and 17)

Four to six months, mostly content and tuning. The sim items here run in the browser shells first, as every design question does; the engine follows through the shared API.

### Iteration 16: the remaining systems (prompts 29 to 35)

| # | item | model | from |
|---|---|---|---|
| 29 | The eight remaining endings: lichdom (undead priests, the necromancer origin from 12), the portal (an allied cult), becoming the mountain, the brood, the emperor, the last flame, the scholar, the sun-eater; each with three rumour routes and its stinger | Opus | `story-and-endings.md` §2 and §3, `endings.json` sketches from 04 |
| 30 | Ambitions: a goal picked at waking, Crusader Kings style, as data over the act vocabulary | Opus | `core-loop.md` §10 |
| 31 | Dreams that pause a sleep: a mid-sleep wake on a world event the dragon would care about, as a forecast hook | Opus | `core-loop.md` §10, the Premonition talent |
| 32 | The event catalogue: the remaining archetypes of the 90 as cards and patterns, in batches by theme, each measured by the interest tool | Sonnet per batch | `event-catalogue.md` |
| 33 | Talents complete: the full `talents-v2.md` list, ranks with every fifth a visible step, duo talents and the skip bonus if pick rates ask for them; pick rates under 20 % retuned | Sonnet | `talents-v2.md`, `core-loop.md` §8 |
| 34 | Difficulty ladder in both directions and Second Breath, with UI | Sonnet | `decisions-review.md` §5, `research-meta-progression.md` §4, `data/modifiers.json` from 04 |
| 35 | Raid open points from D10: night raids, hostage weight and dropped hostages, travel ceiling per era, walkable lair; `guise` discovery and fight-or-flight in a city | Opus | `raid-design.md` §8, `decisions.md` Open |

### Iteration 17: content depth, telemetry, the closed test (prompts 36 to 39)

- **36 Kits complete** (Manu with scripts): four races × five eras with damage states; the valley's eras readable from the air.
- **37 Bank round 3 onward and the pattern bank** (Sonnet script, Opus drafts): every cell at target depth in English and German; patterns to 30 or more; German sample under 5 % defects per round.
- **38 Telemetry and crash reports** (Sonnet): opt-in consent flow, batched events, the funnel and death audit of `research-playtesting-telemetry.md`, Sentry for engine crashes (Steam's error reporting does not cover 64-bit), replays attached to bug reports, a privacy notice.
- **39 Saves, settings, accessibility** (Sonnet): versioned saves with migrations and `tests/saves/`, the bank pinned per run, settings (text scale 100 to 200 %, colour never the only carrier, subtitles for stingers, remapping), the "determinism contract" text in the game.
- Music: commission five pieces plus six stingers (`research-audio.md` §4: lair, raid with four stems on `threat`, wake, montage, stingers), about 15 minutes; the raid piece first as temp stems to test the system.

Manu: a closed group of 20 to 30 testers (Steam Playtest or keys) plays at least one full run each; the human briefing check per build.
Gate (= roadmap phase 5): all eleven endings reachable in the dry run; the three bots meet the ending curves of `research-meta-progression.md` §6 (first ending by run 2, a second kind by run 4, ascension by run 8 to 10); sifter agreement 75 % or better; no unwarned death in the test logs; German defects under 5 %.

## 8. Phase 6: beta, demo, Next Fest (iterations 18 and 19)

Three to four months.

### Iteration 18: the demo and the beta (prompts 40, 41)

- **40 Demo build** (Sonnet): hatching to the end of waking 7 or 8 (45 to 80 minutes), sleeps over 100 years shown and locked, one seed family with the mirror as the voice, three starting situations, no carry-over, the wishlist link on the end screen, its own Steam app and page.
- **41 Deck and performance** (Opus): full controller support and text entry, 30 fps at 800p, the Deck profile, performance passes on the reference tier, localisation QA in German at both resolutions, the credits screen and `THIRD_PARTY_NOTICES.txt` from `assets/LICENSES.md`, the EULA line, the rating survey answered against the worst scene.

Decisions first: D14 (the demo is the playtest for Second Breath).
Manu: Steam Playtest for the full build; the release gate checks of `research-licensing-naming.md` §6 (no share-alike rows, every AI asset logged).
Gate: demo median playtime 60 minutes or more in the playtest; the funnel read from telemetry; no crash and no unwarned death in the open bug list.

### Iteration 19: Next Fest and the date

Register 6 to 7 weeks ahead, all items 3 weeks ahead; the public date 60 or more days out; third-party festivals (Rogue Bots, Strategems, Tacticon) applied to where entry is free; creators contacted from the sheet.
Gate (= roadmap phase 6): 2,000 or more wishlists before the fest and 7,000 toward launch, or the launch plan adjusted; Deck review passed.

## 9. Phase 7: 1.0 and after (iteration 20 onward)

Launch at $19.99 with a 10 to 15 % launch discount, five or six days after the fest or at a sale start; Early Access only as the named fallback with its gates. The first month of patches from replay-attached reports (`workflows.md` §12); a run in progress keeps its data version (beta branch for the old one). Then the long tail as separate decisions: more endings and cards, a second valley template, mod support, and the multiplayer study (an asynchronous shared valley where each dragon's waking is a turn), which the `dragons[]` change keeps cheap.

## 10. Beyond Hoard: the suite

The project's aim is a suite for several games. What Hoard leaves behind for the next one: the `gamedev` repo's skills (Stefan 3D AI's Mr. Mak pack plus `unreal-editor`), `tools/` (setup check, video-watch), the asset, recipe and playtest pipelines in `workflows.md`, the determinism and CI setup of item 17, the scenario authoring pipeline of item 11, and the `rt/` pattern of an engine-free core. The kaiju-tamagotchi idea (`kaiju-tamagotchi-notes.md`) stays parked until Hoard's phase 5 gate; at that point one session reads its notes against this list and writes its own brief.

## 11. Calendar (orders of magnitude, from `research-release-marketing.md` §6)

| span | phases | what Manu does most |
|---|---|---|
| Nov 2026 to Feb 2027 | step 0, iterations 1 to 8 | reviews, German reads, the first playtest round |
| Mar to May 2027 | iterations 9 and 10 | playing the flight shell, editor weekends for the spike, paperwork |
| Jun to Nov 2027 | iterations 11 to 15 | the dragon, editor recipes, the page |
| Dec 2027 to mid 2028 | iterations 16 and 17 | content reads, the closed test |
| mid to late 2028 | iterations 18 and 19 | demo, Playtest, Next Fest (June or October 2028) |
| after the fest | iteration 20 | launch and patches |

Every span moves with the gates; scope is cut inside a phase, never at its gate.

## 12. What runs where

- Cloud sessions on the attached `hoard` repo: all sim, tooling, web and authoring items (1 to 19 except 18, and 29 to 34, 37 to 40), two to four at once, each on its branch.
- Manu's PC: the MSVC check until CI covers it, playing on his GPU, frame-time tables, everything in Unreal and Blender, Wwise, the editor recipes with OFF/ON captures.
- Hired, if and when: a creature rigger (iteration 11 or 12), capsule art and a trailer edit (iteration 15), a composer or music licence (iteration 17), a German proofreader beyond Manu's pass (phase 5), a professional name search and trademark filing (iteration 15).

## 13. Prompts to write, and when

| prompt | write at the start of | from |
|---|---|---|
| 20 Engine foundation | iteration 11 | `research-ue5-integration.md`, spike report |
| 21 Hero dragon asset track (a Manu checklist) | iteration 11 | `research-dragon-ue5.md` §3 and §4, `workflows.md` §7 |
| 22 Flight controller, anim, camera | iteration 12 | `research-dragon-ue5.md` §1, §2, §5, §7 |
| 23 Fire v1 | iteration 12 | `research-fire-ue5.md` |
| 24 Settlement kits and world aging in the engine | iteration 13 | `workflows.md` §6, `world-aging.md` |
| 25 Audio first pass | iteration 13 | `research-audio.md` |
| 26 The lair as a place | iteration 13 | `raid-design.md`, item 16's report |
| 27 Engine UI | iteration 14 | `research-ui-text-strategy.md` |
| 28 Montage | iteration 14 | `world-aging.md`, `research-audio.md` §4 |
| 29 to 35 | iteration 16 | table in section 7 |
| 36 to 39 | iteration 17 | section 7 |
| 40, 41 | iteration 18 | `first-hour.md` §8, `research-release-marketing.md` §1 |

Writing a prompt before its iteration would freeze it against code that does not exist yet; each is written from the previous iteration's reports.
