# 01 · Baseline of the v0.4 sim

What the sim does before any rule change, on generated worlds. Every later item reports against these numbers.

Commands (wall time on 2 cores):

```
python3 tools/sweep.py --seeds 300 --years 3000 --jobs 2 --policy-seeds rest_only=10   # 3603 s
python3 tools/interest.py --bin build/hoard-sim --data data --seeds 1-300 --years 3000 --policy played --genesis --bands --sweep out/sweep/summary.json --jobs 2
```

`rest_only` ran on 10 seeds: it wakes every year and takes over 3 minutes per seed. The bucket analysis ran on 100 seeds (3,913 wake states). Raw output: `out/sweep/summary.md`, `out/interest-bands.txt` (not committed).

## Sleep policies, 300 seeds, 3000 years

| policy | deaths | killers | median death | phase @3000 (alive) | hoard @500 | @1500 | @3000 |
|---|---|---|---|---|---|---|---|
| rest_only (10) | 0 % | | | p1:10 (100 %) | 272 | 382 | 521 |
| moult_only | 12.3 % | human 15, dwarf 13, wounds 5, elf 4 | 2080 | p3:66, p4:195, p5:2 (87.7 %) | 140 | 251 | 292 |
| long_1000 | 7.7 % | dwarf 11, elf 6, human 6 | 1940 | p3:156, p4:121 (92.3 %) | 92 | 222 | 276 |
| raid_then_1000 | 0 % | | | p3:290, p4:10 (100 %) | 68 | 91 | 115 |
| played | 21.3 % | human 30, dwarf 15, wounds 11, elf 7, undead 1 | 1545 | p3:1, p4:227, p5:8 (78.7 %) | 191 | 284 | 305 |
| idle | 0 % | | | p3:300 (100 %) | 40 | 40 | 40 |
| newcomer | 0 % | | | p3:166, p4:134 (100 %) | 159 | 251 | 308 |
| raze_all | 97.3 % | wounds 263, dwarf 12, elf 8, human 8, undead 1 | 675 | p3:8 (2.7 %) | 168 | 193 | 151 |

Growth points per century, years 0 to 500 (might / flame / arcana / lore / dominion):

| policy | 0-500 | 500-1500 | 1500-3000 |
|---|---|---|---|
| rest_only | 180 / 776 / 5.8 / 103 / 186 | 217 / 2673 / 6.0 / 90 / 155 | 253 / 2836 / 6.1 / 85 / 140 |
| moult_only | 18.4 / 1.6 / 1.3 / 2.7 / 1.0 | 5.4 / 5.7 / 0.7 / 1.2 / 0.4 | 8.5 / 17.4 / 1.0 / 1.6 / 0.5 |
| long_1000 | 13.7 / 0.5 / 0.6 / 1.1 / 0.4 | 8.6 / 2.1 / 0.5 / 1.0 / 0.4 | 8.0 / 15.3 / 1.0 / 1.5 / 0.4 |
| played | 18.5 / 3.3 / 1.8 / 4.5 / 1.9 | 6.6 / 12.8 / 1.0 / 1.8 / 0.7 | 4.9 / 15.9 / 0.7 / 1.3 / 0.5 |
| idle | 12.3 / 0 / 0 / 0 / 0 | 5.2 / 0 / 0 / 0 / 0 | 2.2 / 0 / 0 / 0 / 0 |

Largest race at year 3000: played orc 206, human 74; moult_only orc 186; long_1000 orc 154; idle human 205; newcomer human 224.

## Sleep buckets under played (100 seeds, 3,913 wake states)

| bucket (years) | 1 | 10 | 30 | 100 | 300 | 1000 | 3000 |
|---|---|---|---|---|---|---|---|
| survival, all states | 22.1 % | 18.6 % | 15.7 % | 11.9 % | 10.0 % | 10.3 % | 11.3 % |
| survival, decisive (53.5 % of states) | 28.9 % | 22.3 % | 17.0 % | 9.8 % | 6.4 % | 6.9 % | 8.8 % |
| growth (decisive in 100 %) | 0.0 % | 2.3 % | 2.7 % | 4.1 % | 16.1 % | 32.8 % | 42.0 % |

Growth is scored as total points over the bucket, as the brief defines it, so longer buckets win by construction. Points per year would compare rates; the sweep does not store per-bucket points yet.

## Lethality (listed wounds of a waking's acts / current health)

Under played the mean stays between 0.03 and 0.29 in every era × phase cell and the share above 1 is 0 except era 2 phase 4 (0.4 % of 2,314 wakings), era 2 phase 5 (4.3 % of 46) and era 3 phase 4 (0.5 % of 370). Under long_1000 no cell goes above 1 (max 0.65).

## First hour (0 to 150 years after the hatching)

| policy | deaths < 80 y | first tower at waking | Might @100 | hooks answered |
|---|---|---|---|---|
| rest_only | 0 | 76.8 | 156.7 | 149.4 |
| moult_only | 0 | 4.1 | 40.2 | 5.0 |
| long_1000 | 0 | 2.3 | 8.0 | 1.2 |
| played | 0 | 5.4 | 42.0 | 6.7 |
| newcomer | 0 | 5.0 | 16.9 | 3.3 |
| raze_all | 35 (all at year 0, wounds) | 2.1 | 16.7 | 0.8 |

## Perturbation (played, 100 seeds)

First burn +0.1: 47 flip. First sleep one bucket longer: 46 flip. Either: 64 %.

## Agency and interest (played, 300 seeds)

- Hooks whose refs reach a dragon act within 3 hops: 13.5 % (22.6 % of hooks that carry refs).
- Events after the hatching whose cause chain reaches a dragon act: 7.0 %.
- Waking headlines whose chain does: 66.1 %.
- Jaccard against idle 0.901. Outcome entropy 4.03 bits, 47 distinct outcomes, 64 slain, 0 unwarned, 1.78 hooks per waking, 61.5 % answered.

## Bands

| metric | value | band | source | result |
|---|---|---|---|---|
| idle.outcome_entropy_bits | 4.815 | ≥ 4.5 | v0.4 | pass |
| played.outcome_entropy_bits | 4.033 | ≥ 4.5 | v0.4 | **fail** |
| idle / played first_three_top_share | 0.080 | ≤ 0.1 | v0.4 | pass |
| played.death_share | 0.213 | 0.05 to 0.25 | v0.4 | pass |
| idle.death_share | 0.000 | < 0.05 | v0.4 | pass |
| played.hook_coverage | 0.989 | ≥ 0.95 | v0.4 | pass |
| played.unwarned_deaths | 0 | 0 | v0.4 | pass |
| survival buckets winning ≥ 10 % (decisive) | 3 | ≥ 4 | proposal | warn |
| growth buckets winning ≥ 10 % | 3 | ≥ 4 | proposal | warn |
| survival top bucket share | 0.289 | ≤ 0.4 | proposal | pass |
| growth top bucket share | 0.420 | ≤ 0.4 | proposal | warn |
| perturbation flip share | 0.640 | 0.05 to 0.3 | proposal | warn |
| hatchling deaths before 80 | 35 | 0 | proposal | warn |

`interest.py --bands` exits 1 on the one failed v0.4 band.
