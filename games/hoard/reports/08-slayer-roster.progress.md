# 08 progress

Branch `item/08-slayer-roster`. Baseline binary (main 1478254, Release) is in /tmp/claude-0/base/hoard/build; rebuild there if the container restarted.

## Done
- Data: `data/slayers.json`, `slayer_data.{h,cpp}`, plumbing (data.h, data.cpp, data_io.cpp, record.cpp, api.cpp, web/build.sh, build.ps1, smoke.mjs).
- State: `Person.slayer`, `Order`, `World.orders` (state.h).
- `slayer_roster.{h,cpp}` (traits, power, scars, reveals, promotion, succession), `resolve_fight.{h,cpp}`.
- Order turn `rules/slayer_order.cpp` (after person memory, slow ticks), `rules/slayer_march.cpp` (failed march, gate fight), ladder changes in `rules/slayers.cpp`.

## Next
- Build, fix, push; persistence (JSON), person_seen route, briefing lines and text, view scrub, death recap, importance rows.
- Measure against baseline, tune, golden commit (kRulesVersion 11), replays, save check, wasm and web checks, report.

## Blocked
- Nothing.
