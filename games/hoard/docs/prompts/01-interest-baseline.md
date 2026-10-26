# 01 · Interest tool: baseline, sleep-policy sweep, agency metrics

Depends on: nothing · Branch: `item/01-interest-baseline` · Runner: `build/hoard-sim`

Corrections from the 2026-10-26 audit are folded in: the burn-everything policy is `raze_all`; the 1000-year policy is `long_1000`; the time budget is per `--jobs`; the hatchling band is a proposal (warning only).

## Goal

Before any rule changes, record what the v0.4 sim does and build the measurements every later item reports against: a sleep-policy sweep (is the sleep bet real?), agency metrics (does the world react to the player?), a per-waking sleep-bucket analysis, a one-act perturbation test, and a bands file the tool checks against. Python tooling plus small read-only additions to the runner's output.

## Read first

- `tools/interest.py` and `data/policies/*.json`.
- `docs/core-loop.md` sections 7 and 9.
- `research-book-advanced-game-design.md` section 4 (project doc).
- `research-storylets.md` 8.6 and `scenario-spec.md` 8 (the dry-run shape item 10 will need).

## Scope

In:

1. **Sleep-policy sweep** `tools/sweep.py`: policies `rest_only` (always the phase's short window), `moult_only` (always the long window), `long_1000` (1000 years after the first moult), `raid_then_1000` (one raid, then 1000), `played`, `idle`, plus `newcomer` and `raze_all` for the first-hour slice. 300 seeds × 3000 years on generated worlds. Per policy: growth points per century per axis, phase by year 500/1500/3000, deaths and killer, hoard over time, outcome distribution. One table; `out/sweep/<policy>.csv` per seed per century with the dragon's stocks (hoard by category, growth, phase, health) and `<policy>_courts.csv` with each court's wealth, era, anti-dragon, fear, faith, cohesion, region count.
2. **Agency metrics** in `interest.py`: (a) share of hooks whose refs reach a dragon act within 3 hops; (b) the existing Jaccard against idle plus the share of events after the hatching whose cause chain reaches a dragon act within 3 hops; (c) the share of wakings whose headline event's chain does. A dragon act is an `act_*` event or `hook_resolved`.
3. **Sleep-bucket analysis**: under `played`, at every waking, sleep copies of the world for 1, 10, 30, 100, 300, 1000, 3000 years and record the best bucket for survival (lowest P(dead) + 0.5 · P(slayer band ready)) and for growth (most points). Report the share of wakings each bucket wins, all states and decisive states.
4. **Perturbation**: 100 seeds, `played` with the first burn +0.1 or the first sleep one bucket longer; count ending-class flips (alive/dead, killer race, largest race, hegemon).
5. **Lethality ratio**: under `played` and `long_1000`, at every waking, the listed wounds of the waking's acts divided by the dragon's current health, by target era × phase.
6. **First hour**: `newcomer` policy and a first-hour slice (0 to 150 years after the hatching): deaths with year and killer, wakings to the first tower, hooks answered, Might at year 100. Band: no death before year 80 under any policy, `raze_all` included. Proposal band, warning only.
7. **Replays**: one `.hrp` per seed for `played` and `newcomer` in `out/sweep/replays/`; `sweep.py --recorded run.hrp` replays a human recording through the end-of-run metrics.
8. **Bands** `data/bands.json`: core-loop 7 targets (`v0.4`) and research book 4.3 proposals (`proposal`). `interest.py --bands` prints pass/fail, exits non-zero on a failed `v0.4` band, warns on a failed proposal.
9. **Baseline** `reports/01-baseline-v04.md`.

Out: sim rule or data changes beyond what the runner must emit. No new ctest tests.

## Acceptance

- `python3 tools/sweep.py --seeds 300 --years 3000 --jobs N` writes `out/sweep/<policy>.csv` and a summary; the report states the wall time and N. The bucket analysis dominates; `--bucket-seeds` limits it.
- `python3 tools/interest.py --policy played --bands` prints the agency metrics and the bands table.
- Goldens unchanged; snapshots and wakings output are opt-in and not hashed.
- `python3 tools/sweep.py --selftest` (3 seeds × 300 years) checks the CSV columns and row counts.
