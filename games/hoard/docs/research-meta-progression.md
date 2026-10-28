# Meta-progression without power: pacing "several runs to finish"

Excerpt for item 04: the "For Hoard" parts of each section, verbatim. The findings, tables and the 78 sources are in the Project (`claude/research-meta-progression.md`).

## 1. Knowledge as the only progression

**For Hoard.** Carry exactly five files, all knowledge, none altering the sim's start parameters except through pre-history text and tags:

1. `legends.json`, written only on a qualifying death (section 3).
2. `ending_atoms.json`: per ending, each condition atom with a state of `rumor`, `witnessed` or `used`, plus how it was learned.
3. `seen.json`: per scenario id, times seen, option taken, outcome tag. Used for cross-run anti-repetition and for a "new" marker in the log.
4. `bestiary.json`: slayer-order templates and anti-dragon technologies met, with the stage at which they were seen. Mortal technology depends on era, so this teaches when to expect it, never where.
5. `glossary.json`: prophecy phrases mapped to the atoms they turned out to encode (the Heaven's Vault language model). Entries stay unconfirmed until a run proves them.

Nothing about a particular valley carries, since the valley is reseeded. Outer Wilds compresses 20-fold because its secrets are fixed. A Hoard run needs at least 30 wakings by design, so the knowledge dividend should show up as fewer failed runs while run length stays near the 6 to 10 hour floor.

## 2. Runs to first win

**For Hoard.**
- Every run must close a complete arc, because only one owner in ten to one in four reaches the end of the skill-gated games above. A death that reaches the Myth threshold already counts; a run that ends with no legend is the failure case.
- Proposed targets for the headless runner to tune: a median engaged player reaches a first real ending (Mortality or Myth) by run 2 and a second kind by run 4, and Ascension by run 8 to 10, which is 50 to 100 hours. If half of first-time finishers continue, as Hades suggests, Ascension lands at 10 to 20 percent of buyers (my estimate).
- Per-run chance of any real ending for a player using carried knowledge: about 0.25 at run 3, rising roughly 0.05 per run. The Hades pity mechanic (section 4) stays out, because power must never carry.
- Measure time to first ending in hours as well as runs: at 6 to 10 hours a run, three runs is already 18 to 30 hours, longer than most games here take to a first win.

## 3. Legacy and legend systems

**For Hoard.** `legends.json` entry, written only for a qualifying death:

```json
{"id":"L3","run":3,"epithet":"the Ember of Harrow","span":[0,1840],
 "verdict":{"dread":4,"awe":1},
 "deeds":["burned_library:dorun","spared_envoy:aldmere"],
 "fell_to":"order.broken_fang","tags":["library_burned","slain_by_order"],
 "relics":[{"item":"crown_of_teodric","fate":"lost_in_river"}],
 "text":"template string, 40 words at most"}
```

- Cap at three legends active per world, picked by verdict extremity, plus a hall listing all.
- A legend is injected as 1 to 3 pre-history events at years -900 to -200 with `src:"legend"`. It may seed a cult, a slayer order, a monument or a library gap through tags that scenarios and the prophecy generator can read. It never alters Might, Flame, Arcana, Lore or Dominion, the hoard, or the dragon's starting size.
- Mortal memory of it follows the usual tiers: it starts at the tier it was recorded in, so an old oral legend can already be fading.
- Add a Dwarf Fortress style "how it fell" line to every ruin so a reclaim-like scenario can name the cause.
- Make the legends page optional per run (the Blank Page modifier in section 4 hides it), echoing Sunless Sea's choice between carrying the map and starting blind.

## 4. Difficulty without stat carry-over

**For Hoard.** Offer a free-form Pact and a fixed ladder built from the same list. Each modifier changes a sim parameter, never the dragon.

| Modifier | Ranks | Effect |
|---|---|---|
| Short Memory | 3 | Court memory decay rate x1.5 / x2 / x3; burning a library demotes all tiers |
| Quick Study | 3 | Anti-dragon research speed +25 percent per rank |
| No Premonition | 1 | Telegraphs of dangers removed; briefing shows aftermath only |
| Hungry Hoard | 2 | Digestion converts a larger share of each category |
| Early Orders | 2 | Slayer orders form one stage earlier |
| Few Wakings | 2 | Wake cap falls from 50 to 40 then 30 |
| Sparse Dreams | 1 | Chronicle capped at level 0; no kept historian |
| Blind Prophecy | 1 | Prophecy text omits the ending's noun |
| Blank Page | 1 | "What the world remembers" hidden (a Routine Inspection analogue) |

Rewards are textual: modifiers tag the legend ("in an age of Short Memory") and unlock modifier-gated scenarios. Ladder: unlock the next tier by reaching any real ending, never lowered by loss.

## 5. Failure is informative; hidden prerequisites

**For Hoard.** Ending atoms are world-state conditions, so a wiki can list them; the discovery left to the player is how to produce them on a particular seed.
- States: `rumor` (heard), `witnessed` (seen true in a briefing), `used` (an ending fired with it). Guessing never confirms; only witnessing does.
- Seed rumors in four places: a source that mentions another dragon ("the Red Mother sat in Aldmere's hall") naming an atom as hearsay; chronicle level 3 counterfactuals phrased as "had the library stood, the council would have met"; prophecy glossary entries; and the start-of-run "what the world remembers" page.
- The page lists last legends, atoms by state with blanks for unknown ones, slayer orders from the bestiary, and nothing about the new valley.
- Failure surfaces atoms: every death report adds the 1 to 3 conditions that were closest to firing.
- Give each ending atom at least three independent rumor routes, so one missed scenario never locks an ending out.

## 6. Runs as chapters

**For Hoard.**
- Make each run a closed chronicle that ends with a legend, then let the next world refer to it.
- Add a "what the world remembers" page that costs no wake and shows the previous legend as if heard, with no numbers.
- Interest-tool metrics for inter-run pacing:
  1. Curve P(ending e reached by run N), N = 1 to 12, for three bots: amnesiac, carry (uses `ending_atoms` and `bestiary`), and oracle.
  2. Knowledge leverage = P_oracle(e) / P_amnesiac(e) at N = 1. Aim for 3 or more on Ascension and 1.2 or less on Mortality.
  3. New atoms witnessed per run: at least 3 in runs 1 to 3, at least 1 until the last ending is reached.
  4. Share of runs ending in a qualifying Myth death: high in early runs, falling as endings are found.
  5. Hours per run from sim wakings and a human-time model; reject seeds under 6 hours.
- Until real players exist, a bot's share reaching each ending by run N with knowledge carried is the headline figure.
