# Hoard: paper model of the loop and pacing arithmetic

Status 2026-10-24. A 200-line Python toy (`pacing-model.py`, beside this doc) of the loop as redesigned today: dragon growth with partial hoard consumption, mortal research = dread × wealth, dread and awe in three memory tiers, slayer bands, telegraphed death. Its job is to catch order-of-magnitude problems in the start values before items 02 and 03 touch the sim, and to settle the pacing arithmetic for 30 to 50 wakings per run. It is not the sim: five courts, two growth axes, one raid pattern, no cards, no people. Every number not taken from a design doc is a guess and is listed below. Treat the findings as directions for the real interest tool, not as results.

## 1. What the toy does

- Five courts start at era 1 to 3 (pre-history stands in for genesis), wealth 80 to 160 × (1 + 0.5 era), logistic growth 2 % per decade toward a cap of 3000, era advance on cumulative growth at the sim's thresholds [60, 150, 350, 800].
- Research per decade `0.012 × era_mult × (1 + 2·D·(1 − 0.7·A))`, spending 30 % of that share of wealth. Bolt-throwers at 0.2 (era ≥ 2), cannons at 1.6, a slayer order at 1.0 when `D_eff ≥ 0.5` or the written dread stock ≥ 0.25 (a recorded grievance). Band power grows per decade; an attempt is possible at 0.8 × threat; success `min(1, band / 3·threat) × (1 + wounds share)`.
- Dread and awe per court in oral / written / monumental stocks, `E = 1 − Π(1 − x)`, half-lives 30 / 250 / 1000 years, deposits per `act-api.md` section 6 (written at 0.6 when era ≥ 2, monumental at 0.4 with a temple or memorial), repetition factor on awe.
- Dragon: Might and a second axis; phases at 25 / 90 / 250 / 600 total points; moult windows [10, 100, 300, 1000, 3000], rest windows [1, 5, 20, 100, 300]; health 100 + 3 Might; threat 20 + points. Loot digests with a half-life of half a moult window and is consumed; gold above a floor (50 × [1, 2, 4, 8, 16] by phase) is consumed at 15 % per moult window.
- Two bed variants: **window** (core-loop v0.4 wording: Might = 0.6 × sqrt(gold) per moult window slept, so the floor keeps paying) and **consumed** (Might only from the gold consumed, 0.05 per unit; the floor is what the dragon keeps and grows nothing).
- Raid per waking: burn any court with slayers (m 0.5), burn the richest (m 0.4), take 25 % of its wealth per act until stamina runs out, one protective act (awe on the weakest). Wounds from listed defences, dampened by dread; death when wounds exceed health.
- Policies: idle, rest_only, mixed (two rests then a moult), moult_only, played (moult plus the protective act), long_1000 (1000 years after the first moult), raid_then_1000.
- Sleep-bucket analysis every third waking under `played`: for each bucket 1 / 10 / 30 / 100 / 300 / 1000 / 3000, growth gained (scaled by log of length) and slaying odds from 12 rolls on a copy; count which bucket wins growth and which wins survival.

## 2. Results (40 seeds × 3000 years, medians)

### Variant window (floor keeps paying)

| policy | points | phase | wakings | picks/waking | deaths | slain | year of death | courts with slayers | wakings per phase |
|---|---|---|---|---|---|---|---|---|---|
| rest_only | 123 | 3 | 58 | 0.18 | 100 % | 100 % | 340 | 90 % | 16 / 33 / 8 |
| mixed | 107 | 3 | 21 | 0.43 | 100 % | 100 % | 639 | 58 % | 6 / 12 / 3 |
| moult_only | 113 | 3 | 10 | 0.77 | 95 % | 95 % | 985 | 55 % | 3 / 6 / 2 |
| played | 110 | 3 | 10 | 0.78 | 95 % | 95 % | 935 | 54 % | 3 / 6 / 2 |
| long_1000 | 500 | 4 | 4 | 2.5 | 0 % | 0 % | – | 0 % | 2 / 0 / 0 / 2 |

Bucket wins under played: growth 3000 years 73 %, 1000 15 %; survival 3000 years 55 %.

### Variant consumed (Might only from consumed gold)

| policy | points | phase | wakings | picks/waking | deaths | slain | year of death | courts with slayers | wakings per phase |
|---|---|---|---|---|---|---|---|---|---|
| rest_only | 106 | 3 | 64 | 0.14 | 100 % | 100 % | 256 | 90 % | 29 / 29 / 4 |
| mixed | 50 | 2 | 30 | 0.22 | 100 % | 100 % | 508 | 69 % | 18 / 13 |
| played | 38 | 2 | 16 | 0.31 | 100 % | 100 % | 655 | 57 % | 11 / 6 |
| long_1000 | 13 | 1 | 3 | 0.65 | 100 % | 0 % (defences) | 1010 | 0 % | 3 |

Bucket wins under played: growth 300 years 61 %, 100 years 21 %; survival 300 years 54 %, 100 years 26 %.

A scan of the gold-to-Might rate (0.1 to 0.3 per unit) and the written-dread slayer trigger (0.3 to 0.5) in the consumed variant: points 72 to 145, phase 2 to 3, deaths fall to 0 to 10 % as Might makes the dragon too tough for the toy's defences, and long_1000 still ends with more points than played (87 to 182 against 71 to 145) because a court left alone for a thousand years is far richer when raided.

## 3. Findings (directions, not results)

1. **A bed floor that keeps paying makes the long sleep dominant no matter what consumption does.** In the window variant the 1000-year sleeper reaches phase 4 with five times the points and never meets a slayer. Partial consumption as decided today only fixes this if the gold bed converts gold into Might *as it consumes it*; the floor is what the dragon keeps, and it must not grow the dragon. Proposal for item 03: `Might += gold_pts × consumed_gold` per step, `bed_rate × sqrt(gold)` removed or kept at a tenth of today's value. This is a refinement of the decision, not a reversal, and is worth a line in `decisions.md`.
2. **The 30-year oral half-life is a large nerf to mortal memory compared with v0.4** (human `memory_halflife` 150 years, dwarves 500, elves never). With 100-year sleeps, oral dread is gone between wakings; only the written tier (era ≥ 2, deposit 0.6, 250 years) carries it, and it settles around 0.3 to 0.45, under the 0.5 slayer threshold. In the toy no slayer order ever formed under moult-length sleeps until the written-grievance trigger was added. Item 02 should check slayer founding (around year 400 in v0.4) first, and tune in this order: slayer orders form from a recorded grievance (written tier, as item 08 already says), then oral half-life 60 to 100 years scaled by race, then the 0.5 threshold.
3. **Risk has to scale with mortal advancement or the harvest/farm tension is not a bet.** Even the consumed variant leaves `long_1000` ahead on points when the dragon survives, because a thousand undisturbed years make the richest court very rich. That is the intended "farm versus harvest" tension from `ages-sim.md` section 1, and it is only a tension if waking into an era-4 valley can kill. In the toy, cannons plus garrisons deal about 170 wounds to a phase-3 dragon with 550 health: a bruise. Item 01 should measure, in the real sim, the ratio of a full raid's listed wounds to health at each era × phase; if an era-4 raid cannot kill a phase-3 dragon that walks in unprepared, the counters need to grow with era faster than Might grows with gold.
4. **The bucket analysis works as a method and shows the failure modes clearly**: in the window variant the longest bucket wins everything (no bet); in the consumed variant one middle bucket wins both growth and survival on more than half the wakings (a bet with a known answer). The target from `research-book-advanced-game-design.md` 4.3 stands: four or more of seven buckets best in at least 10 % of wake states, none above 40 %. Item 01 implements this on the real sim; item 03 tunes against it.
5. **Written-tier research persistence** (Sellers' missing loop 7) is what makes long sleeps cost something that is not growth: in the toy, research runs at base rate through every sleep regardless of dread, so a 1000-year sleeper wakes to cannons. Keep that: research must never go to zero while a court is at era ≥ 1, and burning the library should set it back (item 02 point 5 covers the spend; the setback on `burn(library)` is item 09's structure effect).

## 4. Pacing arithmetic for 30 to 50 wakings

Decision: 6 to 10 hours, 30 to 50 wakings per run. Observed in the toy and consistent with v0.4's measured phase years (phase 2 by year 50, phase 3 by about 510):

| phase | window (rest / moult) | wakings if moult only | wakings with two rests per moult | years spent |
|---|---|---|---|---|
| 1 hatchling | 1 / 10 | 3 to 5 | 9 to 15 | 30 to 60 |
| 2 young | 5 / 100 | 5 to 6 | 12 to 18 | 400 to 600 |
| 3 grown | 20 / 300 | 6 to 9 | 12 to 20 | 1500 to 2500 |
| 4 ancient | 100 / 1000 | 1 to 3 | 3 to 6 | the rest |

- Moult-only play gives 15 to 20 wakings in 3000 years: too few for the target. Mixed play (rests to answer hooks, moults to grow) gives 30 to 50 in 2500 to 3500 years. So the target is reachable with today's windows **if rests are worth taking**, which means hooks and scenarios must pay on rests (they do: a rest heals a hatchling fully, and a scenario answered is growth through its act). The sleep screen should present rest as the default answer to an open hook and moult as the growth move.
- If the sweep shows players (bots) never resting, shrink the windows to [10, 60, 200, 600, 2000] / [1, 5, 20, 60, 200] rather than lengthening the run in years; 3000 years is already the horizon the valley's history is tuned for.
- Talent picks: v0.4 measured a pick on 6 of the first 10 wakings; the toy gives 0.3 to 0.8 picks per waking depending on growth rate. With five axes and nodes at 3 / 8 / 15 / 25, a run of 40 wakings reaching 90 to 250 total points yields roughly 20 to 30 picks: one pick on most wakings, two on a good one, none on a bad one. That matches the design intent; do not add nodes until the sweep says wakings go dry.
- Run length in hours: 40 wakings × (raid 6 to 10 minutes + wake screen and decisions 3 to 5 minutes) is 6 to 10 hours. The raid's real-time length, not the sim, decides which end of the range a run lands on.

## 5. Guesses in the toy (replace with sim values before trusting any number)

Court growth 2 % per decade toward 3000; starting eras 1 to 3; research base 0.012 and spend 30 %; cannons at anti-dragon 1.6; slayer band start 0.3 × threat, growth 0.02 × threat × (1 + D) per decade, attempt at 0.8 × threat, attempt chance 0.05 × D (1 − 0.7 A) × band / threat; defence damage per act 2 / +8 / +14 / +10 dampened by 0.6 D; raid takes 25 % of the richest court per act, up to 7 acts; loot to growth 0.12 per unit; gold to Might 0.05 per unit consumed (consumed variant); threat 20 + points; the written-grievance slayer trigger at 0.25.

## 6. What to carry into the prompts

- Item 03: add the bed refinement from finding 1 as the first design note, and the bucket targets as acceptance (already there).
- Item 02: add finding 2 as an explicit check: slayer founding year under `played` within 300 to 600 (v0.4: about 400); tune oral half-life before thresholds.
- Item 01: add finding 3 as a measurement: full-raid listed wounds / health per era × phase under `played` and `long_1000`.
