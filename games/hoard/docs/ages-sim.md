# Hoard – the ages simulation

Status 2026-10-14. Design of the between-sleeps simulation ("ages sim"): the part of the core that turns what the dragon did while awake into the world it wakes up to. Companion to `hoard-brief.md`, `production-plan.md` (section 2, the C++ core) and `fire-mechanic.md`. Everything here is engine-agnostic and lives in `sim/ages/`.

## 1. Premise

You are a dragon. You fly out, burn, take, bargain, then sleep in your lair for a century or longer. While you sleep the mortal factions of the valley grow, fight, collapse, remember you or forget you. You wake into the result, changed by what you slept on.

There is no fixed ending. The dragon lives until killed or until one of a few exits happens (ascension, a portal opened by a cult, a deliberate sleep of thousands of years that resets the valley). Growth continues along several axes. The game's question each age is what to take, what to spare, what to promise, and how long to sleep.

The sleep-length dilemma is the central tension and must fall out of the numbers, never a rule: dragon growth per sleep is sublinear in years (hoard digestion with half-lives), mortal advance is roughly linear in years. Short sleeps keep the valley legible and the dragon small. Long sleeps grow the dragon and hand the mortals a thousand years to outgrow it.

The second tension: the dragon profits from civilisation. Bronze-age villages have cattle and rings; books, artifacts, art and royals exist only after the mortals have had time to make them. Every raid is harvest versus farm.

## 2. Design rules

1. The sim never reads player intent. The awake phase emits witnessed acts derived from a world-state diff. The sim derives meaning (betrayal, gratitude, fear, resolve) from act × the faction's current relationship to the dragon.
2. Two inputs only: the act log (what the mortals witnessed) and the hoard (what the dragon brought home). Everything the dragon does resolves into an act, a modifier or a spawn. If a future mechanic cannot be expressed in those three, add one act type and its reaction row deliberately; never special-case the sim.
3. The act vocabulary is closed and small. New fire or magic mechanics change magnitudes, not the vocabulary.
4. State is plain data, serialisable, copyable. Rules are pure functions over state in a fixed order. Deterministic from (seed, data, inputs) on every platform.
5. Race is fixed; culture is mutable. A faction's behaviour comes from its culture, which starts as a copy of the race template and can mutate (corruption, hardening, dragon-raised dynasties). Race stays for identity, loot profile and lifespan.
6. Tags on factions and regions are the universal extension hook. Event cards read tags and numbers. New mechanic = new rule file + tag, never an edit to the base rules.
7. The chronicle (every event with a year) is the wake-up briefing, the ending text, and the debug output.
8. Tune with a headless runner over thousands of seeds, against target bands, before any content.

## 3. State

### World

```
World
  year            int (decades are the step; year advances by 10)
  seed            uint64
  rng             state (splitmix64 / PCG32, own implementation, no std distributions)
  magic           float 0..1      world magic level (v1)
  regions[]       Region
  factions[]      Faction
  dragon          Dragon
  oaths[]         Oath
  modifiers[]     Modifier
  acts[]          Act             the log, appended by the awake phase
  chronicle[]     Event
  tags            set<string>     world-level tags (e.g. "volcanic_winter")
```

### Region (8 to 12; the valley plus neighbours; off-map world is abstract)

```
Region
  id              string
  neighbours[]    region ids
  owner           faction id or none
  land            float 0..1      base productivity
  burn            float 0..1      current burn level, recovers over centuries
  lair            bool            the dragon's region
  tags            set<string>     "wasteland", "lava", "lake", "warded", ...
```

### Race (fixed, 4 to start)

| race  | lifespan | growth | cohesion_decay | era_rate | memory_halflife | loot profile          | alliance gives            |
|-------|----------|--------|----------------|----------|-----------------|-----------------------|---------------------------|
| human | 60       | 0.030  | 0.012          | 1.0      | 150 y           | gold, royals          | tribute, worship          |
| dwarf | 250      | 0.018  | 0.004          | 1.1      | 500 y           | gold, artifacts, siege| they dig the lair         |
| elf   | 800      | 0.008  | 0.0015         | 0.4      | never           | books, art            | they teach magic          |
| orc   | 40       | 0.0    | 0.05           | 0.2      | 50 y            | captives, scrap       | an army                   |

growth = wealth growth per decade as a fraction of land value. cohesion_decay = loss per decade at power/cap = 1. era_rate = multiplier on era advance. Orcs have no growth of their own; they gain wealth only by plunder (rule 5). Orc cohesion collapses on warlord death (ruler lifespan class "warlord").

### Culture (per faction, mutable copy of the race template)

Same fields as Race plus `tags`. Mutations are one-way per faction and are applied by rule files reading tags: `corrupted` (dark elves, chaos dwarves), `hardened` (generations on wasteland), `dragon_raised` (dynasty from a hostage), `crusader`, `undead`. v1 feature; v0 copies the race template verbatim.

### Faction (a polity; a race can have several)

```
Faction
  id              string
  race            race id
  culture         Culture
  regions[]       region ids
  wealth          float >= 0
  cohesion        float 0..1
  era             int 0..4       bronze, iron, castles, gunpowder, arcane
  fear            float 0..1
  faith           float 0..1
  creed           vector (v1): destroyer, protector, gold, knowledge
  memory[]        MemoryEntry    { year, act type, magnitude, tags } with decay by memory_halflife
  ruler           { class: mortal | longlived | immortal | warlord, reigned: years }
  at_war_with[]   faction ids
  tags            set<string>
  anti_dragon     float 0..1     progress of the anti-dragon line (ballistae, wards, slayer orders)
```

Derived (never stored): `power = sqrt(wealth) * era_mult[era] * cohesion`, era_mult = [1.0, 1.3, 1.7, 2.3, 3.0].

### Dragon

```
Dragon
  size            float     grows from gold
  knowledge       float     grows from books; gates speech verbs and foresight
  magic           float     grows from artifacts; gates spells
  faith_pool      float     accumulated from worship; spent on miracles
  hoard           { gold, books, artifacts, art, royals: float }   undigested
  lair            { depth, library, sky: float }                  v1
  tags            "rage", "wounded", ...
  threat          float     derived: f(size, magic) on the same scale as faction power
```

### Oath

```
Oath
  id, parties[2], sworn (year), expires (year)
  clauses[]  { who, pred, params }   pred in: tribute, no_harm, defend, no_walls, hostage, no_settle
  stakes     { breach_by_dragon: effect id, breach_by_them: effect id }
  status     active | breached_by_dragon | breached_by_them | expired
```

### Modifier (timed delta on a variable, with a source)

```
Modifier
  source     string     "dragon.curse", "shock.plague", "culture.hardened"
  target     faction id or region id
  var        string     "cohesion_decay", "land", "invasion_chance", "era_rate", ...
  op         add | mul
  value      float
  from, until  year
```

### Act (input from the awake phase)

```
Act
  year       int
  type       burn | kill | take | defend | show | accept_offering | destroy_altar |
             take_hostage | release | give | miracle | spell
  target     faction id
  region     region id
  magnitude  float 0..1    relative to the victim (fraction of wealth destroyed, of army killed, of wealth taken)
  against    faction id    for defend: the attacker
  witnesses[] faction ids
  payload    map           loot categories for take, item id for give, spell id for spell
```

### Event (chronicle)

```
Event { year, kind, subject, object, magnitude, text_key, tags }
```

## 4. The decade step, in order

Each step is one file under `sim/ages/rules/`. Order is fixed; each rule reads the previous rule's output.

1. **modifiers** – expire old modifiers; compute effective per-faction and per-region parameters (base from culture, then add/mul modifiers).
2. **wealth** – `wealth += growth * sum(land * (1 - burn)) over owned regions * (1 - war_penalty)`. war_penalty 0.4 while at war. Burned wealth is handled at act time, not here.
3. **cohesion** – `cohesion -= cohesion_decay * (power / power_cap)`. power_cap is a world constant (initially 40). Ruler: mortal resets +0.15 on succession (every lifespan years with jitter), longlived +0.05, immortal: decay × 0.1 and era_rate × 0.5, warlord: on death (every ~40 y) cohesion drops to 0.2.
4. **era** – accumulate `progress += era_rate * wealth_growth_this_step`; advance when progress crosses `era_threshold[era]` = [60, 150, 350, 800]. Neighbour diffusion: a faction adjacent (shared border) to one 2+ eras ahead gains `progress += diffusion` per decade (initially 4). War adds siege progress to anti_dragon too. Anti-dragon line: if `fear > 0.5 and era >= 2`, `anti_dragon += 0.02 * fear * era_mult` per decade; at `anti_dragon >= 1` the faction gains tag `slayers` and the slay-attempt rule (8) is enabled.
5. **war** – for each adjacent pair not at war: if `power_a / power_b > 1.5 and cohesion_b < 0.5`, roll `war_chance` (0.15/decade) to start war. Each decade at war: the stronger side takes one region with probability `0.3 * ratio`, the loser's wealth drops by 10%, both pay war_penalty. War ends when the loser has lost 2 regions, or after 5 decades, or by collapse. Orcs: `at_war` with any neighbour whose cohesion < 0.4; orc plunder = 15% of the victim's wealth per region taken, converted to orc wealth.
6. **collapse** – `cohesion < 0.2` → the faction splits: each owned region becomes (or joins) a successor faction of the same race, wealth divided, cohesion 0.6, era inherited, fear/faith/memory copied, tag `successor_of:<id>`. Empires therefore die of their own size.
7. **memory** – decay each memory entry's weight by `0.5^(10 / memory_halflife)`; drop below 0.02. Recompute `fear` and `faith` as sums over weighted entries, clamped to 0..1, plus the creed vector (v1). Entries tagged `betrayal` decay at half speed and block faith regrowth while weight > 0.1.
8. **dragon_rolls** – 
   - tribute: if `faith > 0.5` and an oath with a tribute clause is active (or faith > 0.8 without one), tribute accrues into `dragon.hoard.gold` (and faith_pool += faith * 0.1).
   - slay attempt: if tag `slayers` and `fear > 0.6`: per-decade chance `0.05 * (faction power / dragon.threat)`. Resolution: `success_chance = clamp(power / (dragon.threat * 3))`; on success the dragon is killed (world tag `dragon_slain`, sim stops); on failure the attempt wakes the dragon (see section 7), the faction loses 30% wealth, fear += 0.2.
   - cult progress (v1): `faith > 0.8 and world.magic > 0.6` → `portal_progress += 0.05`.
9. **migration** – elves: if any human neighbour's power > 2× theirs, abandon the border region (owner none, tag `elf_withdrawn`). Dwarves: if at war and losing, region tag `dug_in` (invasion_chance × 0.5 there). Orcs: a new orc faction spawns on an ownerless region with `burn > 0.5` (or a `wasteland` tag) adjacent to a weak faction, chance 0.1/decade. Razing feeds the horde.
10. **land** – `burn -= 0.02` per decade (0.005 if tag `wasteland`); a region gets `wasteland` when burn stays > 0.8 for 10 decades and loses it after 30 decades below 0.2.
11. **oaths** – evaluate every clause against state and the acts of the last decade; breach emits an event, applies stakes, sets status. Expiry emits an event. Clause predicates:
    - tribute: faction wealth >= amount and faction faith > 0.2, else breach_by_them (they stop paying when they stop believing)
    - no_walls / no_settle: region owner's era < 2 or tag `walls` absent; mortals breach when memory of the dragon fades (fear < 0.2)
    - no_harm: no burn/kill/take act by the dragon against the party in the window
    - defend: if the party was invaded (lost a region) in the window and no `defend` act against the attacker is in the log, breach_by_dragon. This is how sleeping through an invasion breaks the oath.
    - hostage: the lineage tag `dragon_raised` still exists in the dragon's lair record
12. **shocks** – low-rate exogenous events, independent of the dragon: plague (wealth −30%, cohesion −0.1, chance 0.01/decade/faction), bad century (land × 0.7 for 10 decades, 0.005/decade/region), volcanic winter (world-wide, triggered only by a `mega_raze` act, land × 0.5 for 5 decades).
13. **chronicle** – every rule appends events; this step compacts repeated events of the same kind into one with a count.

Empire lifetime should fall out of rules 3 and 6 times ruler lifespan: human 200 to 500 years, dwarf ~800, elf thousands, orc one warlord's life. The immortal ruler is stable, stagnant, and dies to a single assassination or crusade event (v1 card).

## 5. Applying the act log (at sleep start)

Acts are applied once, before the first decade step, in log order. Each act: immediate deltas, then a memory entry on the target and on each witness, then reactions.

Immediate deltas: `burn` → target wealth −= magnitude × wealth, region burn += magnitude; `kill` → cohesion −0.05 × magnitude (civilians) or power loss via wealth (soldiers); `take` → wealth −= magnitude × wealth, hoard += payload; `take_hostage` → tag on faction `hostage_taken`, dragon lair record gains a lineage; `defend` → attacker loses an army (wealth −15%), war ends if the attacker has no regions in the party's territory; `give` → target wealth or era progress up; `destroy_altar` → faith −0.3; `accept_offering` → faith +0.1, faith_pool up.

### Reactions: act × relationship

The same `burn` on the target means different things depending on the relationship. These rows are data (`data/reactions.json`), initial values:

| act | relationship of target to dragon | fear Δ | faith Δ | memory tags |
|---|---|---|---|---|
| burn/kill/take | oath active (no_harm or defend) | +0.3·m | → 0.02 | betrayal, harm |
| burn/kill/take | faith > 0.5, no oath | +0.3·m | −0.6·m | harm |
| burn/kill/take | faith ≤ 0.5, power < 0.5·threat | +0.5·m | −0.2·m | harm |
| burn/kill/take | faith ≤ 0.5, power ≥ 0.5·threat | +0.2·m, anti_dragon +0.1·m | −0.2·m | harm, resolve |
| defend | at war and losing (lost a region in the last 2 decades) | −0.1 | +0.5·m | saved |
| defend | at war, not losing | 0 | +0.15·m | helped |
| show | any | +0.15 | 0 | seen |
| give | any | 0 | +0.2·m | gift |
| miracle (protector) | any | 0 | +0.3 | miracle |
| miracle (terror) | any | +0.4 | +0.1 | terror |
| witnessed burn of a neighbour | neighbour was enemy | +0.1·m | +0.1·m | spillover |
| witnessed burn of a neighbour | neighbour was friend or same race | +0.2·m | −0.1·m | spillover |

m = magnitude. Betrayal blocks faith regrowth while its memory weight > 0.1 (rule 7).

## 6. Hoard digestion (dragon growth during sleep)

Each hoard category digests with its own half-life; the digested fraction converts into a growth axis. Growth is the integral over the sleep, so the long sleep gives the bulk and a 100-year sleep a slice.

| category | half-life | converts to | per unit |
|---|---|---|---|
| books | 50 y | knowledge | 1.0 |
| art | 100 y | faith_pool (awe) | 0.5 |
| artifacts | 300 y | magic | 1.0 |
| royals | 100 y | lineage tag `dragon_raised` on a faction of that race; knowledge +0.5 | – |
| gold | 1000 y | size | 0.001 |

`threat = 1 + size * k_size + magic * k_magic` (initially k_size 0.5, k_magic 2.0), on the same scale as faction power. Premature waking (section 7) cuts the sleep short, so digestion stops there.

## 7. Sleep and waking

`sleep(world, years)` runs `years / 10` steps. Wake conditions checked after every step:

- planned: years elapsed
- forced: a slay attempt failed (dragon wakes with tag `rage`: next raid acts get magnitude × 1.3 on burn and the reaction table treats `show` as `burn` 0.2 for that age), or a hoard raid (v1: lure)
- v1 "dream" threshold events: army at the lair's region, oath breached, cult schism. v0 only logs them; v1 lets the shell ask "wake now or sleep on".

Sleep returns the world and the chronicle slice for that sleep. The awake phase then builds the raid map from the world (region owners, war flags, armies present, altars, hostages) and after the raid diffs it to produce the next act log. The diff runs per raid, so magnitudes are relative to what a faction had then.

## 8. What the dragon can do (verbs)

Verbs are generic; situations come from the sim (a drought is a shock, `bless` is an answer the player happens to have). Each verb is gated by a growth axis and costs something. Every verb resolves into an act, a modifier or a spawn.

| verb | gate | cost | resolves to |
|---|---|---|---|
| burn, kill, take, show, defend | none | raid stamina | act |
| take_hostage, release, give | none | – | act |
| terraform (wasteland, lava, lake) | size | a sleep's growth share | region tag + modifier on land |
| demand, threaten, swear oath, teach | knowledge | – | act or Oath; teach = era progress modifier. Implemented 2026-10-15: `demand` (tribute / no_walls / no_settle) swears an Oath when the court is cowed (fear ≥ 0.3 or power < half the threat), else `demand_refused` and resolve +0.05; `teach` is the spell `era_rate × 2` for 10 decades, gate knowledge 1. threaten and swear oath stay v1 |
| bless region, turn plague | faith_pool, protector creed (v1) | faith_pool | modifier land +0.3 × 5 decades |
| rout army, blight | faith_pool, terror creed (v1) | faith_pool | act (miracle terror) + modifier |
| ward region | magic | magic | modifier invasion_chance × 0.2 |
| curse lineage | magic | magic | modifier cohesion_decay × 2 for 10 decades |
| corrupt | magic, world.magic (v1) | magic | culture mutation tag |
| raise undead host | magic | magic | spawn: faction race `undead`, power from magic, war target given |
| incite | knowledge or magic | – | modifier on an orc faction's war target |
| sponsor hero | knowledge | – | spawn: hero entity (v1) |
| dig, raise mountain, build library | size / magic / knowledge | a sleep's growth | lair fields; unlock verbs |

Spells cast before sleeping are modifiers that tick while the dragon is under.

## 9. Offers and envoys

The sim generates offers from faction state at sleep end (so the player meets them at the lair on waking) and mid-raid (v1):

- losing a war → offers tribute for a `defend` clause
- faith > 0.7 → offers an oath with tribute and no_harm; a cult (v1) offers sacrifice for terror miracles
- a rival with power > neighbour's and cohesion < 0.5 → offers to pay for the dragon burning that neighbour
- era 4 and faith > 0.8 → offers the portal / ascension rite (exit)

An offer is an Oath draft plus a text_key. Accepting is a physical act in the lair (the envoy is not eaten). Declining is an act `show` on that faction at magnitude 0.1.

## 10. Extensions (v1, after the base is tuned)

- **Creed**: faith gets a direction (destroyer, protector, gold, knowledge) accumulated from witnessed act types. Miracles available depend on the dominant creed. Terror cults convert population to faith fast (sacrifice) at a cohesion cost and attract crusades from neighbours.
- **World magic level**: raised by elves practising, dragon spells, artifacts in play; suppressed by wasteland and gunpowder. Opens a magic tech line for mortals (archmages, wards, binding) and gates portal/ascension. Worlds diverge between gunpowder-industrial and high-magic.
- **Culture mutation**: corrupt, hardened, dragon_raised, crusader as template rewrites.
- **Artifacts as objects**: named items with owner and location; stolen, traded, used against the dragon. The late-game threat from something the dragon made.
- **Lure**: hoard size raises thieves, slayer attempts and inter-faction wars over the valley.
- **Dreams**: the sleep pauses on threshold events and asks wake/sleep on; knowledge decides how early and how clearly.
- **Heroes and lineages**: named entities with long lives; unify factions, lead slayings, become immortal rulers.
- **Event cards**: `data/events/*.json` with prerequisites on tags and numbers, per era deck, used up and replaced.
- **Exits**: ascension, portal, the millennial sleep.

## 11. Architecture and repository

```
sim/
  ages/
    state.h            all structs above, plain data
    rng.h              splitmix64 + PCG32, float in [0,1), no std distributions
    json.h             vendored single-header JSON (nlohmann) or a minimal reader
    data.h/.cpp        load data/*.json into Races, Constants, Reactions, initial World
    acts.cpp           apply act log + reactions
    hoard.cpp          digestion
    oaths.cpp          clause predicates, offers
    rules/             one file per step in section 4, same order, each `void step_x(World&, const Params&)`
    step.cpp           the ordered decade step
    sleep.cpp          sleep(world, years) with wake conditions
    chronicle.h/.cpp   events, compaction, JSON out, stable hash
  runner/
    main.cpp           CLI, see below
  tests/
    ...                see section 12
data/
  constants.json       every number in this document
  races.json
  reactions.json
  regions.json         the valley map (10 regions)
  factions.json        starting factions (bronze age: 2 human, 1 dwarf, 1 elf, orcs off-map)
  shocks.json
  policies/            scripted dragon behaviours for tests (raze_all, protect_one, alternate, idle)
```

Runner CLI: `ages --data data/ --seed N --years Y [--acts log.json] [--policy name] [--sleep 100] --out out/`
Writes `chronicle.json`, `world_final.json`, `stats.json` (per faction: lifetime, max regions, eras reached; per world: era timeline, orc share, empty-valley flag). `--policy` applies a scripted act log at the start of each sleep of `--sleep` years, repeated until `--years`. A separate `ages-stats` (Python or C++) aggregates `stats.json` over a seed range into bands.

C++17, CMake, no engine headers, no exceptions across the API, compiles with MSVC, GCC, Clang and later Emscripten. Determinism: the only randomness is `world.rng`; iteration order over factions and regions is by id (sorted vectors, not maps keyed by pointer).

## 12. Tests (exactly these)

1. **determinism** – same seed, data and act log twice → identical chronicle hash; a different seed → a different hash.
2. **baseline** – 200 seeds × 3000 years, policy idle. Report, then assert with wide bands: median human faction lifetime in [150, 600] years; dwarf in [400, 1500]; elf > 1200; orc < 150; humans reach era 2 within 1500 years in ≥ 80% of seeds; the valley is not empty (every region ownerless) at year 3000 in ≥ 90% of seeds. Bands are the tuning targets; the test prints the measured values.
3. **reactions** – (a) burn 0.4 on a faction with an active no_harm oath → memory has `betrayal`, faith < 0.05, oath breached_by_dragon; (b) defend while losing → faith rises more than defend while not losing; (c) burn on a faction with power ≥ 0.5·threat → anti_dragon increased, fear rose less than for a weak faction.
4. **oath_breach_sleep** – oath with a defend clause; force an invasion during the sleep (neighbour power ratio set high); no defend act in the log → event `oath_breached_by_dragon` in the chronicle and stakes applied.
5. **policies** – 100 seeds × 2000 years, sleep 100: raze_all → orc region share at the end higher and total valley wealth lower than idle; protect_one → the protected faction's max region count higher than idle and it collapses at least once within 1000 years after reaching ≥ 60% of regions.
6. **golden** – one fixed seed, data and act log → chronicle hash stored in `tests/golden/ages_v0.hash`; the test fails on change. Updating the file is a deliberate commit.

Run: `ctest` after `cmake --build`. Test 2 and 5 are slow; mark them so `ctest -L fast` runs 1, 3, 4, 6.

## 13. Open questions

- Power cap, era thresholds and decay constants are first guesses; the baseline test is the tool to settle them.
- Whether `kill` of civilians should feed a separate `grievance` number or stay inside fear/faith with tags.
- Whether the off-map world should be one abstract "outside" faction that trades, invades and absorbs refugees, so the valley is never a closed system.
- Where the awake-phase diff lives: in the core (preferred, so every shell produces identical logs) or in the shell.

## 14. v0 implementation, 2026-10-14

Built in `hoard-sim/` (C++17, CMake, nlohmann/json vendored). Six tests pass on GCC and Clang with identical chronicle hashes; a 3000-year run takes about 10 ms; baseline 200 seeds × 3000 years in 1 s. Independent review done. `README.md` in the tree has the file list and the constants table.

Deviations from this document, all in data or documented in the README:

- `power_cap` 40 → 3, `war_target_cohesion` 0.5 → 0.6, `war_take_chance` 0.3 → 0.2. At the doc's values nothing ever collapsed or fought.
- Added `land_wealth_scale` 100 and `wealth_upkeep` 0.01: the doc's wealth, power and era-threshold scales were mutually inconsistent and wealth was unbounded.
- Added `threat_scale` 20: threat = (1 + size·k_size + magic·k_magic) · threat_scale, otherwise every bronze-age kingdom outranked the dragon and bred slayers.
- Added `cohesion_decay_floor` 0.25: a razed faction with power ≈ 0 never decayed and became immortal.
- Rule 9 gained two sub-rules the doc implied but never stated: burned land (burn ≥ 0.8) becomes ownerless, so orcs can spawn on it; a cohesive, peaceful faction settles adjacent ownerless land (0.1/decade). Without these raze_all never produced orcs and no faction could ever hold 60 % of the valley.
- Invasions are tracked separately from abandonment (`last_invaded`), so a dragon burn that empties an allied region does not breach the dragon's own defend clause.
- Policy targets follow a faction's successor lineage.

Measured baseline: human median lifetime 330 y, dwarf 665, elf 1220, humans at era 2 by year 1500 in 100 % of seeds, valley never empty. raze_all: orc region share 0.29, valley wealth 45 vs 506 idle, volcanic winter every sleep. protect_one: the protected lineage reaches 6 regions and collapses within 1000 years of dominance.

Known problems for v0.1, in order:

1. Dwarves go extinct in 5/5 seeds (years 450 to 740) and elves in 4/5, always by human annihilation of a one-region faction: with `war_end_regions` 2 a lost war is annihilation for anyone holding two regions or fewer. Candidates: a floor on region loss for the last region (vassalage instead of death), an off-map refuge that re-seeds a race (open question 3), or dwarves and elves starting with two regions.
2. The orc lifetime band is vacuous under idle because nothing burns. The baseline test needs a light burn policy, or orc stats should come from the policies test.
3. Wasteland never forms: burn decays below 0.8 in 7 decades, the 10-decade rule is unreachable. Either raise the burn an act leaves or lower the wasteland threshold.
4. The golden scenario (seed 42, sample log, idle) exercises none of the collapse, abandonment or policy paths; the hash survived every review fix. Add a raze_all golden alongside it.
5. Volcanic winter (burn sum ≥ 3 per act log) fires on every raze_all sleep; it should be rarer than that.
6. Hard-coded numbers to move into `constants.json`: soldier kill factor 0.5, wealth floor 10 in acts, offer expiry 500 and tribute rates in oaths. "Losing" is defined three different ways (acts, migration, oaths); unify.
7. `std::pow` and `%.6f` in the hash are a portability risk for bit-identical traces across libm implementations; use a fixed-point or integer representation in the canonical serialisation before the WASM build.

## 15. v0.1: a history worth watching

Goal of this increment, in the author's words: realistic for a fantasy world (elves do not end up with world domination, and if they do it does not last), interesting as development, surprising from time to time, entertaining to an immortal watching from the mountain. v0 produces a plausible but monotonous history: humans churn, dwarves and elves are annihilated by the first lost war, the slowest-decaying race is the last one standing. Six additions, all data plus rule files, none touching the thirteen base rules except where noted.

### 15.1 Terrain and temperament

Regions get a `terrain` field: `vale`, `marches`, `hills`, `wood`, `fen`, `scar`, `lair`. The map grows to 12 regions so dwarves and elves start with two each: add `deep_hold` (hills, neighbour of hill_hold) and `old_wood` (wood, neighbour of deep_wood); the sandbox layout table gets both.

Races get a temperament block, copied into culture and therefore mutable:

| race | war_appetite | expansion | home terrain | off-home land factor | memory of harm |
|---|---|---|---|---|---|
| human | 1.0 | 1.5 | vale, marches, fen | 0.8 | forgets |
| dwarf | 0.6 | 0.4 | hills | 0.4; never settles off-home | long |
| elf | 0.3 | 0.3 | wood | 0.3; never settles off-home | never |
| orc | 2.5 | 1.0 | scar, wasteland | 0.6 | short |

`war_appetite` multiplies `war_chance`; `expansion` multiplies the settle chance; land value is multiplied by 1.2 at home and by the off-home factor elsewhere. Elves and dwarves therefore stay in their niches unless their culture mutates; humans are the expansive race by numbers, not by script.

Race affinity matrix (−1..1), data: human–dwarf 0.3, human–elf 0.0, human–orc −0.8, dwarf–elf −0.3, dwarf–orc −0.9, elf–orc −0.9, same race 0.5. Negative affinity raises war chance, positive affinity enables alliances (15.4) and raises era diffusion.

### 15.2 Vassalage instead of annihilation

A faction that would lose its last region does not die; it becomes a vassal: tag `vassal_of:<overlord>`, war ends, it keeps the region, pays 20 % of its wealth growth to the overlord each decade, and cannot declare war. Orc overlords are the exception: orcs sack, the faction is destroyed and its population flees (15.3). A vassal regains independence (chance 0.2/decade) when the overlord's cohesion drops below 0.4 or the overlord collapses; the chronicle records both. The overlord's power counts vassal wealth at half weight. Rule file `vassals.cpp`, inserted after war and before collapse.

### 15.3 Population mix and remnants

`Faction.pop` is a map race → share, summing to 1, pure at the start. Every culture value that comes from the race template (growth, cohesion_decay, era_rate, memory_halflife, lifespan, temperament) is the share-weighted blend; `race` stays the founding race for naming, `dominant_race` is derived. Changes:

- a region taken shifts the taker's pop toward the loser's pop by 0.15 per region
- refugees: when a faction loses a region or is destroyed, 0.1 of its pop moves to the neighbour with the highest affinity > 0 (blend by relative wealth)
- successors inherit pop; settling does not change it

Remnants: when no living faction has `dominant_race` R, `world.remnants[R]` is created with strength 0.3 and the race's last region as origin. Strength decays with a 1000-year half-life. Each decade an ownerless region of R's home terrain with strength > 0.1 re-founds a faction with chance 0.05 (wealth 10, cohesion 0.7, era = the race's highest era ever reached − 1, tag `returned`); the chronicle line is "the dwarves return to the hill hold". A new dragon verb `sponsor_remnant` (gate: knowledge, resolves to a spawn) does it immediately. Rule file `remnants.cpp`, after migration.

### 15.4 Regime traits and the balance of power

Culture gets four traits in 0..1: `authority`, `openness`, `militarism`, `piety`. Start values per race with seeded jitter ±0.15 per faction: human 0.5/0.5/0.5/0.5, dwarf 0.6/0.3/0.4/0.4, elf 0.3/0.4/0.2/0.5, orc 0.7/0.2/0.9/0.3.

Effects on existing rules (multipliers read in `modifiers.cpp`, so the base rules stay untouched):
- war chance × (0.5 + militarism); anti-dragon rate × (0.5 + militarism)
- era diffusion × (0.5 + openness) on both sides
- cohesion decay × (1 − 0.4 · authority); succession reset halved when authority > 0.6; a collapse of a faction with authority > 0.6 tags the successors `civil_war` and they start at war with each other
- faith gained from witnessed acts and tribute × (0.5 + piety)

Drift (rule file `regimes.cpp`): at war, militarism +0.02/decade; ten decades of peace, −0.01/decade; burned by the dragon this sleep, authority +0.05 and piety +0.05; sharing a border with a faction of higher openness, openness +0.01/decade. Conquest blends traits like pop.

Affinity between two factions = 0.5 · race affinity + 0.5 · (1 − mean absolute trait distance), in −1..1. It replaces the race matrix wherever 15.1 uses it.

Balance of power: when a faction's power exceeds twice the power of every neighbour and it holds ≥ 40 % of regions, each neighbour with affinity toward the hegemon < 0.3 joins a coalition with chance 0.1/decade; a coalition is a joint war where the members' powers add. This is the rule that ends every empire, elven ones included, without naming any race.

Alliances: a faction attacked by someone gets help from neighbours whose affinity to it is > 0.5 and who border the attacker (chance 0.3/decade of joining). Allied factions do not declare war on each other while affinity stays above 0.5.

### 15.5 The fading

Tolkien's answer to elven permanence. When humans hold more than 60 % of the valley's wealth for ten consecutive decades, every faction with `dominant_race` elf gains tag `fading`: growth × 0.5, cohesion decay × 0.5 (they do not collapse, they thin out), and each decade a chance of 0.03 to depart a non-home region (owner none, the population goes to the elven remnant, strength +0.05). Fading ends if the human share drops below 0.4. An elven hegemony is therefore possible, rare, and bounded: it ends either by coalition or by the fading. Rule file `fading.cpp`, before migration.

### 15.6 Hunger

The dragon eats what it sleeps on. `Dragon.hunger` 0..1. Each decade of sleep, the digested amount of that decade counts as nourishment; if it is below `hunger_need` (constant, 0.4 per decade at size 0, growing with size), hunger rises by the shortfall × 0.05. Hunger above 0.5 shrinks size by `atrophy` (0.01 per decade) and lowers threat; hunger 0.9 forces waking with `wake_reason: hunger`. At wake, feeding acts reduce hunger: `kill` acts and `take` with payload `food` (new hoard category, half-life 20 years, converts to nothing but nourishment). Hunger is shown in the world view and the sandbox. Consequence: a 1000-year sleep on a small hoard wakes a smaller dragon; a long sleep is a bet on the hoard.

Forecast: a `forecast` command (api and library) runs the sleep on a copy of the world with a separate rng and returns projected size, knowledge, magic, hunger at wake, the strongest faction's expected era and whether a slay attempt is possible, each blurred: value × (1 + noise), noise amplitude 0.5 / (1 + knowledge). The sandbox shows it next to the sleep buttons.

### 15.7 Two rare events

Both in `shocks.cpp`, both independent of the dragon:
- great ruler (0.02/decade/faction): a golden age (growth × 1.5, cohesion +0.2 for one lifespan) or a tyrant (authority +0.3, militarism +0.3, cohesion −0.1 at death). Chronicle names the reign.
- succession crisis (0.15 of mortal successions): cohesion −0.3 and, with authority > 0.6, a `civil_war` split of the largest region.

### 15.8 Baseline bands for v0.1 (test 2, replacing the v0 bands)

200 seeds × 3000 years, idle:
- every race has a living faction or a remnant with strength > 0.1 at year 3000 in ≥ 95 % of seeds
- dwarves have a living faction at year 3000 in ≥ 60 % of seeds; elves in ≥ 60 %
- no race holds more than 70 % of regions at year 3000 in ≥ 85 % of seeds
- humans are the largest race by regions at year 3000 in 50 to 90 % of seeds (sometimes they are not; that is the point)
- at least one hegemony (one faction ≥ 50 % of regions) rose and fell within the 3000 years in ≥ 60 % of seeds
- median human faction lifetime in [150, 600]; dwarf in [400, 1500]; elf > 1200 (unchanged)
- humans reach era 2 by year 1500 in ≥ 80 %; valley never empty in ≥ 90 % (unchanged)

Test 5 (policies) keeps its assertions; `raze_all` additionally must wake the dragon by hunger at least once in ≥ 50 % of seeds when the policy takes no food. The other four tests are unchanged. The golden scenario gets a second trace, `ages_v0_1_raze.hash`, recorded from `raze_all` seed 42, so the hash covers collapse, vassalage and remnants.

### 15.9 Also in scope for v0.1 (from section 14's known-problems list)

- Wasteland must be able to form: lower the threshold or raise burn from acts, as data (item 3).
- Volcanic winter should be rare, not every raze_all sleep (item 5).
- Move the hard-coded numbers into constants.json and unify "losing" (item 6).

## 16. v0.1 implementation, 2026-10-15

All of section 15 is in `hoard-sim/` (rule files `vassals`, `remnants`, `regimes`, `fading`, `coalitions`, `hegemony`; `forecast.cpp`; `pop`, traits, remnants, hunger in state and serialisation; `forecast` and `sponsor_remnant` in the wasm API). Six tests pass on GCC and Clang, wasm hashes match both goldens.

Measured baseline (200 × 3000 y, idle): human lifetime 340, dwarf 540, elf 2170; every race alive or a remnant 0.99; dwarves alive 0.99, elves 1.00; no race above 70 % 0.92; humans largest 0.87; a hegemony rose and fell 0.65. raze_all: orc share 0.50, hunger wake 100 %. Five seeds read as varied: hegemonies at different centuries, dwarf expansions, coalitions, vassal cycles, no race dead.

Data changes against section 15 (README has the full table): human growth 0.03 → 0.02, dwarf 0.018 → 0.022; coalition affinity threshold 0.3 → 0.6 (at 0.3 coalitions formed in 7 % of seeds); fading thresholds 0.6/0.4 → 0.7/0.55 (at the doc's values elves fade in nearly every seed and never return); hunger need 0.4 → 0.6 (at 0.4 raze_all never woke by hunger); volcanic winter is a chance (0.1) instead of deterministic; defend's attacker loss 0.15 → 0.3; four small region land values raised to 0.6. Non-spec extension kept: an orc remnant floor of 0.15 so orcs re-seed off-map.

Known monotone patterns: elves withdraw from both border woods in the first 400 years in every seed (the withdrawal rule reads a bare power ratio; it should read affinity and fear); under idle the dragon starves at year 300 every time (tribute does not feed; `hunger_need` could scale with sleep length). Orcs under idle only exist via the remnant floor.

The sandbox game (`docs/sandbox-game.md`, `web/GAME.md`) sits on this version.

## 17. v0.2: people, reasons and tempo

Status 2026-10-15. Goal, in the author's words after playing v0.1: "I can do stuff in order to grow, but it isn't captivating me." Diagnosis: nothing in the valley has a name, nothing asks anything of the dragon, every consequence arrives as a number, and the sleep has one tempo. Six additions. Base rules stay untouched except through modifiers; new behaviour is new rule files and data.

### 17.1 Event ids and causes

Every `Event` gets `id` (monotonic int per world) and `cause`: `{ refs: [event ids], text_key, params }`. The rule that emits an event knows why it fired; it writes the why. Examples:

- `war_started`: refs the last `region_taken`/`plague`/`collapse` that pushed the defender's cohesion down, params {ratio, cohesion}
- `collapse`: refs the succession crisis or war that preceded it, params {cohesion}
- `oath_breached_by_them`: refs the memory decay (no event) → text_key `faded_memory`, or the plague that emptied the treasury
- `slay_attempt`: refs the `burn` acts in memory with the highest weight and the `slayers_founded` event
- `orc_spawn`: refs the burn act / `abandoned` event that emptied the region
- `coalition_formed`: refs the `hegemony` event
- `fading_begins`: refs the hegemony or era event that pushed human wealth over the threshold

Acts applied from the log are events too (`dragon_burn` etc.) so causes can reference them. The chronicle JSON carries ids and causes; `chronicle_hash` includes the ids but not the cause text (so phrasing changes don't break goldens; ref lists are included). Rule of thumb: every event emitted by a rule has at least a text_key cause; refs where a prior event exists.

### 17.2 People

`World.people[]`: `{ id, name, race, faction, role: ruler | heir | hero | sage | priest, born, dies (year or 0 for immortal), traits: {bold, pious, greedy, cunning} 0..1, tags, memory_of_dragon: weight }`. Names from a per-race syllable table in `data/names.json`, deterministic.

- Every faction has a ruler person; succession (rule 3) replaces it with an heir (if one exists) or a new person; the chronicle names both ("Aldric the Bold dies; his daughter Maelis takes the throne of aldmere"). The great-ruler shock names the reign. Tyrant/golden age become traits.
- Heroes: spawned by `slayers_founded`, `coalition_formed`, a lost war (a general), a `civil_war` (a claimant), chance per decade from constants; a hero can lead a slay attempt (success chance × (0.5 + bold)), unite successors (cohesion +0.2 to the faction it joins), or found a lineage. Heroes die by lifespan or on a failed slay attempt.
- Sages and priests appear with era ≥ 2 and piety/openness; they carry the faction's memory of the dragon (while a sage lives who witnessed an act, that memory decays at half speed) and are the envoys for knowledge/faith offers.
- `person_died`, `person_rose`, `person_crowned` events with causes.
- Dragon-side ledger: `Dragon.known_people[]` (ids the dragon has met through an offer, a hostage, a kill or a `show` at their capital) with a relationship score −1..1 updated by acts; this is what the briefing calls "people you know". Hostages are people (`take_hostage` picks the heir or ruler; `release` returns them; digestion of royals becomes "the heir grows up in the lair", tag `dragon_raised` on the person, and the lineage logic keys off persons).
- Lifespans by race (human 60 ± 15, dwarf 250, elf 800 or immortal for a queen, orc 40), scaled by the sleep tempo so that in phase one the dragon meets the same king twice.

### 17.3 Offers with reasons

`Offer { id, from_faction, envoy: person id, kind, target (faction/region/person), motive, stated_reason, is_lie, terms: Oath draft, expires }`.

Kinds and the state that generates them (data table `data/offers.json`, one row per kind with trigger predicate, motive derivation, pretext pool, price):

| kind | trigger | motive (true) | pretext pool | asks of the dragon |
|---|---|---|---|---|
| protect | losing a war, or hegemon neighbour | survival | "they are orcs' friends", "they broke a treaty", "the gods demand it" | `defend` clause |
| burn_rival | rival power > theirs, affinity < 0 | fear of conquest / revenge for a taken region / piety against a rival creed | same pool plus "they plan to slay you" (a lie unless rival has slayers) | `burn` act on target |
| return_hostage | their heir in the lair | succession | ransom offered; "the realm will fall without them" | `release` |
| hide_heir | civil_war or usurper hero present | protect the line | "take the child, raise her" | `take_hostage` (with consent: faith +, no fear) |
| teach | openness > 0.5, era < max of valley | catch up | "we will build you a library" | `teach` |
| bless | plague or bad century on them, protector creed | hunger | "we are your people" | `bless` |
| judge | a `false_dragon` or usurper hero | legitimacy | "he lies in your name" | `kill` on a person |
| purge | undead/necromancer neighbour | survival | "the dead walk" (true) | `kill` on the undead faction |
| ward | holy region, piety > 0.6 | protect the altar | "guard what is yours" | `ward` |
| tribute | faith > 0.7 | awe | none | nothing; offers gold for a `no_harm` clause |
| elope | a royal person tagged `betrothed` with low affinity to the match (data trigger: marriage event) | escape a forced marriage | "I would rather burn" | `take_hostage` of that person; the faction reacts as theft |
| rite | era 4, faith > 0.8, world magic (v1) | exit | — | the exit |

A lie: `is_lie = rng < lie_chance(authority, cunning of the envoy)`; the stated reason is drawn from the pretext pool instead of the motive's text. The dragon with knowledge ≥ k sees "the envoy's eyes say otherwise" (stated vs true shown blurred). If the dragon acts on a lie, the faction gains tag `deceived_dragon:<id>`; the dragon's ledger records it; the next envoy from them is marked. Honest offers whose motive the dragon fulfils give faith × 1.5.

Offers are generated at sleep end as now, and mid-raid when a trigger appears (optional in this version). The envoy is a person; killing the envoy is an act (`kill` magnitude 0.05 on the faction, fear +0.3, and the person dies).

### 17.4 Phases and tempo

`Dragon.phase` 1..5 from size + knowledge + magic thresholds in constants (`phase_thresholds`). Each phase has `short_sleep` and `long_sleep` windows in years:

| phase | short | long (moult) | who can relate |
|---|---|---|---|
| 1 hatchling | 1 | 10 | the same king twice |
| 2 young | 5 | 100 | a bloodline |
| 3 grown | 20 | 300 | long-lived races, orders, sages |
| 4 ancient | 100 | 1000 | elves, institutions, the lich |
| 5 elder | 300 | 3000 | immortals only |

The player chooses any length; the windows are shown as the phase's rhythm and the forecast uses them. A moult (long sleep) is required to enter the next phase once thresholds are met (tag `ready_to_moult`), and a moult cuts stamina on the first raid after (the dragon wakes stiff) while doubling digestion for that sleep.

Sub-decade steps: `step(dt)` with dt in years; all per-decade rates in the rules are scaled by dt/10 (the modifiers rule computes `dt` once per step and every rule reads `p.dt`). Below 10 years the slow rules (era, collapse, migration, remnants, hegemony, shocks) are skipped; wars, memory, oaths, hunger, regimes drift and offers run. Golden traces are recorded at dt = 10 and must not change.

### 17.5 The briefing (narrator, template-based)

`briefing(world, since_year)` in the library returns structured text blocks for the shell to render:

1. Headline: the one event with the highest importance (importance = data per kind × magnitude, hegemony/collapse/slay/exit highest).
2. "While you slept": events in chronological groups, each sentence built from text_key + params + names, with the cause clause appended when refs exist ("because the plague of 1140 had emptied its treasury").
3. People: known people who died, rose or changed; the new rulers of every faction the dragon has an oath with.
4. Threads: oaths and their status, hostages, spells still running, remnants waiting, tags like `deceived_dragon`.
5. Opportunities: current offers (with stated reason), factions at war the dragon could defend, rich undefended targets (wealth high, fear low), a slayer order forming.

Text templates live in `data/text/en.json` so a future LLM can rewrite them per creed/culture without touching the sim. Both pages show the briefing on wake; the game shows it as the wake screen before the raid, the sandbox as a panel above the chronicle.

### 17.6 Interest tool

`tools/interest.py`: runs N seeds × Y years for a list of policies (idle, raze_all, protect_one, alternate, and a "played" policy that mimics a reasonable player: protect the weakest human faction, take gold from the richest, burn whoever has slayers), writes `out/interest/<policy>/<seed>.json` and a summary with per-seed scores:

- outcome entropy across seeds: distribution of (largest race, hegemon race, era of the leader, number of living factions) at the end
- reversals per 1000 years: hegemon fell, vassal freed, a `returned` faction took a region back, a coalition won
- causal depth: share of events whose cause chain reaches ≥ 3 sleeps back
- pacing: coefficient of variation of events per century (bursts beat flat)
- persistence: tags older than 500 years still read by a rule this century
- dragon agency: Jaccard distance between the idle and the policy chronicle for the same seed (event kinds × subjects per century)
- flatness flags: a race dead in all seeds, the same first three events in > 50 % of seeds, no reversal in a seed

Prints a table per policy and the five most and least interesting seeds by a weighted sum. A `--chronicle seed` option prints a narrated chronicle (the briefing templates) for blind judging.

### 17.7 Fixes from 16

- Elf withdrawal reads affinity and fear, not a bare power ratio: withdraw when a human neighbour's power > 2× AND affinity < 0.2, or fear > 0.6.
- Tribute feeds the dragon (`food` category from tribute at 0.3 per gold unit) so an idle dragon with worshippers does not starve at year 300.
- `hunger_need` scales with the phase (a hatchling needs little).

### 17.8 Additions from the blind read (2026-10-15, after the interest run)

- Hegemony hysteresis: a faction becomes hegemon at ≥ 50 % of regions and stops being one only below 40 %, or on collapse; `hegemony` and `hegemony_ended` fire once per episode. (Seed 120 flickered seven times in 200 years at exactly 6 regions.)
- Threads: `Event.thread` (string id) groups the lines of one arc: a war from declaration to end (`war:<a>:<b>:<year>`), a hegemony episode, a civil war between successors, a coalition. The briefing renders a thread as one paragraph with first and last line; the chronicle JSON keeps every event. The hash ignores thread ids.
- War endings get a reason word in tags: `submission`, `exhaustion`, `annihilation`, `coalition`, `defended_by_dragon`, so the narrator does not say "ends in submission" nine times.
- Collapse into N successors emits one `collapse` event with the successor list in params plus one `successor_founded` per successor tagged with the thread; the briefing collapses them to one sentence.
- Interest tool (17.6) ports from the snapshot: `tools/interest.py`, `data/policies/played.json`, the policy keywords `has_slayers`, `weakest_human`, `richest` in policy.cpp. Reversal weighting there: a `vassal_freed` counts only when the vassalage lasted ≥ 100 years or crossed races.

## 18. v0.3: long tails, 2026-10-15

Goal: the author plays v0.2 and is not captivated; an interest run (17.6) showed varied outcomes but causal depth 0.02 (2 % of events have a cause chain reaching three or more sleeps back). Catalogue insight 2: every great event has a long tail. Base rules are untouched except through modifiers; everything below is data, rule files outside the thirteen, or the narrator.

### 18.1 Ages of people

`races.json` gains a `person` block per race (accession_age, lifespan, lifespan_jitter, max_age, generation). A ruler, hero, sage or priest enters grown (born accession_age ago, jittered); an heir is born a generation after the parent and is of age at succession; a person dies of age when the lifespan ends, and a ruler who dies before the reign counter of rule 3 runs out is succeeded at once (the counter still drives the cohesion reset). Measured over 200 idle seeds × 3000 years, age at death median / max: human 67 / 86, dwarf 253 / 321, elf 776 / 1034, orc 41 / 61.

### 18.2 Importance and the narrator

Every event carries `importance` (0..1): the template's value in `data/text/en.json`, plus `briefing.importance_known` (a person the dragon knows), `importance_oath` (a court under oath with it) and `importance_hostage` (a person in the lair, or a court with one there now). "While you slept" drops events below `importance_min` unless their thread holds an important one, so heir births, unknown courtiers and routine reigns stay out. Narrator additions: `{,why,}` places a cause clause mid-sentence; `{ref_year}`, `{o_word}`, `{m_word}` (number words from `misc.numbers`); a cause text keyed `<cause>@<kind of the cited event>` (e.g. `weak_neighbour@plague`) tells the same cause in words that fit what it cites. Several templates were rewritten so they read as written sentences (fealty, collapse, hegemony, fading, hostages, oaths).

### 18.3 Event cards

`data/events/*.json` (one file per catalogue entry, an array of card definitions), evaluated by `rules/cards.cpp` after shocks: a card fires on a faction, region or the world when its trigger holds (`rules/card_match.cpp`, the predicate language documented in `cards.h`) and a roll of `chance × dt/10` passes; its event names the events that satisfied the trigger as cause refs; its effects (`rules/card_effects.cpp`) write tags with an expiry from data into `World.timed_tags`, and inheritable tags pass to successors. Fourteen catalogue entries, 34 definitions:

| entry | trigger → effect | tail tag (decades) |
|---|---|---|
| the grudge | dwarves lose a region, swear fealty or are robbed/burned → grudge; grudge war when strong enough; re-read against the dragon (anti-dragon floor 0.3); settled by land retaken or a gift ≥ 0.3 | `grudge:<actor>` (50) |
| oath of recovery | elves robbed of art/artifacts or sacked → sworn, cohesion +0.2; oath war, or hunters toward the lair (anti-dragon); a gift or land retaken ends it (cohesion −0.3) | `sworn:<actor>` (100) |
| prophecy of the second fire | pious court remembering a burn, the dragon away 200+ years → prophecy; the next act fulfils it as fire (fear, anti-dragon rate ×2) or grace (faith +0.4) | `prophecy` (40), `prophecy_of_fire/grace` (50) |
| dragon-blood dynasty | a dragon-raised line begins → faith and fear floors, decay ×0.8; after 300 years a schism (split, one side faith 0) | `dragon_blood` (100) |
| hostage who came home | a dragon-raised hostage released → rules (0.5), rejected into a border successor (0.3), or betrays (0.2) | `dragon_blood`, `rejected_ward:*`, `turncoat:*` |
| treasure hunters | court bordering the lair, fear < 0.3, hoard ≥ 60 gold, dragon asleep 50+ years → 0.7 lost (fear), 0.3 steal 2 % | `robbed_the_dragon` (50) |
| forgotten dragon | nobody fears or worships the dragon for 500 years → world tag, anti-dragon 0; a `show` reads as a burn; the next act ends it | `dragon_forgotten` (150) |
| pretender who returns | a successor with a claimant of the old line (or the dragon holding its heir) marches on a sibling successor; crowned on a win | `pretender:*` (50), `restored_line` (100) |
| slayer order | slayers founded → order; survives collapse, spreads to allies, martyrs after a failed attempt | `slayer_order` (100) |
| false dragon | faithful court, dragon away 200+ years, tyrant on the throne → faith 0.2, piety and authority up; judged by a kill or disillusioned at his death | `false_dragon:*` (50) |
| the vein | dwarf hills, iron age on → land +0.3, growth ×1.3 for 20 decades; human neighbours press an old claim by war | `vein_struck` (50), `claim_on:*` (80) |
| exodus + return of the exiles | a different people takes a region from a cohesive court → refugees, the region keeps `exiles:<race>`; when that people holds ≤ 2 regions and the holder is weak, they come back by war | `exiles:<race>` (80), `returned_exiles` (100) |
| sack of the capital | the loser's best land taken → −25 % wealth, half to the winner, burn; revenge war later | `sacked:*` (80), `plunder_of:*` (60) |
| old enemies | three wars in 300 years at comparable power → arms race, rematches | `old_enemies:*` (80) |

Engine additions for them: `$year` and the `alive` person field, `min_magnitude` on events, `regions:<race>` world field, `ledger` effect, `loot:<kind>` tags on `act_take`, modifier vars `anti_dragon_rate` and `faith_gain`, `forgotten_show_as_burn`.

### 18.4 Keepers of memory

17.2 says sages and priests carry the court's memory; catalogue insight 5 asks for witnesses. A new sage or priest takes up the oldest memory the court still carries: a card's mark on it or a dragon act it has not forgotten (weight ≥ `keeper_min_weight`, also acts on a realm it descends from). The rise refs that event; keeping a dragon memory slows its decay (`sage_memory_mult` now 0.85, from 0.5). A hero of renown is sworn to the same memory, and four war cards double their chance while a sworn hero lives. The chronicle names a kept memory once per court and role, and only when it is `keeper_named_years` (200) old.

### 18.5 Narrated reactions

`apply_acts` returns `lines` (library and wasm `apply_acts`, `kill_person`): one or two sentences per court that reacted, from `en.json` `reactions` keyed by the row that fired (`burn.oath_active`, `harm.strong.dwarf`, `witness_burn.enemy`, ...), plus one sentence for an offer fulfilled or a lie acted on, a person the dragon knows, or a card's mark (`tag.prophecy.harm`, `tag.grudge.harm`, ...).

### 18.6 Measured

Interest (300 seeds × 3000 years, sleep 100): causal depth idle 0.02 → 0.087, played 0.023 → 0.141; outcome entropy idle 3.78 → 4.25, played 3.67 → 3.75 bits; dragon slain 1 → 9 of 300 played seeds. Baseline bands hold (human lifetime 360, dwarf 560, elf 1985; every race alive or remnant 0.97, humans largest 0.77, hegemony rose and fell 0.68); protect_one now lifts aldmere's realm to 4.24 against 3.70 idle (it failed at 3.86 against 3.88 before this work).

Deviations from the catalogue: card wars require the attacker to be at least as strong (grudge, revenge) or stronger (claims), because wars that ignore the ratio let dwarf holds conquer and assimilate into human courts; the return of the exiles reads "a scarce people" (≤ 2 regions) instead of a remnant above 0.3, because humans driven out by orcs otherwise retook the valley; the forgotten dragon does not breach no_walls/no_settle oaths; the oath of recovery does not suspend the fading; the slayer order gains no artifact (v1).

## 19. Integration notes, 2026-10-15

- Slow rules on short sleeps: `World.slow_clock` accumulates the years of each step; era, collapse, fading, migration, remnants, hegemony and shocks run once every `step_years` of accumulated time, and the era rule reads `Faction.slow_growth`, the wealth growth since its last run. At 10-year steps this is bit-identical to before (goldens unchanged). Before the fix a hatchling resting a year at a time saw a frozen valley: 60 one-year rests produced only people events.
- Stamina (page side): a sleep restores `years / short window of the phase` of the full stamina budget, capped at full. A hatchling's 1-year rest restores all of it; an ancient dragon napping one year restores 1 %.
- `web/build.sh` and `build.ps1` end with `web/inline.py`, which inlines the game's split scripts (`web/game/*.js`) so `hoard-game.html` stays one file.
- `web/game-check.cjs` (66 checks) searches for its situations (sleeps on until a protect offer, an heir, a plague exists, or builds them in a `load_world` lab) instead of relying on one seed, so data changes do not break it.
- Known cosmetic issues: founding rulers show their reign counted from before year 0; a newborn heir can already be betrothed; the save-game JSON carries an offer's true motive and lie flag (the page never shows them).
