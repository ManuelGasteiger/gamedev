# 05 · Chronicle: progress

- State: `Dragon.chron` (historian tenures, held sources, relics), saved only when used; `World.wake_snapshot` and `World.counterfactual` (not saved, not hashed).
- Verbs: `keep_person {id, role: historian}`, `commune {relic, answer}`, recorded, in `apply_input` and the wasm API.
- Sources: `take books` brings up to 4 written tellings of threads the court was cast in (none older than its last burned library); `give books` hands the oldest on; a captive tells its own court's side. `make_source`: an honest court (openness > 0.6) names the true agent, else dread, a `grudge:` tag, the last war.
- Levels per line: 2 where a kept historian's tenure covers the line's year (3 with Lore rank 25), else 1 with a held source or a captive of a cast court, else 0. Text from `data/text/<lang>/chronicle.json`.
- Relics: a take of artifacts finds one at 0.25 x magnitude, by the court's race (constants.json `chronicle.relics`). The last relic with an agenda speaks every waking (D7: none at hatching).
- Level 3: the waking's acts and the sleep re-run twice from the wake snapshot, with and without the headline's act.
- Tests: `chronicle_levels`, `sources_bias`.

Next: build, tests, goldens, the seed 42 renders, the report.
