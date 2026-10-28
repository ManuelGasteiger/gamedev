# 07 people moves: progress

Branch `item/07-people-moves` from origin/main 7d8c372.

## Done

- state: `PersonTraits.list`, virtue, vice, `mem`, `permanent`, `rel`, `flips`; `World.secrets` (commits bfb118a, 8dc3fcf for serialise via `person_json.cpp`).
- `data/moves.json` and loader `move_data.{h,cpp}`, plumbed through data_io, api.cpp, build.sh, build.ps1, smoke.mjs, data hash.
- `person_record.{h,cpp}`: traits at birth (keyed rng "traits"), tracked cap, axes, acquire, witness memory from acts, inheritance at death.
- `moves.{h,cpp}`: `resolve_move`, `read_release`, `release_terms`.
- rules after rule_people: `person_memory.cpp`, `secrets.cpp`, `inquisition.cpp`.
- cards: branch `boost`, effect `fall`; `hostage_came_home.json` extended; `data/events/intrigue.json` (inquisition, purge).
- text: narrator en lines, importance rows, ui reasons en and de.
- Baseline interest (main, played, 300 seeds): scratchpad `interest-before.txt`.

## Next

- Build, run fast tests, commit in steps.
- Tests `move_scores`, `princess_chain`.
- Runner `--move-census`, interest.py metric for persons with dragon memory.
- 300-seed measurement, golden commit (rules 8), wasm and page, checks, report.
