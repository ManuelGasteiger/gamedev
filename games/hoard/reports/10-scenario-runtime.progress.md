# 10 · progress

Branch `item/10-scenario-runtime` from main d1fe9c1.

## Done

- Seed bank moved to `data/scenarios/` (seed-01-10, 11-20, 21-30, vocab). `docs/scenarios/localization-seed-de-draft.json` stays (a draft, not bank). Paths updated: CMakeLists `text_lint_seeds`, `tools/text/review.py`, `tools/text/scenario_strings.py`, `tools/bank/translate.py`.

## Next

- `sim/scenarios/` runtime (namespace `scn`), state in `World.scn` (saved and hashed only when on), runner `--scenarios`, wasm.

## Blocked

- nothing
