# Hoard: talents v2

Status 2026-10-24. Revision of the talent list in `core-loop.md` section 3 for three changes since: fear split into dread and awe with memory tiers (`act-api.md` §6), the raid as sorties in a continuous valley with stamina as the clock and travel flight (`raid-design.md`), and Manu's note that stamina consumption, maximum stamina and travel-flight speed and height should be talents. The structure stays: five axes fed by what the dragon slept on, nodes at 3, 8, 15, 25 points (then 40, 60, 85, 115, 150, 200), two options per node, the other gone, major nodes need a phase, a skip pays +5 health, one reroll per moult, ranks after the last node (`design-references.md` §5). Rule for content stays: every talent changes something you see or feel the moment you take it.

## 1. What changed and why

- **Fear talents** (Terror, Dread Flame, Long Memory, Glamour, Dragon-king) are rewritten for dread, awe and the tiers. A talent may raise dread gains, raise awe gains, change decay of one tier, or change where deposits land (a bard talent writes your acts into the written tier of courts without libraries). "Long Memory" as a flat decay modifier is gone; memory is something you cause in the world (libraries, monuments), so the talents that touch it are about deposits, not half-lives.
- **Flight and stamina** become a visible ladder: budget, drain, carry, travel speed, travel altitude, glide. Might carries the body (budget, carry, laden climb, health), Flame the breath's cost and reach, Arcana the sky (travel altitude and the thermals your fires make), Lore the reading of the valley from the air.
- **Raid-shaped effects** replace a few abstract ones: `Crush` stays; `Earth-shaker` becomes a landing act; `Mountain Tail` is a ground attack on garrisons.
- Numbers are start values for the sweep; the interest tool's "pick rate when offered" decides what gets cut.

## 2. The list

Format: node (points, phase if major): *option A* | *option B*. Flight numbers refer to `raid-design.md` (cruise 35, surge 54, travel 80 at 300 m, carry 300, stamina 100 + 20·sqrt(points)).

### Might (gold bed; the body)

- 3: *Thick Hide* (wounds −20 %, darker plates) | *Deep Lungs* (stamina budget +25)
- 8: *Great Wings* (cruise and surge +15 %, bigger wings) | *Stone Grip* (carry +150, visibly larger claws; a hostage weighs 100 instead of 150)
- 15: *Iron Scales* (wounds −30 %, metal sheen) | *Laden Flight* (gold weighs half for climb and stall; the dragon flies level with a full carry)
- 25, phase 2: *Earth-shaker* (new act Crush: land on a settlement, kill its garrison, dread to all neighbours; horns) | *Old Bones* (heal twice as fast asleep; slayer odds −25 %)
- 40: *Long Haul* (surge drains half) | *Mountain Tail* (tail sweep kills a wall's garrison in one pass, 20 stamina)
- 60, phase 3: *Second Wind* (stamina budget +50; one free sortie's worth of acts after the budget is spent, once per waking) | *Unbroken* (wounds carried into a sleep no longer raise slaying odds)
- ranks: +4 % size, +5 health, +5 stamina budget

### Flame (embers; the breath)

- 3: *Hotter Breath* (+25 % burn per stamina, gold fire) | *Long Breath* (burn −25 % stamina)
- 8: *Sea of Fire* (a burn spreads 30 % to the owner's neighbouring regions; the grid's spread up) | *Clinging Fire* (burned land recovers at half speed, embers ×1.5)
- 15: *White Fire* (wards no longer halve your fire; white fire) | *Molten Gold* (Take from a burning region yields +50 % gold; chests spill when their building burns, no clawing)
- 25, phase 2: *Tower-breaker* (a burn wrecks the region's defences for the rest of the age; towers collapse) | *Dread Flame* (dread from burns ×1.5 and the deposit lands one tier higher where a library exists: they write it down)
- 40: *Far Breath* (range 64 → 90 m, visible longer column) | *Cold Lungs* (inhale 0.26 → 0.1 s; the first second of fire costs no stamina)
- 60, phase 3: *Firestorm* (a settlement with ≥ 50 % burning produces a column of fire the dragon can ride: +climb, and dread to every court that sees it) | *Pyre-maker* (burning a temple or memorial converts its monumental stock to dread instead of removing it)
- ranks: +4 % burn, wider beam

### Arcana (artifacts; the sky and the unseen)

- 3: *Spell-sense* (spells −25 % pool) | *Glamour* (half the witnesses see your acts; shadowed wings)
- 8: *Ward-craft* (wards last twice as long; your lair region is warded, slayers −20 %) | *Necromancy* (raise undead at half cost; your dead never march on the lair)
- 15: *Dragon-sight* (every defence's range and damage shown from any altitude; forecast twice as sharp) | *Hex* (a curse also removes 25 % of the court's anti-dragon progress, up to three stacks)
- 25, phase 2: *Arcane Hide* (bolt-throwers and cannons −40 %) | *Sleeper's Ward* (every sleep wards the lair: no sieges, slayers −30 %)
- 40: *Thermal Sense* (travel flight begins at 200 m instead of 300; thermals over sun-heated rock and fires shown as shimmer and ridden for free climb) | *Mortal Guise* (the `guise` act: walk a city unseen, once per age)
- 60, phase 3: *High Air* (travel flight at 500 m and 100 m/s, above era-4 cannons; the valley crossed in 30 s) | *Unseen Passage* (travel flight leaves no `show`; flyovers below 150 m count as `show` 0.05)
- ranks: +0.2 magic per sleep

### Lore (books, royals; the reading of the world)

- 3: *Old Tongues* (Demand opens at half the Lore; more envoys) | *Keen Eyes* (an envoy's lie is shown)
- 8: *Star-reading* (forecast twice as sharp; shows the likely hook of the next waking) | *Premonition* (you wake when a slaying is being prepared and meet the slayers awake)
- 15: *Patient Mind* (non-gold digestion +25 %) | *Teacher* (Teach is free and lasts twice as long)
- 25, phase 2: *Chronicler* (the chronicle level rises by one for every court whose books you hold; the historian lives one phase window longer) | *Riddle-master* (hostages yield double Lore; a held hero betrays every slayer plan)
- 40: *Hawk's Eye* (from travel altitude you read a settlement's defences, arms progress and the court's dread and awe as words; the courts table fills itself on a flyover) | *Reader of Stones* (monuments and memorials show their stored act and bias when flown over; sources in the hoard reveal their stated and true causes)
- 60, phase 3: *Counterfactual* (level-3 chronicle lines at Lore 60 instead of needing the historian too) | *The Long View* (the forecast for the 1000-year bucket is as sharp as for the 100-year one)
- ranks: +4 % Lore from books

### Dominion (art, tribute, worship; what they feel about you)

- 3: *Tithe* (tribute ×1.5) | *Awe* (awe from protecting and giving ×1.5)
- 8: *Kobold Servants* (thieves fail; hunting feeds +50 %; a kobold line can keep the chronicle) | *Cult of the Mountain* (your most faithful court keeps its cult share ≥ 0.05 and sends pilgrims every age)
- 15: *Terror* (dread from your acts ×1.3; oral dread decays at 2/3 speed) | *Gilded Throne* (the gold bed gives +25 % Might)
- 25, phase 2: *Dragon-king* (courts under oath never break it from a faded memory: the oath is written into their chronicle) | *Brood of Mortals* (dragon-raised lines fight for you: their armies stand at the lair)
- 40: *Bards* (your acts deposit into the written tier of every witnessing court, library or not: they sing it) | *Majesty* (a `show` by landing in the square gives awe ×2 and no dread; your size counts as a monument for awe deposits)
- 60, phase 3: *Living Temple* (every court with awe ≥ 0.5 raises a shrine to you within an age; shrines count as monuments) | *Fear of God* (terror miracles also deposit awe; a court that fears and reveres you never founds a slayer order)
- ranks: +4 % awe gained

## 3. The flight ladder, read across axes

A player who wants to fly far and fast takes Great Wings (Might 8), Long Haul (Might 40), Thermal Sense (Arcana 40), High Air (Arcana 60). A player who wants to carry takes Stone Grip and Laden Flight. A player who wants to burn cheaply takes Long Breath, Cold Lungs, Far Breath. Each is felt within the same waking: the HUD shows travel altitude and speed, the stamina bar's drain rate, the carry gauge's cap. The sweep (item 01) should log pick rate when offered per talent; anything under 20 % when offered is rewritten or cut.

## 4. Where this lands

- `data/talents.json` replaces the v0.4 list; item 02 maps the fear talents to dread and awe terms as written here; item 09 lands Mortal Guise and Earth-shaker as acts; item 16 reads the flight numbers from the dragon's talents through the API (`dragon.flight` block: cruise, surge, travel_speed, travel_altitude, carry_cap, stamina_budget, surge_drain, breath_cost), never from constants in the shell.
- The paper playtest's talent offers (Thick Hide | Deep Lungs; Sea of Fire | Clinging Fire; Old Tongues | Keen Eyes; Dragon-sight | Hex; Terror | Gilded Throne) are unchanged, so the three mock screens still hold.
