# 05b · Known causes, folded tier-4 lines: report

Branch `item/05b-known-causes` from `origin/main` (4659c29). Owner's decisions of 2026-10-28 (fold) and 2026-11-02 (a cause shows only when known).

## What changed

- `data/text/{en,de}/patterns.json`: 33 `.bare` variants (the causal keys below), 48 fold keys (`pat.<id>.fold[.<outcome>|.open][.bare]`, `pat.fold.<template>`) and `pat.list.and`. 163 to 245 keys each. No em dashes.
- `sim/sift/line_text.cpp`: `sift_line_text(..., with_cause)`. Without the cause every key of the fallback chain is tried as `<key>.bare` first, so a closed callback that falls back to `.line.<outcome>` gets the bare one too; `{title}` takes `pat.<id>.title.bare` the same way. Fold keys fall back `fold.<outcome>` to `fold`. `{names}` joins the members ("A, B and C", "A, B und C"), `{year}` is the folded line's year. `sift_text_exists` tells the sifter whether a fold text exists.
- `sim/sift/select.cpp`: the fold (below). `sift_wake` marks every folded thread as shown and adds the members to `named`.
- `sim/sift/sift_util.cpp`: `fold_names`, `fold_lines` read the two new line params.
- `sim/ages/briefing.cpp` (`tell`): a pattern line below level 2 uses the bare text. A line whose full and bare texts differ counts its own cause in `cause_available`, and in `cause_shown` at level 2 and 3. A folded line is told at the lowest level of its members and prints no sources and no because chain.
- `sim/tests/sift_budget.cpp`: the twelve tier-4 filler events are `people_arrived` instead of `slayers_founded` (see Deviations).
- `tests/golden/sift_bank_v1.hash`: regenerated (Goldens).

## Job 1: the keys judged causal

The rule I read by: a line is causal when its words tie an event to an earlier one as its reason or its agent ("because", "after your help", "to answer", "never forgave", "had done half the work", "fulfilled", "you saved it", "left it a waste", "the debt was paid back", "you broke"). A sequence ("and", "until", "when", ";") is not, nor is the public content of a vow, an oath or a prophecy. Below level 2 the bare variant is used, at 2 and 3 the full line.

| key | why |
|---|---|
| `war_won_by_dragon_act.line`, `.line.held`, `.line.turned` | "because you came into it in {b.year}". Bare drops the dragon's part entirely. |
| `war_lost_to_dragon_fire.line` | "your fire ... had done half the work". |
| `vengeance_for_burning.line`, `.line.failed`, `.line.martyred`, `.line.slew`, `.line.marched` | "never forgave your fire of {a.year}": the motive of the order. The raw `slayers_founded` line names no reason. |
| `vengeance_for_burning.open` | "to answer your fire of {a.year}". |
| `vengeance_for_burning.cb.cut`, `.title` | "the vengeance ... swore for your fire". Bare: "the vengeance X swore". |
| `prophecy_answered.line`, `.line.fire`, `.line.grace` | "your fire/grace of {b.year} fulfilled it", "it said you had fulfilled it". The raw `prophecy_fulfilled` line at level 0 names no agent ("In X they said the prophecy was fulfilled"), so the pattern line must not either. |
| `burned_to_waste.line`, `.line.orcs`, `.line.resettled`, `.line.haunted`, `.open` | "Your fire of {a.year} left X a waste in {b.year}": the waste came decades after the fire (pattern clause `caused_by`). Bare: "X became a waste in {b.year}". |
| `burned_to_waste.cb.cut` | "the waste you made". |
| `sack_and_revenge.line` | "in {c.year} the debt was paid back": casts the later war as payback for the sack; clause b may be a plain `war_started`. |
| `dragon_broke_hegemon.line`, `.line.bent`, `.open` | "You broke the hegemony": makes the dragon the agent of the hegemony's end. |
| `temple_to_the_dragon.line` | "after your help of {a.year}". |
| `debt_of_the_saved.line`, `.line.tribute`, `.line.temple`, `.line.feast`, `.line.ingratitude`, `.open`, `.title` | "remembered that you saved it", "owes you its survival", "the debt ... owed you": all assert that the dragon turned the war (the `war_won_by_dragon_act` claim). Bare: "X came through its war, and ...". |

Judged not causal (owner may flip any of these; each needs only a `.bare` key, no code):

- `oath_of_recovery.*`: "swore to win back what you took" is the oath's content, which the raw event already says at level 0.
- `grudge_to_answer.*`: names only the year the grudge began; `.line.mended` ("until you stood by it") and `.line.armed` are sequences.
- `golden_age_burned.*` ("when you came in {b.year}; by {c.year} it was over"), `hegemony_arc.line.dragon` ("until you fell on it"), `plague_war.*` ("Plague weakened X; Y went to war on it"), `ruler_slain.*`, `fall_after_defeat.*`, `tyrant_falls.line.thaw`: sequences that invite the inference but do not state it. The closest calls are `golden_age_burned.line` and `plague_war.line`.
- `war_won_by_dragon_act.cb.cut`, `war_lost_to_dragon_fire.cb.cut` ("... all the same"): say the act did not decide it.
- `martyrs_remembered.*` ("took up their cause"), `order_rise_and_fall.line.fell` ("went down with its court").

## Job 2: the fold rule

- In the sifter (`select.cpp`), when a candidate at tier 4 of kind `line` is taken, every other unused tier-4 `line` candidate with the same fold key and the same year joins it, unless it tells an event already printed.
- Fold key: `pat.<id>.fold` + the line's outcome, or `pat.<id>.fold.open` for an open thread; for a raw event `pat.fold.<template_key>`. A key folds only when its text exists in `patterns.json`, so a pattern or template without a fold text keeps its separate lines. Fold texts exist for the six tier-4 patterns (vengeance_for_burning, oath_of_recovery, prophecy_answered, hostage_raised, dragon_broke_hegemon, temple_to_the_dragon) with every outcome, and for the raw templates slayers_coming, slayers_ready, slayers_founded, slay_attempt_failed, prophecy_fulfilled, prophecy_written.
- "Same year" is the year of the line's event (the closed story, or the lead event), so `{year}` in the fold text is true for every member.
- The folded line keeps the lead's (highest score) params and gets `fold_names` (protagonist ids, lead first, then by score) and `fold_lines` (`thread:event` per member). It takes one budget slot. A tier-4 headline folds its group too, and then takes no budget slot.
- Callbacks never fold (each has its own "which you heard of").

## Numbers

Causes shown (`interest.py`): see the table below.

CAUSES_TABLE

## Before and after

`hoard-sim --data data --chronicle-demo 42` (the `[kind, level]` tags are the demo's):

```
B2 · level 3   before: [headline] Marford founded a slayer order in 1630 to answer your fire of 1620. Even without ...
                       [line] Marfell founded a slayer order in 1630 to answer your fire of 1610.
               after:  [headline] Marford and Marfell founded slayer orders to answer your fire. Even without ...
                       (the freed slot goes to: Marfell set up a memorial to those you killed, ...)
D · level 0    before: The vengeance Gravdusk swore for your fire, which you heard of three wakings ago, came to nothing.
               after:  The vengeance Gravdusk swore, which you heard of three wakings ago, came to nothing.
D · level 0    before: Mireslough, the waste you made, lay as it was.
               after:  Mireslough, the waste, lay as it was.
D · level 0    before: Your fire of 1630 left Tharbarrow a waste in 1720.
               after:  Tharbarrow became a waste in 1720.
```

Played seed 42 (`--seed 42 --years 3000 --genesis --policy played --wakings`), no historian, level 0 unless noted:

```
waking 22, 1600   before: Wenmarch read a prophecy of fire in 1580; your fire of 1580 fulfilled it.
                          Rhonsary read a prophecy of fire in 1510; your fire of 1580 fulfilled it.
                          Nimieth read a prophecy of fire in 1530; your fire of 1580 fulfilled it.
                  after:  Wenmarch, Rhonsary and Nimieth had each read a prophecy of fire; in 1590 they said the prophecies were fulfilled.
                          (two slots freed: Aldbert the Bold, once bold, is bold no more.)
waking 28, 2200   before: A premonition woke you: the slayers of Vhalcrypt, led by Vhalrath, were on their way to your lair.
                          A premonition woke you: the slayers of Ilvorien, led by Ithion, were on their way to your lair.
                  after:  A premonition woke you: the slayers of Vhalcrypt and Ilvorien were on their way to your lair.
waking 16, 1070   before: Torsary read a prophecy in 1040; in 1060 it said you had fulfilled it.
                  after:  Torsary read a prophecy in 1040; in 1060 it said the prophecy was fulfilled.
```

Played seed 7, level 0 and 1:

```
waking 3, 620     before: Duruk won its war on Mortor in 620; your fire on Mortor in 600 had done half the work.
                  after:  Duruk won its war on Mortor in 620.
waking 8, 760     before: Mortor founded a slayer order in 760 to answer your fire of 750. The Chronicle of Mortor lays it at your door.
  (level 1)       after:  Mortor founded a slayer order in 760. The Chronicle of Mortor lays it at your door.
```

## Decisions taken

- Bare variants are a `.bare` suffix on the key, tried along the existing fallback chain. A line with no `.bare` is the same at every level.
- The war-won bare lines drop the dragon's entry altogether rather than keeping "and you came into it" beside the outcome, since side by side they read as the cause.
- `prophecy_answered`: judged causal because the raw `prophecy_fulfilled` line at level 0 names no agent. This is the most frequent tier-4 line, so at level 0 the prophecy story now reads only as the court's claim; at level 2 the full line returns.
- `temple_to_the_dragon.line` is judged causal although the raw temple line says the walls carry "what you did for them": it names the year of the help, which the walls do not give.
- Fold groups are same key and same year, so a fold text never states a year that holds for only one member. Two groups of one key in different years stay two lines.
- Whether a key folds depends on the English text bundle (a fold text exists or not). Without texts loaded nothing folds.
- A folded line is told at the lowest level of its members and prints no sources and no because chain: the members' sources and chains are about different courts, and printing one would mislead.
- The headline folds its own group (the 4-prophecy case of report 14 becomes one headline).
- No bump of `kRulesVersion`: only the sifter's stored lines and named set change.

## Deviations

- `sift_budget`: its fixture filled the sleep with twelve tier-4 `slayers_founded` events on different courts, mostly in one year. With the fold they become one line, so the test's premise (more lines than the budget) no longer held. I changed the kind to `people_arrived` (tier 4, no fold text); the checks are unchanged. Please confirm or tell me which test you want instead.

## Goldens

- `sift_bank_v1.hash` moved (ac1cdd5a8b9ef7a9 to a8d1355832f11a29), in its own `golden:` commit. Cause: folded lines change `w.sift.lines` (text_key, params) and one more person entered `named` from a line that the freed slot let in. On seed 42 played the final world differs from main only in `sift.named` (75 to 76 entries).
- `state_hashes.txt` (checked 1 to 40 natively; wasm 1 to 60 below), the three chronicle texts and the two chronicle hashes are unchanged. Replays and the save expectation pass.

## Open questions

1. The causal list above, especially the "not causal" calls (golden_age_burned, plague_war, grudge mended).
2. `sift_budget` fixture change.
3. The German texts are new drafts in the style of the existing lines; not reviewed.

## Checks

CHECKS

## How to verify in five minutes

```
cmake --build build -j2 && ctest --test-dir build -L fast
./build/hoard-sim --data data --chronicle-demo 42          # waking B2 headline folds; waking D lines have no "you made"
./build/hoard-sim --data data --seed 42 --years 3000 --genesis --policy played --wakings w42.jsonl --out b42
grep -o '"text":"[^"]*had each read[^"]*"' w42.jsonl         # folded prophecies
```
