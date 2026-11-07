# 10 · Scenario runtime, validator, dry run

Branch `item/10-scenario-runtime` from main d1fe9c1. The runtime is namespace `scn` in `sim/scenarios/`; `ages::run_scenario` (the policy runner) is untouched. Nothing paid, no external calls.

## 1. What changed

Added:
- `sim/scenarios/bank.{h,cpp}`: loader of `data/scenarios/*.json` and `vocab.json`; fails on an unknown role, option, field, tag, gate or chooser; bank hash in the data hash.
- `sim/scenarios/predicate.{h,cpp}`: spec section 4 grammar (clauses joined by ` and `, `not`, `bound`, `has_tag`), evaluator; set fields (`person.trait`, `dragon.talents`) compare by membership.
- `sim/scenarios/fields.{h,cpp}`: every vocab field of faction, region, person, item, remnant, act, dragon, lair, world; tag rules; act waking index; the vocab gates.
- `sim/scenarios/bind.{h,cpp}`: role binders with backtracking, act patterns, `is` expressions, `remembers`, `choose`, optional roles, `bind_why`.
- `sim/scenarios/select.cpp`: selection per waking (spec section 6), `wake`, `enable`, `note_seen`.
- `sim/scenarios/present.cpp`, `text_ctx.{h,cpp}`: the briefing's `scenarios[]` block, option act resolution.
- `sim/scenarios/answer.cpp`: `answer_scenario` and outcomes. `sim/scenarios/lab.cpp`: the forced-state world overlay. `sim/scenarios/scn_json.cpp`: state in saves.
- `sim/runner/scn_cmd.{h,cpp}`: `--scenario-bot`, `--scenario-log`, `--scenario-why`, `--lab`.
- `data/bots/scenario_answers.json`, `data/policies/random.json`.
- `tools/scenario_dryrun.py`, `tools/scenario_lab.py`.

Changed:
- `data/scenarios/` (moved from `docs/scenarios/`: the three seed files and `vocab.json`; `vocab.json` gains `gate_rows`). The German draft stays in `docs/scenarios/`.
- `state.h` (`ScnShown`, `ScnState`, `World.scn`), `data.h/.cpp`, `data_io.cpp`, `record.cpp` (data hash, `apply_input` case), `recorded.cpp` (wrapper), `serialise.cpp` (state, only while on), `sleep.cpp` (`scn::wake` after `sift_wake`), `briefing.cpp` (block while on), `person_seen.cpp` (named by a shown scenario).
- `sim/runner/main.cpp`: `--scenarios`, `--min-sleep`, the three scenario flags; replay header `scenarios: true`.
- `sim/wasm/api.cpp`: `load`/`reset` flag `scenarios`, command `answer_scenario`, data `scenarios` and `scenario_vocab` (8 lines). `web/build.sh`, `web/build.ps1`: sources and data (`build.ps1` also lacked `sim/sift`, added).
- `data/constants.json` "scenarios", `data/text/{en,de}/ui.json` (six reason keys, five size words).
- `tools/scenario_check.py` (`--lab`: steps 3 and 4; `--usage`), `tools/interest.py` (`--scenarios`), `CMakeLists.txt` (sources, `text_lint_seeds` glob), `tools/text/review.py`, `tools/text/scenario_strings.py`, `tools/bank/translate.py` (paths), README.
- Seeds: section 9.
