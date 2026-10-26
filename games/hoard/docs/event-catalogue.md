# Hoard – event catalogue

Status 2026-10-14; implementation note 2026-10-15: fourteen entries are data in `data/events/*.json` (`ages-sim.md` section 18): the grudge, the oath of recovery, the prophecy of the second fire, the dragon-blood dynasty, the hostage who came home, the treasure hunters, the forgotten dragon, the pretender who returns, the slayer order, the false dragon, the vein, the exodus with the return of the exiles, the sack of the capital, old enemies. Companion to `ages-sim.md`. Ninety event archetypes for the between-sleeps simulation, grouped by theme, each expressed in the sim's vocabulary (tags, numbers, modifiers, spawns, acts). Inspiration lines are tags, not citations; nothing here is quoted. Entries are candidates for `data/events/*.json` (section 10), not commitments.

## What the sources share

1. A history feels alive when the same pattern recurs at different scales: a reign rhymes with a dynasty, a dynasty with a kingdom. Rule 6 (collapse) is the big rhyme; this catalogue adds the small ones.
2. Every great event has a long tail. Fëanor's oath drives five centuries; Valyria's Doom shapes Westeros a hundred years later. Hence every entry carries a second-order consequence as a tag a later card reads.
3. Pacing is uneven: centuries of nothing, then one decade in which three things break. Shocks should raise each other's chances rather than arrive at a flat rate.
4. The best events are consequences, not dice. Dwarf Fortress legends read as causal chains; CK3 chains fork on state. Triggers below read state first and roll second.
5. Long-lived witnesses make time felt: an elf who remembers the first king, a hold that still counts a 400-year grudge. Elf `memory_halflife` never and dwarf 500 y are this device; chronicle lines should name the witness where one exists.
6. Objects persist where people do not: a sword, a crown, a cup. Artifacts as objects (v1) carry memory the way factions cannot.
7. Decline is better reading than growth. Rome, Númenor, Valyria, the Dwemer: the fall is where the chronicle earns its lines. Hegemony, decadence, collapse should be a path with stations, not one step.
8. Religion is a political actor: Reformation, Alessian Order, Faith Militant, Sigmar's cult. Faith moves wealth and armies. `piety` and the creed vector are the hooks.
9. The outside intrudes: Sea Peoples, Mongols, Chaos from the north, Easterlings. A closed valley is a terrarium; the sim needs a small notion of outside pressure.
10. Peoples vanish and leave traces: Dwemer, Ayleids, Falmer, Mycenae. Remnants (15.3) are the sim's version; ruin tags on regions extend it.
11. Prophecy is misread. Mortals act on beliefs about the dragon; `fear` and `faith` are beliefs, not facts, and events may set them wrongly on purpose.
12. Heroes compress history: one person unites a race, one betrayal splits it, the grandchildren undo both. The great-ruler shock is the seed; heroes (v1) make it a thread.
13. Small events give texture between the large ones (Uberwald, Dwarf Fortress). Low-surprise filler earns its place by a good chronicle line.
14. Mapping: trigger = predicate on tags and numbers; effect = modifier, spawn, tag or act; the second-order consequence is a tag. No card edits a base rule.
15. Surprise is rated from the player's chair: low = expected texture, medium = changes a plan, high = rewrites the valley.

---

## Succession and politics

### The dance of claimants
Inspiration: House of the Dragon; the Anarchy of 1135
Trigger: mortal succession, `authority` > 0.6, ≥ 3 regions, and a `dragon_raised` lineage exists or a great ruler died this decade
Effect: `civil_war` split into two successors at war, cohesion 0.5 each; growth × 0.6 for 5 decades; the winner gains tag `reunited` (cohesion +0.2)
Chronicle: The realm of X split between two heirs, and for fifty years brother burned brother.
Dragon hook: a `dragon_raised` claimant asks for `defend`; backing one side yields a lineage with faith 0.8 and a loser with a `betrayal` memory
Surprise: medium
Needs: v0.1

### The child king
Inspiration: CK3 regencies; Richard II
Trigger: mortal succession where the previous ruler reigned < 15 years, `authority` > 0.5
Effect: succession reset halved; cohesion_decay × 1.5 for 2 decades; 0.3 chance of tag `regent_usurper` (authority +0.1, piety −0.1)
Chronicle: A boy sat the throne of X and his uncles ruled in his name.
Dragon hook: a regency accepts oaths it would otherwise refuse (tribute offered at faith > 0.4 instead of 0.5)
Surprise: low
Needs: v0.1

### The conqueror's heirs
Inspiration: the Diadochi; Charlemagne's grandsons
Trigger: a great-ruler golden age ends while the faction holds ≥ 40 % of regions
Effect: no succession reset; cohesion −0.2; every region whose `pop` dominant race differs from the faction's splits off as `successor_of`
Chronicle: When the great king died, his generals divided the world he had made.
Dragon hook: a `royals` hostage taken in the golden age is the claimant everyone rallies to; `release` within 2 decades cancels the split
Surprise: medium
Needs: v0.1

### Kinslaying
Inspiration: the Fëanorians at Doriath; the Blackfyres
Trigger: `civil_war` between successors of the same race, both with ruler class longlived or immortal
Effect: winner tagged `kinslayers`: affinity to same-race factions −0.4 for 50 decades; faith −0.2; elven kinslayers get `fading` chance × 2
Chronicle: Elf slew elf at the river ford, and the wood never sang of it.
Dragon hook: `incite` on a civil_war pair causes it; kinslayers accept any oath offered (they need a patron)
Surprise: high
Needs: v0.1

### The council of lords
Inspiration: the Elector Counts of the Empire; the Great Council of 101 AC
Trigger: human faction, `authority` < 0.4, ≥ 4 regions, mortal succession
Effect: tag `elective`: succession reset +0.05 instead of +0.15, no succession crisis ever; cohesion_decay × 0.8; openness +0.1
Chronicle: The lords of X gathered and chose a king from among themselves.
Dragon hook: knowledge ≥ 2 allows `demand` a vote; a dragon-backed candidate tags `dragon_crowned` (faith +0.3, fear +0.2)
Surprise: low
Needs: v0.1

### The general from the marches
Inspiration: Rome's barracks emperors; the Age of Three Emperors
Trigger: cohesion < 0.35, `militarism` > 0.6, faction owns a `marches` region with burn < 0.2
Effect: ruler replaced; tag `military_dynasty`: militarism +0.2, authority +0.2, cohesion +0.25 once; war_chance × 1.5 for 10 decades
Chronicle: A general from the border rode on the capital and took the crown he had defended.
Dragon hook: a `defend` act in the marches raises that region's standing; the dragon can make the usurper
Surprise: medium
Needs: v0.1

### Partition by treaty
Inspiration: Verdun 843; Arnor's three kingdoms
Trigger: ≥ 5 regions, `authority` < 0.5, cohesion in 0.3..0.45, not at war
Effect: peaceful split into 2 or 3 successors (no `civil_war`), mutual affinity 0.7 and alliance for 10 decades; each inherits era
Chronicle: The sons of X divided the kingdom at the river, and it was never whole again.
Dragon hook: an oath with the parent passes only to the successor bordering the lair; the others owe nothing
Surprise: low
Needs: v0.1

### The barons' plot
Inspiration: CK3 faction plots; Magna Carta
Trigger: `authority` > 0.7, cohesion < 0.5, wealth in the valley's top third
Effect: 0.5: ruler dies early (succession now, crisis chance × 2); 0.5: authority −0.3 and tag `charter` (cohesion_decay × 0.85, era_rate +0.1, permanent)
Chronicle: The barons of X forced their king to swear limits on his power.
Dragon hook: `curse lineage` doubles the plot chance; `give` to the ruler cancels it this decade
Surprise: medium
Needs: v0.1

### The marriage of two crowns
Inspiration: Castile and Aragon; Targaryen and Velaryon
Trigger: adjacent factions, affinity > 0.6, both mortal rulers, neither at war
Effect: smaller becomes `vassal_of` the larger (0.05/decade); after 10 decades merges (pop blend, cohesion −0.1); mixed-race case tags `half_blood_line` (lifespan × 1.5)
Chronicle: The heir of X wed the queen of Y, and two realms became one.
Dragon hook: a `royals` hostage from either side blocks the match; `give` the hostage back arranges it
Surprise: low
Needs: v0.1

### The pretender who returns
Inspiration: False Dmitry; the Blackfyre pretenders
Trigger: faction tagged `successor_of:X` where X's royal is in the dragon's hoard or a `dragon_raised` lineage of X exists
Effect: spawn claimant: war between the successors (0.3); the winner tags `restored_line`, cohesion +0.2
Chronicle: A man claiming the blood of the old kings landed in the marches with a hired host.
Dragon hook: the hoarded royal is the pretender's proof; `release` of a `dragon_raised` heir makes the claim true (new line faith 0.6)
Surprise: high
Needs: v1 heroes

## War and conquest

### The long siege
Inspiration: Gondolin; Dwarf Fortress sieges
Trigger: war where the defender's region has `dug_in`, or era ≥ 2 and `walls`
Effect: war may run 10 decades; attacker wealth −5 %/decade extra; defender cohesion +0.1 if it holds; attacker anti_dragon +0.05 (siege engineering)
Chronicle: For a hundred years the host of X sat before the gates of Y and could not enter.
Dragon hook: `defend` ends it one way, `burn` on the walled region the other; either becomes a legend
Surprise: low
Needs: v0

### Mercenaries change sides
Inspiration: the Golden Company; Gothic federates
Trigger: faction at war ≥ 3 decades with cohesion < 0.4 and an allied orc faction
Effect: the orcs switch `at_war_with` to their employer and plunder one region; employer tagged `betrayed_by_hirelings` (affinity to orcs −0.5)
Chronicle: The hired orcs of X turned on their paymasters and sacked the treasury they guarded.
Dragon hook: `incite` on the orcs triggers it; a `show` to the orcs instead raises their fear and holds them
Surprise: medium
Needs: v0.1

### The league that outlives its enemy
Inspiration: the Last Alliance; the Delian League
Trigger: coalition war won with ≥ 3 members, hegemon collapsed
Effect: members gain mutual affinity +0.3 for 20 decades and tag `league`; the strongest member counts the others' power at 0.3, becoming the next hegemony candidate
Chronicle: The league that broke X held together, and in time its captain was a king.
Dragon hook: a `defend` act in that war puts the dragon in the league: faith +0.2 in every member
Surprise: medium
Needs: v0.1

### Scorched earth
Inspiration: Dagor Bragollach; the retreat of 1812
Trigger: defender lost a region in the last 2 decades, `militarism` > 0.6, `authority` > 0.6
Effect: defender burns its own border region (burn +0.5); attacker gains nothing there; defender cohesion −0.1; the region may become `wasteland` and later an orc spawn site
Chronicle: Rather than yield the vale, the lords of X set fire to it.
Dragon hook: the dragon profits twice: burned land for orcs it can `incite`, and a faction that now fears fire (fear +0.1)
Surprise: medium
Needs: v0.1

### The invasion from beyond the passes
Inspiration: the Sea Peoples; the Wainriders; the Mongols
Trigger: valley wealth above a lure threshold, no edge faction with `militarism` > 0.5, 0.01/decade; needs new state: an outside-pressure counter
Effect: spawn faction (human or orc) on an edge region with power 1.5 × strongest neighbour, tag `invader`, at war with all neighbours 5 decades, then settles
Chronicle: From beyond the eastern passes came a people nobody had a name for.
Dragon hook: `defend` against invaders earns faith in every witness; or let them break the hegemon
Surprise: high
Needs: v1 lure

### The sack of the capital
Inspiration: Rome 410; the sack of King's Landing
Trigger: war in which the loser's richest region is taken
Effect: loser wealth −30 % instead of −10 %; winner gains half of it and, if loser era ≥ 3, era progress +20; region burn +0.3; loser tagged `sacked` (authority +0.1, piety +0.1, 5 decades)
Chronicle: The host of X entered the golden city of Y and carried off everything that could be lifted.
Dragon hook: a `take` before the sack leaves less for the sacker; or follow the sackers and take from them
Surprise: low
Needs: v0

### Vassal revolt
Inspiration: CK3 liberty factions; the Dornish revolts
Trigger: vassal cohesion > 0.6, vassal wealth ≥ 0.5 × overlord's, overlord at war elsewhere
Effect: independence chance 0.4 this decade regardless of overlord cohesion; war between them; tag `rebel_born` (militarism +0.1)
Chronicle: While X fought in the north, its vassal Y refused tribute and raised banners.
Dragon hook: a `defend` oath with the overlord obliges intervention against its own vassal; sleeping through it breaches
Surprise: low
Needs: v0.1

### The crusade against the heretic neighbour
Inspiration: the Albigensian Crusade; the Alessian Order
Trigger: faction A `piety` > 0.7, adjacent B with different dominant creed or tag `sacrifice_cult`, affinity < 0
Effect: war without the power-ratio requirement; A cohesion +0.15 for its duration; winner gains `crusader`; loser's faith → 0.3
Chronicle: The priests of X declared the altars of Y abominable, and the swords followed the priests.
Dragon hook: a dragon-worshipping faction attracts crusades; `defend`ing it makes a holy war, fear +0.3 across the valley
Surprise: medium
Needs: v1 creed

### Old enemies
Inspiration: the Hundred Years' War; Bretonnia and the Empire
Trigger: two factions at war three times within 30 decades, power ratio within 1.2
Effect: paired tag `old_enemies`: war_chance × 2 between them, affinity −0.3; both militarism +0.1; both era progress +5 per war (arms race)
Chronicle: X and Y had fought so long that neither remembered the cause.
Dragon hook: a `defend` for one side breaks the deadlock; selling the same oath to both is a betrayal waiting to fire
Surprise: low
Needs: v0.1

## Religion and cults

### The reformation
Inspiration: Luther; the Sigmarite schisms
Trigger: era ≥ 3, `piety` > 0.5, `openness` > 0.5, faith (dragon) < 0.3
Effect: 0.4: `civil_war` along piety lines; else tag `reformed` (piety −0.2, openness +0.2, era_rate +0.1); pious neighbours (piety > 0.6) affinity −0.3 toward it
Chronicle: A monk of X nailed his complaints to the temple door and half the realm agreed with him.
Dragon hook: reformed factions forget the dragon faster (memory_halflife × 0.7); `destroy_altar` there barely registers
Surprise: medium
Needs: v0.1

### The god that walks
Inspiration: Sigmar's apotheosis; Talos
Trigger: great ruler with `piety` > 0.7 dies; faction faith (dragon) < 0.2
Effect: tag `deified_king`: piety +0.2, cohesion_decay × 0.8 for 30 decades; faith gained from dragon acts × 0.5 (a rival god holds the altars)
Chronicle: They buried the old king and prayed to him by spring.
Dragon hook: `destroy_altar` triggers a crusade; a `miracle` that outdoes the dead king flips worship (faith +0.4)
Surprise: medium
Needs: v1 creed

### The sacrifice cult
Inspiration: Dwarf Fortress cults; the Drowned God
Trigger: fear > 0.7, faith > 0.5, dominant creed destroyer
Effect: tag `sacrifice_cult`: faith +0.05/decade, cohesion −0.03/decade, faith_pool +0.2/decade; pious neighbours crusade 0.1/decade; after 10 decades 0.3 chance of `civil_war`
Chronicle: In X they began to leave the firstborn on the mountain road.
Dragon hook: `accept_offering` legitimises it (cohesion loss halved); `destroy_altar` ends it (faith −0.3, fear −0.2, "the dragon refused")
Surprise: medium
Needs: v1 creed

### The iconoclasm
Inspiration: Byzantine iconoclasm; the Dissolution of the Monasteries
Trigger: `piety` > 0.6, `authority` > 0.6, era ≥ 2
Effect: wealth −10 %; authority +0.1; loot profile `art` × 0.3 for 10 decades
Chronicle: The priests of X smashed every image older than their god.
Dragon hook: a `take` of art beforehand saves it, and the chronicle credits the dragon with preserving what the mortals destroyed
Surprise: low
Needs: v0.1

### The schism of the mountain cult
Inspiration: the Great Schism; CK3 heresies
Trigger: ≥ 2 factions with faith > 0.6 and different dominant creeds (protector vs destroyer)
Effect: affinity between them −0.5; tags `orthodox` / `heterodox`; war_chance × 1.5 between them; the loser's faith → 0.3
Chronicle: The worshippers of the mountain split over whether it loved them or merely fed on them.
Dragon hook: a `show` to one side (dream, v1) settles it; sleeping through loses half the worshippers
Surprise: medium
Needs: v1 creed

### The copying brothers
Inspiration: Irish monasteries; the Citadel of Oldtown
Trigger: `piety` > 0.5, `openness` > 0.5, era ≥ 2, 5 decades of peace
Effect: region tag `library`: books loot × 2; era diffusion from this region × 1.5; the tag survives collapse and conquest
Chronicle: The brothers of X copied every book that reached the valley.
Dragon hook: the richest `books` target; burning it costs every neighbour era_rate × 0.9 for 10 decades and is remembered as the great loss
Surprise: low
Needs: v0.1

### The millenarian panic
Inspiration: the flagellants of 1348; the year 1000
Trigger: plague shock and a witnessed dragon act within 3 decades; `piety` > 0.5
Effect: tag `millenarian`: faith +0.3 (destroyer), growth × 0.7 for 3 decades, cohesion −0.1; resolves into `sacrifice_cult` (0.3), `reformed` (0.3) or nothing
Chronicle: They said the fire and the fever were one judgement, and sold their fields to await it.
Dragon hook: cheapest faith in the game; `turn plague` here converts the whole faction to protector creed
Surprise: medium
Needs: v1 creed

### The holy war against the dead
Inspiration: the Vampire Wars; the Blights of Dragon Age
Trigger: an `undead` faction holds ≥ 2 regions
Effect: living neighbours gain mutual affinity +0.3 and form a coalition without the hegemon condition; piety +0.1 everywhere; the winner tags `crusader`
Chronicle: The dead marched out of the fen, and for once the living forgot their quarrels.
Dragon hook: a `raise undead host` creates the enemy that unites the valley; every crusader gains anti_dragon +0.1 afterwards
Surprise: high
Needs: v1 world magic

## Magic and the arcane

### The necromancer's tower
Inspiration: Dwarf Fortress necromancer towers; Dol Guldur
Trigger: world.magic > 0.4, an ownerless region with burn > 0.5 or `wasteland`, a `library` or era-4 faction within 2 regions
Effect: spawn faction race `undead`, 1 region, immortal ruler, growth 0, power from world.magic, grows by conquest; tag `tower`
Chronicle: A scholar of X went into the waste and the dead began to walk out of it.
Dragon hook: a stolen hoard book seeds it; one `kill` on the necromancer ends the faction and earns faith everywhere
Surprise: high
Needs: v1 world magic

### The colleges of magic
Inspiration: the Colleges of Altdorf; the Valyrian dragonlords
Trigger: era 4 and world.magic > 0.5
Effect: tag `archmages`: anti_dragon rate × 1.5; home region invasion_chance × 0.5; world.magic +0.02/decade; 0.02/decade `arcane_disaster` (region → `scar`, wealth −40 %)
Chronicle: The towers of X glowed at night and the neighbours stopped sleeping.
Dragon hook: richest `artifacts` source; their wards resist the dragon; they may offer `teach` for a no_harm clause
Surprise: medium
Needs: v1 world magic

### The gunpowder turn
Inspiration: real history; the Imperial Engineers' School
Trigger: faction enters era 3 with `militarism` > 0.6 and `openness` > 0.5
Effect: tag `powder`: world.magic −0.03/decade while any such faction exists; anti_dragon +0.05 once; adjacent elves affinity −0.2
Chronicle: The first cannon in the valley was fired at a wall. The second was aimed higher.
Dragon hook: the strongest anti-dragon driver; `burn` the foundry region (era progress −20) or `ward` the lair
Surprise: low
Needs: v1 world magic

### The theft from the hoard
Inspiration: Dwarf Fortress artifact thefts; the cup taken from Smaug
Trigger: hoard artifacts ≥ 3, dragon asleep ≥ 20 decades, adjacent faction with era ≥ 2 and cohesion > 0.6
Effect: one artifact moves to the faction; anti_dragon +0.15, faith −0.1, fear +0.1; a slay attempt with it has success × 1.5; it can be traded on
Chronicle: A thief of X came out of the mountain with a cup, and the city has never been quieter.
Dragon hook: a dream wake with `rage`; or let it go and follow where it is traded
Surprise: high
Needs: v1 artifacts, lure

### The binding
Inspiration: Earthsea's true names; the Circle of Magi
Trigger: `archmages`, ≥ 3 `seen` memories of the dragon, anti_dragon ≥ 0.8
Effect: replaces the slay attempt: success gives dragon tag `bound` (no acts in their regions for one age) and gold −10 % to them; failure resolves as a failed slaying
Chronicle: The mages of X went up the mountain with a name, and came down with a treaty.
Dragon hook: dragon knowledge > 1.5 × their era defeats it; every `books` gift to a faction raises this risk
Surprise: high
Needs: v1 world magic

### The wild surge
Inspiration: the Winds of Magic; Fade rifts
Trigger: world.magic > 0.7 for 5 decades
Effect: a random region gains `scar` (land × 0.5, permanent) or `fey` (land × 1.3, elf affinity +0.2); world.magic −0.1
Chronicle: The sky over the fen turned the wrong colour for a season, and the fen was not the same after.
Dragon hook: the dragon's own spells push magic up; a surge on the lair region costs size −5 %
Surprise: medium
Needs: v1 world magic

### The scholars who vanished
Inspiration: the Dwemer; the Tower of Babel
Trigger: era 4 with `archmages`, `authority` > 0.7, `openness` < 0.3, 10 decades of peace; 0.01/decade
Effect: wealth −50 %; capital region tagged `vanished_halls` (artifacts loot × 3 for whoever holds it); the race's pop share there goes to its remnant
Chronicle: The scholars of X attempted something, and in the morning their halls were empty.
Dragon hook: ruins to `take` from without a fight; the dragon's `teach` act brings the trigger closer
Surprise: high
Needs: v1 world magic, artifacts

## Plague, famine and the land

### The great plague
Inspiration: the Black Death; the Great Spring Sickness
Trigger: any faction at era ≥ 2, valley wealth above a threshold; 0.003/decade, world-wide
Effect: all factions wealth −35 %, cohesion −0.15 (elves and dwarves −10 % only); 2 decades later each faction abandons a border region with 0.2; human growth × 1.3 for 5 decades
Chronicle: The sickness came up the river and in three years a third of the valley was in the ground.
Dragon hook: `turn plague` on one faction leaves it the only healthy kingdom: faith 0.9 and the next century's hegemon
Surprise: high
Needs: v0.1

### The empty land
Inspiration: post-plague Europe; Nurn
Trigger: region ownerless ≥ 3 decades by abandonment (not burn), adjacent to an orc faction or orc remnant > 0.1
Effect: orcs settle it (new faction or region) without `hardened`; tag `orc_farm`: that faction gains growth 0.005
Chronicle: Where the plague had emptied the vale, the orcs put up tents and, in time, fences.
Dragon hook: orcs with wealth are a new loot source; `incite` them at the weakened neighbour finishes the collapse
Surprise: medium
Needs: v0.1

### The long winter
Inspiration: Fimbulwinter; the Long Night; 536 AD
Trigger: world tag `volcanic_winter`, or bad century on ≥ 3 regions at once
Effect: growth × 0.3 everywhere for 5 decades; orc war_chance × 2; dwarves exempt (stores) and gain affinity +0.2 from neighbours they feed
Chronicle: The snow did not leave the vale for seven years, and the dwarves sold grain for gold.
Dragon hook: `bless region` during famine gives the best faith per faith_pool in the game; `mega_raze` is the usual cause
Surprise: medium
Needs: v0.1

### The river moves
Inspiration: the Yellow River; Dwarf Fortress terrain change
Trigger: `vale` or `fen` region, 0.002/decade
Effect: region land −0.2, one neighbour land +0.2, permanent; a faction losing its only vale takes cohesion −0.1
Chronicle: The river left X's fields and found Y's, and the border treaties became ink.
Dragon hook: `terraform (lake)` is the dragon's version; if the dragon acted this sleep the mortals blame it anyway (fear +0.1)
Surprise: low
Needs: v0.1

### The blight on the wood
Inspiration: the Blight; Mirkwood's shadow
Trigger: `wood` region with burn > 0.3 or adjacent to `wasteland` / `undead`; world.magic > 0.3
Effect: region tag `blighted`: land × 0.5, elf growth × 0.5, fading chance × 2; spreads to adjacent wood 0.05/decade; cured by `bless region` or 30 decades
Chronicle: The leaves of the old wood turned black and did not fall.
Dragon hook: the dragon's fire seeds it; curing it makes the elves teach (world.magic +0.05, dragon magic up)
Surprise: medium
Needs: v1 world magic

### The vein
Inspiration: Moria's mithril; Dwarf Fortress strikes
Trigger: `hills` region with dwarf owner, 0.005/decade
Effect: region land +0.3 for 20 decades; dwarf growth × 1.5; every human neighbour's war_chance toward it × 1.5; dwarf authority +0.1
Chronicle: The dwarves of X struck a vein, and every king in the valley remembered an old claim on the hills.
Dragon hook: the best gold for a century; `incite` humans at the dwarves, then `take` from the winner
Surprise: low
Needs: v0.1

### The water war
Inspiration: the Bronze Age collapse; Mesopotamian canal wars
Trigger: bad century on a `vale` region bordering another faction's `vale`
Effect: war_chance × 3 between the two vale owners for its duration, no power-ratio requirement
Chronicle: When the rains failed, X and Y fought over a ditch.
Dragon hook: `terraform (lake)` ends it; the beneficiary offers tribute
Surprise: low
Needs: v0.1

## Migrations and lost peoples

### The exodus
Inspiration: the Edain entering Beleriand; the Rhoynar
Trigger: faction destroyed by orcs (15.2) or vassalised while cohesion > 0.7
Effect: pop 0.4 (not 0.1) moves to the highest-affinity neighbour; recipient tagged `host_of:<race>`: openness +0.1, cohesion −0.1, era diffusion from the refugees if theirs is higher
Chronicle: The people of X crossed the hills with what they could carry and asked Y for land.
Dragon hook: `give` an ownerless region to the refugees: a new faction with faith 0.7 and tag `dragon_settled`
Surprise: medium
Needs: v0.1

### The return of the exiles
Inspiration: the Noldor's return; Aegon's landing
Trigger: remnant strength > 0.3 and the race's old home region held by another race with cohesion < 0.4
Effect: the remnant re-founds by war: spawn faction with power 20 × strength, at war with the holder, tags `returned`, `restoration_war`
Chronicle: The ships of the old people came back, and they had not come to ask.
Dragon hook: `sponsor_remnant` picks the moment; the returned swear any oath offered (starting faith 0.5)
Surprise: high
Needs: v0.1

### The hidden valley
Inspiration: Gondolin; Rivendell
Trigger: elf or dwarf faction with 1 region, cohesion > 0.7, region `warded` or `dug_in`
Effect: region tag `hidden`: invasion_chance × 0.1, growth × 0.7, era diffusion 0 both ways; removed by a `show` act or a kinslaying
Chronicle: Nobody could say where the last elves had gone, and the maps stopped trying.
Dragon hook: `show` reveals it to the valley (fear +0.3, `betrayal` if an oath exists); keeping it hidden keeps its books for later
Surprise: medium
Needs: v0.1

### The confederation of the scar
Inspiration: the Mongols; the Dothraki
Trigger: ≥ 2 adjacent orc factions whose warlords both reigned < 10 years
Effect: merge into one faction, cohesion 0.8, militarism 1.0, war_appetite × 2 for one warlord's life; the normal warlord collapse then splits it
Chronicle: The khans of the scar swore to one of their number, and the valley learned his name quickly.
Dragon hook: `incite` it at a hegemon; it offers captives for tribute after a `show` (fear exceeds faith)
Surprise: high
Needs: v0.1

### The ships leave
Inspiration: the elves sailing West; the Aldmeri exodus
Trigger: `fading` active 20 decades, elf cohesion > 0.5
Effect: all regions ownerless with `elf_withdrawn`; remnant +0.3; world.magic −0.1; any `library` tag stays for whoever comes
Chronicle: The last ship left the wood-harbour, and the humans moved in the next spring.
Dragon hook: an oath (no_harm plus teach) delays it 20 decades; their departure drops world.magic and with it the portal path
Surprise: medium
Needs: v0.1

### The lost colony
Inspiration: Roanoke; Númenórean colonies gone native
Trigger: faction owns a region not adjacent to its others and the connecting ally collapses
Effect: the region becomes a faction with pop blended from neighbours, era −1, tag `colony_of:<id>`, affinity 0.5 to the parent
Chronicle: Word stopped coming from the far settlement, and when it came again it was in a different tongue.
Dragon hook: the colony knows the dragon from stories only: fear and faith halved; the first `show` there is a clean slate
Surprise: low
Needs: v0.1

### The people who went under the hill
Inspiration: the Falmer under Dwemer rule; the Petty-dwarves
Trigger: refugees of race human or elf arrive at a dwarf faction with `authority` > 0.7
Effect: dwarves tagged `thralls`: growth × 1.2, cohesion −0.1, affinity to that race −0.4; after 30 decades the thrall pop may emerge as a `hardened` `corrupted` faction (civil_war, 0.2)
Chronicle: The elves who fled to the hill hold were given shelter, then lamps, then chains.
Dragon hook: `take_hostage` of the thralls' last royal makes the dragon their saviour in legend (faith 0.6 on emergence)
Surprise: high
Needs: v0.1

## Race-specific

### Humans

### The conquest of the seven crowns
Inspiration: Aegon's Conquest; Æthelstan
Trigger: human great-ruler golden age, `militarism` > 0.6, ≥ 2 human neighbours with cohesion < 0.5
Effect: war_chance × 3 against human neighbours for the lifespan; victories vassalise instead of annexing; at death vassals stay if authority > 0.6, else revolt
Chronicle: In one lifetime X took five crowns and wore them as one.
Dragon hook: the conqueror offers tribute for `defend`; a dragon-backed conquest founds a `dragon_crowned` dynasty (faith 0.8)
Surprise: medium
Needs: v0.1

### The merchant republic
Inspiration: Venice; Braavos; Marienburg
Trigger: human faction owning a `lake` or `fen` region, `openness` > 0.6, `authority` < 0.4, era ≥ 2
Effect: tag `republic`: growth × 1.3, militarism −0.2, ruler class longlived (no succession); gold loot × 1.5; era diffusion from it × 1.5
Chronicle: X stopped having kings and started having ledgers.
Dragon hook: the richest gold behind the weakest walls; republics pay tribute at fear > 0.5 regardless of faith
Surprise: low
Needs: v0.1

### The peasants' war
Inspiration: CK3 peasant revolts; the German Peasants' War
Trigger: human faction, `authority` > 0.7, negative wealth growth 2 decades, `piety` > 0.5
Effect: cohesion −0.2; 0.3: authority −0.3 and `charter`; 0.7: authority +0.1, militarism +0.1; one region burn +0.1
Chronicle: The farmers of X marched on the castle with scythes and a preacher.
Dragon hook: a `burn` on a high-authority faction fires this the next decade; `give` food during it earns faith +0.3
Surprise: low
Needs: v0.1

### The half-elven line
Inspiration: Elros's line; the Dúnedain
Trigger: human faction allied (affinity > 0.6) with elves for 10 decades while era diffusion from them is active
Effect: tag `half_elven_line`: ruler class longlived, era_rate +0.2, world.magic +0.01/decade; affinity to other humans −0.1; after 50 decades 0.3 chance of `hubris` (authority +0.3)
Chronicle: The king of X took an elven wife, and his heirs lived long enough to regret their neighbours.
Dragon hook: the longest-lived mortal line to take hostages from; a `dragon_raised` half-elf is a near-immortal ally
Surprise: medium
Needs: v0.1

### The drowned kingdom
Inspiration: the Downfall of Númenor; the Doom of Valyria
Trigger: `hubris` or hegemony ≥ 50 % of regions, era 4, faith < 0.2, anti_dragon > 0.5
Effect: a full-army slay attempt or a world-magic rite; on failure the capital becomes `lake` or `scar`, wealth −70 %, successors tagged `fallen_line` (cohesion 0.4); world.magic −0.2
Chronicle: The kings of X sailed against the mountain itself, and the sea took the fleet and the harbour.
Dragon hook: the dragon is the target; a `show` with knowledge ≥ 3 may avert it (fear +0.3, authority −0.1)
Surprise: high
Needs: v1 world magic

### The castle century
Inspiration: Norman England; Harrenhal
Trigger: human faction enters era 2 with fear > 0.3
Effect: tag `walls` on every owned region over 5 decades; invasion_chance × 0.6; any `no_walls` oath breaches with stakes; anti_dragon +0.1
Chronicle: X built a castle on every hill, and one of them was pointed at the sky.
Dragon hook: a `no_walls` oath sworn in era 1 will be broken by era 2 as fear fades; the dragon decides whether to enforce
Surprise: low
Needs: v0

### Dwarves

### The grudge
Inspiration: the Great Book of Grudges; Thorin's claim
Trigger: a dwarf faction loses a region or suffers a `take`
Effect: tag `grudge:<actor>`, 500-year half-life: affinity to actor −0.5; war_chance toward actor × 2 ignoring power ratio once per 20 decades; settled by region returned or `give` ≥ 0.3 (cohesion +0.2)
Chronicle: The dwarves wrote the name of X in the book, and the book is long.
Dragon hook: a `take` from dwarves is the longest consequence in the game (anti_dragon floor 0.3 while it holds); `give` it back and they dig the lair
Surprise: medium
Needs: v0.1

### Dug too deep
Inspiration: Durin's Bane; Dwarf Fortress's deepest layers
Trigger: dwarf faction in a `vein` event or wealth > 2 × any neighbour, `authority` > 0.6; 0.02/decade
Effect: region tag `awakened_deep`: spawn hostile faction race `deep_thing` (1 region, immortal, growth 0) taking the hold; dwarves become refugees; the hold keeps artifacts loot × 3 for whoever retakes it
Chronicle: The dwarves of X dug beneath the deep hold and found that something was already there.
Dragon hook: `kill` the deep thing: the dwarves' highest faith event (0.8) and the hoard's best artifact haul
Surprise: high
Needs: v0.1, v1 artifacts

### The engineers' guild
Inspiration: the Dwarf Engineers' Guild; Erebor's defences
Trigger: dwarf faction at war or holding `grudge:dragon`, era ≥ 2
Effect: anti_dragon rate × 2; `dug_in` permanent on home regions; adjacent allies gain anti_dragon +0.02/decade (exports)
Chronicle: The engineers of X built a bolt-thrower that could reach the clouds, and sold the plans.
Dragon hook: `take` the siege works (payload artifacts) sets anti_dragon −0.3 in every faction that bought them
Surprise: low
Needs: v0.1

### The hold without a hall
Inspiration: Thorin's company; the end of Belegost
Trigger: dwarf faction vassalised or holding no `hills` region
Effect: tag `wandering_hold`: growth × 0.5, cohesion_decay × 0.5, remnant +0.2; 0.05/decade a `restoration_war` for the nearest hills region
Chronicle: The dwarves of X lived in human towns and worked human iron and spoke of the hold every evening.
Dragon hook: `sponsor_remnant` or `give` of the hold's region makes the dragon their founding legend: they dig the lair permanently
Surprise: medium
Needs: v0.1

### The dwarves of the scar
Inspiration: the Chaos Dwarfs; the Petty-dwarves
Trigger: dwarf faction holding a `scar` or `wasteland` region for 30 decades (`hardened`), and destroyer faith > 0.5 or an `undead` neighbour
Effect: culture mutation `corrupted`: affinity to dwarves −0.8, to orcs +0.5; cohesion_decay × 0.5; `thralls`; loot profile gains captives
Chronicle: The dwarves of the scar stopped carving their ancestors and started carving something else.
Dragon hook: `corrupt` does it directly; corrupted dwarves deepen the lair faster and offer sacrifices
Surprise: high
Needs: v1 culture mutation, creed

### The moot of the holds
Inspiration: the election of the High King; Durin's returns
Trigger: dwarf longlived succession while ≥ 2 dwarf factions exist
Effect: 0.3: the new king is acknowledged High King (dwarf-dwarf affinity +0.3, alliance chance × 2); 0.1: disputed, a 20-decade `grudge` between holds
Chronicle: The lords of the holds met under the mountain and, after a decade, agreed on a king.
Dragon hook: a `dragon_raised` dwarf is never acknowledged unless the dragon has `give`n ≥ 0.5 to every hold
Surprise: low
Needs: v0.1

### Elves

### The oath of recovery
Inspiration: the Oath of Fëanor
Trigger: elf faction suffers a `take` of `art` or `artifacts` by any actor; ruler longlived or immortal
Effect: tag `sworn:<actor>`: war_chance toward actor × 3, affinity −0.9, cohesion +0.2 and no `fading` while the object is unrecovered; on recovery cohesion −0.3 (the oath's end undoes them)
Chronicle: The elves of the old wood swore to recover what was taken, and the witnesses were afraid.
Dragon hook: the most dangerous theft in the game: a faction that never forgets, with a target; `give` it back is the only release
Surprise: high
Needs: v1 artifacts

### The fading begins
Inspiration: Tolkien's dominion of Men; Ayleid decline
Trigger: 15.5 condition met for the first time
Effect: as 15.5, plus a chronicle marker and openness +0.05 on every human neighbour (they are inheriting a world)
Chronicle: The elves of X began to speak of the valley in the past tense.
Dragon hook: an alliance (no_harm plus teach) slows it; every book taken from fading elves is one the humans never get
Surprise: low
Needs: v0.1

### The sundering of the wood
Inspiration: Malekith's Druchii; the Dalish
Trigger: elf `civil_war` or `kinslayers`, the loser holds a non-wood or `scar` region
Effect: loser's culture mutates `corrupted`: war_appetite 1.5, growth 0.012, affinity to elves −0.9, lifespan kept; loot profile captives and artifacts; creed → destroyer
Chronicle: The elves who lost the war of the wood went east, and what came back wore the same faces.
Dragon hook: the natural ally against elven kingdoms; `corrupt` causes it; dark elves pay in sacrifice and artifacts for `defend`
Surprise: high
Needs: v1 culture mutation

### The teaching
Inspiration: the Eldar and the Edain; Ayleids and Nedes
Trigger: elf affinity > 0.5 to an adjacent human or dwarf faction, elf cohesion > 0.6, `openness` > 0.4
Effect: pupil era_rate × 1.3 for 20 decades, openness +0.1; world.magic +0.01/decade; elves tagged `teachers` (fading chance × 1.3 later)
Chronicle: The elves taught the men of X letters, then stars, then regretted the third.
Dragon hook: the pupil becomes the anti-dragon line's best candidate; the dragon's own `teach` competes and yields faith instead
Surprise: low
Needs: v0.1

### The queen who does not die
Inspiration: Galadriel; the Everqueen
Trigger: elf great ruler with `piety` > 0.5
Effect: ruler class immortal: cohesion_decay × 0.1, era_rate × 0.5; human neighbours' affinity −0.01/decade (envy); ends by assassination or crusade card
Chronicle: The queen of the wood had ruled for six hundred years and did not look it.
Dragon hook: she remembers every act personally (no memory decay while she reigns); `take_hostage` her and the whole race's fear → 0.9 with a `sworn` oath
Surprise: medium
Needs: v0.1, v1 heroes for the name

### The closed wood
Inspiration: Thranduil's realm; Summerset's isolation
Trigger: elf `openness` < 0.3 and a human neighbour tagged `powder`
Effect: elf regions `warded` (invasion_chance × 0.3), era diffusion 0 both ways; world.magic does not decline inside them; elf growth × 0.8
Chronicle: The wood closed its borders when it smelled powder.
Dragon hook: the last `books` after the humans burn theirs; a `teach` oath from them is the dragon's late knowledge route
Surprise: low
Needs: v1 world magic

### Orcs

### The warlord's table
Inspiration: Attila's death; Dwarf Fortress goblin civil wars
Trigger: orc ruler death (every ~40 y)
Effect: as rule 3, plus 0.3: split into two factions with `civil_war`; 0.1: a successor with cohesion 0.8 (great warlord)
Chronicle: The warlord died at table, and by morning there were three warlords.
Dragon hook: a `show` within 2 decades of the death crowns the next warlord: fear 0.8, tribute oath in captives available
Surprise: low
Needs: v0

### The horde comes down
Inspiration: the Waaagh!; Azog's host
Trigger: orc wealth > 2 × orc starting wealth, warlord reigned > 20 years, `militarism` > 0.8
Effect: war on the richest neighbour ignoring ratio; war_take_chance × 2 and power × 1.5 for 3 decades; then cohesion −0.4 whatever the outcome
Chronicle: The scar emptied and the whole of it came down the pass singing.
Dragon hook: `incite` aims it; `defend` the target for massive faith, or follow the horde and `take` from the sacked (orcs forget in 50 y)
Surprise: medium
Needs: v0.1

### Orcs learn to farm
Inspiration: Nurn; the Orsimer strongholds
Trigger: orc faction holds a `vale` region 20 decades without war
Effect: tag `settled_orcs`: growth 0.01, war_appetite 1.2, cohesion_decay 0.02, ruler class mortal; affinity to humans −0.4; other orc factions −0.5 toward it
Chronicle: The orcs of the lower vale brought in a harvest and did not know what to do with it.
Dragon hook: orcs with gold pay tribute; `incite` the wild orcs against the settled ones keeps the race wild
Surprise: medium
Needs: v0.1

### The mixed horde
Inspiration: the Orsimer; the Uruk-hai
Trigger: orc faction took a region with human pop share > 0.3, warlord reigned > 20 years
Effect: pop +0.3 human; tag `mixed_horde`: lifespan 50, growth 0.01, cohesion_decay 0.03, ruler class mortal with succession resets, war_appetite 1.8
Chronicle: The warlord's heir had his mother's eyes and his father's appetite.
Dragon hook: a horde that outlives one warlord is a real power; `take_hostage` the heir and it serves the dragon for an age
Surprise: medium
Needs: v0.1

### The shaman of the mountain
Inspiration: orc shamans of Gork and Mork; goblin religions in Dwarf Fortress
Trigger: orc fear > 0.6 and `piety` > 0.4
Effect: faith +0.3 (destroyer); tag `dragon_horde`: captives flow to the hoard as `food` each decade; war target follows the dragon's last `incite` at no cost; cohesion +0.2 while faith > 0.5
Chronicle: The orcs of the scar painted the mountain on their shields.
Dragon hook: the cheapest army in the game; `accept_offering` keeps it; one `defend` against them anywhere breaks it (`betrayal`)
Surprise: low
Needs: v1 creed

### The quarry slaves
Inspiration: the thralls of Angband; the Alessian revolt reversed
Trigger: orc faction destroyed in war by a human faction with `authority` > 0.7
Effect: orc pop 0.3 joins the winner as `orc_thralls`: growth × 1.1, cohesion −0.1, militarism +0.1; 30 decades later 0.2 chance of a `civil_war` from the thrall share
Chronicle: X put the orcs of the scar to work in the quarries, and their grandchildren remembered the scar.
Dragon hook: `incite` the thralls (target: a faction's own pop) spawns the revolt as a faction with faith 0.6 toward the dragon
Surprise: high
Needs: v0.1

## The dragon as a figure in their history

### The prophecy of the second fire
Inspiration: the Prince That Was Promised; the Dragonborn prophecy
Trigger: dragon asleep ≥ 30 decades; a faction with `piety` > 0.6 and a `burn` memory of weight > 0.3
Effect: tag `prophecy`: the first act on waking is read as fulfilment: `burn` → faith × 0.5, fear +0.3, anti_dragon rate × 2; `show` or `give` → faith +0.4; one age
Chronicle: The priests of X named the year the mountain would wake, and began counting.
Dragon hook: the first act sets the age's meaning; knowledge ≥ 2 shows the prophecy in the briefing
Surprise: medium
Needs: v1 creed

### The slayer order
Inspiration: the Dwarf Slayer cult; the Grey Wardens
Trigger: tag `slayers` gained
Effect: tag `slayer_order`: survives collapse (passes to successors), spreads to allies (anti_dragon +0.02/decade there); after a failed attempt the host gains cohesion +0.1 and, if world.magic > 0.3, one artifact (a slayer's weapon)
Chronicle: The Order of the Broken Fang was founded in X, and its first ten masters died on the mountain.
Dragon hook: `kill` the host region's army at magnitude > 0.5 disbands it (fear +0.4); `give` to the host's rival makes the order a political problem
Surprise: medium
Needs: v0.1, v1 artifacts

### The dragon-blood dynasty
Inspiration: the Targaryens; the Septims
Trigger: `dragon_raised` lineage applied by digestion of a royal
Effect: tag `dragon_blood`: faith floor 0.3, fear floor 0.2, cohesion_decay × 0.8; pious neighbours affinity −0.2; after 30 decades 0.3 chance of `dynasty_schism` (civil_war where one side has faith 0 and anti_dragon +0.3)
Chronicle: The kings of X claimed the mountain as grandfather, and some of them meant it.
Dragon hook: the longest investment in the game; a `hostage` clause keeps it; the schism is where a dragon-blood slayer comes from
Surprise: medium
Needs: v0.1

### The treasure hunters
Inspiration: Beowulf's thief; the burglar of Erebor
Trigger: hoard gold above a lure threshold, dragon asleep ≥ 20 decades, adjacent faction with fear < 0.3
Effect: 0.05/decade an expedition: 0.7 fails (fear +0.2, memory `seen`); 0.3 steals 2 % of gold (faction wealth up, faith −0.1, anti_dragon +0.05); each success raises the next chance
Chronicle: Three sons of X went up the mountain with sacks. One came down.
Dragon hook: a dream wake; `kill` the thieves' faction is expected; `give` exactly what was stolen is the surprise (faith +0.3, tag `generous_wyrm`)
Surprise: medium
Needs: v1 lure

### The door in the temple
Inspiration: the Mythic Dawn; the Unbidden
Trigger: faith > 0.8, world.magic > 0.6, era 4; needs new state: portal_progress per faction
Effect: portal_progress +0.05/decade; pious neighbours crusade 0.1/decade; at 1.0 the exit is offered; if declined (`show` 0.1) the cult may open it anyway: spawn `outsider` faction hostile to all, dragon included
Chronicle: The cult of X built a door in the temple that opened onto nowhere yet.
Dragon hook: the exit, or the mistake; `destroy_altar` stops it, absence lets the mortals open a door the dragon did not choose
Surprise: high
Needs: v1 world magic, exits

### The forgotten dragon
Inspiration: Smaug's long absence; Westeros after the last dragon
Trigger: every faction's fear < 0.1 and faith < 0.1, dragon asleep ≥ 50 decades
Effect: world tag `dragon_forgotten`: all `no_walls` / `no_settle` oaths breach; anti_dragon → 0 everywhere; the next `show` counts as `burn` 0.3 in the reaction table; treasure-hunter chance × 3
Chronicle: In X the dragon was a story told to children, and the children did not believe it.
Dragon hook: the long sleep's price and prize: a valley that walks into fire unprepared, and a dragon nobody built ballistae for
Surprise: medium
Needs: v0

### The false dragon
Inspiration: false prophets of the Empire; Alduin's priests
Trigger: faith > 0.5, dragon asleep ≥ 30 decades, great-ruler tyrant
Effect: faction faith → 0.2, piety +0.3, authority +0.3; at his death faith regrows only if the dragon acts `show` or `miracle` this age, else tag `disillusioned` (faith gain × 0.5 for 20 decades)
Chronicle: The king of X announced that the mountain spoke to him alone, and raised taxes in its name.
Dragon hook: a pretender holds your worship; one `kill` at 0.1 on his capital reads as judgement (faith +0.4)
Surprise: high
Needs: v1 creed

### The hostage who came home
Inspiration: Maeglin; Theon Greyjoy
Trigger: `release` act returning a `dragon_raised` hostage
Effect: 0.5: the hostage rules (succession now, faith 0.7, `dragon_blood`); 0.3: rejected, founds a border successor in `civil_war`; 0.2: betrays the dragon (anti_dragon +0.3); needs new state: a dragon-side memory ledger so the dragon can hold a grudge
Chronicle: The prince returned from the mountain after twenty years and his brothers did not recognise him.
Dragon hook: the whole event is the hook; knowledge ≥ 3 lets the briefing hint which branch the hostage will take
Surprise: high
Needs: v0.1, v1 heroes for the betrayal branch

## Age-defining events (at most once per game)

### The breaking of the vale
Inspiration: the drowning of Beleriand; the Red Mountain eruption
Trigger: world.magic > 0.8, or `mega_raze` with magnitude > 0.8, or the drowned kingdom failing on a hegemon
Effect: two adjacent regions become `lake` / `scar` permanently; every faction wealth −30 %; world tag `sundered`; era progress −20 % everywhere; world.magic −0.3; every remnant +0.1
Chronicle: The year the vale broke in two is the year all later years are counted from.
Dragon hook: the dragon causes it (terraform at a size threshold) or survives it; a hit on the lair rewrites the lair fields
Surprise: high
Needs: v1 world magic

### The people out of the fen
Inspiration: the Edain's arrival; the Skaven emerging
Trigger: year > 1500, no world tag `new_race`, 0.001/decade; needs new state: a race table that can grow at runtime
Effect: spawn a fifth race from a data template (lizardfolk, hobgoblins) with 1 to 2 regions on an edge or `fen`; all affinities 0; culture heavily jittered
Chronicle: A people came out of the fen that no song had mentioned.
Dragon hook: the only faction with no memory of the dragon: a blank slate; its loot profile is unknown until the first raid
Surprise: high
Needs: v1 (data extension)

### The king who stopped dying
Inspiration: Nagash; the Ayleid sorcerer-kings
Trigger: human great ruler in an `archmages` faction, world.magic > 0.6, authority > 0.8
Effect: ruler class immortal; tag `lich_king`: culture mutates `undead` over 20 decades (growth × 0.5, cohesion_decay × 0.1, affinity to the living −0.5); the holy-war event follows; ends by assassination card or dragon `kill`
Chronicle: The king of X did not die when he should have, and then did not die at all.
Dragon hook: a rival immortal; `kill` him for faith +0.5 in every living faction, or ally against the living
Surprise: high
Needs: v1 world magic, culture mutation, heroes

### The sundering of a race
Inspiration: the Chaos Dwarfs; the Falmer
Trigger: `corrupted` factions hold ≥ 30 % of a race's regions for 50 decades
Effect: they become a new race entry (lifespan and loot profile from the mutated culture, own remnant); world tag `sundered:<race>`; affinity between the two races −0.9 permanently
Chronicle: After the long war of the holds nobody called the eastern dwarves dwarves any more.
Dragon hook: `corrupt` over several ages produces a race of the dragon's making; its loot profile is whatever the dragon trained it to produce
Surprise: high
Needs: v1 culture mutation

### The fall of the long empire
Inspiration: Rome 476; Gondor's waning
Trigger: one faction has held ≥ 50 % of regions for 50 decades
Effect: at collapse every region becomes a successor with era −1, cohesion 0.4; orcs or invaders spawn on two border regions; era diffusion × 0.5 valley-wide for 20 decades (dark age); `library` tags survive
Chronicle: The empire did not fall in a day, but the chronicle only has room for the day.
Dragon hook: a dragon that `defend`ed it loses its tribute at once; one that `incite`d orcs inherits weak targets with nothing worth taking
Surprise: medium
Needs: v0.1

### The great migration
Inspiration: the Völkerwanderung; the Sea Peoples
Trigger: world tag `volcanic_winter` or `sundered`, and ≥ 3 factions with cohesion < 0.4
Effect: every faction below 0.4 moves 0.3 pop to its neighbours; 2 to 3 regions change owner by settlement without war; orc and human remnants +0.2; all affinities jittered ± 0.3
Chronicle: For two generations every road in the valley was full and every village half empty.
Dragon hook: the world after `mega_raze` is a migration; `give` land to steer who ends up next to the lair
Surprise: high
Needs: v0.1

## Exits, as seen from the mortals' side

### The ascension
Inspiration: Earthsea's dragons going west; the Valar withdrawing
Trigger: dragon exit `ascension` chosen
Effect: every faction's faith becomes religion: creed vector frozen as its god, fear → 0, `slayer_order`s dissolve; the chronicle runs 50 idle decades as epilogue
Chronicle: The mountain was empty in the morning, and the temples were fuller than they had ever been.
Dragon hook: the epilogue's texture is the creed left behind: a protector god's valley or a destroyer god's
Surprise: low
Needs: v1 exits, creed

### The door stays open
Inspiration: the Oblivion Crisis; the Breach
Trigger: dragon exit `portal` chosen
Effect: the cult's region becomes `scar`; the cult tagged `gatekeepers` (immortal ruler, faith 1.0 to an absent god); others crusade against it 20 decades; world.magic +0.3; 0.2 chance an `outsider` faction comes the other way
Chronicle: The dragon went through the door it had asked for, and the door stayed open a while.
Dragon hook: the dragon leaves a mess; the mess is the ending
Surprise: medium
Needs: v1 exits, world magic

### The quiet mountain
Inspiration: Smaug's 170 years; Alduin cast through time
Trigger: dragon sleeps > 3000 years (exit `millennial_sleep`)
Effect: the valley runs to completion: `dragon_forgotten`, remnant and return cycles, hunger atrophy; on wake size × 0.5, knowledge intact; the lair region tagged `old_mountain` and its neighbour may be an era-4 town
Chronicle: The mountain was quiet for so long that they built a town on its foot and called the town after the quiet.
Dragon hook: the reset; the dragon wakes a legend in a valley that cannot imagine it
Surprise: high
Needs: v0.1 (hunger)

### The carts of gold
Inspiration: Bard's arrow and the division of Erebor's hoard
Trigger: world tag `dragon_slain`
Effect: slayer faction cohesion +0.3, faith and fear → 0, tag `dragonslayers` (affinity from all +0.2 for 20 decades, then −0.2 from envy); lair region tagged `hoard_unguarded`: every neighbour's war_chance toward its owner × 3 for 10 decades
Chronicle: The mountain's gold went down the valley in carts, and the valley fought over the carts for a century.
Dragon hook: none; this is the dragon's ending text
Surprise: low
Needs: v0.1

---

## Features the catalogue needs most (beyond v0.1), ranked by dependent entries

1. World magic level: 14 entries (necromancer, colleges, powder, binding, surge, vanished scholars, blight, drowned kingdom, closed wood, holy war, door in the temple, breaking of the vale, lich king, door stays open).
2. Creed vector: 10 entries (god that walks, sacrifice cult, schism, millenarian, crusade, shaman, prophecy, false dragon, dwarves of the scar, ascension).
3. Artifacts as objects: 5 entries (theft, vanished scholars, oath of recovery, slayer order, dug too deep).
4. Culture mutation (`corrupt`, `crusader`, `undead`): 5 entries (dwarves of the scar, sundering of the wood, sundering of a race, lich king, crusade winners).
5. Heroes and lineages: 4 entries (pretender, immortal queen, hostage who came home, lich king); several more would read better with a name.
6. Exits: 3 entries (door in the temple, ascension, door stays open).
7. Lure (hoard-driven thieves and invaders): 3 entries (theft, treasure hunters, invasion from beyond).
8. Dreams (sleep pauses): 3 entries (theft, schism, treasure hunters) are far better if the player can wake.
9. New: an abstract outside world with a pressure counter: 3 entries (invasion from beyond, people out of the fen, great migration). Open question 3 in `ages-sim.md`.
10. New: a runtime-extensible race table (people out of the fen, sundering of a race) and a dragon-side memory ledger (hostage who came home, false dragon), 2 entries each.
