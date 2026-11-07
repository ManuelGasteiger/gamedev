# Hoard: how the world shows its age

Status 2026-10-24. Rule for both shells: the shell never remembers what happened; it renders the sim's state at the moment of waking, and the sim carries what time did to a place. Smoke a hundred years after a burning is a bug. This doc lists what the shell shows for each sim state and age, what the sim must store for that, and how the sleep montage uses the same mapping to show time passing.

## 1. Two clocks

- **Within a waking** (between sorties, minutes of play, no sim time): fires burn out or smoulder, smoke columns thin, charred frames stand, fled crowds stay fled, garrisons regroup after about a minute, spilled loot stays where it fell. Nothing rebuilds. This is the only time the flight shell shows smoke.
- **After any sleep** (one year or a thousand): no fire, no smoke, no fled crowds, no spilled loot (the diff already counted what was carried home; what lay in the street is the court's again or lost). Everything else below is read from the sim.

## 2. A burned settlement, by years since the act and the court's response

The sim decides the response (rebuild, fortify, abandon, memorialise) through its existing rules: wealth and cohesion rebuild, dread and era build walls, collapse and high dread on a border abandon, the slayer-order and sack cards raise memorials, awe raises shrines and temples. The shell maps the result:

| years since | court rebuilt (wealth, cohesion) | court fortified (dread ≥ 0.3, era ≥ 2) | abandoned (collapse, or dread ≥ 0.7 on a border, or wasteland) |
|---|---|---|---|
| 1 to 5 | charred frames, scaffolds, tents at the edge, black fields, a pyre site; fewer figures | the same plus a palisade going up, a watch post, soldiers | charred frames, no figures, carts gone, first weeds |
| 5 to 30 | new roofs in the court's current era kit, a plaza where the worst block stood, fields green again; a shrine if awe rose | stone walls and towers in the kit, a garrison, range rings; the burned quarter rebuilt in stone first | roofless walls, scrub over the fields, a road fading; an orc camp if orcs spawned (sim rule), squatters' huts if a neighbour settled it |
| 30 to 100 | an ordinary town of its era; only the memory tiers show: a chronicle house if written, a statue, memorial or temple if monumental | an ordinary fortified town; the walls stay because walls stay | old ruins, trees through the floors on wood terrain, farmland if a neighbour's settle rule took the region (fields, a hamlet), grey ground if `wasteland` |
| 100 and more | the same; the town may have moved its centre (the sim's capital can change) | the same | a landmark ruin the cards can use (the necromancer's tower rises where a waste borders scholars; a legend may name it); forest on wood, heath on marches, reeds on fen |

Rules behind the table:

- **Char and black fields** show only while the region's `burn` value is above 0.4 and the act is under 20 years old; after that, scrub or green by terrain. The sim's `burn` decays slowly because it is land productivity; the shell must not read it as soot forever. Use `years_since_last_burn` for soot, `burn` for how well the fields look.
- **Walls appear because of the sim, never because of the raid.** A court at era ≥ 2 with dread ≥ 0.3 builds walls over about five decades (the castle-century card); the shell shows walls when the `walls` tag is set, in that court's kit. It must not show "walls because the player burned here" by itself.
- **Memory tiers are visible objects.** Written: a chronicle house or scriptorium in the capital (the `library` tag or a historian). Monumental: the monument object itself (temple with the dragon's sign, memorial stone, statue), placed where the act happened when the region allows, else in the capital. Oral memory has no object; a town that only tells stories looks like any town. This is the same rule as the chronicle levels: what the player can see is what the court wrote or built.
- **Abandoned regions change by terrain**, over the sim's own timescales: vale and marches become farmland if a neighbour settled (the settle rule), heath and scrub otherwise; wood regrows; fen reeds return; scar stays scar; `wasteland` is grey ground and dead trees until the tag lifts (30 decades below 0.2 burn).
- **Figures** follow the sim's population share and era: a rebuilt rich town has crowds; an abandoned one has none; a fortified one has soldiers on the walls. Pilgrims and envoys walk the roads only when an arrival exists.

## 3. What the sim must store (per settlement or region)

Most of it exists; the rest is small:

- `last_burn_year`, `last_sack_year`, `rebuilt_count`, `destroyed_share_at_last_burn` (from the diff)
- `abandoned_year` (when the owner became none by abandonment or collapse), `resettled_year`
- the tags already planned: `walls`, `library`, `dragon_temple` and `memorial` monuments (item 02), `wasteland`, `ruin` (with the year, so a ruin has an age), `orc_camp`, `farmland` (set by the settle rule when a region is farmed from a neighbouring capital rather than settled with its own town)
- the era kit and the owner's race, as today

Everything in the table is derived from these in the shell; no shell keeps history of its own.

## 4. The sleep montage shows the same mapping

The montage after sleep (sandbox-game.md) replays the chronicle on the map. With this mapping it can show the strongest image the game has: time passing over a place you changed. The burned town smokes, goes black, sprouts scaffolds, gets roofs, gets walls, gets a statue of the hero who died fighting you; or goes quiet, loses its roofs, greens over, becomes a field with a hamlet, becomes a forest with a ruin in it. Montage rule: for sleeps under 30 years, show the rebuild stages event by event; for longer sleeps, one frame per age (the story sifter's ages block) with the state at the end of each age. The flight shell's valley flyover on the next waking should match the montage's last frame exactly, since both read the same state.

## 5. Where this lands

- Item 02 (dread/awe, memory tiers): monuments as objects, placed where the act happened.
- Item 09 (acts): `last_burn_year`, `destroyed_share` written by the diff; the `ruin` tag with a year.
- Item 13 and item 16 (shells): the mapping table as the single rendering rule for both; the sandbox's current "char fades as burn recovers" becomes "soot by years since, fields by burn".
- Item 14 (sifting): ages named in the montage.
- `raid-design.md` section 2: the line about "smoke from a city burned last waking" holds only for a rest of a year or less; otherwise the signs are the table's.
