# 05 · Chronicle levels, historian, sources, artifact voice: report

Branch `item/05-chronicle`. Tests confirmed by the owner: `chronicle_levels`, `sources_bias` (both pass, `ctest -L fast` 20/20).

## What was built

- **Levels per briefing line.**
  - 2 when a kept historian's tenure covers the line's year.
  - 3 when there is also Lore of 25 growth points or more.
  - 1 when the hoard holds a source about a court the line is about: its books, or a captive of that court.
  - 0 otherwise.
  - Level 0 prints no cause. Level 1 prints each source's stated cause, two lines when two sources disagree. Level 2 prints the true cause, at most two events from item 14's `because_chain`, never one the line already names. Level 3 adds one counterfactual to the headline.
  - The legacy narrator (bank off) uses the same rule through `Narrator::sentence(e, with_cause)`.
- **Sources.**
  - `take books` from a court that keeps records brings up to 4 of its tellings (`take_sources` × magnitude), none older than a library the dragon burned.
  - `give books` hands the oldest on.
  - `make_source` gained the brief's bias rules: an honest court (openness > 0.6) names the true agent (`stated_cause: "true"`). Otherwise the court blames, in this order: the dragon when dread is high, its `grudge:` tag, its last war.
  - `make_event_source` tells a single event, for lines the bank did not match.
- **Historian.** New verb `keep_person {id, role: historian}`, recorded and replayable, also in the wasm API.
  - The person must be held at the lair and be a sage, priest or scribe, or carry `lettered` (item 07 adds the tag).
  - Tenures end on release or death. The chronicle has a gap from that year.
  - A kept person tagged `sleeps_in_hoard` ages at a tenth. Nothing grants the tag yet.
- **Relics and the voice (D7: only when found).** A `take` with artifacts finds a relic at 0.25 × magnitude, at most one per court and three in all. Which one depends on the court's race:
  - iron crown: emperor
  - grave crown: lichdom
  - sun disc: sun eater
  - star harp: ascension

  The last relic taken speaks once per waking:
  - `found` right after it is taken.
  - Otherwise the first fact it watches that holds (slayers ready, cannon, an undead court, a praying court, strong magic), else `idle`.

  New verb `commune {relic, answer}`:
  - `ask` prints the plain state of what it watches.
  - `refuse` silences it for 3 wakings.
  - `accept` tags `crowned_by:<relic>` and `boon:<agenda>`. The tail is `living_unite` for ascension; for the others, dread rises by 0.1 in every court.
- **Level 3 counterfactual.**
  - At wake the world is copied, only when level 3 holds.
  - After the next sleep, that waking's acts and the sleep run twice from the copy, once with all acts and once without the headline's act.
  - The line says "would not have come to pass" or "would have come anyway".
  - Only acts of the last waking can be removed. The copy is not saved, so a loaded game has no counterfactual until its next waking.
- **Text.** `data/text/<lang>/chronicle.json` holds 53 keys in English and German, read as a fourth bundle part.
- **Interest metric.** `interest.py` reports `causes_shown_share` per level, rendering each waking at levels 0, 1 and 2.
- **Report tool.** `hoard-sim --data data --chronicle-demo 42` prints the playtest screens.

## Numbers

Causes shown, 60 played generated seeds: level 0 0 %, level 1 65 %, level 2 100 % (targets 0 and ≥ 80 %).

## Seed 42 (played, plus a kept historian, books from every court that writes, a raid for a relic)

```
Waking A · year 550 · level 0
- Nimieth entered the iron age.

Waking B · year 560 · level 2, historian Halric the Bold
- [headline] Kin of a captive held by you came from Alddale to your ledge.
- The tyrant of Cordale, risen in 500, died in 560.

Waking C · year 1270 · the historian is dead, 21 sources held
- [level 1] Rhonsary declared war on Berley. The Chronicle of Rhonsary lays it at the door of Berley. The Chronicle of Berley lays it at the door of Rhonsary.
- [level 1] Ghazgash declared war on Cordale. The Chronicle of Cordale lays it at the door of Ghazgash.
- [level 0] Your fire of 970 left Goldstead a waste in 1070.

Waking B2 · year 1640 · level 3
- [headline] A premonition woke you: the slayers of Nimieth ... were on their way to your lair, because your fire over Gorgewaste in 1630 was never forgiven; a bold hero led them. Even without your fire over Gorgewaste in 1630, the slayers of Nimieth at your door in 1640 would have come.

Waking D · year 2030
Voice: The iron crown, taken from Tormarch, speaks when you are alone: "Kings knelt under me. They can kneel again."
commune(ask): The iron crown answers you plainly. Nobody has cannon yet. Slayers are ready in the valley.
```

## Decisions taken

- "Lore ≥ 25" read as growth points; ranks never get there (rank 3 at year 3540 on seed 42).
- "A source about the court" read per court, as the brief's scope 1 says: books taken from a court let the dragon read that court's telling of later events too. A book that only covered events before it was taken would almost never meet a briefing line, since briefings are about the sleep that follows.
- Pattern lines that name their cause in their own text (item 14) stay at every level. Open question for the owner below.
- Relics live on the dragon and are hashed, so the state goldens move (rules version 8). Chronicle hashes are unchanged.
- Agenda facts for `ask` are what each voice watches. When item 04 lands, `ask` should print its ending's condition state from the Endings block.

## Not done

- Scope 6, tag filtering: lines carry no court, race, era or mood tags yet. Only the two-cause limit and "no cause twice" are built.
- The `because` and counterfactual clauses quote the narrator's English briefs, because the narrator has no German file. The surrounding templates are German.
- The page does not render `level`, `sources`, `voice` or `chronicle` yet; the texts are appended to each line's `text` (item 13).

## Open for the owner

- With no historian, should pattern lines that carry their own "because" still show, or only the bare event?
