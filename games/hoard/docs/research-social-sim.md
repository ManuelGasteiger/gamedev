# Social simulation as Hoard's interaction layer (excerpt)

Excerpt for items 06, 07 and 08: the "For Hoard" notes of sections 1, 2 and 4, and sections 3 and 6 in full. Section 5, the other findings and the sources stay in the Project (`claude/research-social-sim.md`); bracketed numbers refer to its source list.

## 1. For Hoard (from section 1)

- Store each event once as `{id, y, kind, actors, labels[], desc{viewpoint: template}}`. A court's chronicle renders its own `desc`, which gives the biased histories for free. A prophecy is a label that rules query with "true in history", so devout characters read later events as fulfilment.
- Character record, all integers, JSON, no free text:

```json
{"id":"c812","race":"human","court":"r7.crown","born":-34,"role":"heir",
 "traits":["curious","ambitious"],"acq":["dragon_touched"],
 "rel":[{"to":"c19","kind":"parent","w":60},{"to":"D","kind":"hostage_of","w":-20}],
 "mem":[{"ev":"e771","y":-12,"sev":3,"src":"witness","tier":"oral","s":1.0,"ax":{"dread":3}}],
 "know":2}
```

- Base traits come from a list of about 12 and are capped at 3, with up to 2 acquired through events. Axes `dread`, `awe` and `debt` (each -5 to +5) are derived each tick as the clamped sum of `s * sev * ax` over memory entries, so memory is the single source of truth.
- Memory source scales strength: witness 1.0, read 0.8, told 0.6 (the Bad News source and strength idea). Proposed decay per 25-year generation, scaled by lifespan: oral halves, written loses 10 percent until the book burns, monumental floors at 0.3 until the statue falls. Burning a library demotes that court's written entries to oral. Heirs inherit 50 percent of negative and 25 percent of positive strength.
- What the dragon can know, per character: level 0 name and role (dream headlines); level 1 a two-word personality summary from a court source or kept historian; level 2 traits and relations; level 3 the memory entries about the dragon (high Lore, or an interrogated captive).

## 2. For Hoard (from section 2, Versu)

Use practices for anything that lasts longer than one exchange, and keep them as plain data: `{id, roles[], actions[{if[], then[]}]}`. Proposed set: Hostage (taker, held, kin), Oath (swearer, holder, expiry), Courtship (dragon, consort), Pilgrimage, Vendetta. A broken Oath spawns a Vendetta practice, the Versu pattern for norm violations. Skip the execute-score-undo loop: 3000 years in 10 ms leaves no room for dry runs, so score with additive rules as CiF does. Cap the number of simultaneously active named characters per court (about 8) to keep rule counts and debugging bounded.

**The six moves (CiF-style, responder side).** Each move is data: intent, hard preconditions (access, dragon skill such as Speech or Mortal Guise), responder weights, an accept threshold, and effects that write a memory entry on both parties. Score = sum of trait weights below + shared modifiers; accept if score is at least the threshold; a margin under 2 selects the reluctant narration (CiF picks the most specific matching instantiation). Ties use a roll seeded from (seed, act log).

| trait | Teach | Corrupt | Bargain | Threaten (comply) | Court |
|---|---|---|---|---|---|
| curious | +4 | +1 | +1 | 0 | +3 |
| proud | -2 | -1 | -3 | -4 | +1 |
| greedy | 0 | +4 | +2 | +1 | +1 |
| ambitious | +1 | +3 | +1 | -1 | +3 |
| devout | -2 | -4 | -1 | -2 | -3 |
| honest | +1 | -3 | +1 | 0 | 0 |
| craven | +1 | +1 | +3 | +4 | -2 |
| vengeful | 0 | +1 | -1 | -3 | -2 |
| threshold | 2 | 3 | 2 | 1 | 4 |

Shared modifiers: `awe` counts for Teach and Court (cap +4), `dread` for Threaten and Bargain (cap +4), `debt` for all (cap +3). Memory term: each `mercy` entry +2, each `betrayal` -3, each earlier success of the same move +1 (CiF "times true"), clamped to plus or minus 6. Corrupt adds `dragon_touched` and a discoverable Secret. Threaten on a proud or vengeful target writes a hidden `humiliated` entry, which feeds later slayer birth. Teach writes `taught` (severity 2, awe +2, debt +1). Release has no roll. It writes a memory read through the trait. The default reading is `mercy` (debt +2, awe +1), proud reads `insult` (a severity 2 grudge), vengeful reads `weakness` (no gratitude) and craven reads `reprieve` (dread +1).

## 3. Middle-earth Nemesis system (for item 08)

**Record.** The patent family describes each nemesis as a name and title fixed per game, a tribe, appearance parameters, a power level starting at 1 to 4 with a cap, a rank, relationship links (rivalry, bodyguard, follower), a fighting style and a trait set [14]. Ranks run overlord, warchief, captain, soldier, with veteran, elite and legendary prefixes tied to completed missions. Traits are grouped as invulnerabilities, hate triggers that cause rage, attack moves, weapon bonuses, fear weaknesses, vulnerabilities and miscellany. As power rises the engine can drop weaknesses and add strengths, but an orc with a sword immunity must keep at least one weakness [14]. The patent gives no counts of traits or voice lines, and the GDC session page and interviews I could reach give none either [16].

**Promotion.** A soldier becomes a captain by killing the player or a captain, or through an opportunist trait that fills a vacancy. If a higher-ranked member dies a lower one may rise, and rivals of a harmed member can be strengthened. Warchiefs stay hidden until a dominated nemesis or special mission draws them out. Outcomes are probabilistic, weighted by relative power, mission type and a player interaction score that grows with each encounter [14]. Wikipedia adds that the orc the player meets most is likelier to return from death [21] (secondary). Changes are queued and applied on triggers: player death, zone change, level-up, passing time [14]. Forum players describe each player death as one hierarchy turn in which captains resolve missions and some die or level up [18], and a patent-based summary says orcs keep running missions while Talion is gone [19]. Hierarchy size is uncertain: the patent text suggests about five warchiefs [15], a forum post says four [18].

**Memory and scars.** Event records are queued and dialogue is selected by matching a stored record (fled, burned, killed the player) or recent player actions to indexed audio. Scars are few and match the cause of defeat, and a face covering appears after a set number of deaths [14]. The patent does not describe a dedicated offline power-struggle simulation, only trigger-based updates (uncertain on what shipped).

**What the team learned.** The first game had orc-to-orc personal relationships and multiple factions with morale and discipline meters. The relationship layer was cut as too complicated [17], and the faction hierarchy screen became a tangle the director compared to a Christmas tree [20]. Shadow of War added bonds, betrayal and shaming [21][24]. Betrayal was described as a tunable knob for when the player is doing too well [22]. Advanced classes were added to stop unlucky trait rolls from producing near-invulnerable orcs [23]. Design intent was love-to-hate enemies and hate-to-love followers [16][24]. The earliest inspirations were tabletop recurring villains, sports career narration and Batman's scarred rogues [25].

**For Hoard: slayer roster.**
- Orders, not individuals, are born from grievance. An order spawns in a court when a `grief` memory of severity 3 or more about the dragon (hostage death, burned town, corrupted heir) survives at s of 0.3 or higher and the era allows organized hunters.
- Ranks: Hunter (unnamed, counted), Slayer (named, max 6 per order), Master (max 2, shielded by Slayers), Grandmaster (1). Start power 1 to 4, cap 12, resolved per 10-year tick.
- Traits map to the five growths: `fireward` counters Flame, `plate` Might, `runeward` Arcana, `waxed ears` Dominion, `forewarned` Lore. Add one hate trigger (smoke, banners) that raises power on contact, and one fear that the dragon can exploit. Keep a minimum of one weakness at every power level [14].
- Each tick a Slayer rolls a mission weighted by relative power. Surviving a dragon encounter gives a scar (burn, claw, tail, deaf) and +1 power. Killing a dragon ally (envoy, consort, vassal) promotes. A fallen Master is replaced by the opportunist or the highest power. A harmed Slayer's rival gains +1 [14]. 50 years of peace costs -1 power unless the Slayer has the `zealot` trait.
- Succession carries the grudge: the new Grandmaster inherits 50 percent of negative memory strength. Relationships stay at three types (rival, mentor, kin) because Monolith cut richer ones [17][20].
- Telegraph: the briefing lists band power, known traits and an arrival estimate. Unknown traits show as `?` and are revealed by captives, spies or books.

## 4. For Hoard (from section 4, Crusader Kings 3)

Source numbers (third-party guides, version-dependent): friend +60, best friend +120, rival -60, nemesis -120; an heir inherits 25 percent of the deceased's positive opinions and 50 percent of the negative ones.

- Use the same structure for courts joining anti-dragon coalitions: a blocker list (hook, bond, terror, hostage held), a strength threshold of 80 percent, and a discontent counter that ticks per decade.
- Run Court and Teach as slow personal schemes (progress per 10-year tick, success rolled at the end, cap 95 percent). Run Corrupt as a hostile scheme with a secrecy value and a per-tick discovery roll by priests and spymasters.
- Hooks give the dragon a diplomatic currency: `hook_weak` (one use, 10 years, from a returned hostage or a gift) and `hook_strong` (reusable, from a secret). A court holding a strong hook never joins a coalition against the dragon.
- Reuse the inheritance numbers and relationship opinions above as starting values for `rel.w`.

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

**For Hoard: corrupted-princess chain** (Ilsa, curious and ambitious, daughter of Edric, who is craven; all numbers are proposals to tune).

| Step | Rule fired | State written |
|---|---|---|
| Y0 Threaten Edric, demand Ilsa as hostage | craven +4 and dread +3 give 7 against threshold 1: complies | Edric `taken_hostage` sev 3; Ilsa `taken` sev 2; court gets an oral rumour at s 0.6 |
| Y0 to 4 Teach Ilsa | curious +4, ambitious +1 give 5 against 2; each success adds +1 to the next | Ilsa `lettered` (acquired), `taught` sev 2; historian-candidate flag |
| Y5 Corrupt Ilsa | ambitious +3, curious +1, debt +1 give 5 against 3: accepts | `dragon_touched`; a Secret on her court, discovery roll 5 percent per decade |
| Y6 Release | default reading `mercy` | Ilsa debt 3, awe 3; Edric learns of the change only if the Secret surfaces |
| Sleep 60 years | Ilsa is queen; she builds a shrine and keeps a written chronicle | court memory moves to the written tier; faith rises, Envoy and Pilgrim rows open |
| Secret exposed | devout nobles form a purge faction; those holding a hook or bond with Ilsa cannot join; ultimatum at 100 percent discontent | civil war or Ilsa's fall |
| Aftermath | grief memory sev 3 about the dragon on that court | Slayer order spawns; heir inherits 50 percent of dread and 25 percent of awe |
