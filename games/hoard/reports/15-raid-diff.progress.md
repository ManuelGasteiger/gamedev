# 15 progress

Branch `item/15-raid-diff` from main d1fe9c1.

Done
- sim/raid: snapshot.h (World.sortie, saved and hashed only while open), diff_book.h and diff_data.cpp (data/raid_diff.json), report.h, report_json.cpp, diff.cpp, sortie.cpp.
- recorded wrappers `begin_sortie`, `end_sortie` and their `apply_input` cases; `sleep` closes an open sortie.
- wasm commands `begin_sortie`, `end_sortie` (sim/wasm/raid_api.cpp), runner `--raid-report`.
- docs/raid_report.schema.json, tools/raid_report_check.py, text keys en and de.
- web/game/raid.js; the panel in game.template.html and the passive wound flush go through it.

Next
- native build and ctest -L fast; wasm build; smoke --golden 1-60; check.cjs; game-check.cjs; l10n-check.
- acceptance report (seed 42), direct vs routed act log, report file, README.

Blocked: nothing.
