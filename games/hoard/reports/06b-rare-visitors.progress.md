# 06b rare visitors: progress

Branch `item/06b-rare-visitors` from origin/main 4659c29.

## Done

- Step 0, measurement: wakings log carries each court's lair level; `tools/interest.py` prints arrivals per 1000 years, same kind on consecutive wakings, counts in the first 40 wakings, years to rough and exact lair knowledge.
- Baseline (main, played, 100 genesis seeds x 3000 y): scratch `base.txt`. Per run in the first 37 wakings: envoy 11.8, prophet 23.1, slayer 10.6, thief 21.7, pilgrim 1.7, refugee 1.3, kin 0.35. Lair: 56 % of courts rough (median 30 y), 11 % exact (median 170 y).
- Step 1 (commit "lair knowledge from evidence"): `lair_oral`/`lair_written` stocks, evidence table, fade per step (`fade_lair_knowledge` in step.cpp before arrivals), show gives nothing. With flight 0.04 it was far too slow (4 % rough, median 695 y; envoys and thieves vanished).
- Step 2 in progress (uncommitted): world cooldowns per kind, one visitor of a kind at the ledge, no kind on consecutive wakings, slayer clock readies only with lair knowledge and when the slayer_band row rests not; flight seen by courts with land near the lair; evidence retuned (flight 0.1, returned 0.4, hostage 0.5, thief 0.4). Tests arrival_gating/arrival_cooldown adjusted.

## Next

1. Tune to targets (per 40-waking run: envoy ~4, thief ~3, slayer ~3, prophet ~2-3, pilgrim ~2, refugee ~1-2, kin <=1), then 100-seed interest run.
2. game-check labs (`seeLair` sets `lair_oral`), wasm build, smoke, l10n, game-check.
3. golden commit (kRulesVersion 9), report.

## Blocked

Nothing.
