# 14 · Story sifting

Branch `item/14-story-sifting`. The briefing is now picked by a sifter: a bank of story patterns matched inside the decade step, thread records that span sleeps, a scored headline, a line budget by sleep length, callbacks on threads the player saw, and court sources computed over one true thread.

## What changed

- `data/patterns/bank.json`: 25 hand-written patterns (15 episodes, 10 stories) over the sim's real event kinds. `docs/patterns/seed.json` is gone: 37 of its kinds do not exist in the sim (`dragon_burn` is `act_burn`, `slay_attempt` is `slay_attempt_failed`, and so on). `docs/patterns/vocab.json` is regenerated from the sim's `emit` calls, the cards and a 20-seed census (148 kinds). The schema gains `any_of` (alternatives in one clause), `where` (a path equal to a variable, e.g. `c.winner`), `tier` and the pattern's `tags` (`prophecy`, `ending`). `tools/pattern_check.py` passes.
- `sim/sift/`: `acceptor.cpp` (Winnow-style matching at the end of every step, indexed by event kind, unless and unless_forever, once_per, the 64-partial cap, lapses, deaths), `score.cpp` (the eight features and weights of research section 5), `select.cpp` (headline, tier 5, ages, tier 4, callbacks, greedy fill, the minimum), `because.cpp`, `source.cpp` (`make_source`, `sources_conflict`), `line_text.cpp` (`pat.*` keys through the item 19 renderer), `sift_json.cpp`, `bank_data.cpp`.
- State: `World.sift` holds the threads, the emitted episodes and stories, once/barred keys, the headline history, the people already named and the last waking's lines. It is saved and hashed once the bank has run.
- Briefing: `lines` (kind, tier, score, text_key, params, because_chain, sources, text), `sources` with `disputed`, and the headline and "while you slept" from the sifter. The old fields keep their shape, so the page needs no change. The world view exports `sift_threads`.
- Data: `importance.json` "tiers" (213 templates, 23 at tiers 4 and 5, 10.8 %), `constants.json` "sift" (every weight and threshold), `data/patterns/base_rates.json` from `tools/sift_rates.py` (300 seeds). Texts in `data/text/en/patterns.json` and a German draft in `data/text/de/patterns.json`; the German is not reviewed.
- `hoard-sim --no-bank`; `--golden` and the wasm `golden` command clear the bank before genesis. `interest.py` prints the section 7 metrics, the headline shares and the completion rates.

## Decisions taken

- Episodes and stories live in `World.sift`, outside the chronicle (the brief put them in it). The sim's history is the same with the bank on or off, so no rule reads a sifter event and the bank can change without new state goldens. `decisions.md` has the entry.
- The same-court penalty is the run-phase diversity penalty, 0.20 at waking 1 falling to 0.05 at waking 30. Points 5 and 6 of the brief give it two values (falling, and 0.15); I took the falling one.
- Stakes is importance × (0.5 + 0.5 × the magnitude decile of the kind over the dry run). The research asks for the percentile within the era; per kind was what the dry run supports with enough events.
- Only tier 3 and above may headline (brief point 6); a sleep without one has no headline and the page shows its "quiet" line.
- A template's tier is lifted (to 2 at importance 0.3, to 3 at 0.5) only by what importance.cpp adds on top of the template: a known person, a sworn court, a hostage. A plain own act stays at tier 1.
- "This sleep" means the events after its `sleep_begin`, as the old briefing read it; events of the raid before the sleep are not news.
- A thread opens above a third of its clauses (a three-clause pattern opens at its second event).
- Under 30 years, only a thread still under way takes a second line, and never with an event another line already tells. No candidate repeats an event already shown.
- Sources: `blame` falls back to the court it last fought (the brief's "top grudge"; courts keep no grudge list yet), a generation is 30 years for the oral re-roll, a source for a thread already gone is not told.
- The interface for item 05 takes `Params` as well: `briefing_lines(World, Params, since, budget)`, `make_source(World, Params, match, court)`, `sources_conflict(World, a, b)`.

## Acceptance

| item | result |
|---|---|
| seed 42: war won because of a dragon act, by a pattern, because chain reaching the act | met: `war_won_by_dragon_act` closes in 980, "Cordale held out against Berley until the war wore itself out in 980, because you came into it in 950."; its chain is the kernel (war, act, end) |
| seed 42: two disagreeing accounts from `make_source` | met: waking 4, Gravdusk blames Marholm and Marholm blames Gravdusk over one match; the second is marked disputed |
| no pattern headlines above 15 % | met: highest is `vengeance_for_burning` at 9.8 % of all headlines |
| shown threads get a closing or cut line ≥ 60 % | met: 63 % |
| acceptor cost under 30 % of the step time | met: 5 to 13 % (table below) |
| goldens unchanged with the bank off | met: `state_hashes.txt` (300 seeds) and the three chronicle texts and two chronicle hashes unchanged; wasm agrees on seeds 1 to 60 |
| one new golden with the bank on | `tests/golden/sift_bank_v1.hash`, checked by `test_golden` |
| every section 7 metric in its band or reported | see the metrics table |

Tests (Manu's list): `sift_acceptor`, `sift_budget` (3, 5, 7, 9 lines plus one headline plus both tier-5 lines), `sift_bank_v1.hash`. `ctest -L fast` 15 of 15. Replays and the save expectation hold the sifter's state, so they are regenerated in the golden commits (rules 5).

## Acceptor cost

User time of one 3000-year played run (generated world), best of three, machine shared with two other jobs:

| seed | bank on | bank off | cost |
|---|---|---|---|
| 42 | 1.208 s | 1.066 s | +13 % |
| 7 | 1.450 s | 1.384 s | +5 % |

The cost includes the selection at every waking. At 64 partials per pattern the cap never had to be lowered.

## Metrics over 300 played seeds (interest.py, research section 7)

11 894 wakings, 10 809 headlines.

| metric | band | result | why, when outside |
|---|---|---|---|
| headlines with D ≥ 0.6 | 40 to 70 % | 58 % | |
| shown threads later ended | ≥ 60 % | 63 % | |
| reversals in headline threads | 1 to 3 | 0.06 | kernels are two to four events and most carry one fortune sign for the protagonist; wars now count for winner and loser. Needs longer patterns or a fortune over the whole cause chain |
| briefings naming a person from an earlier one | 30 to 60 % | 8 % | most stories are about courts; people enter as cast (rulers, vowers, hostages). Item 07's people with places should move this |
| selection lift | > 0.15 | +0.04 (+0.06 where candidates outnumber lines) | short sleeps have fewer candidates than lines, so everything shows; scores sit between 0.3 and 0.8 |
| tier 5 shown | 100 % | 100 % | |
| wakings with fallbacks below the floor | ≤ 10 % | 21 % | ten-year sleeps rarely hold two candidates above 0.30; never padding means the minimum takes tier-2 events below it |
| largest share of one pattern in a run | ≤ 15 % | 13 % mean, 29 % worst run | the worst runs have few pattern headlines |
| patterns that never headline | flag | none | |
| lines against L(y) | exact | never over; 23 % of wakings fill L exactly | never padded |

Headline share: raw events 65 %, vengeance_for_burning 9.8, prophecy_answered 7.1, grudge_to_answer 6.2, oath_of_recovery 2.7, order_rise_and_fall 2.4, golden_age_burned 0.8, war_won_by_dragon_act 0.6, war_lost_to_dragon_fire 0.6, debt_of_the_saved 0.6, treasure_hunt 0.5, tyrant_falls 0.5, dragon_broke_hegemon 0.4, burned_to_waste 0.3, ruler_slain 0.3, temple_to_the_dragon 0.3, martyrs_remembered 0.3, hostage_raised 0.2, succession_war 0.2, plague_war 0.2, sack_and_revenge 0.2, hegemony_arc 0.1, the_fading 0.1, exiles_return 0.1, last_of_their_kind 0.04, fall_after_defeat 0.03.

## Completion per pattern (share of 300 seeds that complete it at least once; keep 10 to 60 %)

war_won_by_dragon_act 64, war_lost_to_dragon_fire 65, vengeance_for_burning 33, order_rise_and_fall 35, martyrs_remembered 28, grudge_to_answer 15, oath_of_recovery 56, prophecy_answered 99, hostage_raised 11, ruler_slain 38, burned_to_waste 69, the_fading 28, sack_and_revenge 42, treasure_hunt 26, tyrant_falls 94, plague_war 81, succession_war 84, hegemony_arc 43, dragon_broke_hegemon 12, temple_to_the_dragon 14, debt_of_the_saved 40, golden_age_burned 78, fall_after_defeat 16, exiles_return 36, last_of_their_kind 25.

Flagged everywhere (over 60 %): the two war episodes, prophecy_answered, burned_to_waste, tyrant_falls, plague_war, succession_war, golden_age_burned. Over 3000 years nearly every run has one plague war or one tyranny, so "at least once per seed" flags common arcs; one tightening round took grudge_to_answer from 69 to 15 and ruler_slain from 89 to 38. I kept the rest, since they are sound filler episodes; per-waking rates would be the better test for them.

## Example briefings

**10 years, played** (seed 42, waking 28, budget 5 lines plus headline and tier 5)

- *headline, tier 4, 0.80:* Berley read a prophecy of fire in 1710; your fire of 1730 fulfilled it.
- *callback, tier 3, 0.82:* The slayer order of Torwick, which you heard of in 1310, never came for you.
- *line, tier 4, 0.80:* Rhonsary read a prophecy of fire in 1680; your fire of 1730 fulfilled it.
- *line, tier 4, 0.79:* Ilvorien read a prophecy of fire in 1710; your fire of 1730 fulfilled it.
- *line, tier 4, 0.46:* A premonition woke you: the slayers of Nimieth, led by Ithion, were on their way to your lair, because your fire over Gorgewaste in 1730 was never forgiven; a bold hero led them.
- *line, tier 4, 0.77:* Galley read a prophecy of fire in 1710; your fire of 1730 fulfilled it.

**100 years, played** (seed 42, waking 6, budget 7 lines plus headline and tier 5)

- *headline, tier 3, 0.65:* Plague weakened Gravdusk in 540; Marholm went to war on it in 580, and the war ended in 600.
- *callback, tier 2, 0.68:* The tyrant's death in Cordale, which you heard of four wakings ago, changed little.
- *line, tier 3, 0.38:* Breric, ruler of Alddale, died at the age of 69.
- *line, tier 2, 0.32:* The people of Gravdusk were driven from Lithholt in 600.
- *line, tier 2, 0.45:* Marholm declared war on Gorul, while the horde of Gorul was leaderless.
- *line, tier 2, 0.32:* Gorul swore fealty to Marholm to keep its last land.
- *line, tier 2, 0.32:* Torwick swore fealty to Rhonsary to keep its last land.
- *line, tier 2, 0.65:* Treasure hunters of Rhonsary came for your hoard in 690.

**1000 years, long_1000** (seed 42, waking 7, budget 9 lines plus headline and tier 5)

- *headline, tier 5, 0.65:* Rhonsary was wiped out; its people scattered as refugees, after the fall of Mireslough to Brurat in 1590.
- *line, tier 5, 0.65:* Duroth was wiped out; its people scattered as refugees, after the fall of Mireslough to Gorgash in 1630.
- *line, tier 5, 0.65:* Ghazzag was wiped out; its people scattered as refugees, after the fall of Mireslough to Uzgul in 1670.
- *line, tier 5, 0.65:* Alddale was wiped out; its people scattered as refugees, after the fall of Windfell to Durgash in 1690.
- *line, tier 5, 0.65:* Gorak was wiped out; its people scattered as refugees, after the fall of Mireslough to Mogoth in 1750.
- *line, tier 5, 0.65:* Gormokak was wiped out; its people scattered as refugees, after the fall of Mireslough to Gorulmok in 1800.
- *callback, tier 4, 0.62:* The vengeance Berley swore for your fire, which you heard of two wakings ago, came to nothing.
- *callback, tier 4, 0.62:* The vengeance Torsary swore for your fire, which you heard of at your last waking, came to nothing.
- *line, tier 4, 0.67:* Rhonsary read a prophecy in 1210; your grace of 1330 fulfilled it.
- *line, tier 4, 0.67:* Torsary read a prophecy in 1190; in 1340 it said you had fulfilled it.
- *line, tier 2, 0.45:* Brurat declared war on Rhonsary, because the horde smelled weakness after the disputed succession of 1520 in Rhonsary.
- *line, tier 2, 0.48:* Krulak swore fealty to Alddale to keep its last land.
- *line, tier 2, 0.45:* Vhalmor declared war on Gravdusk, because Gravdusk was weak.
- *line, tier 2, 0.45:* Vesstead declared war on Gorgashzag, while the horde of Gorgashzag was leaderless.
- *line, tier 4, 0.34:* You moulted and woke in your grown phase, stiff from the long sleep.


The ten-year example shows the cost of "tier 4 always gets a line": four prophecies fulfilled by one raid take four lines. The thousand-year one has no ages block, since no new hegemon or valley era rose in it.

## Open questions for Manu

1. The human check: 15 pairs are in `reports/14-pairs.md` (key at the end). Three raters; keep the weights at 75 % agreement.
2. Should repeated tier-4 lines of one pattern in one waking fold into one line ("Berley, Rhonsary, Ilvorien and Galley read prophecies of fire; your fire of 1730 fulfilled them all")?
3. Reversals and people from earlier briefings are far below their bands. Accept until items 07 and 08 add people with places and orders with rosters, or tune now?
4. The German pattern lines are a draft for your review.

## How to verify

```
cmake --build build -j2 && ctest --test-dir build -L fast
python3 tools/pattern_check.py docs/patterns/pattern.schema.json docs/patterns/vocab.json data/patterns/bank.json
seq 1 300 | xargs -P 2 -I{} ./build/hoard-sim --data data --golden {} | tr -d '\r' | sort -n | diff - tests/golden/state_hashes.txt
python3 tools/interest.py --seeds 1-300 --policies played --genesis --jobs 2
./build/hoard-sim --data data --seed 42 --years 3000 --genesis --policy played --wakings out/w42.jsonl --out out/b42
```
