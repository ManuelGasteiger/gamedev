# Hoard: the awake phase (raid design v0.1)

Status 2026-10-24. The first design of the raid as play: what the player does between waking and sleeping, how it feels, and how it becomes acts in the sim's closed vocabulary. Decisions taken tonight: one continuous valley (not arenas), several sorties per waking with a shared stamina budget, two shells (the tactical sandbox and the flight game) producing the same act log, and the bed refinement (Might from consumed gold). Companion docs: `act-api.md` (the acts), `fire-mechanic.md` and `flight-feel.md` (the two feel pillars, not restated here), `hoard-brief.md` (the prototype's numbers), `sandbox-game.md` (the tactical shell), `core-loop.md` section 4 (defences, wounds, death).

## 1. The shape of a waking

```
wake screen ──► lair (hoard, envoys, scenarios, talents, build) ──► sortie 1 ──► lair (drop loot, audiences) ──► sortie 2 … ──► lair ──► sleep
```

- A waking is 6 to 10 minutes of play (decision: 30 to 50 wakings in 6 to 10 hours). Two or three sorties is the norm; one for a hatchling, four for an ancient dragon with a big stamina budget and far targets.
- **Stamina** is the clock. Budget per waking `100 + 20·sqrt(growth points)` (core-loop). Acts cost stamina as in the sandbox table (burn 15 per 0.25 magnitude, take 10, kill 10, defend 12, roar 3, hostage 20, crush 20). Flight costs nothing at cruise; surge drains 1 per second; breathing fire drains with the stream (the fire mechanic's inhale and column). A sortie ends when the player lands at the lair, when carry is full, or when stamina is gone (the dragon can still fly home, it can do nothing else). Stamina budget, drain per act and per surge, carry cap, travel-flight speed and the altitude at which it begins are all talent-bound (`talents-v2.md`): a hatchling flies low and slow and tires fast; an ancient dragon crosses the valley in half a minute.
- **Carry** is the second limit. Carry cap 300 gold-equivalents, +150 per Might talent that says so (hoard-brief's boon). Books, artifacts, art and a hostage have weights (a hostage is 100 and disables the breath while carried). A laden dragon climbs only with surge or a gentle angle; this is the prototype's "gold is heavy" and it is what makes a second sortie a choice rather than a chore.
- **The lair between sorties** is where the strategy layer lives in the flight shell: landing on the ledge opens the lair panel (hoard by category, growth, envoys as figures at the ledge, scenario cards, talents pending, build, keep). Dropping loot is a flyover of the pile with a release. Audiences happen here and only here, so a sortie is never interrupted by text.
- Within a waking no sim time passes. Settlements do not rebuild between sorties; defenders regroup (archers return to towers after a minute), fires burn out, fled villagers stay fled. Days and nights are presentation (a waking starts at dawn and the light turns as stamina drains), not rules.

## 2. One continuous valley

The valley is one flyable space with the twelve regions of the sim's graph as its districts. Decision cost and how we pay it:

- **Scale.** Dragon 16 m long, wingspan 18 m, houses 6 × 7 m (prototype, felt right). Region about 900 m across, settlement footprint about 150 to 250 m, the valley about 3.5 × 2.5 km with the lair on its rim (`sandbox-game.md` has the layout: vale, marches, hills, wood, fen, scar, the volcano). Cruise 35 m/s, surge 54 (prototype). Lair to the far corner at cruise is about 100 s, too long for several sorties. So:
- **High flight.** Above 300 m the dragon enters travel flight: 80 m/s, no stamina cost, nothing on the ground can reach it in the early eras (archer ceiling 110 m, ballista 170 m, cannons 450 m from era 4). Lair to far corner at travel speed is about 45 s; region to region 10 to 15 s. The arms race reaches the sky in the late game: an era-4 court with cannons denies travel flight over its region, and the briefing says so ("Dorun's cannons reach the high air"). Descending to act is the moment of commitment and the moment the defences' range rings light up.
- **Reading the world from the air** is why the valley is continuous. On the way to a target the player sees the state the sim produced, aged by the years slept (`world-aging.md`): after a one-year rest, charred frames and a city still smoking; after a century, that city rebuilt in stone with walls it did not have, or a field with a hamlet where it stood, or a forest with a ruin in it; banners on new walls, a slayer camp with its standard on the lair's border, an army on a border between two courts at war, a pilgrim column on the road to the mountain, an envoy walking the same road, a temple with your sign on it, a library's scriptorium, a foundry's chimney, a memorial stone where a slayer fell. Each is a sim fact rendered as a thing (the same mapping the sandbox uses for tags, now in the flight shell); the shell remembers nothing itself. A flyover below 150 m over a settlement is a `show` act of magnitude 0.1; the valley being continuous means the player is seen on the way, and dread and awe move because of it. Travel flight above 300 m is not seen (the sim's `show` needs witnesses within range).
- **Hot settlement.** Only one settlement at a time runs at full fidelity (fire grid at 2 m cells, crowd flee, defenders with lead aiming); it is the one the dragon is nearest to below travel altitude. The others run a cheap state: fire as a settlement-level burn value with smoke, defenders as range rings and a hit chance, crowds as a few figures. The hot settlement switches by distance, seamlessly; the fire grid of a settlement left burning keeps a coarse spread so the player returns to the fire they started. This is how the continuous valley fits a PC that struggles with full Unreal: twelve cheap settlements and one hot one, streamed by distance (Unreal's World Partition later; one three.js scene with LOD now).
- **Content.** Twelve settlements from the village and hold kits by race and era (`hoard-brief.md`'s asset route), each with its special buildings placed by sim tags: library, temple or altar, foundry, keep, granary, walls, towers, a monument. The kit is the content; the sim places it.

## 3. Shells report facts; the core makes acts

Both shells (the tactical sandbox and the flight game, later Unreal) talk to the sim the same way, and the sim never knows which. The tactical shell already emits acts directly from its action panel. The flight shell cannot: a fire spreads where it spreads, a swoop picks up what is there. So the flight shell reports **facts**, and the core's `raid_diff` turns facts into acts with magnitudes relative to what the court had at that moment (`ages-sim.md` open question 4 is settled: the diff lives in the core).

`RaidReport`, sent at the end of every sortie (and on landing at a capital for speech acts, which are acts already):

```
RaidReport
  sortie          int
  settlements[]   { region, owner,
                    destroyed: { house: value, keep, library, temple, foundry, walls, granary, tower, altar, monument },
                    collected: { gold, books, artifacts, art, food, items[] },
                    killed:    { soldiers, garrison, civilians, persons[] },
                    taken:     persons[] (hostages carried to the lair this sortie),
                    presence:  { lowest_altitude, seconds_below_150, roared: bool, landed_in_square: bool } }
  armies[]        { attacker, defender, region, killed_share }
  lair            { dropped: { gold, books, artifacts, art, food, items[] }, received[], fought[] }
  spells[]        acts already (cast is a channel with a target; the shell emits the act)
  speech[]        acts already (the court panel at a capital emits demand, swear, teach, broker, reveal, judge, release, take_hostage with consent)
  wounds          health lost this sortie, by source defence
```

`raid_diff` rules (start values; data in `data/raid_diff.json`):

| fact | act | magnitude |
|---|---|---|
| `destroyed.house + keep` at a settlement | `burn` on the region | destroyed value / the settlement's total building value at sortie start (so a village and a city both read as a share) |
| `destroyed.library/temple/foundry/walls/granary/tower/altar` above 0.5 of that structure's value | `burn` with `structure` | same share; one structure act per structure per sortie |
| `collected.gold` | `take` with payload gold | gold / court wealth at sortie start |
| `collected.books/artifacts/art/food`, `items[]` | `take` payload per category | per category share of what the court held |
| `killed.soldiers` or `armies[].killed_share` | `kill` who army | share of the army |
| `killed.garrison` | `kill` who garrison | share of the garrison |
| `killed.civilians` | `kill` who civilians | killed / settlement population |
| `killed.persons[]` | `kill` person each | 0.05 |
| `taken[]` | `take_hostage` each | – |
| `armies[]` where the defender is a court the dragon has an oath with, or the attacker is at war with it, and the attacker's army was hit | `defend` with `secondary` attacker | killed share |
| `presence.landed_in_square` | `show` land 0.5 | – |
| `presence.roared` | `show` roar 0.3 | – |
| `presence.seconds_below_150 ≥ 3` and nothing else at that settlement | `show` flyover 0.1 | – |
| `lair.dropped` | hoard update (no act) | – |
| `lair.received[]` | `receive` per group | – |
| `lair.fought[]` | `fight` per person | – |

Rules of the diff: one act per (type, region, structure) per sortie, magnitudes accumulate within the sortie and are reported once, so a long burn is one `burn` of 0.6, not six of 0.1; a `burn` on a settlement also produces `kill` civilians from the fire's own toll (the crowd system reports who did not get out), so burning a city is never free of dread; witnesses are the sim's job (owner, neighbours, sages and envoys present), never the shell's.

The tactical shell keeps its panel but goes through the same `raid_diff`: its "Burn 0.4" becomes a fact `destroyed.house = 0.4 × value`. One path, two shells.

## 4. The acts as things you do

| act | in the flight shell | in the tactical shell |
|---|---|---|
| burn | breathe on buildings; fire spreads by the grid; stone burns at half rate; wards (blue dome) halve it; the structure you aim at is the one that counts as `structure` | hold the key, the ring grows |
| take | swoop below 15 m over spilled gold and chests; the keep's vault and the library's shelves spill their category when the building collapses or when the dragon lands and claws it open (a 2-second channel) | slider |
| take_hostage | land at the keep after its roof is gone or its gate broken; grab the marked person (ruler or heir glow at a capital); carrying disables breath | button at a capital |
| kill (army, garrison) | fire or claws on soldiers; garrisons stand on walls and towers (killing a tower's crew silences it for the sortie) | button when an army is present |
| kill (civilians) | the fire's toll, plus deliberate fire on a fleeing crowd (the crowd system reports it) | implicit in burn magnitude |
| kill (person), judge | land in the square; the court panel opens; pick the person; `judge` is the same with a verdict and a tagged person | panel |
| fight | a hero or slayer stands his ground on the border or the ledge; a ground duel: bite, claw, tail, fire against shield, ward-cloak, bolt; his counter trait shows as equipment (fireward cloak, plate, runes); his fear can be triggered (fire for one who fears fire routs him) | button; the sim rolls |
| defend | attack an army that is on another court's border; the sim knows who attacks whom | button |
| show | fly low (flyover), roar (key), land in the square (land) | roar button; the others implicit |
| demand, swear, teach, broker, reveal, release | land in a capital's square and the court panel opens: the ruler, heir, sages, priests with traits as words; the speech acts are buttons with their gates shown; the envoy's audience uses the same panel at the lair | panel |
| guise | land outside a city at night; choose a purpose; a short montage (not a walking sim) and the result; discovery rolls by the sim | panel |
| spells (bless, ward, curse, miracle, corrupt, raise_undead, haunt, incite, terraform, consecrate, sponsor) | a channel from the air over the target region or at the altar; a visible effect (a dome, a blight spreading, the dead rising from a battlefield); pool cost, no stamina | panel |
| hunt | fly over herds or forest, grab | button |
| receive, keep, dismiss, commune, build, pick_talent, sleep, exit | the lair panel | lair panel |
| ignore | not answering a hook or scenario by sleep | same |

## 5. Danger in real time

Core-loop's rule holds: every danger is shown before it can kill. In real time that means:

- **Range rings** on every tower, ballista and cannon, visible from the air at any altitude, with the listed damage per hit on approach (hover or the first second inside the ring shows "bolt-thrower, 8").
- **Telegraphed shots**: a bolt or cannonball has a visible wind-up and flight; it can be dodged; archers are pressure, not surprise. Fear dampens all of it as the sim says (terrified garrisons flee their posts, visibly).
- **Health** is the bar; wounds carry between sorties within a waking and heal only in sleep. A sortie that starts under 30 % health shows the warning the briefing would show.
- **The lethal second confirm** of the tactical shell becomes a **lethal zone**: when the next listed hit would kill, the HUD flashes and the dragon's controls get a half-second of invulnerable climb once per sortie. Death is still possible and still the player's fault; it cannot be a surprise.
- **Slayers**: the named slayer stands where the briefing said he would (the lair's border or the ledge); his known traits show as gear; `?` traits show as a blank cloak until the first exchange.
- **Death** ends the run as core-loop says: the death screen names the killer and the three biggest hits with the numbers shown before each.

## 6. The lair as a place

Landing on the ledge is the only way into the strategy layer during a waking, and it should feel like coming home: the hoard as a pile that grows across wakings (gold by Might, books in the nest's library once built, artifacts on a shelf, the crown that speaks on its own stone), the kept people at their places (the historian at a desk, the consort, hostages), the envoys and pilgrims waiting at the ledge as figures with their reason as a line when approached, the scenario cards as things in the world (a letter on the ledge, a kestrel, a sealed challenge). The panel is the interface; the place is the fantasy.

## 7. What to build first

The flight shell exists as the three.js prototype (one village, flight, fire, loot, archers). The next step is not the fire or the flight (both are better than the rest already) but the loop:

1. The continuous valley at the scale above with twelve settlement stand-ins placed from `world` (owner colour, era kit, special buildings from tags), the lair on the rim, high flight, the hot-settlement switch.
2. `RaidReport` from the shell and `raid_diff` in the core, with the tactical sandbox routed through the same diff.
3. One full waking: wake screen (reuse the sandbox's), two sorties, drop loot, one audience at the ledge, sleep, the montage, wake again and see the smoke where you burned.

That is prompts 15 and 16. The fire and flight pillars continue in their own docs.

## 8. Open

- Should the fire's civilian toll always count (dread from every burn) or only when the crowd is targeted? Proposal: always, scaled by how many got out; a quick burn of an empty granary kills nobody.
- Night raids: does darkness lower archers' accuracy and raise awe for being seen as a shape against the stars? Later.
- Whether carrying a hostage should also slow the dragon (yes, as weight) and whether a dropped hostage dies (yes).
- The travel-flight ceiling per era, and whether wards reach the sky.
- How much of the lair is walkable versus a panel.
