# Hoard: the core loop (v0.4)

Status 2026-10-17. Written after the playtest of v0.3 ("I fly around, grab loot, sleep; the changes aren't noticeable and don't affect me"). Decisions taken with the author on 2026-10-16: the arms race is the spine, growth comes from loot and sleep length through talents, death is final, the starting world varies per seed with a pre-history and rare irreversible events. Ambitions come later.

## 1. What was wrong

Measured on v0.3 before this work:

- A dragon playing the `played` policy for 1500 years ends at size 0.15, still a hatchling. Gold digests with a 1000-year half-life at 0.001 size per unit, so a raid's gold is worth about 0.003 size. Growth was invisible.
- Nothing could threaten the dragon while awake, and it died in 1 of 300 played runs asleep. Every choice was safe, so no choice mattered.
- A dragon playing normally barely changed which history happened (3.8 bits against 3.6 idle).
- Every seed started with the same four realms on the same map; the seed only changed the dice.
- Oaths sworn out of fear broke after one year: the tribute clause checked faith, and a cowed court has none (reproduced: `demand tribute`, sleep 1 y, `oath_breached_by_them`, cause `faded_memory`).

## 2. The loop

```
 wake ──► read what happened ──► pick talents ──► raid: answer the hooks, loot, burn, defend
  ▲                                                                 │
  │                                              wounds, stamina, defences, fear
  │                                                                 ▼
 the world moves on: eras, defences, slayers, forgetting ◄── sleep: choose how long (the bet)
```

Three rules hold it together:

1. **They grow by time, you grow by hoard.** Mortals advance on their own clock and every era brings a counter to dragons. You only grow by digesting what you brought home.
2. **Sleep length is the bet.** Longer sleep: more growth, healed wounds, but the valley advances, forgets you and arms. Shorter: you stay ahead of them and remembered, but grow slowly and wake hurt.
3. **Every danger is shown before it can kill you.** Death is final, so it has to feel like your own fault (Into the Breach: every attack telegraphed). Towers have visible range and listed damage, slayers are named in the briefing before they come, and Lore can wake you before a slaying.

Plague Inc is the closest relative: you evolve with points while the world researches a cure, and the more you are noticed the faster it does. Here fear is notice: terror makes courts pay and obey, and it also feeds their anti-dragon research.

## 3. Growth

### Axes and what feeds them

| axis | fed by | also raises | shows as |
|---|---|---|---|
| Might | the gold you sleep on (not eaten: a bed), food | size | bigger body, darker plates, spikes, horns |
| Flame | what you burned this age (embers) | | fire colour orange, gold, white, blue; wider beam |
| Arcana | artifacts | magic (spell pool) | runes on the scales, purple wing tips, aura |
| Lore | books, royals | knowledge | golden crest, glowing eyes |
| Dominion | art, tribute, worship | faith pool | glittering hoard, banners of the faithful |

Growth points are permanent and never spent; the pools (size, magic, knowledge, faith) still pay for spells and terraforming as before.

### Sleep length decides the amount, relative to your phase

Digestion runs on the phase's clock: half-lives are fractions of the phase's moult window (books 0.25, art and royals 0.5, artifacts 1.0, embers 0.3, food 0.1), and the gold bed gives Might at `bed_rate × sqrt(gold)` per moult window slept. So a hatchling's 10-year moult and an ancient dragon's 1000-year moult both digest a hoard in about the same share, and both are worth sleeping. A rest (the short window) digests a tenth of that. Sleeping past the moult window keeps growing Might from the bed, so a very long sleep still pays; the valley it wakes to is the price.

Phases come from total growth points (25, 90, 250, 600) and a moult as before.

### Talents

Each axis has nodes at 3, 8, 15 and 25 points (then 40, 60, 85, 115, 150, 200 as content grows). At each node you choose one of two talents; the other is gone (a build, not a checklist). Major nodes need a phase, which means a moult. After the last authored node, every 25 points gives a repeatable rank: a small bonus to the axis' main number and a visible step (scale, beam width, glow).

You pick on waking, after the briefing, so you can answer the world: the elves founded a mage guild, so you take Iron Scales or White Fire. Picks can wait; the HUD shows them as pending.

Rule for content: every talent changes something you see or feel the moment you take it.

First list, to cut freely:

**Might** (gold bed)
- 3: *Thick Hide* (wounds −20 %, darker plates) | *Deep Lungs* (+25 stamina)
- 8: *Great Wings* (flight +25 %, bigger wings) | *Mountain Tail* (Kill army −40 % stamina, +50 % effect, tail spikes)
- 15: *Iron Scales* (wounds −30 %, metal sheen) | *Fury* (a wound this age gives +30 % burn for the rest of it, red veins)
- 25, phase 2: *Earth-shaker* (new action Crush: land on a settlement, kill its garrison, fear to all neighbours; horns) | *Old Bones* (heal twice as fast asleep, slayers −25 %)
- ranks: +4 % size, +5 health

**Flame** (embers)
- 3: *Hotter Breath* (+25 % burn per stamina, gold fire) | *Long Breath* (burn −25 % stamina)
- 8: *Sea of Fire* (a burn spreads 30 % to the owner's neighbouring regions) | *Clinging Fire* (burned land recovers at half speed, embers ×1.5)
- 15: *White Fire* (wards no longer halve your fire, white fire) | *Molten Gold* (Take from a burning region yields +50 % gold)
- 25, phase 2: *Tower-breaker* (a burn wrecks the region's defences for the rest of the age) | *Dread Flame* (burns frighten witnesses ×1.5 and are remembered 30 % longer)
- ranks: +4 % burn, wider beam

**Arcana** (artifacts)
- 3: *Spell-sense* (spells −25 %) | *Glamour* (half the witnesses see your acts; shadowed wings)
- 8: *Ward-craft* (wards last twice as long; your lair region is warded, slayers −20 %) | *Necromancy* (raise undead at half cost; your dead never march on the lair)
- 15: *Dragon-sight* (see every defence's range and damage before you act; forecast twice as sharp) | *Hex* (a curse also halves the court's anti-dragon progress)
- 25, phase 2: *Arcane Hide* (bolt-throwers and cannons −40 %) | *Sleeper's Ward* (every sleep wards the lair: no sieges, slayers −30 %)
- ranks: +0.2 magic per sleep

**Lore** (books, royals)
- 3: *Old Tongues* (Demand opens at half the knowledge; more envoys) | *Keen Eyes* (an envoy's lie is shown)
- 8: *Star-reading* (forecast twice as sharp, shows the likely hook of the next waking) | *Premonition* (you wake when a slaying is being prepared, and meet the slayers awake)
- 15: *Patient Mind* (non-gold digestion +25 %) | *Teacher* (Teach is free and lasts twice as long)
- 25, phase 2: *Long Memory* (mortals forget you 40 % slower: fear, faith and oaths hold) | *Riddle-master* (hostages yield double Lore; a held hero betrays every slayer plan)
- ranks: +4 % Lore from books

**Dominion** (art, tribute, worship)
- 3: *Tithe* (tribute ×1.5) | *Awe* (faith from protecting and giving ×1.5)
- 8: *Kobold Servants* (thieves fail, hunting feeds +50 %) | *Cult of the Mountain* (your most faithful court keeps faith ≥ 0.5 and sends pilgrims every age)
- 15: *Terror* (fear from your acts ×1.3, decays slower) | *Gilded Throne* (the gold bed gives +25 % Might)
- 25, phase 2: *Dragon-king* (courts under oath never break it from a faded memory) | *Brood of Mortals* (dragon-raised lines fight for you: their armies stand at the lair)
- ranks: +4 % faith gained

## 4. The arms race

### Defences

Every settled region shows its defences, derived from its court each time the world is read. Numbers live in `data/defences.json`.

| defence | when | hurts you | can be seen as |
|---|---|---|---|
| levies | any settlement | 2 per act there, × era | spears around the houses |
| bolt-throwers | era ≥ 2, anti-dragon ≥ 0.2 or slayers | 8 per act, 3 per second flying low inside the range | towers with a range ring |
| slayer band | the court has slayers | 10 per act, comes to the lair as a hook | banner camp |
| wards | elves at era ≥ 2, or a ward spell | your burns there are halved | blue dome |
| cannons | era ≥ 4 | 14 per act, 5 per second in range | squat towers, smoke |

Fear dampens all of them (terrified garrisons flee): damage × (1 − 0.6 × fear). A hero at the court adds 5 at its capital.

### Wounds and healing

Health = 100 + 3 × Might + talents. Wounds stay until you sleep: a sleep heals `years / rest window` of your health, so a hatchling's one-year rest heals everything and an ancient dragon's one-year nap almost nothing (the same rule as stamina). Wounds you carry into a sleep make a slaying easier (success × (1 + wounds share)).

### Death

Health reaching zero ends the game: the court whose defences struck last is named as your killer in the chronicle. A slaying while asleep ends it too. The save is deleted; the only way on is a new world. Save points can come later.

What keeps it fair: listed damage before every act, range rings, a warning when you fly into a tower's range, "slayers forming" and "slayers ready" lines in the briefing, the forecast's slaying line, and the Premonition talent.

## 5. Waking up

The wake screen leads with what changed for you, then the rest:

1. **Hooks**: one to three situations waiting for you this age, each with a place on the map, what resolves it, what it pays, and what happens if you ignore it. Hooks are this age's quests and give the raid a reason.
2. The headline and "while you slept", as in v0.3.
3. Growth: points gained per axis and the talent picks they opened.
4. Envoys.

First hooks (`data/hooks.json`):

| hook | when | answer | pays | if ignored |
|---|---|---|---|---|
| thief in the hoard | a court near the lair with low fear, gold in the hoard, a long sleep | Take or Kill at the thief's region | the gold back ×1.5, fear | the gold is gone, the court gets richer |
| slayers at the gate | a court with slayers | Kill the band at the lair's border (a fight: wounds) | an artifact, fear everywhere | they try to slay you in your next sleep |
| army at the mountain | a court at war with your ally or burned by you, adjacent to the lair | Kill army | food, fear | they burn the lair's slopes (hunting falls) |
| a city burning | a sack this sleep | Take there | double loot | |
| pilgrims | faith ≥ 0.5 | land at the lair and receive them, or eat them | faith pool and Dominion, or food and fear | the faith cools |
| the dead walk | an undead court or a necromancer | Defend or Burn their region | the living's faith | the dead spread |
| a hero's challenge | a court with a bold hero | Kill the hero (a fight) | an artifact, fear | the court gains cohesion and anti-dragon |
| a royal in flight | a capital fell this sleep | Take hostage on the road | a royal and its lineage | the line goes to the conqueror |
| a vein struck | dwarves found riches | Take there | double artifacts | |
| a rival god | a false dragon or a cult of someone else | Burn their temple | faith comes back to you | the cult grows |

**Pacing.** A storyteller (RimWorld's Cassandra is the model: tension builds, a big event, a breather) tracks the years since the last major event. The quieter the valley has been, the likelier a major card or a storyteller event (plague, comet, migration, civil war, a hero rising, the dead walking, an age of magic). Every waking has at least one hook.

## 6. A different world every seed

- **Map:** the twelve-region graph stays, but terrain is dealt per seed (hills, woods, fens, scars and vales in different places) and every region gets a generated name.
- **Start:** a starting situation drawn from a deck: rival human kingdoms, a dwarf empire in decline, an elven realm with human colonists, an orc horde already in the scar, free cities of mixed peoples, the ruins of a fallen kingdom. Three to six courts, with names, traits, wars and vassals.
- **Pre-history:** 300 to 900 years run before the egg hatches (Dwarf Fortress: history before play), so you start among ruins, grudges and successor states. The first screen tells the legends of the valley.
- **Irreversible events:** a race can die out for good; a new people can arrive from beyond the rim; an empire with an immortal ruler can hold for a thousand years; a cataclysm can turn a vale into a scar or a fen into a lake; an age of magic can begin.
- **Undead without you:** a necromancer's tower rises where a waste borders a court of scholars, and the dead of a razed city rise if it stays empty for a century.

## 7. What we measure

The interest tool gains: start variety (distinct starting situations and maps over 300 seeds), outcome entropy on generated worlds, the dragon's death rate per play style, hook coverage (wakings with at least one hook) and the share of hooks answered by the played policy. Targets: entropy ≥ 4.5 bits on generated worlds, no identical first three events in more than 10 % of seeds, played deaths between 5 % and 25 % in 3000 years, idle deaths under 5 %, hooks on 95 % of wakings.

## 8. Rules taken from other games

Sources and numbers in `docs/design-references.md`.

- **Storyteller cycle** (RimWorld): counted in wakings, pressured by quiet years. A major waking, then at least one breather; after three wakings without a major the next sleep forces one. Chance of a storyteller event per sleep = 0.15 + 0.25 × quiet years / phase long window, capped at 0.8. Breather wakings still lead with something, a medium event instead of a catastrophe. No random severity spikes.
- **Slayer warning ladder** (Into the Breach, fair permadeath): forming, ready, attempt, each at least one waking apart and each in the briefing. Metric: deaths with no warning in the previous briefing, target 0.
- **Adaptation** (RimWorld threat points): slayer and siege strength scale with hoard and growth × era × an adaptation factor that drops after a near-death and climbs during a streak of quiet sleeps.
- **The arms race on screen** (Plague Inc's cure bar, XCOM's doom clock): every court's anti-dragon progress is shown, and the briefing reports 25/50/75/95 % ("Dorun's engineers are halfway to bolt-throwers").
- **Numbers before acts**: exact wound, which defences strike, health after; a lethal act needs a second confirm; the death recap lists the three biggest hits with the numbers shown before each.
- **Ranks** (Diablo's paragon, idle games): rank cost grows about ×1.1 per rank if growth income keeps rising; every 5th rank is a bigger visible step.

Later candidates from the same research: a skip on a talent pick that pays a small permanent bonus, duo talents across two axes, a valley-wide doom bar with a joint slaying countdown.

## 9. v0.4 as built (2026-10-19)

Measured on generated worlds, 100 to 300 seeds × 3000 years after the hatching; details in README.md (S1, S2, S3 sections).

- **Growth:** the played dragon has a talent pick after its first moult and on 6 of its first 10 wakings, reaches phase 2 by year 50 and phase 3 by about year 510. With hook rewards, Lore, Arcana and Dominion reach 43, 29 and 22 points per run (10, 6 and 5 without).
- **Arms race:** bolt-throwers are researched from about 80 years after the hatching, slayer bands are founded around year 400 and ready around 620, cannons are researched from about 660 (cannons unlock by anti-dragon research at era 2, not by era 4, because a valley the dragon keeps burning never reaches era 4).
- **Death:** played 21 to 25 % of runs, every slaying warned in the briefing before; idle 0 %; burning everything 96 to 99 %.
- **World:** nine starting situations (none above 16 %), a different map and names every seed, first three events identical in at most 1 %; a race dies out in about 15 %, newcomers arrive in 10 to 20 %, undead rise on their own in about 20 % of idle runs.
- **Wakings:** every waking leads with an event of at least medium importance; hooks on 99 to 100 % of wakings, 1.8 to 2.1 per waking, 17 kinds; the played policy answers 62 %.

## 10. Not yet

Ambitions (Crusader Kings style goals picked at waking), the exits (ascension, portal, the long sleep), dreams that pause the sleep, the Unreal port.
