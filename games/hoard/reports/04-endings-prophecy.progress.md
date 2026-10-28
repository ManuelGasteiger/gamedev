# 04 · Endings, prophecy, legends: progress

Branch `item/04-endings-prophecy` from origin/main 7d8c372.

## Done

- All code, data, tests, tools; report draft `reports/04-endings-prophecy.md`.
- Golden commits (rules 8): chronicles + replays + save, state hashes (300), page (wasm, both HTML).
- Checks: `ctest -L fast` 20/20; `node web/smoke.mjs --golden 1-60` matches; `node web/l10n-check.cjs --lang pseudo,de` passed.
- Measurements: `tools/endings_rates.py` (300 seeds), `tools/endings_runs.py` (15 chains), in the report.

## Next

- `node web/game-check.cjs` (running).
- Interest tool after (300 seeds, idle,raze_all,played, --genesis) into report section 2b; baseline is in the session scratchpad (`main/interest_before.txt`), its played line is copied into the report.

## Blocked

- nothing
