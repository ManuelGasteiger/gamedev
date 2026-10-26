# Hoard: the sandbox game

`web/hoard-game.html` is the playable low-poly version of the ages simulation (design: `docs/ages-sim.md`, build notes: `docs/sandbox-game.md`). One self-contained HTML file: the sim runs as WASM inside it, three.js r158 is inlined from web/vendor (MIT). Open it from disk in any current browser.

Build: `bash web/build.sh --html` (it ends with `python3 web/inline.py web/hoard-game.html`). The page's v0.2 code lives in `web/game/*.js` (text: names and the narrator's templates; narrate: the narration strip; court: court card, person verbs, Known people; envoys: audiences, oaths and tasks; rhythm: phases and the Sleep tab; wake: the wake screen; v0.4: growth, look, effects, defences, wounds, arms, hooks, legends, death, sound); `inline.py` puts them into the built page.

## The age loop

1. **Raid.** You wake at the lair with a stamina budget (`100 + 20 × size`, +30 in rage, halved on the first raid after a moult); a sleep restores `years / the phase's rest window` of it (`restedStamina` in `web/game/rhythm.js`), so a hatchling's one-year rest restores it fully and a shorter rest of an older dragon only partly. Fly over regions and act; every act costs stamina and is applied to the world immediately. The narration strip above the action panel says in one sentence what happened and, one line per court, how each took it (the sim's `lines` from `apply_acts` when it sends them; otherwise built in the page from the before/after deltas, the act and the court's standing: an active oath reads as betrayal, a faithful court is disillusioned, a strong one hardens, a weak one cowers, its enemies approve, its kin are dismayed). The last six lines stay, older ones fade. Small floating deltas stay over the tile.
2. **Courts.** Over a court's land the side card shows the court: ruler (name with epithet, age, traits as words, years reigned), heir (age, betrothal), heroes, sages, priests, each with your relationship if you have met them, and what you may do to each: take ruler or heir as hostage (at the capital; `apply_acts take_hostage` with `item` = the person), kill (`kill_person`; 10 stamina, the court remembers the murder). The "Oaths and tasks" card lists what accepted offers still ask of you ("defend Aldmere against Dorun until 600 (Aldmere pays 3.4 gold a decade)").
3. **Lair.** Land on the lair (`L`, or click it while over it). Tabs: Hoard, Envoys (audiences: the envoy by name and role, the stated reason in quotes, the ask with its target named, the price, "the envoy's eyes say otherwise" when your knowledge sees the lie, "this court has lied to you before"; accept, decline, or eat the envoy), Oaths and tasks, Known people (everyone met, alive or dead, relationship, the events that shaped it; hostages with years in the lair and whether they were raised as yours, with Release), Spells, Sleep, Chronicle.
4. **Sleep.** The Sleep tab shows the phase's rhythm (hatchling, young, grown, ancient, elder) and two buttons from its windows: Rest (the short sleep: 1, 5, 20, 100, 300 years) and Moult (the long one: 10, 100, 300, 1000, 3000), the moult highlighted when `ready_to_moult`; each with its forecast. Any length is still possible below. During the sleep the hoard digests into size (gold), knowledge (books), magic (artifacts), faith pool (art); hunger rises when the digestion falls short of `hunger_need`.
5. **Wake.** The returned chronicle plays back on the map as a time-lapse (Space pause, `+`/`-` speed, Esc skips straight to the wake screen; or tick "skip the time-lapse"). The ticker names people, courts and regions and only shows events above `briefing.importance_min` (the event's `importance` when the sim sends it, else the per-kind weight in `data/text/en.json`), plus anything about someone you know or a court you are sworn to, so the deaths of your acquaintances always show. Then the wake screen: a page of the chronicle from `briefing` (headline, the century's paragraphs with their cause clauses, a thread of three or more lines told as its first and last, people, threads, opportunities). Each opportunity has a "go" that closes the page, selects the court and flies there (an envoy's offer lands you at the lair on the Envoys tab). Enter or Esc begins the raid; "Last sleep" in the bar reopens the page.

Endings: slain by a slayer order (dark screen, final chronicle, restart). Waking by hunger (0.9) shrinks the dragon by atrophy (0.01 size per decade above 0.5) and is shown in the HUD, the wake screen and the model scale.

## Controls

| key / mouse | action |
|---|---|
| WASD / arrows | fly (camera-relative; the camera follows the dragon) |
| click a tile | fly there; clicking the lair while over it lands |
| wheel, right-drag | zoom, pan |
| hold B, release | burn the region under you, magnitude 0.1 to 0.8 by hold time |
| L | land at the lair (flies there first if away) |
| C | chronicle drawer (filterable) |
| Esc, Enter | close the wake screen and begin the raid |
| Esc | close the open panel (game sheet, chronicle), blur a focused control, take off from the lair, skip the time-lapse to the wake screen |
| Space, +, - | pause / speed in the montage |
| 1, 2, Esc | growth screen (v0.4): take the first or second talent, close and let the choice wait |

Keys typed inside an input or select never fly the dragon; Esc blurs the control.

## Verbs and what gates them

All acts need the stamina listed; spells and miracles are paid from a growth axis instead. The action panel shows the exact reason when a verb is unavailable.

| verb | needs | cost | sim call |
|---|---|---|---|
| Burn | an owned region | 15 per 0.25 magnitude | `apply_acts` burn |
| Take | a settlement; categories from race and era (gold and food always; books era ≥ 2 or elf; artifacts era ≥ 3 or dwarf; art era ≥ 2; royals at a capital without a held heir) | 10 | `apply_acts` take with payload |
| Kill army | an army standing here (armies stand on the defender's border region of each war) | 10 | `apply_acts` kill, feeds hunger |
| Defend | the region's owner is at war and the attacker's army stands here | 12 | `apply_acts` defend (the attacker loses wealth; the war ends if it holds nothing of theirs) |
| Roar | anyone living here | 3 | `apply_acts` show 0.3 |
| Hostage / Release | a capital; release needs a held heir of that house | 20 / 5 | `apply_acts` take_hostage / release |
| Take a person hostage (court card) | the ruler or heir, at their capital, no hostage of that house held | 20 | `apply_acts` take_hostage with `item` = person id |
| Kill a person (court card, or eat an envoy) | over their court's land; an envoy in the lair | 10 | `kill_person` (the court remembers the murder: fear +0.3, cohesion drops; under an oath it is a betrayal) |
| Give | gold in the hoard | 5 | `apply_acts` give with payload gold |
| Altar: accept offering / destroy | a capital whose owner's faith > 0.5 | 5 / 8 | `apply_acts` |
| Miracle (protect / terror) | faith pool ≥ 1 | 1 faith pool | `apply_acts` miracle |
| Demand (tribute, no walls, no settle) | a capital; knowledge ≥ 0.5; the court must be cowed (fear ≥ 0.3 or power < half your threat), otherwise it refuses and gains resolve | 8 | `demand` (new) |
| Ward | region; magic ≥ 1 | 1 magic | `cast` |
| Curse | faction; magic ≥ 1.5 | 1.5 magic | `cast` |
| Bless | region; faith pool ≥ 1 | 1 faith pool | `cast` |
| Teach | faction; knowledge ≥ 1 (era progress ×2 for 10 decades) | free | `cast` (new spell) |
| Incite | an orc faction with a neighbour it is not at war with; knowledge or magic ≥ 0.5 | free | `cast` with `against` |
| Raise undead | an ownerless region that is burned ≥ 0.5, wasteland or scar; magic ≥ 3 | 3 magic | `cast` (spawns a faction at war with the strongest neighbour) |
| Sponsor remnant | a remnant camp on this tile (two tents) | 10 | `sponsor_remnant` |
| Terraform (lake, wasteland) | size ≥ 2; not the lair | 30 stamina + 10 % of size | `terraform` |
| Corrupt | not in this build (`enabled: false` in constants) | | |

Spells cast from the lair's Spells tab use the same `cast` command with a target dropdown. The dragon's modifiers appear as badges on the labels (warded, blessed, cursed, taught), in the selected region's facts, in the Spells tab, and in the chronicle as `spell_cast` and `spell_expired` events.

## How the page talks to the sim

`sim(cmd)` wraps `ages_cmd` (JSON in, JSON out). Commands used: `load`, `reset`, `world` (since v0.2 also people, dead_people, known_people, hostages, phase, offers with envoy, stated_reason, target_name, eyes_say_otherwise, marked) (the view: regions with terrain, owner, burn, tags, wealth share, capital flag; factions with capital, at_war_with, allies, overlord, hostage flag, altar flag, power; armies; offers; catalogue of spells, terraform and demand; modifiers; oaths), `apply_acts` (its `lines` drive the narration strip when present), `accept_offer`, `decline_offer`, `kill_person`, `cast`, `demand`, `sponsor_remnant`, `terraform`, `sleep` (with its `briefing`), `briefing` (when a sleep result carries none), `forecast`, `load_world` (autosave, import, and the check script's labs), `hash` (unused by the page).

Sim text can arrive half filled when `data/text/en.json` is newer than `web/ages.wasm`: the page fills the placeholder from the event (by id) when it can and drops it otherwise (`cleanSimText` in `web/game/text.js`), so no `{...}` reaches the player.

Witnesses of an act are the owners of the neighbouring regions, computed in the page. Stamina exists only in the page. The age's acts are the events returned by each call (`act_*`, `spell_cast`, `oath_sworn`), kept per age for the chronicle. Autosave to localStorage after every age; Game sheet has new game by seed, export and import.

## What is rendered for each kingdom event

| event | on the map | also |
|---|---|---|
| war_started, joined_war, coalition_formed | a red pulsing arrow from the attacker's border region into the defender's; cube soldiers in the attacker's colour on the border; the defender's tiles flash red | ticker + toast |
| war_ended, coalition_ended, vassal_sworn | arrows and soldiers removed; a vassal's tiles flash in the overlord's colour and show the overlord's flag | ticker |
| region_taken | the tile's ring recolours with a flash, "X takes Y" floats | ticker |
| region_settled, race_returned, orcs_spawned, undead_raised | ring appears with a puff in the new owner's colour, floating label | ticker |
| region_abandoned, elves_withdrew, elves_departed | ring removed, "abandoned" floats | ticker |
| collapse, faction_destroyed | shards fly off every tile of the faction, an expanding white ring | ticker |
| successor_founded | the new ring with a puff | ticker |
| era_advance | the settlement is rebuilt in the new era's shape (huts, long houses, wall and keep, towers, spire), a rising gold ring, "era N" floats | ticker |
| plague | a green haze and drifting motes over each tile of the faction, "plague" floats | ticker |
| wasteland_formed / healed | the tile goes grey (and back); label badge | ticker |
| dug_in | a gate block | ticker |
| spell_cast / spell_expired | ring flash in blue; "X fades" floats and the badge disappears | chronicle |
| oath_sworn / breached / expired | floating text at the party's capital; betrayal by the dragon flashes the screen red | Oaths tab |
| slay_attempt_failed, dragon_starving, dragon_slain | screen flashes red | wake summary / ending |
| tribute_paid, act_* | quiet (chronicle only) | |
| everything else (golden age, tyrant, crisis, hegemony, fading, remnant, bad century, volcanic winter) | toast and ticker; volcanic winter flashes every tile grey | chronicle |

Always on: every tile shows a label with the owner swatch, owner name, region name, terrain and badges (burn %, wasteland, capital, warded, cursed, vassal). The selected region's panel lists owner, terrain, race, era, wealth and share, fear, faith, power, wars, allies, tags and modifiers. The HUD shows year, age, stamina, size, knowledge, magic, faith pool, hunger (red when atrophy starts), threat, the phase with its progress to the next ("moult!" when ready) and dragon tags, and the Last sleep button. The legend (bottom left, can be hidden) explains the colour code.

## v0.4: growth, the arms race, death, a generated valley, hooks (real S1, S2 and S3 API)

The S1 screens run on the real wasm (`talents`, `pick_talent`, `world.dragon.{growth, next_node, talents, ranks, picks, effects, look, wounds, health_max, slain}`, `regions[i].defences`, `factions[i].anti_dragon_next` and `slayers`, `apply_acts` → `wounds` and `slain`, `sleep` → `growth_gained` and `healed`), and so do S2's (`reset {seed, genesis: true}`, `legends`, `world.genesis {template, prehistory_years, hatched_year, deal}`, `regions[i].name`; years stay absolute, the egg hatches at `genesis.hatched_year`; a classic world sends an empty genesis and empty names). S3's hooks are real too: `world.hooks`, `hooks_resolved` [{id, kind, reward_text}] on `apply_acts` and `kill_person`, `answer_hook {id, choice}` for choices (pilgrims), `fight_damage` on fight hooks. The action panel shows an answer row for every hook of the region the dragon is over, which sends exactly the act the hook wants (or `answer_hook`); a fight that could kill asks twice.

- **Growth.** Five axis chips in the HUD (points and progress to the next talent or rank; the pools move into their tooltips) and a "talent ready" badge. After the wake page, or from the badge, a chip or the lair's Growth tab: the growth screen over a close-up of the dragon, axis bars (points, next node, what feeds it), then the open pick as two cards (name, effect, what you will see; keys 1 and 2; "Later" lets it wait). Taking one: the card rises, the other is struck out, the camera leans in, the dragon flashes in the axis colour and a chime plays. With nothing to pick the screen shows each axis' talents taken, open, locked and to come, and the rank rule (a rank costs `rank_every` points past the last node, each next one `rank_growth` times dearer, every `milestone.every`th rank a bigger step) (`web/game/growth.js`). The phase chip and the rhythm line count growth points against `phase.next_phase_points`.
- **Look.** Plates, fire colour (beam, throat and flames), wing scale, spikes, horns, runes, crest, eyes, veins, shadow, aura, a glittering hoard and banners at the lair; the sim adds the rank and milestone steps (scale, beam, aura, eyes, glitter) to the look numbers itself (`look.js`).
- **Defences** from `regions[i].defences`: levies (spears), bolt-throwers (towers and a range ring), slayer band (a camp), wards (a blue dome; burns ×0.5), cannons (squat towers, smoke). Radii arrive in map units (40 and 55: a ring covers its own region). Labels and the panel give the numbers after fear, hero and your armor; a ring shows within 5 units of its edge, all of them with Dragon-sight. Inside a ring you take its passive damage every second (sent as `wound` acts in batches): the dragon flashes, the screen edge reddens, the number rises, a thud; the first time per ring and age a warning names the towers and their damage (`defences.js`).
- **Before you act.** For a harmful act (burn, take, kill, hostage, crush) at a defended region the panel lists the hits in strike order and the health left; each row shows its wounds; a lethal act is marked and runs on a second press within 4 s (`wounds.js`).
- **Health** bar in the HUD. The Sleep tab shows the bet of each length: healing, growth per axis and the danger line (`bet.js`). The real forecast carries no growth or healing, so the page estimates both the way `hoard.cpp` digests on the phase clock and `wounds.cpp` heals, over the years the forecast expects you to sleep, and marks them "(estimate)"; the forecast's `growth_gained` and `healed` win when a build sends them. The danger line comes from the slayer clock, not the forecast's blurred `slay_possible`: only a band that is ready when you lie down can try in this sleep.
- **Arms race.** The court card shows a court's progress to its next defence (`anti_dragon_next`, measured from the last unlock it passed as the sim does) and its slayer stage; the wake page lists the 25/50/75/95 % steps told while you slept (`anti_dragon_progress` events), every court's bar with what the sleep added, and the slayer clock: forming (strength), ready (odds at your wounds, they strike in the next sleep), at the door after a premonition (`arms.js`).
- **Premonition.** A sleep the talent cut short (wake reason `premonition`, event `slayers_coming`) leads the wake page with the band waiting at the foot of the mountain; it strikes in the next sleep without another warning (world tag `slayers_coming:<court>`).
- **Hooks**. The wake page leads with them (what, where, answer, pays, if ignored, go); pins over their regions, a "This age" card, answered ones ticked; the next wake page says what you answered and what you let go; a fight hook wounds through the sim's `wound` act (`hooks.js`).
- **New world** (Game sheet, the death screen) and the first start of a fresh save reset with `genesis: true` and open the legends page before the first flight: a title for the starting situation, the pre-history's legends with how long before the hatching each happened, the courts as they stand; region names from the world view everywhere (`legends.js`, `text.js`). A cataclysm that changes a region's terrain rebuilds the valley at the next refresh. `game.newGame(seed)` without options still deals the classic map (the check's seed 39).
- **Death.** The line of the dragon ends: by the defences (who, where, the three biggest hits with the numbers shown before them) or by slayers in your sleep (the court and the band's leader); the life's highlights; the autosave is deleted; New world (`death.js`).

## Checking

`PLAYWRIGHT_BROWSERS_PATH=... node web/game-check.cjs` runs the whole checklist (seed 39 since v0.2; where a check needs a situation the history may not hold, it sleeps on until the situation exists or builds it in a `load_world` lab instead of relying on the seed: a protect offer with armies, a court the panel lets you make a demand of, plague, settling and the spells running out, a court with an heir (one-year rests), two envoys in person, an orc horde, open land to settle; age 1 scripted raid, sleep 100; accept a protect offer, kill, defend, demand, teach, breach; curse, sleep 300; ward, hard burns, sleep 300; incite; growth and hunger runs; keyboard, mouse, Esc, Space; 1024 px; a `load_world` lab for altar, miracle, bless, sponsor, raise undead, terraform; an orc lab when the seed raises no horde) and the v0.2 items: the page is self-contained with the six split scripts inlined; rest and moult from the phase windows, a 1-year rest advances the year by one and opens the wake screen; narration after a burn; the court of Vessary names ruler and heir; taking the heir through the court card puts them in Known people; the wake screen after 100 years has a headline, paragraphs, people, threads and opportunities, no unfilled placeholder, closes on Enter and Esc and reopens from Last sleep; the time-lapse tells no raw ids; a "go" flies to its court; an envoy audience with a quoted reason and eat-the-envoy; the Oaths and tasks card after accepting a protect offer; synthetic `lines` and `importance` take precedence. It writes one screenshot per item to `web/shots/` (`v02-*.png` for the v0.2 items) and exits 0 only with zero console errors. The v0.4 section (`--v04` runs it alone, about five minutes) first runs on the real wasm: the S1 fields and screens show on the classic map, no hooks, legends or names do, and a lab world with a slayer band ready against a hoard that makes the odds certain ends in the death screen after one rest; a fresh start and New world open the legends of a generated valley at its hatching year, and every label carries the sim's region names. Then with the classic seed 39 and a lab hook (a real `vein` hook loaded with `load_world`): the hook card and pins, the strike preview, a hook answered by a take, wounds as shown, the sleep bet (the estimate has to match the moult's real `growth_gained`), the wake page led by the hooks, the growth screen (key 1, the flourish, the badge, the lair tab), two look sets and blue fire, the defences lab (all five, rings with Dragon-sight, 3 s inside a ring, the warning, the court's arms race, a band ready on the sleep tab with its odds, the premonition waking), a lethal act refused then taken, the death screen and New world after it (`v04-*.png`). `node web/check.cjs` does the same for the sandbox (briefing panel and people table included). three.js r158 (MIT) is inlined from `web/vendor/three.r158.min.js` at build time, so the page needs no network.

## Known gaps

- `corrupt` is disabled by data; the row says so.
- Offers are regenerated from faction state on every `world` call, so an act that changes a court's situation (ending its war with `defend`, raising its faith) can make its envoy vanish or a new one appear mid-raid. Answer envoys first; the tab says so.
- `pay_to_burn_rival` offers carry a tribute clause with an implied rival; the clause text says so but the sim has no explicit "burn X" check, so the oath only ever expires or breaks on tribute.
- The portal rite (exit) is listed as an offer when a court reaches era 4 with faith > 0.8 but accepting it swears an empty oath; exits are v1.
- Envoy figures, armies, camps and arrows are placed by the page from the view (nearest border region), not by the sim.
- Hover highlighting and click picking use the GPU raycast; headless swiftshader renders a few frames per second, so the check script waits on frame counts rather than time.
- No mobile layout; 1024 px wide is the floor.
