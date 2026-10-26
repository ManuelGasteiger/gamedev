# 02 · Dread and awe, memory tiers: report

Branch `item/02-dread-awe-memory`, on main after item 19. Tests as you chose: `dread_awe_update`, `memory_tiers`, the extended `reactions`.

## 1. What changed

Sim:
- `Faction.fear` is gone. Each court holds `dread` and `awe` as `{oral, written, monumental}` stocks; `monumental` is the sum over the court's `monuments[]` (`{region, act, bias, built, dread, awe}`). Effective values `1 − Π(1 − x)` (`memory_tiers.h`). Memory entries gain `axis`, `tier`, `strength` and `act` (the index in `World.acts`); faith entries keep v0.4's decay.
- `data/reactions.json` is two parts: `gains` (act-api §6.2, one row per act and sub-kind) and `rows` (v0.4's relationship rows as multipliers, with their faith, research and tags; witness rows for enemy, kin and the rest).
- Deposits (`deposit`): oral at full strength, written at 0.6 when the court has a sage, a historian or a region tagged `library`, monumental at 0.4 into the monument where the act happened (a temple turns half the dread into awe, a memorial the reverse). `rep` on awe from the court's own memories of the last 30 years; `show` awe × (1 − D).
- Witnesses are derived by the sim: the owner at 1.0, neighbouring courts at 0.5 (Glamour halves the neighbours). What a shell sends is ignored. Every witnessed act reacts now, not only burns.
- Decay per tier (`decay_tiers`): oral 60 years at human memory, scaled by the race's `memory_halflife` (elves and undead never forget orally, dwarves 200, orcs 20), Terror slows oral dread to 2/3; written 250; monumental 1000. A living sage or priest still lengthens it through `eff.memory_halflife`.
- `burn` with `item: library` cuts written stocks and entries to 0.1 and removes the region's `library` tag; `item: temple` or `altar` razes the monuments there; a region that turns to waste loses its monuments.
- Unlocks: pilgrims hook at A ≥ 0.15; no envoy lies at A ≥ 0.30; faith floor 0.3 at A ≥ 0.50; demands sworn at D ≥ 0.30; slayer orders and bands at D ≥ 0.50, readying at the pressure D (1 − 0.7 A) over 0.5; garrisons flee at D ≥ 0.70 with defence × (1 − D). The view exports these as `unlocks`, plus `slayer_pressure` and `oath_stability`.
- Research (`era.cpp`): `research_base × era × (1 + 2 D (1 − 0.7 A)) × wealth / (wealth + 20)` per decade, and the court pays 50 wealth per unit of progress. Below era 2 the v0.4 dread research stays.
- Oaths: `no_walls` and `no_settle` hold while dread or awe stays at 0.2, and break by chance at `0.05 × (1 − stability)` per decade, stability `1 − 0.8 D (1 − A)`, half again as stable at A ≥ 0.30, never with Dragon-king. A keyed draw, so the world stream does not move. A kept oath adds 0.02 awe per decade.
- Tribute paid out of dread scales with D / 0.3.
- Cards and hooks: `fear` keys are `dread` (and `awe` where the gains table has one: pilgrims received or eaten). New card conditions `dread`, `awe`, `dread_written`, `awe_years`, `monuments`, `max_dread`, `max_awe`; new effect `monument`. Three cards in `data/events/monuments.json`: a dragon temple after 50 years of awe ≥ 0.5, a memorial when a slayer order is founded, a memorial after a heavy burn (30 %).
- Forecast: `courts[]` with projected dread, awe, pressure and slaying odds. Briefing: `courts[]` with both, the tiers and `memory_tier`. Narrator: `defend.saved`, `give.gift`, `show.awe` lines when the act leaves the court in awe; a `resentment` cause.
- Saves: the hash is checked on the saved JSON before it is read (integers re-typed as written), then a rules-2 `fear` becomes `dread.oral` and half the faith becomes `awe.oral`. `tests/saves/v1-seed4.json` loads and continues.

Talents (`data/talents.json`, nodes 0 to 3 as in talents-v2): Stone Grip, Laden Flight and Chronicler replace Mountain Tail, Fury and Long Memory. Dread Flame: burns × 1.5 dread and written at full strength where the court keeps records. Terror: dread × 1.3, oral dread decays at 2/3. Awe: protecting and giving × 1.5 awe and faith. Dominion ranks add awe. A `flight` block with the v0.4 numbers; the view exports `dragon.flight` (cruise, surge, stall, turn, travel speed and altitude, carry, stamina budget, surge drain, breath cost, hostage and gold weight). Stone Grip, Laden Flight and Chronicler change nothing in the sim yet (items 16 and 05 read them).

Page: shows dread, awe and the memory tier where it showed fear; floats for both. Glamour and the witnesses are the sim's now; Fury's page code went with the talent. `web/game-check.cjs` reads the new fields and labs (fear → dread, the two removed talent ids), and three setups that leaned on one history now hold for any: the defend check sleeps on until a court that an army is marching on asks for protection; the collapse ticker is matched on "fell" (both narrator templates say it; the old /collapsed/ matches neither and only passed while the first collapse was below the ticker threshold); the conquest lab gives the losing court cohesion 0.5 and some wealth, so it loses land before it falls apart (at 0.15 it was under collapse_cohesion 0.2). No check was added or dropped; 90 of 90 pass.

Tools: `sweep.py` court columns, the snapshots' tier and monuments.

## 2. Numbers

All on 2 cores; the interest tool on 300 generated worlds × 3000 years (5 minutes), the sweep on 100 (39 minutes), the rest named.

Bands (`interest.py --policy played --genesis --bands`, 300 seeds):

| metric | v0.4 | now | band | result |
|---|---|---|---|---|
| idle.outcome_entropy_bits | 4.815 | 4.843 | ≥ 4.5 | pass |
| played.outcome_entropy_bits | 4.033 | 3.869 | ≥ 4.5 | fail (failed in v0.4 too) |
| idle / played first_three_top_share | 0.080 | 0.080 | ≤ 0.1 | pass |
| played.death_share | 0.213 | 0.080 | 0.05 to 0.25 | pass |
| idle.death_share | 0 | 0 | < 0.05 | pass |
| played.hook_coverage | 0.989 | 0.991 | ≥ 0.95 | pass |
| played.unwarned_deaths | 0 | 0 | 0 | pass |

Played deaths: 23 slain, 1 by wounds, median year 2140 (v0.4: 64 slain, median 1545).

Slayer founding under played (first order after the hatching, 150 seeds): median 320 years (v0.4 on the same tool, 60 seeds: 305). Every one of 896 foundings had dread_eff ≥ 0.5. The path there: at the brief's values (oral 30 years) and my first research constants the first order came at 750; once research was scaled to the courts' wealth (about 20 to 40) it came at 420, but played deaths fell to 4 %; oral 60 years and research base 0.016 give 320 and 8 %. The written-grievance trigger moved nothing measurable and stays off.

Agency (played, 300 seeds), before / after: hooks reaching a dragon act 13.5 % / 11.0 % (of hooks with refs 22.6 % / 20.3 %); events whose chain reaches an act 7.0 % / 7.1 %; waking headlines 66.1 % / 65.5 %; Jaccard against idle 0.901 / 0.903.

Policies on the classic map (40 seeds each):
- `protect_one`: Aldmere's awe at year 500 is 0.07 (median); above 0.5 in no seed; the pilgrims hook by year 1000 in 1 seed; a dragon temple somewhere in 3000 years in 20 seeds. **Fails.** The policy gives 0.1 every 100 years (awe 0.02 a time, 0.03 at steady state with a 60-year oral half-life) and defends only when Aldmere is attacked; Aldmere has no sage before era 2, so nothing is written down. Reaching 0.5 needs about 0.15 awe per waking with a written tier, or 0.34 without.
- `raze_all`: the highest awe of any court at any waking is 0.002 (median 0). Research: by year 300 the burned courts hold 0.63 anti-dragon progress against 0.06 in idle, about 31 of their wealth spent on it (50 per unit; the strong-court resolve row is in the same number), when the burned valley holds 142 wealth and the idle one 364.

Library burn (`memory_tiers` test): a written stock of 0.084 drops to 0.0084 with `burn(library)` and gets no new deposit from that burn; 0.5 halves to 0.25 in 250 years. A court at oral 0.2 and written 0.5 (dread_eff 0.6) falls to 0.24 at once, its oral stock and a tenth of what was written.

Sweep (`sweep.py`, 100 seeds): the policy with the most growth points at the end, per seed: raze_all 28 %, long_1000 25 %, played 24 %, moult_only 23 %, raid_then_1000 0 % (rest_only left out: 10 seeds). Deaths: moult_only 6 %, long_1000 3 %, played 9 %, raze_all 74 %. Buckets under played (4,086 wake states): survival decisive 28.0 / 20.9 / 14.5 / 9.1 / 8.4 / 9.1 / 9.9 % for 1 to 3000 years, growth 0 / 1.9 / 2.5 / 4.2 / 17.5 / 32.8 / 41.0 %; perturbation flips 40 % (burn) and 43 % (sleep). No death before year 80 except raze_all (33, all wounds, as in v0.4). Largest race at the end under played: orc 76, human 19, dwarf 5.

Seed 42, played, classic map (4 courts; the last snapshot at or before the year, dread / awe / tier):

| year | Aldmere | Vessary | Dorun | Ilvareth | others |
|---|---|---|---|---|---|
| 100 (50) | 0.41 / 0.14 / oral | 0.41 / 0.10 / oral | 0.57 / 0.13 / oral | 0.40 / 0.05 / oral | |
| 500 (410) | 0.13 / 0.30 / oral | 0.33 / 0.00 / oral | 0.58 / 0.16 / oral | 0.85 / 0.25 / oral, a memorial | Mardale 0.29 / 0.13 / oral |
| 1500 (1220) | gone | 0.67 / 0.15 / written, a memorial | gone | 1.00 / 0.00 / oral | Mardale 0.34 / 0.15 / written; five more at 0.09 to 0.24 dread |

Ilvareth is elven: its oral tier never fades. Mardale and Vessary write it down once they have sages.

Goldens: every state and chronicle hash moved (`golden:` commit, rules 3). Seed 1's chronicle first differs at year 650, the hatching waking: a wound of 8.7 is 9 because garrisons no longer shrink with dread under 0.7; from year 660 wars end by submission where a court spent wealth on research. Over the run: 95 fewer `anti_dragon_progress` lines, 83 more collapses, one dragon temple. Native and wasm agree on seeds 1 to 60.

Speed: a 3000-year played run on seed 42 took 1.1 s against 0.5 to 0.8 s for the v0.4 build, measured with the page check running on the other core. The v0.4 dragon is slain in 2370, this one lives to 3540, so part of the difference is more wakings.

## 3. Decisions taken

- The truth is the stock per tier; memory entries are the record (what, when, which tier, which act). Faith keeps its v0.4 memory model.
- Acts without a magnitude count as m = 1 in the gains.
- "Sage or envoy present at the target" needs people with places (item 07); witnesses are owner and neighbours for now.
- Oaths keep a memory condition beside the stability roll: with stability alone, a court that forgot the dragon entirely (D = 0) would hold `no_walls` for ever.
- Research start values are runner-set (`research_base` 0.016, wealth half 20, 50 wealth per unit); the formula's shape is the brief's.
- The written-grievance trigger exists (`grievance_written`, off): founding holds without it.
- Spells (`curse`, `teach`, `bless`, `raise_undead`) have gain rows but no reaction yet: they are not logged acts until item 09.

## 4. Deviations

- Oral half-life 60 years instead of 30 (the brief's second lever). At 30 played deaths fell to 4 %, under the band.
- `protect_one` fails its acceptance (see §2): the policy gives 0.1 every hundred years and defends only when attacked.
- `burn(library)` and `burn(temple)` are `act.item` values; item 09 makes them structures.

## 5. Open

1. The `protect_one` criterion: is the policy the right test (it wakes every 100 years), or should give and defend gain more?
2. Played outcome entropy fell from 4.03 to 3.87 bits (the band is 4.5; it failed in v0.4 too). Orcs are still the largest race in most played valleys.
3. Played deaths are 8 % (band 5 to 25 %, v0.4 21 %). Inside the band; say if you want them nearer v0.4.

## 6. How to verify in five minutes

```
cd games/hoard
cmake -S . -B build && cmake --build build -j && ctest --test-dir build -L fast
python3 tools/interest.py --bin build/hoard-sim --data data --seeds 1-60 --years 3000 --policy played --genesis --bands
./build/hoard-sim --data data --seed 42 --years 1600 --policy played --snapshots out/s42.jsonl --out out/s42
```
