# 10 · progress

Branch `item/10-scenario-runtime` from main d1fe9c1.

## Done

- Seed bank moved to `data/scenarios/` (seed-01-10, 11-20, 21-30, vocab). `docs/scenarios/localization-seed-de-draft.json` stays (a draft, not bank). Paths updated: CMakeLists `text_lint_seeds`, `tools/text/review.py`, `tools/text/scenario_strings.py`, `tools/bank/translate.py`.
- Runtime `sim/scenarios/` (namespace `scn`): bank, predicate, fields, bind, select, present, text_ctx, answer, lab, scn_json. State `World.scn` (saved and hashed only while `on`). `scn::wake` runs in `sleep.cpp` after `sift_wake`. Briefing gains `scenarios` while on. `answer_scenario` recorded (wrapper in `recorded.cpp`, case in `record.cpp apply_input`).
- Runner: `--scenarios`, `--scenario-bot NAME` (data/bots/scenario_answers.json), `--scenario-log FILE`, `--lab SPEC`. Replay header carries `scenarios: true`.
- wasm: load/reset `scenarios: true`, command `answer_scenario`, data `scenarios` and `scenario_vocab` (build.sh, build.ps1; build.ps1 also lacked sim/sift, added).
- Tools: `tools/scenario_check.py` steps 3 (always) and 4 (`--lab`), `--usage`; `tools/scenario_lab.py` helper; `tools/scenario_dryrun.py`; `tools/interest.py --scenarios`.
- Goldens: seeds 1-30 state hashes equal with the bank off.

## Next

- Full build and `ctest -L fast` (the first run segfaulted because state.h changed mid-build; rebuild running).
- Forced-state run of the 30 seeds, then dry run 300 x 6, fix seeds the runtime shows wrong.
- Interest before/after; wasm build and page checks; report.

## Seed changes so far

- hero_at_dawn: hero `memory_has: sworn_to_memory` -> `tag: sworn_to` (people_rise.cpp writes `sworn_to:<mark>` on the hero; no memory line of that name exists).
- the_one_you_spared: spared `memory_has: spared_by_dragon` -> `tag: pardoned` (judge spare writes the tag).
- princess_returns: agent `memory_has: schooled:intrigue` -> require `has_tag(agent, schooled:intrigue)` (it is a tag, not a memory line).

## Blocked

- nothing
