# Hoard – the sandbox game

Status 2026-10-15. A playable, low-poly browser version of the whole loop (raid, lair, sleep, wake) on top of the ages simulation (`docs/ages-sim.md` sections 1 to 15). Purpose: test every interaction the game will have, not flight feel. The three.js flight prototype is not reused.

## Rules of the build

1. The sim is the only authority. The page never changes faction state itself. It builds the raid map from `world` (the wasm `ages_cmd` API), lets the player act, and emits acts in the sim's format. Sleep, offers, forecast, remnants, oaths: all through the API.
2. Low poly, colour coded, readable from above, flat shading, one directional light with static shadows, fog for depth. Models come from the asset bundle: `web/assets.json` maps game keys to files under `assets/gltf/`, `web/inline.py` packs them into the page as GLB (2026-10-16). In use: the MattBas placeholder dragon and a subset of Quaternius Ultimate Fantasy RTS (houses, town centres, towers, wall, gate, temple, mine, trees, pines, rocks). The Stylized Nature kit is not used here: its trees carry 4 to 10 MB of textures and 4k to 10k triangles each.
3. One self-contained HTML file (`web/hoard-game.html`), built from `web/game.template.html` by the same build script as the sandbox (wasm and data embedded as base64/JSON). Three.js r158 (UMD, MIT) is inlined from `web/vendor/` at build time, so the file works from disk, offline, and as a published artifact. (The first build referenced a cdnjs URL that did not exist; the page failed with "THREE is not defined".)
4. Every mechanic in the sim has at least one visible cause and one visible effect on the map.

## View

Orthographic camera, three-quarter view (yaw 45°, pitch ~50°), zoom with wheel, pan with right-drag or edge; the valley fits the screen at default zoom. The dragon's shadow on the ground reads altitude. Day/night tint by the day cycle is optional; a mild colour shift per era of the strongest faction is enough.

## The valley

- The valley is one generated low-poly heightfield (`web/game/terrain.js`, seeded by the game seed, rebuilt on New game; 2026-10-16). The 12 regions of `regions.json` are warped Voronoi cells around their sites (`REGION_LAYOUT` plus up to 1.4 units of jitter). Relief per terrain: vale gentle lowland, marches rolling, wood rolling under dense forest, hills ridged mountains with pines and rocks, fen at water level with pools and reeds, scar broken ridges with water-filled ravines, lair a basalt volcano (7.5 high, glowing crater) with the cave on a ledge at its foot. A mountain rim closes the valley at the back and the sides and stays low towards the camera; the sea lies on the south-west side. One river rises in the northern rim, runs through a lake on the sea side of the valley and reaches the sea, each leg the cheapest path over the terrain, kept away from settlements. Settlement sites are flattened plateaus (radius 2.3), so the old tile-top convention (children at y = 0.5 of a group standing on the plateau) still holds. Ground colour per face from the region terrain, rock on steep faces, snow above 5.4, sand and mud at the waterline. Software GL (headless checks) gets a quarter of the flora and no shadows.
- Owner shown by a coloured outline ribbon just inside the region border and a small banner post at the settlement; faction colour = race hue + id hash (as in the sandbox). Vassals show the overlord's colour on the banner and their own on the outline. Hover and flashes use the same outline.
- Settlement (`buildSettlement`, keys and fits in `web/game/models.js`): a town centre at the site and count - 1 houses on a spiral around it facing inwards, count = 2 + round(wealth share × 10) capped at 14. By era: 0 wooden huts and a post ring (FirstAge level 1), 1 bigger wooden houses, a fortified centre and a watch tower, 2 stone houses, a stone centre, the watch tower and a ring of 14 wall segments, 3 the largest stone houses plus four stone corner towers on the wall, 4 the same with the arcane spire and its glowing tip above the centre. The RTS pack's red accent material takes the owner's colour; everything darkens towards char with burn. Capital of a faction (its richest region) gets a crown marker; that is where royals and the altar live; the altar is the RTS stone temple, `dug_in` the closed wooden gate, a dwarf-owned region adds the mine.
- Burn: the region's ground chars from the settlement outward with a noisy front (embers on the front while burn > 0.3); its trees blacken and lose their canopies above 0.75; 10 to 40 flame billboards spread wider as the burn grows; the wasteland tag turns the ground grey and kills the canopies; char fades as burn recovers.
- Tags as small icons over the tile: warded (blue ring), dug_in (grey gate), altar (white column, when faith > 0.5), hidden/withdrawn (dashed ring), library/other v1 tags ignored.
- Armies: when two factions are at war, a group of 4 to 9 small cubes in the attacker's colour stands on the border of the defender's region (between tile centres). Coalitions show one group per member. These are the targets of `kill` and `defend`.
- Envoys: after a sleep, each offer is a small figure (cylinder + sphere) walking from its faction's capital to the lair over ~10 s; it waits at the lair entrance. Offers are only visible as figures; the lair overlay lists them with accept/decline.
- Remnants: a faint camp (2 tents) on a region of their home terrain that is ownerless, if strength > 0.1.

## The dragon

The MattBas placeholder dragon (`assets/gltf/creatures/dragon-placeholder/dragon.glb`, CC BY-SA 4.0, credited in the Game sheet), embedded at build time by `web/inline.py` and parsed with the three.js r158 GLTFLoader (`web/vendor/gltf-loader.r158.js`, converted from the ES module for the UMD build). Clips by state: Flying-loop while airborne (faster while moving), Idle-loop when landed or asleep; the breath bends the Head bone towards the target on top of the clip. The throat glow, the gold crest (knowledge) and the purple wing tips (magic) are pinned to the Jaw, Head and outer wing-finger bones; size scales the model 1.0 to 2.5; rage tints it red. Flies 5 units above the ground under it (smoothed, looking 1.5 ahead so it clears the volcano); lands on the ledge in front of the cave. Controls: WASD/arrows or click-to-fly on a region. Speed is generous; this is not a flight test. The dragon always starts an age at the lair and ends it by landing on the lair (press L or click the lair).

## Raids (the awake phase)

An age starts at the lair with a stamina budget: `stamina = 100 + 20·size` points. The player flies over a region; a context panel (bottom centre) lists the actions available there, each with its cost and what it will do, greyed with the reason when unavailable:

| action | available when | cost | emits |
|---|---|---|---|
| Burn | owned region | 15 per 0.25 magnitude, magnitude chosen by holding the key (0.1 to 0.8), shown as a growing ring | act `burn` |
| Take | settlement present; categories from the owner's race loot profile and era (gold always; books era ≥ 2 or elf; artifacts era ≥ 3 or dwarf; art era ≥ 2; royals at a capital) | 10 | act `take` with payload, magnitude from the amount slider |
| Kill army | an army is present | 10 | act `kill` (soldiers) |
| Defend | the region's owner is at war and the attacker's army is here | 12 | act `defend` with against |
| Roar | any region | 3 | act `show` 0.3 |
| Hostage | capital, no `hostage_taken` tag | 20 | act `take_hostage` |
| Release | capital of the race of a held lineage | 5 | act `release` |
| Give | any owned region; gold from the hoard (slider) | 5 | act `give` |
| Altar | altar present: Accept offering / Destroy altar | 5 / 8 | acts |
| Miracle | faith_pool ≥ cost | faith_pool | act `miracle` (terror toggle) |
| Spell | magic ≥ cost: ward, curse, bless (uses faith_pool), incite (orc faction present), raise undead (ownerless burned region), corrupt (v1: disabled) | magic | modifier/spawn through the API (`spell` act or a `cast` command, see API notes) |
| Sponsor remnant | remnant camp present | 10 | `sponsor_remnant` |
| Terraform | size ≥ 2: wasteland, lake | 30 | `terraform` command |

Witnesses: computed in the page as the owners of neighbouring regions, as the sandbox does. Magnitude for Burn: the held ring; for Take: the slider. Each act is applied immediately through `apply_acts` so the player sees the map change (wealth boxes shrink, burn appears, fear/faith deltas as floating text over the tile). The act log for the age is kept for the chronicle and the diff.

Fire: a short orange beam from the throat to the ground, flame sprites spawn on the tile, boxes blacken proportional to magnitude. Keep it a few hundred lines at most; the fire mechanic is not this prototype's job.

Rage (after a failed slaying): the dragon starts the age red-tinted, stamina +30, burn magnitudes × 1.3 (the sim already applies this; the page only shows it).

## The lair (overlay, not a separate scene)

Landing on the lair opens a panel over the darkened map:
- hoard by category with digestion half-lives, growth axes (size, knowledge, magic, faith_pool), hunger bar with "need per decade", threat, tags, lineages, remnants.
- envoys: each offer with the clauses in plain words, Accept / Decline (decline emits `show` 0.1 as the sim does).
- active oaths with status.
- spells to cast before sleeping (same as in the raid but targeted by a dropdown; they become modifiers that tick during sleep).
- sleep: 50 / 100 / 300 / 1000 / custom, with the forecast line (blurred by knowledge) next to it; Sleep button.
- a "Chronicle" tab with the full history so far, filterable.

## The sleep montage

After Sleep: the panel closes, the map dims, and the chronicle of that sleep plays back on the map at an adjustable speed (default 20 years per second, Space to pause, +/- speed, Esc to skip): a year counter at the top, each event rendered as it happens: region ownership changes recolour the ring with a flash; wars put cube armies on the border; region_taken moves them; collapse splits the banner into successors with a puff; burns and wasteland spread the decal; era_advance rebuilds the settlement boxes; plague darkens a faction's tiles; great ruler / tyrant / crisis / coalition / vassal / fading / remnant events are toasts in the top-left with the chronicle sentence (reuse the sandbox's `eventText` formatter). Dream-worthy events (slay attempt, oath breached, army at the lair) flash the screen red. At the end: dawn tint, "Year X, you slept N years, woke: reason", the dragon's growth deltas, then the raid begins (envoys start walking).

Because the sim returns the whole sleep at once, the montage is a replay of the returned chronicle, with the final world state applied progressively per event where it can be derived (ownership, wars, burns, era) and in one step at the end for everything else.

## v0.2: people, consequences in words, the rhythm of sleep (2026-10-15)

Diagnosis after playing ("I can do stuff in order to grow, but it isn't captivating me"): the valley had no names, nothing asked anything of the dragon, consequences were numbers, and every sleep had the same tempo. The sim's section 17 (`docs/ages-sim.md`) adds people, offers with reasons, phases and the briefing; the page shows them:

- **Wake screen** after every sleep, from `briefing`: a chronicle page (serif text, about 36 em line length, the year as a marginal note), headline, the century's paragraphs with their cause clauses (a thread of three or more lines told as its first and last), people, threads (oaths, hostages, spells, remnants, marks like `deceived_dragon`), opportunities with a "go" that selects the court and flies there. Enter / Esc begin the raid; Last sleep in the HUD reopens it. The time-lapse stays and Esc skips straight to this page.
- **Narration strip** after each act: one sentence on what happened, one per court on how it took it (the sim's `lines` when `apply_acts` returns them; until then built in the page from the deltas, the act and the court's standing). Last six lines, fading.
- **Courts**: the selected court's ruler, heir, heroes, sages and priests with ages, traits as words, reign, betrothal and relationship; take hostage (ruler or heir at the capital, `take_hostage` with `item`), kill (`kill_person`, remembered), release from the lair. Lair tab Known people: everyone met, alive or dead, with the events that shaped the relationship, and the hostages being raised.
- **Envoy audiences**: the envoy as a person, the stated reason in quotes, the ask with its target, the price, knowledge-gated hints; accept, decline, eat the envoy. Accepted tasks in "Oaths and tasks" ("defend Aldmere against Dorun until 600 (Aldmere pays 3.4 gold a decade)").
- **Phases**: HUD chip with progress; the Sleep tab offers Rest and Moult from the phase's windows (1 / 10 years for a hatchling up to 300 / 3000 for an elder) with a forecast each.
- **Ticker**: names instead of ids; events at or above `briefing.importance_min`, or about someone the dragon knows or a court it is sworn to.
- The 2D sandbox shows the briefing as a panel above the chronicle and a people table.

Settled since: a sleep restores `years / rest window` of the stamina budget (page), the slow rules accumulate over one-year rests and the starting courts' people have plausible ages (sim); the wasm sends `lines` and `importance`. The check finds its situations (sleeping on or a `load_world` lab) instead of relying on one seed's history.

## v0.4: talents, the arms race, death (page side, 2026-10-18)

The page for `docs/core-loop.md`, built against `docs/v04-plan.md`. S1 (growth, talents, defences, wounds, the slayer clock, death), S2 (genesis, legends, region names) and S3 (hooks, answered from a row in the action panel or by `answer_hook`) run on the real API since 2026-10-19; the mock is gone. A fresh save and New world start a generated valley with its legends.

- One file per job in `web/game/`: `growth.js` (HUD chips, badge, growth screen, wake section), `look.js` (look keys on the dragon and the lair, fire colour, the flourish), `effects.js` (the effects the page applies: stamina, speed, burn and kill costs and magnitudes, fury, witnesses, Crush), `defences.js` (models, rings, passive wounds, labels, strike order), `wounds.js` (health bar, hit feedback, lethal second press, hit log), `bet.js` (the sleep bet: healing and growth per length, estimated page-side while the forecast carries neither, and the danger line from the slayer clock), `arms.js` (each court's progress to its next defence, the slayer stages, premonition), `hooks.js` (wake cards, pins, HUD list), `legends.js` (the legends page), `death.js` (the death screen), `sound.js` (WebAudio cues after the first click or key). The template stays the composition root: markup, `actOutcome` after every act (wounds, hooks, death), the frame loop.
- Page-side numbers: Crush costs 20 stamina; a ring shows 5 units before its edge; passive wounds go to the sim in batches of 3 or every second; a lethal act needs a second press within 4 s; defence radii arrive in map units (the 400 × 360 layout) and are scaled by `WORLD_SCALE`.
- Beyond the contract, read when present: `forecast.growth_gained` and `forecast.healed` for the sleep bet (S1 does not send them; the page estimates and says so), `apply_acts.hooks_resolved` (S3), `factions[i].slayers {stage, strength, odds}` and the world tag `slayers_coming:<court>` for the slayer clock.

See `web/GAME.md` for what each screen shows and how the check drives it.

## Endings and exits

- `dragon_slain` (a slaying in sleep, or since v0.4 the wounds of the defences): the death screen names the killer and the year, lists the three biggest hits with the numbers shown before them and the life's highlights, deletes the save and offers only a new world.
- hunger wake: the dragon wakes smaller; the wake screen shows the size delta.
- Exits (ascension, portal) are v1; not in this build.

## Persistence and sharing

- Autosave to localStorage after every age (try/catch, silent on failure); "New game" with seed; Export/Import world JSON (works from disk; the artifact viewer blocks downloads, say so in the UI).

## API notes (what the page needs from `ages_cmd`)

Existing: load, reset, world, apply_acts, accept_offer, decline_offer, sleep, forecast, sponsor_remnant, load_world, hash; since v0.2 also briefing, people, kill_person (and `sleep` returns its briefing). Coming from the sim: `importance` (0..1) on events and `lines` ([{faction, text}]) on the apply_acts result; the page uses both when present. Needed in addition, implemented in the library and api.cpp:
- `cast` {spell, target, region?}: ward, curse, bless, incite, raise_undead, with gates and costs from `constants.json` (magic / faith_pool); returns the modifier or spawn it created and the chronicle events. Spells not in the sim yet are added as modifiers/spawns per `ages-sim.md` section 8; `corrupt` stays disabled.
- `terraform` {region, kind}: wasteland | lake, gate on size, cost as a size fraction from constants; sets the tag and a land modifier.
- `demand` {target, pred}: tribute | no_walls | no_settle, gate on knowledge (`demand_knowledge`); sworn when the court is cowed (fear ≥ `demand_fear` or power below the weak ratio of threat), else refused with a resolve gain and a `demand_refused` event. Added 2026-10-15 with the `teach` spell (era_rate ×2 modifier) and `spell_expired` events emitted when a dragon modifier runs out.
- `world` must include for every region: terrain, owner, burn, tags, settlement wealth share, whether it is its owner's capital; for every faction: capital region, at_war_with, allies, coalition membership, vassal/overlord, hostage tags, altar flag (faith > 0.5), loot profile; armies: a list of {attacker, defender, region} pairs derived from current wars (the page needs to know where to put cubes); dragon: everything from the sandbox plus stamina does not exist in the sim (page-side only).

## Verification

Playwright script `web/game-check.cjs` (the full checklist and results are in `web/GAME.md`); the original brief: load the page, wait for ready, fly to a region (simulate by calling the page's own `game.flyTo(regionId)` test hook), burn 0.4, take gold, roar, land at the lair, accept the first offer if any, sleep 100 with the montage at max speed, assert: year advanced, chronicle non-empty, wealth of the burned faction lower than before, no console errors; screenshot before and after. One run; keep it under a minute.

## Out of scope

Real flight, the fire grid, sound, mobile layout, multiplayer, the three.js flight prototype.
