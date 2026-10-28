# 05 · Chronicle levels, historian, sources, artifact voice

Model: Opus 5.5 (or Fable 5.1) · Effort: high · Depends on: 02, and the interfaces of 14 (`briefing_lines`, `make_source`, `sources_conflict`; see `prompts/14-story-sifting.md`) · Branch: `item/05-chronicle`

Note (2026-10-24): item 14 now owns thread detection, headline scoring, the line budget, the because-chain and the source transform. This item owns the chronicle level as a filter over 14's `because_chain`, the historian as a lair role, sources as hoard objects (using 14's `make_source` for their content), and the artifact voice. Where this brief below describes picking stated causes or printing because-lines, read it as calling item 14.

## Goal

Make understanding a progression. The sim already records every event's cause (`Event.cause` with refs, 17.1). This item decides how much of that the briefing may print, by chronicle level: 0 dreams (headlines, no causes), 1 sources in the hoard (each court's biased account), 2 a kept historian (the true "because" lines), 3 high Lore (one counterfactual per headline). It adds sources as objects, the historian as a lair role, and a second voice: an artifact with an agenda that tells the truth selectively. No new causal model; this is a filter and two kinds of record.

## Notes for this tree (2026-10-28)

- D7 (owner, 2026-10-28): the artifact voice is not present from hatching. It speaks only once the dragon holds an item with an agenda. Do not seed one at genesis.
- Item 14 owns the because-chain and the source content. This item calls `because_chain` and `make_source`; the briefing JSON already has `lines`, `sources` (with `disputed`) and `sift_pair`.
- Item 07 runs in parallel and owns person traits and `lettered`. Read the historian's eligibility from `role` (sage, priest, scribe) now and from a `lettered` tag when it exists.
- Rules version is 7 (`sim/ages/record.h`). The golden recipe deletes `tests/golden/ages_v0*.hash` and `sift_bank_v1.hash`, runs `build/test_golden` twice, then regenerates `state_hashes.txt` (300 seeds), the replays and the save check; see `reports/14-story-sifting.md` for the commands.
- Rules live in `sim/ages/rules/`; the act code in `sim/ages/acts.cpp`; people in `rules/people.cpp` and `rules/people_rise.cpp`. `Person` (`sim/ages/state.h`) has `PersonTraits{bold, pious, greedy, cunning}` as doubles and `memory_of_dragon`.
- Player text is in `data/text/<lang>/*.json` (`narrator.json`, `ui.json`, `patterns.json`, ...), never in `en.json`; every new key needs a `de` line too (`node web/l10n-check.cjs --lang pseudo,de`).
- New rules draw from `rng_for(seed, tag, id)` (`sim/ages/rng.h`), never `world.rng`, so no existing draw moves.
- A new state-changing verb needs a wrapper in `sim/ages/recorded.cpp` and a case in `apply_input`.
- Item 14 built the sifter in `sim/sift/` (`briefing_lines`, `make_source`, `sources_conflict`, `because_chain`; all take `Params`). Item 06 built arrivals (`data/arrivals.json`, the `receive` and `hunt` verbs); `broker` and `keep` do not exist yet (item 09).
- Items 04, 05 and 07 run in parallel on separate branches. Touch shared files (`state.h`, `serialise.cpp`, `briefing.cpp`) in small, separate commits so merges stay simple.

## Read first

- `docs/story-and-endings.md` section 1 and `docs/paper-playtest-wakings.md` (the three screens this item must be able to render; the closing note).
- `docs/research-book-procedural-storytelling.md` sections 3.2 (perspective rationalisation, the source record), 3.7 (tagged text with contradiction filtering), ch 15 and ch 17 "For Hoard".
- `docs/research-storylets.md` section 3 "For Hoard" (briefing voices follow Six Ages' advice rules: priority line, two fallbacks, no two voices making the same point, insight gated by skill).
- `docs/research-social-sim.md` section 1 "For Hoard" (one event stored once, rendered per viewpoint).
- `docs/ages-sim.md` 17.1 (ids and causes), 17.5 (the briefing), 18.2 (importance and the narrator), 18.4 (keepers of memory).
- `docs/act-api.md` 2.5 `commune`, 2.6 `keep`.
- `chronicle.cpp`, the briefing code, `data/text/<lang>/`.

## Scope

In:

1. **Chronicle level** `world.chronicle_level` derived each waking: 0 by default; 1 if the hoard holds at least one source about the court or event in question (per line, not global); 2 if a living historian is kept at the lair; 3 if `dragon.lore ≥ lore_counterfactual` (start 25) in addition. The level is per briefing line: a line about court X at level 1 needs a source about X.
2. **Briefing filter** in the narrator: level 0 prints the event sentence without its `{,why,}` clause; level 1 prints the *stated* cause from the best source about that court (section 3) with the source named ("the Chronicle of Dorun says …"); level 2 prints the true cause from `Event.cause`; level 3 adds one counterfactual line for the headline event, computed by re-running the sleep on a copy of the world with the headline's deepest dragon-act cause removed from the log and diffing the two chronicles for the first divergent major event (the sim is fast enough: one extra sleep per waking). Two sources about the same court that disagree are both printed, one line each, at level 1.
3. **Sources** as objects. When an event with importance ≥ `source_min_importance` resolves, every court that witnessed or suffered it writes a `Source {id, event_id, author_court, year, true_cause_ref, stated_cause_ref, stated_cause_key, medium: oral | written, lost_if: library_burned | court_collapsed}`. The stated cause is picked from the author's own properties: a grudge tag, an affinity, a `deceived_dragon` mark, piety (divine judgement), with the true cause kept when none fits and the court is `honest` (openness > 0.6). Sources live in the court's library (`written`) or as a tale (`oral`, decays with the oral tier). `take books` from a court brings a slice of its sources into the hoard (`Dragon.sources[]`); `give books` hands them on; a held royal contributes their court's sources at level 1 for their own court only. `burn(library)` destroys the written sources of that court.
4. **Historian** as a `keep` role (item 09 lands the act; until then, a lair command `keep_person {id, role}` in the API). A historian must be a person with `role: sage | priest | scribe` or `trait: lettered`. While alive and at the lair, `chronicle_level ≥ 2`. The historian ages by race on the sleep tempo (17.2, 18.1) and dies; the briefing says so, and the chronicle has a gap from that year. Long-lived keepers (elf, undead scribe, kobold line via the Kobold Servants talent, a mortal who sleeps in the hoard and ages at a tenth) are the puzzle; implement elf and `sleeps_in_hoard` (tag, ages ×0.1) now, the rest as tags read by the ageing rule for later items.
5. **Artifact voice.** Items gain `agenda` (`lichdom`, `ascension`, `emperor`, `sun_eater`, none). At most one speaking item is active at a time (the most recently taken with an agenda). Each waking it may speak one line (templates keyed `voice.<agenda>.<state>`), chosen by a priority list over world state (slayers ready, cannons researched, a cult founded, a condition of its ending met or missing). It never states a fact the sim contradicts; it leaves things out. `commune {accept | refuse | ask}`: `ask` prints the true condition state for its ending (one free level-2 line); `accept` grants a boon as a talent-like tag plus `crowned_by:<item>` on the dragon, which biases that ending's conditions (lichdom: an undead court's priests may offer; ascension: faith floor +0.1 in temple courts) and adds a tail (the living unite, or dread +0.1 everywhere); `refuse` silences it for 3 wakings.
6. **Tagged text** for the briefing: every template line carries tags (court, race, era, mood); a line whose tag conflicts with current state or with a line already printed in this briefing is dropped; "because" clauses hold one or two causes from the fixed cause vocabulary, never more.

Out: scenario text (item 10), the page rendering (item 13; expose `briefing.lines[].level`, `.source`, `.voice`), person-level memory (item 07).

## Design notes

- Determinism: sources and the voice line are chosen with `world.rng`; the counterfactual re-run uses a copy with a forked rng seeded from (seed, waking) so it cannot perturb the main world.
- Cost: one extra sleep per waking at level 3 only.
- Serialization: sources are part of the save; old saves load with an empty source list and level 0.
- Keep `chronicle_hash` independent of level, sources and voice lines (presentation), so goldens change only if genesis or rules do.

## Acceptance

- Seed 42 under the played policy renders the three wakings of `paper-playtest-wakings.md` in structure: waking 1 at level 0 (no causes), a waking with a kept historian at level 2 (true "because" lines), a waking after the historian's death at level 1 with two sources about one event disagreeing.
- `interest.py` gains `causes_shown_share` per level; level 2 shows ≥ 80 % of available cause clauses, level 0 shows 0.
- A `burn(library)` on a court removes its written sources from later `take books`.
- An item with agenda `lichdom` speaks within 2 wakings of being taken; `commune(ask)` prints the lichdom condition state.
- Goldens unchanged (presentation only) unless the agenda boon is taken, which is an act and therefore in the log.

## Tests (confirmed by the owner, 2026-10-28)

1. `chronicle_levels`: the same sleep rendered at levels 0, 1, 2 yields 0, stated, true cause clauses for a fixed event.
2. `sources_bias`: a dwarf court with `grudge:<orc>` records an orc-caused sack with `stated_cause = grudge` while an `honest` human neighbour records the true cause.

## Report

Standard report, plus the rendered waking 2 and waking 3 texts for seed 42 (as the page would show them), and the count of templates added to `data/text/<lang>/`.
