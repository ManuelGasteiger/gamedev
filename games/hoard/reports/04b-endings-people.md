# 04b · Endings and people follow-ups

Branch `item/04b-endings-people` from origin/main 4659c29. Owner's decisions of 2026-11-02. Nothing paid, no external calls.

## 1. What changed

Added:
- `sim/runner/ascend_bot.{h,cpp}`, `data/bots/ascension.json`: the Ascension test bot and its probe (runner only). Bots `oracle:ascension` (page stamina) and `oracle:ascension:free` (no budget). It picks up to 5 target courts covering at least 3 races (best awe + faith - dread), and every waking: defends them against attackers, miracles while the faith pool lasts, accepts offerings where faith > 0.5, gives gold, shows itself where dread ≤ 0.4, takes art and gold from the richest non-target, spends spare stamina on more gifts (least faithful first), accepts tribute oaths and receives the targets' visitors, casts `teach` on the target nearest the next era, never harms a target, sleeps short. Talents from `steer_ascension` (Awe, Cult of the Mountain, Gilded Throne, Brood, Dragon-king first). It writes `ascension.json` (per condition: hold, best counts, parts, conditions met together).
- `tools/ascension_probe.py`: runs the bot over seeds and prints the tables below.
- `sim/ages/person_seen.{h,cpp}`: whether the player knows a person (see 3).

Changed:
- `data/moves.json`: Court threshold 4.
- `data/endings.json`, `data/text/{en,de}/endings.json`: Mortality conditions and their met/missing lines.
- `data/constants.json` people: `priest_min_era` 1, `keeper_second_era` 2; `sim/ages/data.{h,cpp}`; `rules/people_rise.cpp`: priests from era 1, a second sage and a second priest from era 2.
- `sim/ages/serialise.cpp` (`people_json`, world view `people` and `dead_people`), `briefing.cpp` (people block): only people the player knows.
- `sim/ages/offers.cpp`: a show, defend or give meets the heir as well as the ruler.
- `sim/runner/{bot,main,analyse}.cpp`, `tools/interest.py`: bot spec `oracle:ascension[:free]`, census of tracked and seen people by role.
- `web/game/court.js`: the empty court line reads "no one you have met or heard named". `web/game-check.cjs`: shows itself at every capital before the court check; seed 40 instead of 39 (see 5).
- `sim/ages/record.h`: `kRulesVersion` 9 (golden commit).

## 2. Numbers

### Ascension: is it reachable with today's rules?

No. 120 seeds × 3000 years, generated worlds, thresholds unchanged. Ready in 0 of 120 runs with the page's stamina and 0 of 120 without a budget. The bot dies to slayers in 67 % of runs (median end year 2830).

| condition | ever met (stamina / free) | longest hold, max | best value reached (max, p90, median over runs) |
|---|---|---|---|
| peoples (4 courts of 3 races awe ≥ 0.6 and faith ≥ 0.4; 2 at faith 0.5) | 51 % / 49 % | 580 y / 720 y | courts 11, 6, 5; races 5, 3, 3 |
| temples (those 4 with a dragon temple; 3 older than 100 y) | **5 % / 7.5 %** | **170 y / 200 y** | with temple 11, 5, 3; older than 100 y 5, 3, 2 |
| craft (magic ≥ 0.6 or a court in the arcane era) | 98 % / 96 % | 2930 y | arcane courts 11, 10, 6 |
| voice (a priest in a cult court that buried one; cult courts of 3 races) | 48 % / 46 % | 690 y / 1170 y | courts with the priest line 10, 5, 2 |
| mild (mean dread of the cult courts ≤ 0.5) | 85 % / 88 % | 570 y | lowest mean dread 0.17 |
| strong (3 cult courts at castles; slayers ready or cannons) | 69 % / 61 % | 1830 y | cult courts at castles 10, 6, 4 |
| dominion (Dominion ≥ 30, 4 give/miracle acts) | 100 % | 2960 y | Dominion 571, 406, 248 |

Most conditions met at one waking: 6 of 7 in 3 runs (free: 6), 5 in 20 (16); median 3. In every one of those best runs the missing condition was temples. Most conditions held their 150 years at the same waking: 6 at best, median 2.

Blocker: **temples**. The bot meets peoples, voice, mild and strong for centuries, but only 3 courts (median) ever have a dragon temple at once, and only 2 an old one. Why, from the code: the only temple is the `dragon_temple_raised` card (`data/events/monuments.json`), which needs 50 years of high awe and the court's `dragon_temple` tag absent, and it sets that tag for 300 decades with `inherit: true`. The monument itself is not inherited: a court that collapses, splits or is succeeded passes the tag but not the temple to its successors, who can then never raise another for 3000 years. In seed 3 (bot) 5 courts had a temple and 7 had the tag without one. So with today's rules temples cannot reach 4 and hold, and the stamina budget is not the limit (free changes nothing). This is item 12's consecrate, or a data question for the card; I did not change it.

Second limit: the strong mortals kill the bot. With a court holding slayers ready or cannons (condition 6), it dies in two thirds of the runs before year 3000, as the research intends.

### Mortality harder

Target chosen: played ready share under 20 % (well under 26 %) and knowledge leverage as close to 1.2 as a condition that still needs awe allows. The variants were run on the same binary (played: 150 seeds × 3000 y; first runs: 100 chains, `endings_runs.py`).

| variant | played ready | oracle first run | amnesiac first run | leverage |
|---|---|---|---|---|
| before (awe 0.4, sage or priest, hold 10, received within 2 wakings) | 27.3 % (25.0 % of 300) | 0.97 (0.95) | 0.25 | 3.80 |
| awe 0.5, hold 50 | 8.0 % | 0.76 | | |
| + the voice keeps the dragon's memory (keeper) | 0.7 % | 0.20 | | |
| awe 0.5, keeper, hold 30, audience = an arrival from the court | 18.7 % | 0.35 | 0.14 | 2.50 |
| **awe 0.5, keeper, hold 50, audience = an arrival from the court (taken)** | **17.3 %** | **0.22** | **0.14** | **1.57** |

Taken: the voice needs awe 0.5 and a sage or priest tagged `keeper` (who took up the dragon's memory), held 50 years; the audience is any arrival from that court in the last two wakings, received or not. Why: every variant that keeps "the dragon received them" lets a player who knows the condition force it (receive every visitor), so leverage stays above 3. Moving the audience to the court's side (it sought the dragon out) is what brought leverage down; the keeper and the longer hold bring the played share down. 1.57 misses 1.2: the remaining gap is the oracle's policy itself (no burning, gifts and protection raise awe in every court).

Final binary, 300 played seeds: Mortality ready in 15.3 % (before 25.0 %); voice ever met 25.7 % (88.7 %), audience 24.0 %. Ascension 0 %, Myth 71.0 %.

### Exits (decision 3)

No change needed. The sim adds an `exit` offer row for every enabled ending that is ready at the waking (`exit_offers`, called from `generate_offers`); the page lists every offer row in the envoy audiences, with Accept running the exit. The pacing bots (`attach_bot`) take any ready exit. The Ascension test bot takes only Ascension's (it is a probe).

### Court threshold 4 (decision 4)

Census (every move scored on every tracked person at every waking, 100 played seeds): Court accepted 30 % at threshold 3 on main, 18 % at 4 on this branch (2 points under the 20 to 80 % band). Other moves: teach 37 %, corrupt 31 %, bargain 68 %, threaten 61 %. `move_scores` needed no change (it checks scores, not the threshold).

### People per court

Played, 100 seeds × 3000 years, after year 300 (`interest.py`):

| | main | this branch |
|---|---|---|
| tracked people per court | 2.93 | 3.90 |
| by role: ruler, heir, hero, sage, priest | 1.00, 0.82, 0.30, 0.42, 0.40 | 1.00, 0.81, 0.28, 0.80, 1.01 |
| with a dragon memory per court (brief: 2 to 5) | 1.72 | **2.26** |
| seen by the player per court (played policy) | all | 1.33 |

Interest tool, played, 100 seeds × 3000 years, main data and binary before, this branch after:

| | outcomeH | distinct | rev/1000y | causal | persist | events | slain | maj/10w | arrivals/waking |
|---|---|---|---|---|---|---|---|---|---|
| before | 3.77 | 24 | 7.47 | 0.086 | 0.395 | 5818 | 11 | 40.5 | 2.13 |
| after | 3.65 | 25 | 7.13 | 0.127 | 0.406 | 6609 | 15 | 49.8 | 2.02 |

More events (keepers rising), causal depth up (keepers name the memory they took up), majors per 10 wakings up from 40.5 to 49.8.

The cap of 8 (ruler, heir, 2 heroes, 2 sages, 2 priests) is the tracking cap from item 07 and stays. What the courts support: every court holds one region in generated worlds, so the region count cannot be the measure (my first try with "two regions or more" never fired); the era is. Sages need era 2 and openness > 0.5, priests now era 1 and piety > 0.5, the second of each era 2. Heroes stay as they were (renown 0.02 per decade, cap 3; they also die leading slayings): more heroes would mean more slayer leaders and more usurpers, which changes the slayer odds the owner tuned.

## 3. Decisions taken

- A person is known to the player when: in the known ledger, a hostage, an envoy at the ledge or a waiting visitor; or the hoard holds a captive or a source (books) of their court; or a historian is kept, who names the rulers and heirs of every court. Dead people show only from the ledger. `world.people` in the sim and in saves stays complete; only the views (`people` command, world view, briefing people block) filter.
- A show, defend or give at a court meets its heir as well as its ruler (before: the ruler only). Without it the court card would show only the ruler, and an heir would be unknown until taken.
- Mortality's audience reads `arrival` events from the court (any kind). The brief's "received or brokered" is the old reading; the rumor routes are unchanged.
- The Ascension bot is a runner tool, not a policy file: it needs per-court choices (targets, races, attackers) a policy cannot express. Its costs are the page's `COST` table.

## 4. Deviations

- Court acceptance at threshold 4 is 18 %, under the item 07 band of 20 to 80 %. Taken as the owner decided.
- Leverage 1.57, not ≤ 1.2.
- `web/game-check.cjs` seed 39 → 40: the new history of seed 39 (fixed start; more priests draw names from the world stream) had no court at war asking for protection within 12 centuries, and the envoy check needs one. Seed 40 does at the first waking.

## 5. Goldens

Rules 8 to 9 in one commit `golden: ...` (chronicle hashes, the three chronicle texts, `sift_bank_v1.hash`, 300 state hashes, replays, save expectation), plus the page rebuild. First differences in the chronicle texts: seed 1 at 210 (a keeper priest rises in Halley where a sworn hero rose), seed 2 at 180 (two piety priests in Cormarch and Marsary), seed 3 at 190 (a keeper again in Galstead, a piety priest in Galmere): priests from era 1 and second keepers. From there the world stream moves (each new person draws a name and four traits from it). The fixed-start chronicle hashes move for the same reason.

## 6. Open questions

1. Temples: should a successor inherit the dragon temple monument along with the tag, or should the tag be shorter than 300 decades? Either would let Ascension's temples condition reach 4; it is the one that blocks.
2. Mortality's audience now does not need the dragon's act. Keep that reading, or keep "received" and accept leverage above 2?
3. Court at 18 %: under the band; leave it?
4. People with a dragon memory per court is 2.26, inside 2 to 5 at the low end. More heroes would raise it but change slayer odds; say if you want that.

## 7. How to verify

```
cmake --build build -j2 && ctest --test-dir build -L fast
python3 tools/ascension_probe.py --bin build/hoard-sim --data data --seeds 1-120 --jobs 2      # Ascension table
python3 tools/endings_rates.py --bin build/hoard-sim --data data --seeds 1-300 --jobs 2         # Mortality played share
python3 tools/endings_runs.py --bin build/hoard-sim --data data --chains 100 --runs 1 --bots amnesiac,oracle:mortality --jobs 2
python3 tools/interest.py --bin build/hoard-sim --data data --seeds 1-100 --policies played --genesis --jobs 2   # people and move census lines
node web/smoke.mjs --golden 1-60
```
