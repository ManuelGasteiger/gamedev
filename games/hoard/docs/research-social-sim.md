# Social simulation as Hoard's interaction layer (excerpt)

Excerpt for item 06: section 6 (access gating) in full, and the "For Hoard" notes of section 1. The whole document with sources stays in the Project (`claude/research-social-sim.md`).

## 1. For Hoard (from section 1)

- Store each event once as `{id, y, kind, actors, labels[], desc{viewpoint: template}}`. A court's chronicle renders its own `desc`, which gives the biased histories for free. A prophecy is a label that rules query with "true in history", so devout characters read later events as fulfilment.
- Memory source scales strength: witness 1.0, read 0.8, told 0.6 (the Bad News source and strength idea). Heirs inherit 50 percent of negative and 25 percent of positive strength.
- What the dragon can know, per character: level 0 name and role (dream headlines); level 1 a two-word personality summary from a court source or kept historian; level 2 traits and relations; level 3 the memory entries about the dragon (high Lore, or an interrogated captive).

## 6. Social gating, bonds with consequences, time skips

**Findings.** Quality-based narrative gates a storylet on numeric qualities meeting requirements. Salience-based selection chooses the best-fitting, most specific content for the current state, picks randomly among ties, and can assemble a poor dramatic sequence unless arc variables are included. Thresholds are set mostly by trial and error. In Neighborly, characters interact only when they share a location, relationships are two directed scores from -50 to 50 plus labels, and a life event happens only if every participant agrees. Only major events are tracked, and readers infer the connective tissue. Raw output needs story-sifting to find arcs. In the Skyrim port, NPCs form social goals when the player enters their location. Moirai-style setups run a coarse shared history before play (250 years for Dwarf Fortress, 140 for Bad News) and an epilogue after (40 years in Bad News) while entities persist. Wildermyth ages characters with campaign time, lets them gain traits, and alternates handcrafted and procedural layers.

**For Hoard: access gating.** Evaluate per court every 10-year tick while the dragon sleeps. A visitor type spawns only if every "requires" clause holds, and the briefing prints the satisfied clauses as the "because" line. Limit to one arrival per court per tick, cooldown 100 years for the same (court, kind, cause), and pick the most specific kind when several qualify.

| Visitor | Requires | Blocked when | Move |
|---|---|---|---|
| Pilgrim | faith 2 or more, lair known to region, awe 2 or more, open route | dread above awe + 2 (they leave offerings at a foothill shrine) | Worship, Teach |
| Envoy | written or monumental memory of lair, ruler in war, collapse or plague, or ambitious; a gift or ask; cohesion 40 percent or more | last envoy killed within 2 generations; dread 4 or more with no debt | Bargain |
| Thief | gold rumour from a raid or display in the last 150 years, thief guild (iron era on), lair exact | dragon in lair; wards raise the Arcana needed | Steal |
| Slayer band | order alive, grievance memory sev 3 or more, bolt-throwers or better, lair exact, band power at least lair threat | band power below threat (shown as "gathering") | Avenge |
| Scholar | Teach memory with that court, awe 1 or more, dread 3 or less, a library | no safe route | Teach |
| Kin | relative of a hostage or consort, ransom or grief, lair known | dread 5 | Bargain, Avenge |

(The scale here is the research's -5 to +5; item 06's brief restates the table on the sim's 0 to 1 scale.)
