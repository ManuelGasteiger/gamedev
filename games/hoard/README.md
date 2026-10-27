# hoard-sim

Headless, deterministic C++17 simulation of the mortal factions of the valley while the dragon sleeps. Spec: `docs/ages-sim.md` (sections 1 to 19), `docs/event-catalogue.md`. This is v0.3: v0.2 (people, offers with reasons, phases, the briefing) plus long tails (section 18): event cards in `data/events/`, keepers of memory, importance on every event, narrated reactions to the dragon's acts.

## Build, run, test

```
cmake -S . -B build
cmake --build build -j
./build/hoard-sim --data data --seed 1 --years 3000 --out out/s1
./build/hoard-sim --data data --seed 1 --years 2000 --policy raze_all --sleep 100 --out out/r1
./build/hoard-sim --data data --seed 1 --years 1000 --acts my_log.json --out out/a1
cd build && ctest                # all tests (ctest -L fast skips baseline and policies)
./build/hoard-sim --data data --golden 1-60            # state-hash golden: `seed hash` per seed (generated world, played, 3000 y)
./build/hoard-sim --data data --seed 3 --years 3000 --policy played --genesis --trace t.jsonl --out out/t3   # state hash per sleep
python3 tools/first_div.py a.jsonl b.jsonl            # first checkpoint and part where two traces differ
tools/check_banned.sh                                  # determinism gate on sim/ (also the banned_ops test)
cd build && ctest -L fast        # determinism, reactions, oath_breach_sleep, golden
python3 tools/ages_stats.py --bin build/hoard-sim --seeds 1-200 --years 3000 [--policy raze_all]
python3 tools/ages_stats.py --read out/stats
python3 tools/interest.py --bin build/hoard-sim --data data --seeds 1-300 --years 3000 --policies idle,raze_all,played --out out/interest --jobs 4 --read-set 2
./build/hoard-sim --data data --seed 3 --years 600 --sleep 300 --briefing --out out/b3   # prints the last sleep's briefing
```

The runner writes `chronicle.json`, `world_final.json`, `stats.json` and `briefing.json` into `--out` (`--narrate` adds `narrated.json`, the narrator's sentence for every event; `tools/interest.py` uses it for its narrated chronicles). `--acts` applies an act log once before the first sleep; `--policy` applies the policy's act template at the start of every sleep, targets resolved against the current world. A 3000-year run takes about 0.3 s (people and cards dominate).

Builds with GCC and Clang here (identical chronicle hashes); no GNU extensions, no exceptions across the public API.

## Replays, saves and bug reports

`hoard-sim ... --record run.hrp` writes every input the run made (each outermost API call: `apply_acts`, `sleep`, `pick_talent`, offers, hooks, `kill_person`, spells, `terraform`, `demand`, `load_world`) as JSON lines, with a state-hash checkpoint after every sleep. `hoard-sim --data data --replay run.hrp` rebuilds the start world from the header (seed, generated or not), replays the inputs and checks every checkpoint; exit 3 names the first checkpoint that differs. A replay that fails here but passed on the machine that recorded it is a determinism bug; `--trace` on both builds and `tools/first_div.py` find the year and the part.

To replay a bug report from the page (Game, Report a problem): decode the blob (`python3 -c "import base64,json,sys; d=json.loads(base64.b64decode(sys.stdin.read())); open('r.hrp','w').write(d['hrp']); open('save.json','w').write(d['save'])" < blob.txt`), check out the build it names, then `--replay r.hrp`.

Saves (`sim/ages/save.h`): a header (`format_version`, `rules_version`, build, data hash, seed, year, state hash) and the world. Loading refuses a newer format and any save whose recomputed state hash differs.

## Browser sandbox

`web/ages-sandbox.html` is a single self-contained page (the wasm module and the six data files are embedded as text) for trying acts and sleep lengths against the sim: valley map (12 regions), faction table (dominant race and pop share, vassal status, traits), dragon panel with hunger, world panel (regions by race, remnants, wars, a sponsor-remnant control), act builder, offers, sleep controls with a forecast line, chronicle feed, world export/import. Double-click it or serve it; nothing is fetched.

Build on Windows:

1. Download the `wasi-sdk-25.0-x86_64-windows.zip` (or newer) release from https://github.com/WebAssembly/wasi-sdk/releases and extract it, e.g. to `D:\tools\wasi-sdk` (that is the default; otherwise set `$env:WASI_SDK` to the folder that contains `bin\clang++.exe`).
2. `powershell -ExecutionPolicy Bypass -File web\build.ps1`
3. Open `web\ages-sandbox.html`.

On Linux/macOS: `WASI_SDK=/path/to/wasi-sdk web/build.sh` (needs python3 for the embedding step). Both scripts compile every `sim/ages/**/*.cpp` except `data_io.cpp` (file IO) plus `sim/wasm/api.cpp` with `-fno-exceptions -DJSON_NOEXCEPTION -mexec-model=reactor -O2`, write `web/ages.wasm` (about 1.2 MB), and generate the HTML from `web/sandbox.template.html` by replacing `%%WASM_BASE64%%` and `%%DATA_JSON%%`.

`sim/wasm/api.cpp` exports a three-function C ABI (`ages_cmd(json) -> json`, `ages_alloc`, `ages_free`); commands are `load` (data includes `events`, the cards of `data/events/` joined into one array), `reset`, `world`, `apply_acts`, `accept_offer`, `decline_offer`, `sleep`, `briefing`, `people`, `kill_person`, `forecast` ({years}: a blurred dry run on a copy of the world), `sponsor_remnant` ({race, region}), `cast`, `terraform`, `demand`, `catalogue`, `hash`, `load_world`. `apply_acts` and `kill_person` return `lines`: `[{faction, text}]`, what the act meant to each court that reacted. The page's WASI shim covers fd_write/fd_close/fd_seek and a few more; only those three are imported. Emscripten would also work later with the same `api.cpp`.

Checks: `node web/smoke.mjs` loads the wasm in Node, runs a short scenario and verifies that the wasm chronicle hashes for both golden scenarios equal `tests/golden/ages_v0.hash` and `ages_v0_1_raze.hash`. `PLAYWRIGHT_BROWSERS_PATH=... node web/check.cjs` drives the page headless (burn, apply, read the forecast line, sleep 100; then reset and sleep 1000 on an empty hoard and assert hunger rose) and writes `web/sandbox-check.png`. `node web/game-check.cjs` drives `web/hoard-game.html` through the playtest checklist in `web/GAME.md` and writes `web/shots/*.png`; `web/build.sh --html` re-embeds the existing wasm without recompiling.

## Layout

```
CMakeLists.txt                  library `ages`, runner `ages`, six ctest targets labelled fast/slow
data/constants.json             every number in the doc, plus the ones added below
data/races.json                 race templates (doc section 3) plus temperament, trait start values, affinity rows, orc off-map remnant
data/reactions.json             act x relationship reaction rows (doc section 5)
data/regions.json               12 regions with terrain: lair, east/west vales and marches, hill hold + deep hold (hills), elder/deep/old wood, north scar, south fen
data/factions.json              bronze-age start: aldmere, vessary (human, 2 regions each), dorun (dwarf, hill_hold + deep_hold), ilvareth (elf, elder_wood + deep_wood)
data/shocks.json                plague, bad century, volcanic winter (now a chance per mega raze)
data/events/*.json              event cards, one file per catalogue entry (docs/ages-sim.md 18.3)
data/importance.json            importance per event kind for the briefing and the storyteller (language-neutral)
data/text/<lang>/               per language: narrator.json (the narrator, English only for now), ui.json, lexicon.json,
                                names.json, locale.json (CLDR via Babel, tools/text/cldr.py), scenarios.json and .meta,
                                patterns.json; one bundle per language, hashed into data_hash
sim/text/                       the renderer: tokens, lexicon, names, numbers, render, bundle (+ pseudo), batch, reason
data/names.json, offers.json    17.2 name syllables, 17.3 offer kinds
data/policies/*.json            idle, raze_all, protect_one, alternate
sim/ages/ages.h                 public API: load_data(_from_json), apply_acts, sleep, chronicle_hash, offers, JSON in/out, run_scenario
sim/ages/ages.cpp               load_data_from_json, accept_offer / decline_offer
sim/ages/state.h                all plain-data structs
sim/ages/rng.h                  splitmix64 seeding + PCG32, uniform() from the top 24 bits
sim/ages/json.h                 vendored nlohmann/json 3.12.0 (MIT)
sim/ages/data.h/.cpp            Params/Constants structs and loaders from JSON text (no file IO)
sim/ages/data_io.cpp            file loaders: load_data, load_params, load_world, load_policy (left out of the wasm build)
sim/ages/world_util.h           lookups, power/threat/affinity formulas, dominant race, vassal and realm helpers, losing(), memory helpers, emit()
sim/ages/names.h/.cpp           deterministic successor and orc faction names
sim/ages/acts.cpp               apply act log: immediate deltas, reactions, witnesses, rage, feeding, the sponsor_remnant spell
sim/ages/reaction_lines.h/.cpp  the `lines` of apply_acts from narrator.json "reactions" (row that fired, offers, known people, card marks)
sim/ages/importance.h/.cpp      importance per event (template value plus known/oath/hostage boosts), template variants
sim/ages/briefing.cpp           17.5 narrator: briefing blocks, event_sentence (cause clauses, threads)
sim/ages/cards.h, card_data.cpp event card definitions and their JSON loader (the trigger language is documented in cards.h)
sim/ages/offers.h/.cpp          17.3 offers with reasons, fulfilment, the dragon's ledger, kill_person
sim/ages/spells.cpp             section 8 verbs: spells, terraform, demand
sim/ages/hoard.cpp              hoard digestion with half-lives plus the lair hunt; returns the decade's nourishment
sim/ages/hunger.cpp             hunger step (shortfall, atrophy, waking) and feeding acts
sim/ages/forecast.cpp           forecast(): sleep on a copy with a derived rng, blurred by 1/(1+knowledge)
sim/ages/pop.h/.cpp             population mix: culture blend, conquest shift, trait blend, refugees
sim/ages/oaths.h/.cpp           clause predicates, stake effects, offer generation
sim/ages/step.h/.cpp            the ordered decade step
sim/ages/sleep.cpp              sleep(), wake reasons, chronicle_hash
sim/ages/chronicle.h/.cpp       compaction, canonical serialisation, FNV-1a hash, JSON out
sim/ages/serialise.cpp          world/act log JSON round trip, world_view_json (derived power/era/threat/offers), sleep_result_to_json
sim/wasm/api.cpp                C ABI for the wasm build (ages_cmd)
web/                            build.sh, build.ps1, sandbox.template.html, game.template.html, GAME.md, smoke.mjs, check.cjs, game-check.cjs, vendor/three.r158.min.js (offline copy for the check only); generated: ages.wasm, ages-sandbox.html, hoard-game.html
sim/ages/policy.cpp             policy template resolution (targets follow a faction's successor lineage), run_scenario loop
sim/ages/rules/rules.h          rule signatures
sim/ages/rules/modifiers.cpp    expire modifiers, effective per-faction/region parameters: trait and temperament multipliers, terrain land, vassal wealth
sim/ages/rules/wealth.cpp       growth from land, war penalty, upkeep
sim/ages/rules/cohesion.cpp     decay by power/power_cap, succession by ruler class
sim/ages/rules/era.cpp          era progress, diffusion, anti-dragon line, slayers tag
sim/ages/rules/war.cpp          declarations (hostility by affinity), sides with allies, capture with pop/trait blend, vassalage on the last region, war end
sim/ages/rules/coalitions.cpp   coalitions against a hegemon and alliances of neighbours; called from rule_war between declaration and fighting
sim/ages/rules/vassals.cpp      vassal tribute, independence, absorption of a collapsing same-race vassal (after war, before collapse)
sim/ages/rules/collapse.cpp     split into successors (civil_war when authority is high), landless removal with refugees
sim/ages/rules/memory.cpp       memory decay, fear/faith recompute, betrayal block
sim/ages/rules/dragon_rolls.cpp tribute, slay attempts
sim/ages/rules/fading.cpp       the fading of elf factions under human wealth dominance (before migration)
sim/ages/rules/migration.cpp    elf withdrawal, razed-land abandonment, settling (home terrain only for dwarves/elves), dwarf dug_in, orc spawn
sim/ages/rules/remnants.cpp     remnant bookkeeping, decay, re-founding on open home terrain (after migration)
sim/ages/rules/regimes.cpp      trait drift: war, peace, dragon fire, open neighbours (after remnants, before hegemony)
sim/ages/rules/hegemony.cpp     max realm bookkeeping, hegemon tag, hegemony_rose/fell events
sim/ages/rules/land.cpp         burn recovery, wasteland tag
sim/ages/rules/oaths.cpp        clause evaluation, breach events and stakes
sim/ages/rules/shocks.cpp       plague, bad century, volcanic winter, great rulers (golden age / tyrant), succession crises
sim/ages/rules/cards.cpp        event cards: tag expiry and inheritance, trigger, roll, fire (after shocks)
sim/ages/rules/card_match.cpp   the trigger predicates with variable binding and backtracking
sim/ages/rules/card_effects.cpp card effects: tag, field, modifier, memory, war, spawn_person, crown, split, spawn_faction, ...
sim/ages/rules/card_ctx.h       the context a card is evaluated in (bound variables, cause refs)
sim/ages/rules/people.cpp       17.2 lives: ages, deaths, heirs, crowning, orphans (people_life.h shares its helpers)
sim/ages/rules/people_rise.cpp  heroes, sages and priests, keepers of the court's oldest memory, named reigns
sim/ages/rules/hostages.cpp     people in the lair: taken, raised there, released
sim/ages/rules/threads.cpp      17.8 closes threads whose arc ended
sim/ages/rules/chronicle.cpp    compact this decade's repeated events, score importance
sim/runner/main.cpp             CLI, stats.json
sim/tests/check.h               CHECK macro
sim/tests/fixtures.h            shared fixtures (sample act log, oath builder)
sim/tests/stats.h               per-seed stat collection for the slow tests
sim/tests/<name>.cpp            determinism, baseline, reactions, oath_breach_sleep, policies, golden
tests/golden/*.hash         golden chronicle hashes (ages_v0: seed 42 sample log idle; ages_v0_1_raze: seed 42 raze_all); updating them is a deliberate commit
tools/ages_stats.py             runs the binary over a seed range or reads existing out dirs, prints the v0.1 band table
tools/interest.py               17.6 interest metrics per policy (causal depth, outcome entropy, ...) and narrated chronicles
```

## v0.1: what changed in behaviour

v0 histories were monotone: humans churned, dwarves and elves died to the first lost war, the last race standing was the slowest to decay. v0.1 (doc section 15) adds, all as data plus new rule files: terrain and race temperament (dwarves and elves stay in their hills and woods, humans spread), vassalage instead of annihilation (a faction losing its last region swears fealty, pays tribute, and breaks free when the overlord weakens; orcs still sack), population mix with refugees and conquest blending (a human kingdom that eats the woods slowly becomes part elf), remnants that re-found a vanished race on open home terrain (or when the dragon sponsors them), four regime traits that drift and feed war chance, diffusion, cohesion and faith, coalitions against any hegemon and alliances among like-minded neighbours, the fading of the elves under human dominance, dragon hunger with a forecast of the coming sleep, and two rare events (great rulers, succession crises). Wasteland now forms under repeated razing, volcanic winter is a 10 % chance per mega raze instead of a certainty, and the former hard-coded numbers (kill soldier factor, give floor, offer expiry and tribute rates) live in `constants.json`; "losing" has one definition in `world_util.h`.

Measured baseline (200 seeds x 3000 y, idle, after the review changes below): human median lifetime 340, dwarf 540, elf 1325; every race alive or a remnant in 98 %, dwarves alive 98 %, elves 100 %, no race above 70 % of regions in 90 %, humans the largest race in 85 %, a hegemony rose and fell in 60 %, humans at era 2 by 1500 in 100 %, valley never empty. Policies (100 seeds x 2000 y): raze_all ends with orc share 0.48 vs 0.02 idle, wealth 3 vs 672, and wakes the dragon by hunger in 94 % of seeds; protect_one lifts aldmere's max realm (own plus vassal regions) to 4.17 vs 3.77 and reaches 60 % dominance then collapses in a handful of seeds. The golden hashes were re-recorded for the review changes (elf withdrawal pressure, lair hunt, refugees before the region transfer).

## Tuned constants

v0 baseline (200 seeds x 3000 y, idle) with the v0 values: human median lifetime 330, dwarf 670, elf 1220, humans in era 2 by 1500 in 100 % of seeds, valley never empty. All within the doc's bands.

| constant | doc | final | why |
|---|---|---|---|
| `power_cap` | 40 | 3 | At 40, power/power_cap stays near 0.1 for a 2-region kingdom and cohesion never decays below the collapse threshold: human median lifetime was 940 y, no wars, no collapses in 3000 years. 3 puts human lifetimes at 300-500 y and dwarves around 700. |
| `war_target_cohesion` | 0.5 | 0.6 | Successor states sit at cohesion 0.6 for a long time and were never attackable, so a protected kingdom could not expand. 0.6 makes fresh successors valid targets and gives `protect_one` a measurable effect. |
| `war_take_chance` | 0.3 | 0.2 (v0), 0.15 (v0.1) | Elves were being annihilated by human neighbours at median 1150-1180 y (band > 1200). Slower conquest keeps them around 1220-1250 across seed ranges. In v0.1, 0.2 still had a human faction above 70 % of regions in 40 % of seeds. |

Changed again for v0.1 (data only; the section 15 bands could not be met at the doc's values):

| constant | section 15 / v0 | v0.1 | why |
|---|---|---|---|
| `human.growth` | 0.030 | 0.020 | At 0.03 humans out-grew the two-region dwarf and elf starts so fast that one human faction held > 70 % of the valley in 40 % of seeds and humans were the largest race in 96-98 % (band 50-90 %). |
| `dwarf.growth` | 0.018 | 0.022 | Dwarf median lifetime fell toward 500 once humans slowed; 0.022 keeps hill factions viable and dwarves alive at 3000 in 99 %. |
| `deep_hold`, `elder_wood`, `deep_wood`, `old_wood` land | 0.5 / 0.6 / 0.5 / 0.5 | 0.6 each | With 0.5-land second regions the non-human starts were poorer than one human vale; equal 0.6 land is what makes "two regions each" an actual balance. |
| `dug_in_invasion_mult` | 0.5 | 0.25 | Dwarves losing a war were conquered before `dug_in` mattered; 0.25 makes a hill hold a real siege. |
| `elf_withdraw_ratio` | 2.0 (doc) / 4.0 (v0) | 1.0 | Withdrawal is no longer a hard power-ratio switch (it fired in every seed by year 200: elder_wood at ~190, deep_wood by 400 in a quarter of seeds). It is now a per-decade chance = the neighbour's war chance against the elves (war_chance x militarism/temperament) x hostility (affinity below `affinity_alliance`) x how far the ratio sits past this value, and 1 when actually at war. With the ratio at 1.0 any stronger hostile human is a slow pressure; a friendly one none. Elves hold both woods for 1000 years in 29-34 % of seeds (was 22-29 %), first withdrawal median 250-390 y (was 190). |
| `elf.cohesion_decay` | 0.0015 | 0.0012 | Elves that keep their woods grow into 3-region realms, which decay and collapse into one-region successors, pulling the lifetime median (founders plus successors) under 1200 at 0.0015; 0.0012 keeps it at 1240-1620 across seed ranges. |
| `coalition_affinity` | 0.3 | 0.6 | At 0.3 almost no neighbour ever joined a coalition (affinity between human successor states sits at 0.3-0.5), so hegemonies ended only by collapse. 0.6 gives the "no race > 70 %" band its 0.92. |
| `act_defend_attacker_loss` | 0.15 | 0.3 | With vassalage, the protected faction's attackers simply re-declared the next decade; a defend act that costs the attacker 30 % of its wealth is what makes `protect_one` measurable (realm 4.30 vs 4.19 idle). |
| `fading_human_share`, `fading_end_share` | 0.6 / 0.4 | 0.7 / 0.55 | Measured as the human share of total faction wealth (the doc's words), 0.6/0.4 fades the elves in almost every seed with no way back (elf median lifetime 3000: they thin out but never collapse, so the valley stagnates); 0.7/0.55 makes it a mid-history event (42 % of seeds, median start year 960, a few endings). |
| `hunger_need` | 0.4 | 0.6 | At 0.4 a dragon sleeping 100 years on a raze_all hoard never went hungry; 0.6 per decade makes an empty hoard wake the dragon after ~300 y and a raze_all hoard (gold only, 1000-y half-life) still starve it. |
| `shocks.volcanic_winter.chance` | (always) | 0.1 | Item 5 of section 14: a mega raze now has a 10 % chance of a volcanic winter. |

Added constants not in the doc (needed to make the rules close):

| constant | value | why |
|---|---|---|
| `land_wealth_scale` | 100 | `growth * land` per decade is 0.03 * 1.4 = 0.04 wealth, which never reaches era threshold 60. Land is scaled to 100 wealth-units per full region. |
| `wealth_upkeep` | 0.01 | Without a sink, wealth grows without bound and power never stabilises. 1 % per decade gives a steady state of growth/upkeep. |
| `abandon_burn` | 0.8 | A region burned above this is abandoned by its owner (if they hold others) so that burned land becomes ownerless and orcs can spawn on it, as the doc's orc rule assumes. Abandonment is not an invasion: defend clauses look at `last_invaded`, set only when another faction takes the region. |
| `threat_scale` | 20 | `threat = (1 + size*k_size + magic*k_magic) * threat_scale`. The doc's threat of 1 against bronze-age powers of 4-10 put every faction in the "strong" reaction row and had an era-1 kingdom slay the dragon by y1230 under raze_all. At 20 a fresh dragon has threat 20; the strong row (power >= 10) is reached by a faction with ~200 wealth at era 1 or any era 3 kingdom. |
| `cohesion_decay_floor` | 0.25 | Decay term is `cohesion_decay * max(power/power_cap, floor)`, so a razed faction with power near 0 still ages and does not become immortal. |
| `settle_chance`, `settle_max_burn`, `settle_min_cohesion` | 0.1, 0.5, 0.5 | Cohesive, peaceful factions settle adjacent ownerless, not-badly-burned land. Without it the two border scars and withdrawn elf woods stay empty forever and no faction can ever reach 60 % of regions. |
| `hoard_per_unit.hunt` | 0.08 | Nourishment per decade from game around the lair: `hunt x sum(eff_land x (1 - burn))` over the lair and its neighbours (about 0.2 on the green starting map against a need of 0.6). Without it the sim starts with an empty hoard and every idle dragon, whatever the seed, hits hunger 0.9 at exactly year 300. Now an idle dragon lasts 550-610 years and a raze_all one (the land around the lair burned) 990-1990, still waking by hunger in 94 % of policy seeds. |
| `orc_spawn_*`, `settle_*`, `succession_jitter`, `tribute_rate`, `war_siege_anti_dragon`, `reaction_*`, `losing_window_decades` | see file | Numbers the doc implies but does not give. |

## v0.4 S1: the dragon

Growth, talents, health and wounds, defences, the slayer clock and the oath fix (`docs/core-loop.md` sections 1, 3 and 4; contract in `docs/v04-plan.md`). Data: `data/talents.json`, `data/defences.json`, the `dragon` section of `constants.json`.

```
sim/ages/growth.h/.cpp          growth points per axis, size from Might, the egg's hoard (dragon.start_hoard)
sim/ages/hoard.cpp              digestion on the phase clock (half-life = share x long window), the gold bed (bed_mode consumed: gold above the phase floor is eaten and gives Might), nourishment
sim/ages/talents.h/.cpp         effects and look merged from talents and ranks, ranks (geometric cost, milestones), picks
sim/ages/defences.h/.cpp        a settled region's defences derived from its court, the next anti-dragon unlock
sim/ages/dragon_acts.h/.cpp     defences strike harmful acts, the wound act, wards, embers, spread, linger, molten gold
sim/ages/wounds.h/.cpp          health, wounds, death by defences, healing per sleep
sim/ages/rules/slayers.h/.cpp   the slayer clock: forming, ready (a waking later), attempt (next sleep), premonition, adaptation
sim/ages/rules/anti_dragon.cpp  dread research below era 2, anti_dragon_progress at 25/50/75/95 % to the next defence
sim/ages/dragon_json.h/.cpp     world view and save fields, the talent catalogue
sim/ages/dragon_data.cpp        loaders for the three data sources
```

Rules beyond the contract text: courts below `anti_dragon_min_era` that fear the dragon and remember its fire research at `dread_research` x fear x burn memory per decade (without it raze_all terrifies the valley into a harmless ruin and never dies); slayer bands are `fearless`; the slaying odds set the band's strength ((2 + 0.1 sqrt(hoard) + 0.02 growth) x era multiplier x adaptation) against health x `health_div`; the gold bed eats `gold_burn` (0.15) of the gold above `gold_floor_base` x `floor_phase_mult[phase]` per long window slept (10 x 1, 2, 4, 8, 16) and pays `gold_pts` (1.0) Might per gold eaten (`bed_mode window` keeps the old `bed_rate` x sqrt(gold) bed); both run at `bed_overtime` (0.1) past a sleep's first long window and double in a moult; the egg lies on 40 gold.

Changed from the plan's start numbers: health per Might 3 -> 1 (raids never reached a third of health), points per book/artifact/art 0.12/0.15/0.12 -> 0.3/0.4/0.3 and per ember 2.5 -> 3 (Lore and Arcana never reached a node), food 0 -> 0.05 Might, food half-life 0.1 -> 1.0 window (a stockpile fed only the first decades of a moult), bed rate 0.4 -> 1.0 (picks on 3 of the first 10 wakings), levies era scale 1, fear damping 0.6 -> 0.5. The `played` policy takes a mixed payload, burns its target, picks talents by a preference list and sleeps short, short, long (at least 10 years).

Measured (300 seeds x 3000 years, default world): played dies in 16.7 % of runs (all slayings, each named in the briefing before the fatal sleep; median year 1640), raze_all in 99 % (232 to the defences it provoked, 65 slayings; median 1660), idle never; v0.3 had 3.0 %, 0 % and 0 %. The played dragon has a talent pick on 6 of its first 10 wakings and after its first moult, moults into phase 2 by year 50 (within 5 long sleeps), phase 3 by year 510 (median; v0.3 reached phase 2 in 13 % of runs, never phase 3) and phase 4 in 90 %, and still gains a rank or talent on 38 % of its late wakings. Test 2 bands are unchanged.
## v0.4 world (S2): genesis, pre-history, irreversible events, storyteller

`reset {seed, genesis: true}` / `load {..., genesis: true}` / `ages --genesis` build a generated world from `data/genesis.json`; the default fixed start keeps the v0.3 behaviour for the tests and goldens, and the S2 world rules and cards run only in generated worlds (world tag `generated`). Years stay absolute: the pre-history runs from year 0 and the egg hatches at `genesis.hatched_year`; pre-history events carry the tag `prehistory`. Command `legends` returns `{ok, events: [{year, text, importance}]}`; the world view gains `genesis {template, prehistory_years, hatched_year, deal}` and region `name`, `empty_years`.

```
data/genesis.json               terrain deck and names, nine start templates, pre-history length, legends, extinction, undying, magic, storyteller
data/events/world_*.json        arrivals, cataclysms, the undying, magic, undead, extinction, the storyteller's deck (generated worlds only)
sim/ages/genesis_data.h/.cpp    genesis.json loader
sim/ages/genesis_map.cpp        the terrain deal (deck, connected runs, lair rules) and region names
sim/ages/genesis_start.cpp      start templates: courts, ruins of a fallen kingdom, wars, fealties, alliances, marks, remnants
sim/ages/genesis_world.cpp      make_genesis: the fresh world, the pre-history while the egg lies in the mountain, the hatching
sim/ages/legends.cpp            the pre-history's highlights for the first screen
sim/ages/genesis_json.h/.cpp    the S2 fields of the world JSON
sim/ages/rules/world.cpp        rule_world (after the cards): empty land and razed regions, then the rules below
sim/ages/rules/magic.cpp        world.magic during and after an age of magic
sim/ages/rules/undying.cpp      undying rulers: cohesion floor, overthrow
sim/ages/rules/extinction.cpp   a people without a court assimilates and dies out (world tag extinct:<race>)
sim/ages/rules/storyteller.cpp  the waking cycle, deck draws, the wake guarantee, the major-card multiplier
sim/ages/rules/card_world.cpp   card effects arrive, spawn_undead, terrain, die_out, undying
```

## v0.4 hooks (S3): what waits for the dragon when it wakes

Every sleep ends (after the storyteller and the offers) with one to three hooks drawn from `data/hooks.json`, the situations of this age (`docs/core-loop.md` section 5). A template's `when` is a card trigger (the language of `cards.h`, plus `this_sleep` on event conditions) evaluated per court or region; `force_if` makes it certain (Premonition's slayers, the Cult's pilgrims), `priority` weights the draw, a kind offered on the previous waking draws at `repeat_mult`, `cooldown_years` keeps a court or person from repeating, and a `fallback` (the rich caravan) is drawn only when nothing else calls. `second` and `third` are the chances of more hooks. Rewards and ignore effects are flat key maps (hoard kinds, `gold_share` of the court's wealth, `growth_<axis>`, faith pool, fear and faith on the court, its neighbours or the other side; ignore: wealth, cohesion, anti-dragon, faith, the lair's slopes, gold to the court); a `fight` wounds the dragon through the wound path for `health_share` of its health after armor.

An act of the raid that matches a hook (its act at its region on its court; `kill_person` is a kill naming the person; `defend` protects the other side against the court) resolves it in `apply_acts`: the fight first, then the reward and `hook_resolved` (cause params: the growth points paid, the fight's damage). Pilgrims are answered with `answer_hook {id, choice}` (receive | eat). Hooks still open when the next sleep starts apply their ignore effect and emit `hook_ignored`. Killing slayers at the gate disperses the band (`disperse_slayers`). Star-reading or Dragon-sight (a sharper forecast) adds `hook_preview {kind, title, region}` to `forecast`.

API: the world view and the save carry `hooks: [{id, kind, source, title, text, region, faction, person, other, resolve: {acts, region, target, person, against, choices}, reward_text, ignore_text, fight, fight_damage, offered, expires_year}]` (open hooks only; `expires_year` is the waking's year, since the next sleep starts then); `sleep` returns `hooks`; `apply_acts` returns `hooks_resolved: [{id, kind, reward_text}]`; `answer_hook {id, choice}` returns `{ok, events, hooks_resolved, wounds, slain}`. The played policy (`answer_hooks` in `data/policies/played.json`) answers one hook per raid, the most pressing first, while the wounds after the defences' strike and the fight stay under half its health. `stats.json` gains `hooks` (wakings, with_hook, offered, answered, by_kind, fights, reward_growth); `tools/interest.py` prints coverage, hooks per waking, kinds, the answered share and the growth paid by hooks.

```
data/hooks.json                 templates, draw numbers
sim/ages/hooks.h, hooks.cpp     generation (candidates, forced, weighted draw, fallback), expiry
sim/ages/hook_resolve.cpp       matching acts, fights, rewards, answer_hook, ignore effects
sim/ages/hook_policy.cpp        a policy answering hooks
sim/ages/hook_data.cpp          hooks.json loader; card_preds.h exposes the trigger parser
sim/ages/hook_json.h/.cpp       hooks in the save and the view, the forecast preview
```

Measured (300 seeds x 3000 years, `tools/interest.py`): on the default world idle gets 2.00 hooks per waking on 100 % of wakings (11 kinds, herd largest at 25 %), played 1.66 on 99.4 % (15 kinds, library largest at 16 %) and answers 63 %; raze_all's burns and takes answer 36 % in passing. Played dies in 7.7 % of runs (23 slayings, all warned; 13.0 % without hooks on the same tree, since slayers met at the gate disperse), raze_all in 99 %, idle never. Played's fights cost 48 wounds on average (a fifth of health after armor). Hooks pay about half of played's Lore, Arcana and Dominion: 43, 29 and 22 points per run against 10, 6 and 5 without hooks. On generated worlds (`--genesis`): idle 2.05 per waking on 100 %, played 1.78 on 98.9 %, 17 kinds, answered 62 %, played deaths 21.3 % (25.7 % without hooks).

## Item 06: access gating for visitors

Nobody comes to the lair at random. `data/arrivals.json` lists eight kinds (envoy, pilgrim, thief, slayer band, scholar, kin, refugee, prophet) with `requires`, `blocked_when`, `cooldown_years`, `specificity` and a `person_role`. `rule_arrivals` runs after oaths: per court per tick the most specific kind whose gates hold, none blocked, off cooldown, comes. One visitor per court per decade (the slayer band is exempt), and a kind already waiting for that court does not come twice. An arrival carries `because`, the gate keys that let it through. Visitors wait at the lair until the dragon sleeps; `arrivals_depart` sends them home, and whoever walked home gives the court exact knowledge of the lair. A killed envoy blocks the court's envoys for `envoy_kill_generations` x 30 years.

Lair knowledge is `Faction.lair_known` (year learned, -1 never) and `lair_exact`. A witnessed act within `sight_regions` of the lair, a show at a capital or a returned hostage teaches it; it fades with the oral tier when no written record keeps it. `route_open` is false when an orc court or wasteland sits on every path.

Verbs: `receive {id, outcome}` (receive | eat | turn_away; the outcomes of a kind are its `answers`) and `hunt {region}` (food from the lair slopes or unowned land). The thief, slayers and pilgrims hooks carry an `arrival` key and are generated from the arrival. The briefing has a `ledge` block: one line per waiting visitor with its because-line.

```
data/arrivals.json              kinds, gates, answers
sim/ages/arrival_data.cpp       loader
sim/ages/arrival_gates.cpp      gate values, lair knowledge, route_open
sim/ages/arrivals.cpp           arrival bookkeeping, ledge text
sim/ages/arrival_acts.cpp       receive and hunt
sim/ages/rules/arrivals.cpp     the rule
```
