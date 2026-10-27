# Story sifting and headline selection: a model for Hoard's briefing

Status 2026-10-24. How the briefing picks what to tell after a sleep of 10 to 3000 years. Condensed copy for item 14: the "For Hoard" parts, the pattern language, the score and every table in full, the findings shortened. The full text and the numbered sources the brackets refer to are in the Project (`claude/research-story-sifting.md`).

## 1. Ryan: curation, story recognition, story sifting

- The dissertation argues that emergent stories are closer to nonfiction because they actually happen in the storyworld, and that emergent narrative fails when raw simulation output is taken for a narrative. Its remedy, curationism, curates that material into a narrative artifact and presents it as a media experience [1].
- The architecture: a chronicler records what happens, a story sifter searches the chronicle for narratively potent material, a narrativizer turns it into a story [1].
- "Story recognition" (2015) is the earlier name. Trusting player traces was rejected, since a Prom Week study showed players do not reliably play in storylike ways; author-specified event sequences and story-understanding frameworks were the proposed routes [2].
- Kreminski credits the 2018 dissertation with coining "story sifting", notes that Ryan's Sheldon sifter wrote its patterns as procedural Python, and cites Ryan's "sifting heuristics" as higher-level encodings of tellability [3][4].
- Tellability (secondary, Short on ch. 4): take the most interesting final event, walk back through causally linked events, stop at one that needs no explanation, a method taken from Labov [5]. Labov's own account matches: narration starts at the most reportable event and recursively finds causes until it reaches an unremarkable one [6]. The most reportable event is the least common one with the greatest consequences; reportability costs credibility, and the causal chain buys it back. Where the teller starts the chain is a choice [7].
- Ryan's "pains" include utility-driven systems whose causes are too diffuse to narrate (Prom Week), remedied by causal bookkeeping [5]. Improbability is among the pleasures he lists [8]. He names overgenerate-and-curate as the strategy [9]. Uncertain: these rest on Short's summaries.
- Bad News live: a hidden wizard live-codes the player's commands and queries the simulation for dramatic fragments sent to the actor by chat [10][11].

**For Hoard**
- Name the stages in code: the sim log is the chronicler, patterns (§2) the sifter, the briefing and the court sources the narrativizers. Cause refs are Ryan's causal bookkeeping and already exist.
- Render a thread Labov's way: its headline event is the most reportable one (max importance × magnitude). The "because" chain walks cause refs backward and stops at the first event with importance ≤ 1 or no cause. Historian level prints one hop, high Lore three.
- Bad News split for the historian: the sifter ranks, the kept historian picks from the top 5 with its own bias (+0.1 for threads naming its court of origin). The dream level shows the sifter's raw top pick.

## 2. Kreminski and the pattern languages

- **Felt** (ICIDS 2019): Datalog queries over DataScript; ordered event sets with other events allowed between; `not-join` excludes intervening events; recursive rules give indirect causation. Four interns wrote 85 actions in a week after one day of training [12].
- **Winnow** (AIIDE 2021): event clauses form an ordered kernel, `?` variables unify across clauses, and clauses may call inference rules such as an `ancestor` test over causal pointers. Each pattern compiles to an acceptor: on a new event a partial match dies on an unless-hit, forks when the event fits its next clause (the parent keeps waiting), and otherwise passes. Cost per event in JavaScript averaged 93 ms at a pool of 100 and 912 ms at 1000. Proposed pruning: drop matches idle for K events or whose bound character died, and stop forking once all non-event variables are bound. No ranking [3].
- **Synthesifter** (INT 2020) builds patterns from examples; overfitting to incidental properties is the risk [13].
- **Select the Unexpected** (ICIDS 2022), the only ranking heuristic with a rater test. A property's likelihood is count(p)/N over the N matches of the same pattern, and L(m) = (1/|P_m|) Σ count(p)/N; lower is more surprising. 38 of 45 judgments agreed with the heuristic (84.4%). Weaknesses: correlated rare properties count twice, and rarity is judged within the pattern only [14].
- **Authoring for Story Sifters** (2022): give each event several tags; write actions first, look for surprising stories, then write patterns; never write actions only to satisfy patterns; keep movement-level noise out of the chronicle. Incremental sifting allows foreshadowing [4].
- **Why Are We Like This?** (FDG 2020): action preconditions are Felt patterns; players see the 5 top-ranked actions [15].
- **Loose Ends** (AIIDE 2022): 12 Winnow templates serve as storytelling goals; unadopted partial matches past 33% completion are promoted to visible goals [16].
- **Composable patterns** (Lyu, Millard, Gibbins, FDG 2025): JSON patterns with `unless`, `unless-forever` and a time limit. A completed pattern emits a high-level event into the same log, and story patterns match those: three levels from 7 patterns. About 1 in 4 started high-level events and 1 in 5 started stories completed; a run held 350 to 450 partial and 100 to 150 completed matches [19].

**For Hoard: a pattern mini-language.** JSON, matched incrementally inside the sim step by C++ acceptors in Winnow's style, so partial matches are save state.

```json
{
  "id": "vengeance_for_burning",
  "level": 2,
  "clauses": [
    {"as": "a", "kind": "burn", "subject": "dragon", "object": "?site",
     "tags_any": ["city", "capital"], "bind": {"?court": "a.object.court"}},
    {"as": "b", "kind": "order_founded", "subject": "?court", "object": "?order",
     "tags_all": ["slayer"], "caused_by": {"ref": "a", "max_hops": 3}, "within_years": 200},
    {"as": "c", "kind": ["slayer_attack", "order_disbanded", "slayer_killed"],
     "subject": "?order", "within_years": 300}
  ],
  "unless": [{"between": ["a", "b"], "kind": "court_destroyed", "subject": "?court"}],
  "unless_forever": [{"kind": "oath_sworn", "subject": "?court", "object": "dragon"}],
  "once_per": ["?court"],
  "outcome_by": {"c.kind": {"slayer_killed": "fell", "slayer_attack": "struck", "order_disbanded": "faded"}},
  "emit": {"kind": "story.vengeance", "subject": "?court", "object": "dragon",
           "magnitude": "max(a.magnitude, c.magnitude)", "tags": ["grudge", "dragon"]}
}
```

- Clause fields: `kind` (string or list), `subject`, `object`, `tags_all/any/none`, `magnitude_gte`, `same_thread_as`, `caused_by` and `not_caused_by` over cause refs with a hop limit, `within_years` from the previous clause. Clause order is time order; other events may fall between.
- `?x` binds on first use; `bind` reads a field of a bound entity; `dragon` is a constant.
- `unless` kills a partial; `unless_forever` also bars that cast from re-matching; `once_per` stops repeats per binding.
- `emit` writes a compound event whose cause refs are the kernel events and whose `thread` is the match id. A level-n pattern sees only events below level n, so composition terminates. Levels: 0 sim events, 1 episodes (siege, succession crisis), 2 stories.
- Index acceptors by the `kind` of their next clause and their unless clauses, so an event touches only matches waiting for it. Prune partials whose window lapsed or whose bound person died (unless `survives_death`), cap open partials at 64 per pattern (evict the lowest score), and do not fork once all non-event variables are bound. Winnow's cost grew linearly with pool size; measure Hoard's cost (uncertain).
- Bank: 20 to 30 patterns for v0.5, 40 later. The 300-seed dry run keeps patterns that complete in 10 to 60% of seeds and flags those that complete never or everywhere.

**For Hoard: threads that span sleeps.** Every match is a persistent record:

```json
{"match": 913, "pattern": "vengeance_for_burning", "cast": {"?court": "aldmere", "?order": "o7"},
 "events": [4412, 5120], "stage": 2, "of": 3, "state": "open",
 "opened": 14, "advanced": 210, "shown": [{"waking": 3, "stage": 2, "year": 212}]}
```

- States: latent (under a third of clauses bound), open (a third or more, Loose Ends' threshold), closed, cut (unless hit, window lapsed, cast dead).
- At each waking, a thread with a non-empty `shown` whose stage or state changed since its last showing becomes a callback candidate, with text keyed by transition: closed with its outcome, cut ("came to nothing"), or advanced. The waking delta gives "two wakings ago"; beyond five wakings, say the year.
- A thread that opens and closes inside one sleep is an ordinary candidate. A shown thread that is cut always yields a candidate, so the player never loses a thread without being told.

## 3. Drama managers as pickers

- **Façade**: beats carry a precondition, a weight, a priority, and effects on story values. The sequencer takes the satisfied set minus used beats, keeps the top priority tier, scores each beat by fit to the tension arc, multiplies by weight and draws in proportion [21].
- **Search-based drama management**: plot quality is a weighted sum of 0 to 1 features; plot mixing early, plot homing late [22].
- **IPOCL** (Riedl and Young): coherence through causal links, believability through intention frames: a motivating step, steps toward one goal, a final step reaching it [23].
- **Left 4 Dead** (Booth, GDC 2009): bosses come from a shuffled deck with no back-to-back repeat. Low probability plus high drama makes moments memorable [24].

**For Hoard**
- Headline selection in Façade's order: candidates, priority tier, score (§5), arc-fit multiplier, then a seeded draw among the top 3 in proportion to score. fit = exp(−2·|T_target − T_thread|), T_thread being the thread's stakes for the dragon on 0 to 1.
- SBDM's mixing then homing as a run-phase weight: the diversity penalty (§4) falls linearly from 0.20 at waking 1 to 0.05 at waking 30, and a bonus for threads tagged with the seed's prophecy or an ending condition rises from 0 to 0.15.
- Booth's deck rule: one pattern never headlines two wakings in a row.
- IPOCL gate on "because": a person-centred line needs a motivating event among its cause refs (memory entry, grudge, oath). Without one, print the event alone.

## 4. Summaries of long logs in shipped games

- **Dwarf Fortress** legends mode: ages are named after the dominant powers and list their events chronologically. The XML export groups events into collections (battle, war, site conquered, ...), each with a parent and an ordinal such as "third siege" [26][27].
- **Civ VI** historic moments: 161 moments worth 0 to 5 era score; zero-point moments only mark transitions [28].

**For Hoard: how many lines per sleep**

| tier | use |
|---|---|
| 0 | log only, feeds systems |
| 1 | courts table only |
| 2 | may fill a line |
| 3 | may headline |
| 4 | always gets a line in its waking |
| 5 | irreversible valley change (race gone, court destroyed, new era); always shown, outside the budget |

- Keep templates at tiers 4 and 5 under 15% of the bank (Civ: 20 of 161, 12%).
- Budget excluding headline and tier 5: L(y) = min(9, 3 + ⌊2·log10 y⌋). One year gives 3 lines, 10 years 5, 100 years 7, 1000 years 9.
- Granularity follows y. Under 30 years a thread may take two lines, event by event. From 30 to 299 years, one line per thread. From 300 up, an ages block comes first: one line per age, at most three, ages cut at era changes or a new hegemon and named after it as DF does, then the thread lines.
- Fill greedily above a score floor of 0.30, with −0.15 per already-picked line about the same court and −0.20 per line from the same pattern. Never pad: show fewer lines if fewer qualify, minimum 2, the rest from raw tier 2+ events.
- At most 2 callback lines per briefing, inside L.
- DF's ordinals for repeated episodes: "the third siege of Kell Hold".

## 5. Is it a story: metrics and rater evidence

- **Causal density.** Statements with more direct causal connections were more often included in readers' summaries [33].
- **Surprise.** StU's 84.4% over 45 judgments [14]; tellability rises with unexpectedness and closeness [34][35].
- **The strongest negative result.** Lessard et al. (FDG 2026): neither improbability nor dramatic-situation heuristics related significantly to enjoyment; world improbability and narrative improbability differ, and narration counted as much as material [36].
- **Character continuity.** Indexter: recall was faster with a shared protagonist [37][38].
- **Reversals.** Retellings with none felt flat, up to about three felt best, many felt incoherent [39].
- **Goal relevance.** IPOCL [23].

**For Hoard: the thread score.** Features on 0 to 1, from the log and dry-run tables.

| term | definition | weight |
|---|---|---|
| D dragon link | 1 if a kernel event is a dragon act; 0.6 if one descends from a dragon act within 4 cause hops | 0.20 |
| S stakes | max over kernel events of importance/5 × magnitude percentile within its era | 0.15 |
| C causal density | cause links among the thread's events / (events − 1), capped at 1 | 0.15 |
| K continuity | share of kernel events sharing the protagonist, +0.5 if the protagonist was in an earlier briefing, cap 1 | 0.15 |
| P proximity | 1 for the lair region, a sworn or tributary court, or a person in the dragon's memory; 0.5 neighbour region; 0.2 elsewhere | 0.10 |
| U surprise | 1 − L(m) by StU, N from the 300-seed dry run per pattern and era | 0.10 |
| R reversals | protagonist fortune sign changes: 0 → 0.2, 1 → 0.7, 2 or 3 → 1, 4 → 0.6, 5+ → 0.3 | 0.10 |
| X irreversibility | 1 if the thread ends a court, dynasty, race or era | 0.05 |

score = Σ w·f, +0.20 if it closes or cuts a thread already shown, +0.30 at tier 5, −0.25 if its pattern headlined in the last 3 wakings; the headline also takes the arc-fit multiplier (§3).

- U stays small because of Lessard's result. Base rates per (pattern, era, property) are computed offline, so U is a table lookup and determinism holds. Against StU's double counting, merge properties that co-occur above 0.8 in the dry run.
- The least likely property supplies a "remarkable because" clause when the chronicle level allows.
- All weights are proposals for the headless runner, re-fitted after the human check in §7.

## 6. Two accounts of one event

- **Talk of the Town**: nine evidence types; strength is the sum of evidence and decays; lies are recorded as lies [42]. **Gossamer**: perception and retelling can mutate memories [43].
- A possible-worlds belief model with nested beliefs agreed with readers 95% of the time [44].
- Labov on the teller's transformations: the chosen starting point decides who looks responsible; tellers polarize protagonist and antagonist, delete or substitute events, and use idioms that remove the attacker's agency [7].
- Hansen's taxonomy (TUNA): contradiction across voices is the hard case for models [45].

**For Hoard**
- One true thread (§2), many renderings. A court source is the match plus a transform:

```json
{"source": "aldmere_annal_212", "match": 913, "court": "aldmere", "medium": "written",
 "start": 5120, "omit": [4410], "blame": "dragon", "passive": [4410],
 "stated_cause": "grudge:wyrm", "hedge": 0, "archetype": "court_flatterer"}
```

- Labov's transforms as rules: `start` is the earliest event in the backward chain whose agent is some other party; `omit` lists events where this court harmed another; `blame` is the dragon at dread ≥ 0.6, else the court's top grudge; `passive` covers its own acts that survive the cut.
- Media follow the memory tiers. Oral sources re-roll `blame` each generation with p = 0.15 toward the court's current enemy and carry `hedge` 1. Written sources freeze when written. Monumental ones keep only the most reportable event and its blame.
- Contradiction is computed symbolically: two sources on one match conflict when `blame` differs, when `start` differs across a dragon act, or when one omits an event the other prints. The briefing marks the second line as disputed.
- Stated causes are sim facts. A court that blames the dwarves acts on that grudge, and a pattern can sift the result (`misdirected_vengeance`).

## 7. What the interest tool should measure about briefings

| metric | target band (proposal) |
|---|---|
| headlines with D ≥ 0.6 | 40 to 70% |
| shown threads that later get a closing or cut line | ≥ 60% |
| mean reversals in headline threads | 1 to 3 |
| briefings naming a person from an earlier briefing | 30 to 60% |
| selection lift: mean score of shown lines minus mean of all candidates | > 0.15 |
| tier 5 events shown in their waking | 100% |
| wakings needing fallbacks below the floor | ≤ 10% |
| largest share of headlines from one pattern in a run | ≤ 15% |
| patterns that never headline in 300 seeds | flag |
| line count against L(y) | exact |

Human check per build, after StU: 3 raters, 15 pairs each, top-scored candidate against a random one; keep the weights while agreement stays at 75% or above.

## 8. Not found or uncertain

- Ryan's sifting heuristics, the Sheldon pattern count, Bad News curation detail; Kreminski's dissertation.
- No published sifter combines weighted features with a human test of the weights, so the §5 weights are untested proposals.
