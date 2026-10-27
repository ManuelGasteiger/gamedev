# Hoard: decision log

One line per decision, newest first, with the doc that carries the detail. A new session reads this before anything else. Open questions live at the bottom.

## Decided

| date | decision | detail in |
|---|---|---|
| 2026-10-27 | The sifter only watches: episodes and stories live in `World.sift`, outside the chronicle, so a run tells the same history with the pattern bank on or off and the bank can change without new goldens. Runner-set: tiers per template in `importance.json`, the same-court penalty is the falling diversity penalty (0.20 to 0.05), a sleep with no tier-3 candidate has no headline. | `reports/14-story-sifting.md` |
| 2026-10-26 | Fear is gone: dread and awe per court in oral, written and monumental stocks (item 02). Runner-set start values: oral half-life 60 years at human memory (scaled by race), research 0.016 per decade x era x dread factor x wealth/(wealth + 20), 50 wealth per unit of research; the written-grievance slayer trigger stays off (founding holds without it). | `reports/02-dread-awe-memory.md` |
| 2026-10-26 | Witnesses are the owner (1.0) and the neighbouring courts (0.5); the sim derives them and ignores what a shell sends. Sage and envoy presence waits for people with places (item 07). | `act-api.md` §4, `reports/02-dread-awe-memory.md` |
| 2026-10-26 | `no_walls` and `no_settle` hold while the court remembers the dragon (dread or awe at 0.2) and break by chance at 1 - stability per decade (resentment). | `act-api.md` §6.4, `reports/02-dread-awe-memory.md` |
| 2026-10-26 | Valleys that remember dragons are a start dimension, as data (item 02 start values, item 12). | `first-hour.md` §12, `implementation-plan.md` D5 |
| 2026-10-26 | German name of the Endings page: „Enden“. | `docs/style-de.md` |
| 2026-10-26 | The HUD may wrap to two rows at 1024 px (game-check j.1024 height under 80). | `reports/19-text-renderer-l10n.md` §4 |
| 2026-10-26 | Hatchling-safe century as a band, warning only (no bot death before year 80). | `data/bands.json`, `reports/01-baseline-v04.md` |
| 2026-10-26 | One repository: `gamedev` on GitHub (private), with Hoard in `games/hoard/` beside the shared skills and tools; LFS for binaries; sim and tooling agents clone without LFS. Supersedes the separate `hoard` repo assumed in prompts 17 and 18. | `implementation-plan.md` §3, `audit-2026-10-26.md` |
| 2026-10-24 | The shell never remembers: everything a shell shows about the past is read from sim state at wake, aged by the sim's own fields (`last_burn_year`, `ruin` with an age, monuments, `abandoned_year`); smoke exists only within a waking. | `world-aging.md`, preamble rule 9 |
| 2026-10-24 | Stamina budget and drain, carry, travel speed and travel altitude are talents; the shells read all flight numbers from the API's `dragon.flight` block. | `talents-v2.md`, `raid-design.md` §1 |
| 2026-10-24 | Bed refinement: Might comes from the gold the bed consumes; the floor is what the dragon keeps and grows nothing. | `pacing-model.md` finding 1, `prompts/03-digestion.md` |
| 2026-10-24 | The raid takes place in one continuous valley (the twelve regions as districts of one flyable space), with travel flight above 300 m and one "hot" settlement at full fidelity at a time. | `raid-design.md` §2 |
| 2026-10-24 | A waking is several sorties with a shared stamina budget; carry weight and stamina are the limits; the lair between sorties is where audiences and scenarios happen; no sim time passes within a waking. | `raid-design.md` §1 |
| 2026-10-24 | Two shells, one act log: the tactical sandbox and the flight game both report facts (`RaidReport`); the core's `raid_diff` makes acts and magnitudes. Unreal is a third shell later. | `raid-design.md` §3 |
| 2026-10-23 | Digestion consumes the hoard partially: each sleep converts a share of every category into growth and that share is gone; the gold bed keeps a floor. Raids stay necessary all game. | `act-api.md` §7, `prompts/03-digestion.md` |
| 2026-10-23 | Run length target: 6 to 10 hours, 30 to 50 wakings per run. Node thresholds, storyteller cadence and bank size derive from it. | `act-api.md` §7, `scenario-spec.md` §7 |
| 2026-10-23 | Implementation agents work from a GitHub repo, one branch per item; Manu reviews and merges. | `prompts/README.md` |
| 2026-10-23 | Order: interest-tool baseline first (sleep-policy sweep, agency metrics), then dread/awe + memory tiers, then the rest. | `prompts/README.md` |
| 2026-10-23 | The act vocabulary is closed (40 acts, v1.1). The real-time raid produces exactly these acts with continuous magnitudes. | `act-api.md`, `act-api.schema.json` |
| 2026-10-23 | `judge` is its own act, not a `kill` payload. | `act-api.md` §7 |
| 2026-10-23 | Secret acts (`corrupt` on a person, `haunt`, `commune`, `keep`, `build`, undiscovered `guise`) have no witnesses until revealed. | `act-api.md` §4 |
| 2026-10-23 | Scenarios are data matched at runtime; the LLM authors offline against the schema; the sim stays deterministic. Runtime LLM only ever for flavour text. | `scenario-spec.md`, `story-and-endings.md` §6 |
| 2026-10-23 | Chronicle levels are a filter on which `Event.cause` lines the briefing may print; the historian is a kept person whose presence sets the level. No new causal model. | `paper-playtest-wakings.md` (closing note), `prompts/05-chronicle.md` |
| 2026-10-22 | Fear splits into dread and awe; faith stays as a cult's share of a court. | `story-and-endings.md` §4, `act-api.md` §6 |
| 2026-10-22 | Divine ascension is the hardest ending and is planted at hatching as a prophecy mortals also know and react to. | `story-and-endings.md` §2–3 |
| 2026-10-22 | Several endings as condition checks on world state, never scripted; the game should take several runs to finish; knowledge carries between runs, power never. | `story-and-endings.md` §3 |
| 2026-10-22 | Civilisations get longer memory through books, historians and monuments (three tiers). | `story-and-endings.md` §4 |
| 2026-10-22 | How real-time raid actions map to sim acts is deferred until the acts are stable in the sim. | `story-and-endings.md` §7 |
| 2026-10-16 | Arms race is the spine; growth from loot and sleep length through talents; death is final; starting world varies per seed with pre-history and rare irreversible events. | `core-loop.md` |
| 2026-10-14 | Race is fixed, culture is separate and mutable; the sim never reads player intent, only the act log and the hoard. | `ages-sim.md` §2 |
| 2026-10-13 | Unreal Engine for the final game; logic and feel first in a portable architecture (C++ sim, browser shell). | memory, `production-plan.md` |

## Open

- **Technical defaults written into prompts 17 to 19 (2026-10-25), standing unless Manu objects:** the repository layout with `sim/`, `rt/`, `web/`, `unreal/` inside `games/hoard/` (`research-ue5-integration.md` §1); the sim built with `-ffp-contract=off` and MSVC `/fp:strict`, own `pow`/`exp`/`log`, hashes of bits never text (`research-determinism-ci.md` §1 and §2; one golden regeneration expected); the sim enters Unreal as a prebuilt library behind a C interface, the real-time core compiles from source in its own module; the `.hrp` replay is the bug report and the telemetry unit; the C++ core renders all player text from per-language files (`research-localization.md`); wasm stays on wasi-sdk.
- **From `first-hour.md` §12, five decisions:** the hatchling-safe century as a band (no bot death before sim year 80 under any policy); the voice present from hatching in every seed; valleys that remember dragons as a start dimension; the demo cut (hatching to the end of waking 7 or 8, 45 to 80 minutes, sleeps capped at 100 years, no carry-over; corrected from "first moult plus two wakings" after the Next Fest research); Second Breath on, off or asked for a first run.
- **UI** (`research-ui-text-strategy.md`, proposal): split the wake page into four screens (Wake with Courts and Chronicle tabs, Lair as four fixed camera stations with flat text panels, Sleep, Montage); type scale 24/28/34/40/52 dp, which clears Valve's Deck floor (9 px minimum, 12 px recommended at 1280×800); pad controls with hold-to-confirm for sleep and irreversible acts; one `ui_tokens.json` for CSS and UMG; screens built in the browser first, then ported; no embedded browser in the shipped UI. Rich Text only for card and chronicle bodies (Epic flags it as costly in UMG).
- **Telemetry** (`research-playtesting-telemetry.md` §3, proposal): opt-in, default off, no network request before the choice; phase 1 has no server, only a downloadable `.hrp`; no live A/B in the premium build (beta branches and bots instead).
- **German, concrete** (`research-localization.md`): Manu reads the unreviewed German draft of the seeds (`scenarios/localization-seed-de-draft.json`); his defect rate sets the sample size per bank round (60 scenarios with no defect bounds the rate under 5 %).
- **Reopened by the decision review (2026-10-24, `decisions-review.md`):** localisation (proposal: ship English and German, make the scenario schema language-aware before authoring round 1, Manu reviews German); platform and input (proposal: design for Steam Deck Verified from the start, controller as a first-class input, mobile only ever as the tactical shell); the name (clear a candidate before the Steam page, phase 4). Hedges proposed, not yet taken: `World.dragons[]` instead of a singleton (preamble rule 8 assumes yes); a "Second Breath" softer modifier beside the harder ladder (data only until playtests say).
- Engine tripwire: Unreal stays, decided for real by the UE spike's three numbers on Manu's PC (phase 3); Unity 6 is the named fallback with the same sim and data.
- Raid open points (`raid-design.md` §8): the fire's civilian toll (proposal: always counts, scaled by who got out), night raids, hostage weight and dropped hostages, travel-flight ceiling per era, how much of the lair is walkable.
- Paper playtest answers (`paper-playtest-wakings.md`, seven questions). The one that matters most: do the "because" lines or the hooks naming the player's own act make the world feel reactive?
- `guise` discovery odds; discovered dragon in a city: fight or flight.
- Which endings ship in v0.5 (proposal: Mortality, The Myth, Ascension).
- Name collision: "Hoard" (2011, Big Sandwich Games) is also a dragon-burns-villages game.
- GPU: the GTX 980 is below Lumen's minimum; upgrade, rent, or stay, decided from the spike's Lean numbers (`implementation-plan.md` D12).
