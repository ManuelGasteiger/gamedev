# Hoard: the act API (v1 freeze)

Status 2026-10-23 (evening: dread/awe start values and the day's decisions added in sections 6 and 7). One vocabulary for everything the dragon can do, consolidated from `ages-sim.md` (sections 3, 8, 9, 17), `sandbox-game.md` (raid table, lair, API notes), `core-loop.md` (talents, hooks, defences) and `story-and-endings.md` (chronicle, guise, keep, corruption). This is the contract that scenarios, hooks, endings, the chronicle and, later, the real-time raid all write against. The machine-readable form is `act-api.schema.json`.

Rules carried over and kept:

1. The vocabulary is closed. A new mechanic changes magnitudes, gates or payloads, or adds one act with its reaction row. It never special-cases the sim.
2. Everything resolves into an **act** (witnessed, logged, reacted to), a **modifier** (timed delta), a **spawn** (faction, person, structure), an **oath** (data object) or a **lair field**.
3. The sim never reads intent. Meaning comes from act × the target's relationship to the dragon, read from the reaction table.
4. Every act is logged with `year`, `source` (raid, lair, sleep, scenario, hook) and, when it answers something, `answers` (hook id or scenario id). The log is the chronicle's raw material and the only thing scenarios may name in preconditions.

## 1. Shape of an act

```
Act
  id          int            monotonic per world
  year        int
  type        enum           section 2
  target      Ref            {kind: faction|region|person|item|remnant|structure, id}
  secondary   Ref?           the attacker in defend, the recipient in broker, the structure in burn
  magnitude   float 0..1     relative to the target (share of wealth, of army, of population); 0 for acts without one
  payload     object?        per type, section 2
  source      enum           raid | lair | sleep | scenario | hook
  answers     string?        hook or scenario id
  witnesses[] faction ids    derived by the sim (owner, neighbours, anyone with a sage present), never supplied
```

Costs (stamina, pools, growth share) are paid by the shell before the act is emitted; the sim validates gates and refuses with a reason, it does not deduct stamina. Dread and awe deltas are the reaction table's job (`data/reactions.json`); the tables in section 2 list the intended sign, section 6 the start values.

## 2. The vocabulary

Gates: M Might, F Flame, A Arcana, L Lore, D Dominion (growth points), plus phase and talent names from `core-loop.md`. "Pool" means the spendable pools (magic, knowledge, faith). Dread/awe: the sign of the typical reaction on the target (witness rows differ).

### 2.1 Force

| act | target | secondary | payload | gate | cost | resolves | dread/awe |
|---|---|---|---|---|---|---|---|
| burn | region | structure? (library, temple, foundry, walls, altar, granary, tower) | – | none | stamina per 0.25 m | act; with a structure: that structure's tag removed and its specific tail (library: memory reset and era_rate ×0.9 on neighbours; foundry: era progress −20, anti-dragon −0.1; temple: faith −0.3, pious neighbours crusade) | ++ / − |
| kill | region (army, garrison, civilians) or person | – | {who: army, garrison, civilians} when region | none | stamina | act; person: dies, memory entry on its court; garrison: defences disabled for the age | ++ / − (judge rows: +awe when the person was a false dragon, usurper, necromancer) |
| crush | region | – | – | Might 25, talent Earth-shaker | stamina 20 | act (kill garrison) + fear modifier on neighbours | ++ / 0 |
| fight | person (hero, slayer, champion) | – | – | none | stamina, wounds by the sim's duel roll | act; the person dies or flees with a scar; the dragon takes listed wounds; drops an artifact when the person carried one | + / + (an open fight won is awe, a slaughter is dread) |
| destroy_altar | region | – | – | none | stamina | act; faith −0.3, cult tags removed | + / − |

### 2.2 Acquisition

| act | target | payload | gate | cost | resolves | dread/awe |
|---|---|---|---|---|---|---|
| take | region | {gold, books, artifacts, art, food: amounts} by the owner's loot profile and era; `item: id` for a named artifact | none | stamina 10 | act; hoard += payload; `loot:<kind>` tags for cards | + / − |
| take_hostage | person (at a capital, or on the road during a `royal in flight` hook) | – | none | stamina 20 | act; the person moves to the lair with role `hostage`; faction tag `hostage_taken`; consent flag when it answers a `hide_heir` offer (then awe, not dread) | ++ / − (or 0 / + with consent) |
| hunt | region (ownerless or own lair slopes) | – | none | stamina 5 | hoard.food; no witnesses unless the region is owned (then `take` food) | 0 / 0 |
| receive | lair (pilgrims, refugees, petitioners present) | {outcome: receive, eat, turn_away} | none | – | receive: faith_pool and Dominion, person events; eat: food and dread; turn_away: faith cools | eat ++ / −; receive 0 / ++ |

### 2.3 Protection and gift

| act | target | secondary | payload | gate | cost | resolves | dread/awe |
|---|---|---|---|---|---|---|---|
| defend | region | attacker faction | – | none | stamina 12 | act; attacker loses an army, war may end `defended_by_dragon` | 0 / ++ (the attacker: +dread) |
| give | faction or remnant | – | {gold, artifact id, books, food, region id} | none | stamina 5; the payload | act; wealth or era progress up; a region: the recipient settles it (refugees, exiles, a remnant: new faction tagged `dragon_settled`) | 0 / ++ |
| release | person (hostage, prisoner) | – | {terms?: oath draft} | none | stamina 5 | act; `hostage who came home` card decides the branch; a corrupted or taught person carries its tags home | 0 / + |
| bless | region | – | {kind: land, plague, blight} | Dominion, faith pool | faith pool | act (miracle protector) + modifier land ×1.3 or shock cancelled | 0 / ++ |
| ward | region | – | – | Arcana | magic pool | modifier invasion ×0.2; on the lair: slayer odds down | 0 / + |

### 2.4 Presence and speech

| act | target | payload | gate | cost | resolves | dread/awe |
|---|---|---|---|---|---|---|
| show | region or faction | {kind: roar, flyover, land} | none | stamina 3 | act, magnitude by kind (0.1 flyover, 0.3 roar, 0.5 land at the capital); the decline of an offer is `show` 0.1 | + / + (the forgotten-dragon rule reads it as burn) |
| demand | faction | {clause: tribute, no_walls, no_settle, no_harm_to: faction, hostage, disband: slayers, tear_down: structure, pilgrimage} | Lore (Old Tongues halves it) | – | oath sworn when the court is cowed (dread ≥ 0.3 or power below the weak ratio) else `demand_refused` and resolve +0.05 | + / 0 |
| swear | faction | {terms: oath draft, answers: offer id} | none (accepting an envoy) | – | oath object; honest offers fulfilled give faith ×1.5 | 0 / + |
| teach | faction or person | {subject: letters, craft, magic, war, intrigue, dragon_lore} | Lore (subject-specific thresholds; magic also Arcana) | knowledge pool | faction: era_rate or trait modifier for 10 decades; person: trait or skill tag (`schooled:<subject>`) that resolves when they return (card branches) | 0 / + (magic and war also raise anti-dragon later) |
| incite | faction | {against: faction} | Lore or Arcana | – | modifier on war target; orcs follow for free with the `dragon_horde` tag | + / 0 (the target, if it learns: +dread) |
| broker | faction | {with: faction, pact: marriage, peace, alliance, trade, vassalage} | Lore 15 (Patient Mind or Old Tongues) | knowledge pool | both courts gain affinity and the pact tag; a marriage merges lines per the `marriage of two crowns` card; witnesses: both | 0 / ++ |
| reveal | faction | {secret: hidden_region, plot, heir_location, slayer_plan, lie} | Lore (Keen Eyes shows lies) | – | the target learns the fact: a hidden valley found, a plot exposed (barons' plot cancelled or triggered), a slayer plan betrayed | 0 / + (the betrayed party: +dread, `betrayal` if sworn) |
| judge | person | {verdict: guilty, spare} | Lore 8 | – | a public kill or pardon at the court's capital, framed as judgement: faith +, not dread, when the person carried `false_dragon`, `usurper`, `necromancer`, `tyrant` tags; otherwise resolves as `kill` person | 0 / + (or kill's row) |

### 2.5 Faith and arcana

| act | target | payload | gate | cost | resolves | dread/awe |
|---|---|---|---|---|---|---|
| accept_offering | region (altar) | – | none | stamina 5 | faith +0.1, faith pool; legitimises a sacrifice cult (cohesion loss halved) | + / + |
| consecrate | region (temple or altar) | – | Dominion 8 (Cult of the Mountain) | faith pool | the structure gains `dragon_temple`: memory tier monumental, faith floor 0.3 in the owner; required by ascension | 0 / ++ |
| miracle | faction | {kind: protector, terror} | Dominion, faith pool | faith pool | act; protector: faith +0.3; terror: dread +0.4, army routed | terror ++ / +; protector 0 / ++ |
| curse | faction or person | {kind: lineage, research, blight} | Arcana | magic pool | modifier: cohesion_decay ×2 (lineage), anti-dragon rate −25 % per stack up to 3 (research, each dearer), land ×0.5 (blight) | + / − |
| corrupt | faction or person | {toward: dark, undead, chaos, dragon} | Arcana 8, world magic (faction) or Arcana 3 (person) | magic pool | faction: culture mutation tag over 20 decades; person: tag `corrupted:<toward>` plus taught intrigue if present, resolves on release (agent, schism, inquisition branches) | ++ / − if witnessed, else 0 / 0 (secret acts have no witnesses until revealed) |
| raise_undead | region (ownerless, burned, or a battlefield) | {against: faction} | Arcana 8, Necromancy halves | magic pool | spawn faction race `undead`, war target set; `holy war against the dead` follows | ++ / − |
| haunt | person | {message: warning, summons, prophecy, nightmare} | Arcana 15 (Dragon-sight) or Lore 15 | magic pool | the person gains a memory entry as if visited; a prophet may found a cult, a king may abdicate, a slayer may desert; no witnesses | 0 / + (prophets), + / 0 (kings) |
| commune | item (an artifact with an agenda) | {answer: accept, refuse, ask} | Arcana or Lore per item | – | the artifact's bargain resolves: a talent-like boon with a tail tag on the dragon (`crowned_by:<item>`, pushes one ending), or the item goes quiet; the chronicle records the voice | 0 / 0 |

### 2.6 Lair

| act | target | payload | gate | cost | resolves |
|---|---|---|---|---|---|
| keep | person or item | {role: hostage, historian, scribe, consort, priest, champion, ward} | role gates: historian Lore 3, consort Lore 8 or Guise, priest Dominion 3 | the person must be at the lair (hostage, pilgrim, envoy who stays, someone brought by `guise`) | lair record; the person ages by race and sleep tempo; a historian raises the chronicle level while alive; a consort founds a line; a priest keeps faith from fading in its court |
| dismiss | person | – | none | – | the person leaves; resolves as `release` without terms |
| build | lair | {what: library, nest, altar, vault, forge, gate} | Might, Lore, Arcana, Dominion per structure | one sleep's growth share | lair field; unlocks (library: Lore digestion +25 %; vault: thieves fail; altar: pilgrims may arrive; gate: envoys may arrive in phase 1) |
| terraform | region | {kind: wasteland, lake, lava, mountain} | Might (size) | a sleep's growth share | region tag + land modifier; mountain on the lair region is the `Mountain` ending's precondition |
| sponsor | remnant or person | {kind: remnant, hero, claimant} | Lore | knowledge pool | spawn: a returned faction (`dragon_settled`), a hero with `sworn_to_dragon`, a claimant with a war |
| guise | faction | {purpose: visit, court, learn, spy, preach} | Arcana 15 (Mortal Guise) | magic pool; one visit per age | the dragon walks a city unseen: visit grants a `seen` memory with awe and no dread; court opens `keep` consort on a willing person; learn: Lore from the court's library without taking; spy: reveals a plot or slayer plan for free; preach: faith + and a cult seed. Discovery roll per visit (cunning of the court's sages): a discovered guise reads as `show` 0.5 plus `deceived` on the court |

### 2.7 Sleep (not witnessed; logged for the chronicle and the endings)

| act | payload | gate | resolves |
|---|---|---|---|
| sleep | {years, wake_on: [slaying_prepared, army_at_lair, oath_breached, thief, cult_schism, hunger], dreams: on/off} | none; `wake_on` entries by Lore (Premonition adds slaying) | runs the sim; hoard digestion; wounds heal; talents may open |
| pick_talent | {node, choice or skip or reroll} | the node's points and phase | growth; skip pays +5 health; one reroll per moult |
| cast_before_sleep | any 2.5 spell with source `sleep` | as the spell | a modifier that ticks during the sleep |
| ignore | {hook or scenario id} | none | logged as `ignored`: the hook's "if ignored" branch fires and scenarios may name it. Scenarios must always offer it |
| exit | {kind: ascension, lichdom, portal, mountain, brood, emperor, last_flame, scholar, mortality, sun_eater} | the ending's condition set holds (`data/endings.json`); offered on the wake screen, never forced | ends the run with that ending's epilogue; `The Myth` is not an exit, it is a death that qualifies (added 2026-10-23 evening, v1.1) |

## 3. What the shell owes the sim per act

- `source`, `year`, `target` and `magnitude`; `answers` when the act was chosen from a hook card or scenario option.
- For `burn`, `take`, `kill` on a region: magnitude relative to the owner's current wealth, army or population at the moment of the act (the raid diffs per act, not per age).
- For `fight`, `guise`, `teach` on a person: the person id; the sim rolls the outcome with its own rng so replays stay identical.
- Never: witnesses, dread/awe deltas, reaction rows. The sim derives those.

## 4. Witnesses and secrecy

Default witnesses: the target's owner, every neighbouring court, and any court with a sage or envoy present at the target. `corrupt` on a person, `haunt`, `guise` (unless discovered), `commune`, `keep` and `build` have no witnesses; they enter the chronicle as the dragon's own acts and mortals learn of them only through `reveal`, a card (an inquisition finds the princess) or a discovery roll. This is what makes intrigue possible without a second rule set.

## 5. Fifteen scenarios written against it (stress test)

Each: precondition (what in the log or state fires it), the options as acts. The last column lists what the vocabulary lacked before this freeze; everything is now covered.

| # | scenario | precondition (log and state) | options → acts | needed |
|---|---|---|---|---|
| 1 | The scribe among the hostages | `take_hostage` this age at a court with era ≥ 2; the hostage's court has a sage | keep(person, historian) · release(person) · kill(person) | keep |
| 2 | A burning library | `burn` on a region tagged `library` within two wakings; neighbours' era_rate fell | give(faction, books) · ignore · teach(faction, letters) | burn.structure |
| 3 | The hero at dawn | a hero with `sworn_to_memory` of a `burn` act by the dragon; slayer stage `ready` | fight(person) · haunt(person, nightmare) · demand(faction, disband) | fight, haunt |
| 4 | The princess returns | `release` of a person tagged `corrupted:dark` and `schooled:intrigue`; her court's piety > 0.6 | reveal(faction, plot) · ignore · guise(faction, spy) | corrupt.person, teach.person, reveal |
| 5 | The inquisition | card `inquisition` fired on a court after scenario 4; its mages' research halted | give(faction, artifact) · judge(person, spare) · incite(neighbour, against) | judge |
| 6 | The thief with the cup | card `theft from the hoard`; the item is now at a court | take(region, item) · demand(faction, tear_down: nothing, clause hostage) · give(faction, gold) as the "generous wyrm" | take.item |
| 7 | Two crowns | adjacent courts with affinity > 0.6; the dragon holds an heir of one | broker(faction, marriage) · release(person, terms) · ignore | broker |
| 8 | Pilgrims in winter | faith ≥ 0.5 at a court; a bad century on it; pilgrims at the lair | receive(receive) · receive(eat) · bless(region, land) | receive |
| 9 | The lich-crown speaks | an item with agenda `lichdom` in the hoard; an undead court exists | commune(item, accept) · commune(item, refuse) · give(faction, artifact) | commune |
| 10 | A door in the temple | portal_progress ≥ 0.8 at a cult; pious neighbours crusading | consecrate(region) · destroy_altar(region) · defend(region, crusader) | consecrate |
| 11 | The dwarves ask for fire | a dwarf court with `grudge:<orc>` and era ≥ 2; openness > 0.5 | teach(faction, craft) · give(faction, artifact) · ignore | teach.subject |
| 12 | The quiet valley | `ignore` on three hooks in a row; dread < 0.1 everywhere | show(faction, land) · guise(faction, preach) · sleep(long) | ignore as a precondition |
| 13 | The mountain | Might ≥ 600; lair region terraformed `mountain`; a court settled its slopes | terraform(region, mountain) · sleep(3000) · demand(faction, pilgrimage) | terraform.mountain, demand.pilgrimage |
| 14 | The consort | `guise` with purpose `court` succeeded; a willing person tagged | keep(person, consort) · dismiss(person) · release(person) | guise.court, keep.consort |
| 15 | The false dragon | card `false_dragon` on a faithful court | judge(person, guilty) · guise(faction, preach) · miracle(faction, protector) | judge |

Scenario 11 shows the one soft spot: "lend them fire" has no literal act. It resolves as `teach(craft)` plus the chronicle line; if the fire itself must be an object later, it is an artifact payload on `give`, not a new act.

## 6. Reactions: dread and awe (start values)

From `research-faith-and-culture.md` section 5. Every number is a start value for the headless runner, not a sourced one. These replace the single `fear` stat; `faith` stays as a cult's share of a court and is fed by awe (see `research-faith-and-culture.md` sections 1 and 6).

### 6.1 Update per act

For each witnessing court with weight `w` (owner 1.0, neighbour 0.5, a court with a sage or envoy present 0.5):

```
dD = g_D(act) · m · w · (1 − D_eff)
dA = g_A(act) · m · rep · w · (1 − A_eff)
rep = max(0.25, 0.5^n)      n = acts of the same type the court witnessed in the last 30 years
```

`show` awe is further multiplied by `(1 − D_eff)`: a feared dragon appearing is a threat. Deposits go into the court's oral tier at full strength, into the written tier at 0.6 if the court has a library or a historian, into the monumental tier at 0.4 if a monument exists (6.3). A kept oath adds +0.02 awe per decade while it holds.

### 6.2 Base gains at m = 1 for the owner

| act | dD | dA | act | dD | dA |
|---|---|---|---|---|---|
| burn | +0.35 | −0.10 | give | 0 | +0.20 |
| burn (temple) | +0.35 | −0.25 | release | 0 | +0.10 |
| burn (library) | +0.30 | −0.20 | bless | 0 | +0.25 |
| kill (army) | +0.25 | −0.05 | show | +0.10 | +0.20 |
| kill (civilians) | +0.30 | −0.15 | defend (the defended) | 0 | +0.25 |
| kill (person) | +0.25 | −0.10 | defend (the attacker) | +0.20 | 0 |
| crush | +0.40 | −0.10 | receive (receive) | 0 | +0.10 |
| fight, won in the open | +0.10 | +0.15 | receive (eat) | +0.25 | −0.10 |
| take | +0.10 | −0.05 | receive (turn_away) | 0 | −0.05 |
| take (item) | +0.15 | −0.10 | miracle (protector) | 0 | +0.30 |
| take_hostage | +0.20 | −0.05 | miracle (terror) | +0.40 | +0.10 |
| take_hostage (consent) | 0 | +0.10 | judge (tagged target) | 0 | +0.15 |
| destroy_altar | +0.15 | −0.15 | judge (untagged) | as kill (person) | |
| raise_undead | +0.40 | −0.20 | broker | 0 | +0.20 |
| curse (witnessed) | +0.20 | −0.10 | consecrate | 0 | +0.15 |
| demand (sworn) | +0.10 | 0 | teach | 0 | +0.10 |
| demand (refused) | +0.05 | −0.05 | accept_offering | +0.05 | +0.10 |
| incite (if learned) | +0.15 | −0.10 | sponsor | 0 | +0.15 |
| guise (discovered) | +0.25 | −0.10 | guise (undiscovered) | 0 | +0.05 |

Secret acts (`corrupt` on a person, `haunt`, `commune`, `keep`, `build`, undiscovered `guise`) write nothing on any court until a `reveal`, a card or a discovery roll makes them known; then the row above applies to the court that learned it, with `w` 1.0.

Witness rows (neighbours) keep the `reaction` table's spillover logic from `ages-sim.md` section 5: an enemy burned gives the witness a little awe as well as dread; a friend or same race burned gives dread and takes awe.

### 6.3 Memory tiers

Dread and awe are stocks per tier per court, not single numbers. `E = 1 − Π(1 − x_k)` over the tiers gives `D_eff` and `A_eff`. Proposed half-lives: oral 30 years, written 250, monumental 1000. `burn` on a library multiplies the court's written stocks by 0.1. Every monument stores one `act_id` and a bias: a dragon temple converts half of the dread in what it remembers into awe; an anti-dragon memorial does the reverse. The two accounts of one event in the paper playtest come from this.

### 6.4 Unlocks

| level | effect |
|---|---|
| A_eff ≥ 0.15 | pilgrims may reach the lair |
| A_eff ≥ 0.30 | a cult can found; honest envoys; oaths ×1.5 stable |
| A_eff ≥ 0.50 | shrines become temples; `consecrate` opens; faith floor 0.3 |
| D_eff ≥ 0.30 | demands accepted; tribute proportional to D |
| D_eff ≥ 0.50 | slayer orders can form; research rate × (1 + 2·D·(1 − 0.7·A)) |
| D_eff ≥ 0.70 | garrisons flee; defence value × (1 − D) on raids |

Oath stability `1 − 0.8·D·(1 − A)`. Slayer pressure `D·(1 − 0.7·A)`. Institution strength `I = sqrt(A·D)`; a cult becomes a temple religion at `I ≥ 0.35`. Awe alone stays a cult, dread alone feeds slayers. Research spend: a court's anti-dragon research per decade is `base × era × dread_factor × wealth share`, and it spends wealth that would otherwise compound (Sellers' missing loop 3), so a burn on a poor court calms it and a burn on a rich one escalates.

## 7. Decisions taken 2026-10-23

- **Digestion consumes the hoard, partially.** Each sleep converts a share of every hoard category into growth and that share is gone; the gold bed keeps a floor (so a hatchling's first moult still pays). Raids stay necessary all game. Numbers in `prompts/03-digestion.md`.
- **Run length target:** 6 to 10 hours, 30 to 50 wakings per run. Node thresholds, the storyteller cadence and the scenario bank size (`scenario-spec.md`) derive from this.
- **Closed vocabulary for the raid.** The real-time raid produces exactly the acts above with continuous magnitudes derived from what happened; no act outside this list.
- `judge` stays a separate act (scenarios bind to it by name; its reaction row differs).

## 8. Open

- `guise` discovery odds and whether a discovered dragon in a city is a fight or a flight.
- How the real-time raid derives magnitudes (per act, from the world diff); deferred until the acts are stable in the sim.
