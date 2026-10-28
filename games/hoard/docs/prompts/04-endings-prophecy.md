# 04 · Endings, prophecy, legends across runs

Model: Opus 5.5 · Effort: high · Depends on: 02 · Branch: `item/04-endings-prophecy`

## Goal

Give the run a target. Endings are condition sets on world state, evaluated every decade, each with a continuity window, exposed as progress lines on the wake screen and offered as the `exit` act when they hold. Three ship first: Mortality (cheapest), The Myth (a qualifying death that enters the next world's pre-history), Divine Ascension (hardest). One prophecy per seed is generated at hatching from the ascension conditions in riddle form; mortals know it and react through three cards. Knowledge carries between runs through a legends file; power never does.

## Notes for this tree (2026-10-28)

- `data/events/prophecy.json` already holds the v0.4 `prophecy` and `prophecy_fulfilled` cards, and `sim/ages/legends.cpp` builds the hatching legends from pre-history (`LegendsDef` in `genesis_data.cpp`). Extend both; do not start a second prophecy.
- `offers.cpp` already proposes a `rite` with motive `exit` when faith and era are high. Fold it into the registry rather than keeping two exit paths.
- No `witch` or `god_voice` tag exists. For Mortality, use a living `sage` or `priest` in the court as the stand-in and name it in the report.
- Endings decided by the owner (D6): Mortality, The Myth, Ascension.
- Rules version is 7 (`sim/ages/record.h`). The golden recipe deletes `tests/golden/ages_v0*.hash` and `sift_bank_v1.hash`, runs `build/test_golden` twice, then regenerates `state_hashes.txt` (300 seeds), the replays and the save check; see `reports/14-story-sifting.md` for the commands.
- Rules live in `sim/ages/rules/`; the act code in `sim/ages/acts.cpp`; people in `rules/people.cpp` and `rules/people_rise.cpp`. `Person` (`sim/ages/state.h`) has `PersonTraits{bold, pious, greedy, cunning}` as doubles and `memory_of_dragon`.
- Player text is in `data/text/<lang>/*.json` (`narrator.json`, `ui.json`, `patterns.json`, ...), never in `en.json`; every new key needs a `de` line too (`node web/l10n-check.cjs --lang pseudo,de`).
- New rules draw from `rng_for(seed, tag, id)` (`sim/ages/rng.h`), never `world.rng`, so no existing draw moves.
- A new state-changing verb needs a wrapper in `sim/ages/recorded.cpp` and a case in `apply_input`.
- Item 14 built the sifter in `sim/sift/` (`briefing_lines`, `make_source`, `sources_conflict`, `because_chain`; all take `Params`). Item 06 built arrivals (`data/arrivals.json`, the `receive` and `hunt` verbs); `broker` and `keep` do not exist yet (item 09).
- Items 04, 05 and 07 run in parallel on separate branches. Touch shared files (`state.h`, `serialise.cpp`, `briefing.cpp`) in small, separate commits so merges stay simple.

## Read first

- `docs/story-and-endings.md` sections 2 and 3.
- `docs/research-faith-and-culture.md` section 5 "Ascension check" (the seven conditions; item 12 supplies the cult fields, so this item defines the condition interface with placeholders that read awe and faith until then).
- `docs/act-api.md` section 2.7 (`exit`) and `act-api.schema.json`.
- `docs/core-loop.md` section 6 (genesis, legends, pre-history) and `docs/research-book-procedural-storytelling.md` sections 3.6 (count master stories) and ch 14 (per-seed variables that reframe the bank).
- `docs/research-storylets.md` section 4 "For Hoard" (legacy: knowledge carries, never power).
- `rules/cards.cpp`, `card_match.cpp`, `cards.h` (the predicate language), genesis code (S2 in v0.4), `data/text/<lang>/`.

## Scope

In:

1. **Ending registry** `data/endings.json`: one entry per ending with `id`, `conditions[]` (each a `card_match` predicate plus `hold_years` and `lapse_years`), `offer_text_key`, `epilogue_text_key`, `exit_kind`. A new rule `rules/endings.cpp` after cards: for each ending, per condition, track `held_since` and `last_lapse`; a lapse longer than `lapse_years` resets `held_since`. An ending is `ready` when every condition has held for `hold_years`. World gains tag `ending_ready:<id>`; the briefing gets an "Endings" block with per-condition state (met, years held, what is missing in words).
2. **The `exit` act**: accepted only when `ending_ready:<kind>` holds; emits `ending` event with the epilogue; runs 50 idle decades as epilogue (catalogue entry "The ascension") and stops. The shell shows the epilogue chronicle.
3. **Mortality**: conditions: a person tagged `witch` or `god_voice` alive in a court with `awe_eff ≥ 0.4` and the dragon has `received` or `brokered` with that court within 2 wakings; hold 1 decade. The offer arrives as an audience (item 06) or, until then, as an offer row. Epilogue: the dragon as a mortal in that court's chronicle for one lifespan.
4. **The Myth**: not an exit. On `dragon_slain`, if the dragon's reign is ≥ 500 years, at least two courts hold it at monumental tier, and its name appears in ≥ 3 legends, the death qualifies: the death screen says so and the run writes a legend entry to the profile-level `legends.json` (outside any save): `{world_seed, name, reign, killer, deeds[] (top 5 acts by magnitude × witnesses), ending: "myth"}`.
5. **Legends across runs**: genesis reads `legends.json` and, for each entry, with probability 0.5 per entry (at most three legends active per world, picked by verdict extremity), weaves one to three events into the new world's pre-history at years −900 to −200 with `src: legend` (a court worships or fears "the old dragon of the west", a slayer order traces its founding to its death, a monument stands, a library has a gap) as tags and a legend text, never as a stat bonus. The legend's memory starts at the tier it was recorded in, so an old oral legend may already be fading. The first screen shows it. The profile holds five files, all knowledge (`research-meta-progression.md` section 1): `legends.json` (entry shape in that doc's section 3), `ending_atoms.json` (per ending, per condition: `rumor | witnessed | used` and how it was learned; a condition becomes `witnessed` only when a briefing showed it true, never by guessing), `seen.json` (scenario seen-log, item 10), `bestiary.json` (slayer-order templates and anti-dragon technologies met, with the stage at which they were seen), `glossary.json` (prophecy phrases mapped to the conditions they turned out to encode, unconfirmed until a run proves them). Every death report adds the one to three conditions that were closest to firing to `ending_atoms.json` as `rumor`. Give each condition at least three independent rumor routes (a source mentioning another dragon, a level-3 counterfactual, a glossary entry, the start-of-run page) so one missed scenario never locks an ending out.
6. **Ascension**: the seven conditions from the research doc as predicates; where a condition needs cult fields that do not exist yet (`cult share`, `fervor`, `high priest`), read `awe_eff` and `faith` with the thresholds scaled, and leave a `TODO(item12)` marker in `endings.json` only (not in code) so item 12 swaps the predicate. Hold 150 years, lapse 30. Condition 6 (some court has slayers ready or cannons) stays as written: the ending needs the mortals who can kill you.
7. **Prophecy object**: at genesis, generate `world.prophecy = {ending: "ascension", lines[]: text keys with slots, known_by: [court ids], year}` from the ascension conditions through templates in `data/text/<lang>/` (`prophecy.loved_by_peoples`, `prophecy.house_of_stone`, `prophecy.old_craft`, `prophecy.one_voice`). It exists as a book item in one court's library (the most lettered) and is among the legends shown at hatching. A `take books` from that court or `teach(letters)` by that court to another spreads `known_by`.
8. **Mortals react**: three cards in `data/events/prophecy.json`: `prophecy_cult` (a court that knows the prophecy with `awe_eff ≥ 0.3` and piety > 0.5 founds a cult that works toward the conditions: faith growth ×1.5, builds a shrine), `prophecy_slayers` (a court that knows it with `dread_eff ≥ 0.4` and militarism > 0.5 founds slayers early: `anti_dragon` +0.2 once), `prophecy_schism` (a court holding both a dragon cult and the prophecy splits the cult: Order vs Reformed, as in the paper playtest waking 3). Each names the prophecy event in its cause refs.

Out: lichdom, portal, mountain, brood, emperor, last flame, scholar, sun-eater (registry entries may exist with `enabled: false` and a one-line condition sketch each, so the data shape is proven against all eleven); cult mechanics (item 12); the page (item 13, but expose everything it needs through `world` and `briefing`).

## Design notes

- Conditions use the existing `card_match` predicate language so authors write them like cards. Extend the language only if a condition cannot be expressed; list each extension in the report.
- The profile-level `legends.json` path comes from the shell (`load` gets a `profile` argument); the headless runner takes `--profile path`. Tests run with an empty profile.
- Difficulty check: over 300 played seeds × 3000 years, ascension should be `ready` in under 5 % of runs and Mortality in 20 % or more. If ascension never gets ready, report which condition blocks and its best value reached; do not loosen the thresholds in this item.
- Inter-run pacing (targets from `research-meta-progression.md` sections 2 and 6, to be tuned later, measured now): three bots, amnesiac (empty profile), carry (reads `ending_atoms` and `bestiary` to steer), oracle (all conditions known); report P(ending reached by run N) for N = 1 to 12 and the knowledge leverage P_oracle / P_amnesiac at N = 1 (aim ≥ 3 on ascension, ≤ 1.2 on Mortality). Proposed targets: a first real ending (Mortality or Myth) by run 2, a second kind by run 4, ascension by run 8 to 10. A difficulty ladder (Short Memory, Quick Study, No Premonition, Hungry Hoard, Early Orders, Few Wakings, Sparse Dreams, Blind Prophecy, Blank Page) is data only in this item: `data/modifiers.json` with the sim parameter each one changes, no UI.

## Acceptance

- `data/endings.json` with 11 entries, 3 enabled; `rules/endings.cpp` evaluates them; the briefing shows an Endings block with per-condition state.
- Seed 42 played: the prophecy appears in the hatching legends, is held by one court, and at least one of the three cards fires within 1000 years in ≥ 30 % of 300 seeds.
- The Myth: a scripted death at year 900 in a world with two temples writes a legend entry; a fresh genesis with that profile shows a pre-history line about it.
- Ascension ready in < 5 % of played runs; Mortality in ≥ 20 %.
- Goldens: the prophecy changes genesis; record new goldens in the separate commit.

## Tests (confirmed by the owner, 2026-10-28)

1. `ending_continuity`: a condition that holds 140 years, lapses 40, holds 20 is not ready; one that holds 150 with a 20-year lapse is.
2. `myth_legend_roundtrip`: `dragon_slain` under the qualifying state writes `legends.json`; genesis with it adds exactly one legend tag.

## Report

Standard report, plus: per ending, the share of 300 played runs where each condition was ever met and where the ending was ready; the prophecy text for seed 42; the card fire rates.
