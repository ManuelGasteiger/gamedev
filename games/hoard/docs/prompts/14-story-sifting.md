# 14 · Story sifting: patterns, threads, headline selection, line budget

Model: Opus 5.5 (or Fable 5.1) · Effort: high · Depends on: 02 (05 and 10 build on it; can run in parallel with 05 if the interfaces below are agreed first) · Branch: `item/14-story-sifting`

Corrected 2026-10-27 to the tree: event templates are in `data/text/en/narrator.json`, importances in `data/importance.json` (item 19); the pattern schema, vocabulary, seed bank and lint are in `docs/patterns/` and `tools/pattern_check.py`, with `text_key` removed (keys are `pat.<id>.*`, `data/text/<lang>/patterns.json`). The measured sim cost is about 0.5 to 1 s per 3000 years on this box, not 10 ms (audit §11 item 4): the acceptor budget is 30 % of the measured step time. Tests (Manu, 2026-10-27): `sift_acceptor`, `sift_budget`, the golden `sift_bank_v1.hash`.

## Goal

Replace "importance per event template plus thread grouping" with a sifter: a bank of story patterns matched incrementally inside the sim step (partial matches are save state), a thread record that spans sleeps, a scored headline pick, a line budget that depends on the sleep length, and callback lines when a thread the player saw earlier ends or comes to nothing. Court sources (item 05) become transforms over the one true thread, and contradictions between them are computed, never guessed.

## Read first

- `docs/research-story-sifting.md` entirely; section 2 has the pattern mini-language and the thread record, section 4 the tiers and the line budget, section 5 the thread score, section 6 the source transform, section 7 the metrics.
- `docs/ages-sim.md` 17.1 (event ids and causes), 17.5 (the briefing), 18.2 (importance and the narrator), 18.8 (threads as built: `Event.thread`, war and hegemony episodes, collapse grouping).
- `docs/paper-playtest-wakings.md` (the three screens the output must be able to produce; in the Project) and `docs/core-loop.md` section 5 (wake screen order) and section 8 (the storyteller cadence this item reuses as the tension arc).
- `chronicle.cpp`, the briefing code, `rules/cards.cpp` and `card_match.cpp` (the predicate machinery; reuse what fits), `data/text/en/narrator.json`.

## Scope

In:

1. **Pattern bank** `data/patterns/*.json` in the mini-language of research section 2. A schema (`docs/patterns/pattern.schema.json`), a kind vocabulary (`docs/patterns/vocab.json`, best effort; align it with the real event kinds in `narrator.json` first and report the differences), thirteen hand-written patterns (`docs/patterns/seed.json`) and a lint (`tools/pattern_check.py`) exist and pass; start from them. The language: ordered clauses with `kind`, `subject`, `object`, `tags_all/any/none`, `magnitude_gte`, `caused_by` / `not_caused_by` over cause refs with a hop limit, `within_years`; `?x` variables bound on first use; `unless` (kills a partial), `unless_forever` (bars that cast), `once_per`; `emit` writes a compound event (level 1 episode or level 2 story) whose cause refs are the kernel events. A level-n pattern sees only events below level n. Start bank: 20 to 30 patterns, hand-written, covering the catalogue's long tails.
2. **Acceptors** `sim/sift/acceptor.cpp`: Winnow-style incremental matching inside the decade step: index open partials by the `kind` of their next clause and their `unless` kinds; on an event, a partial dies on an unless hit, forks when the event fits its next clause (the parent keeps waiting), else passes; stop forking once all non-event variables are bound; prune partials whose window lapsed or whose bound person died (unless `survives_death`); cap open partials at 64 per pattern, evicting the lowest-scored. Measure the cost against the measured step time and report it; if it exceeds 30 %, reduce the bank or the cap and say so.
3. **Thread record** (save state): `{match, pattern, cast, events[], stage, of, state: latent | open | closed | cut, opened, advanced, shown[]}`. A partial becomes `open` at a third of its clauses (Loose Ends). The wasm `world` export lists open and recently closed threads.
4. **Thread score** per research section 5: dragon link 0.20, stakes 0.15, causal density 0.15, continuity 0.15, proximity 0.10, surprise 0.10, reversals 0.10, irreversibility 0.05; +0.20 for closing or cutting a thread already shown, +0.30 at tier 5, −0.25 if the pattern headlined in the last 3 wakings. Surprise uses per-(pattern, era, property) base rates computed offline by the dry run and shipped as data (`data/patterns/base_rates.json`), so it is a table lookup and determinism holds; merge properties that co-occur above 0.8. All weights in `data/constants.json`.
5. **Headline selection** in Façade's order: candidates, top priority tier, score, arc-fit multiplier `exp(−2·|T_target − T_thread|)` with `T_target` from the existing storyteller cadence (a major waking wants high stakes, a breather medium), then a seeded draw among the top 3 in proportion to score. One pattern never headlines two wakings in a row. A diversity penalty on same-court lines falls from 0.20 at waking 1 to 0.05 at waking 30; a bonus for threads tagged with the seed's prophecy or an ending condition rises from 0 to 0.15.
6. **Tiers and budget**: event templates get a tier 0 to 5 (0 log only, 1 courts table, 2 may fill a line, 3 may headline, 4 always a line in its waking, 5 irreversible valley change: always shown, outside the budget). Keep tiers 4 and 5 under 15 % of templates. Line budget `L(y) = min(9, 3 + ⌊2·log10 y⌋)` excluding headline and tier 5. Granularity by sleep length: under 30 years a thread may take two lines event by event; 30 to 299 one line per thread; 300 and up an ages block first (one line per age, at most three, ages cut at era changes or a new hegemon and named after it), then thread lines. Fill greedily above a score floor of 0.30 with −0.15 per picked line about the same court and −0.20 per line from the same pattern; never pad; minimum 2 lines; at most 2 callback lines inside L. Ordinals for repeated episodes ("the third siege of Kell Hold").
7. **Callbacks**: a thread with a non-empty `shown` whose stage or state changed since its last showing is a candidate with text keyed by transition (closed with its outcome, cut "came to nothing", advanced). A shown thread that is cut always yields a candidate. The waking delta gives "two wakings ago"; beyond five wakings, the year.
8. **"Because" rendering** Labov's way: the headline event is the most reportable (importance × magnitude); the chain walks cause refs backward and stops at the first event with importance ≤ 1 or no cause. Chronicle level 2 prints one hop, level 3 three (item 05 owns the level filter; this item supplies the chain). IPOCL gate: a person-centred line needs a motivating event among its cause refs (memory entry, grudge, oath), else the event prints alone.
9. **Source transform** for item 05: a court source is `{match, court, medium, start, omit[], blame, passive[], stated_cause, hedge, archetype}` over one true thread. Rules: `start` is the earliest event in the backward chain whose agent is another party; `omit` lists events where this court harmed another; `blame` is the dragon at dread ≥ 0.6, else the court's top grudge; `passive` covers its own surviving acts. Oral sources re-roll `blame` each generation with p = 0.15 toward the court's current enemy and carry `hedge` 1; written sources freeze; monumental ones keep only the most reportable event and its blame. Two sources conflict when `blame` differs, when `start` differs across a dragon act, or when one omits an event the other prints; the briefing marks the second as disputed. Stated causes are sim facts a pattern can sift (`misdirected_vengeance`).
10. **Interest tool** gains research section 7's metrics.

Out: chronicle levels, the historian and the artifact voice (item 05), scenarios (item 10), the page (item 13).

## Interfaces agreed with item 05

`sift.h`: `std::vector<ThreadLine> briefing_lines(const World&, int since_year, int budget_override = -1)` returning lines with `{thread_id, tier, score, kind: headline | line | callback | age, text_key, params, because_chain: [event ids], sources: [source ids]}`; `Source make_source(const World&, MatchId, FactionId)`; `bool sources_conflict(const Source&, const Source&)`.

## Design notes

- `chronicle_hash` must not include thread scores or line selection (presentation). Pattern `emit` events are part of the chronicle and therefore hashed: goldens change when the bank is loaded. Run the golden suite with the bank off and add one golden with the bank on.
- Lessard et al. found rarity does not predict enjoyment; keep the surprise weight small and the dragon-link weight large. The human check (3 raters, 15 pairs) is the owner's job per build; log the pairs the tool would show.
- Determinism: the seeded draw uses (seed, waking, bank hash).

## Acceptance

- Seed 42 under `played`: a war won because of a dragon act is produced by a pattern (war_won_by_dragon_act) with the because chain reaching the act; two disagreeing accounts come from `make_source` for two courts over one match with differing `blame`. (The paper playtest's names are mock-ups; the shape is what counts.)
- Over 300 played seeds: every metric in point 10 inside its band or reported with the reason; no pattern headlines above 15 %; shown threads get a closing or cut line ≥ 60 % of the time.
- Acceptor cost under 30 % of the measured step time at the start bank size.
- Goldens unchanged with the bank off; one new golden with the bank on.

## Tests (confirmed by the owner)

1. `sift_acceptor`: a scripted log with burn → order founded (caused by) → slayer attack completes `vengeance_for_burning` once per court; an oath sworn between the first two clauses bars that cast for good.
2. `sift_budget`: line counts for sleeps of 1, 10, 100, 1000 years are 3, 5, 7, 9 plus headline plus tier-5 lines.
3. Golden `sift_bank_v1.hash` (seed 42, played, bank on).

## Report

Standard report, plus the acceptor cost table, the per-pattern completion rates over 300 seeds, the headline-share table, and three example briefings (10, 100, 1000 years) rendered as text.
