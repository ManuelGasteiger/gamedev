# 10 · Scenario runtime, validator, dry run

Model: Opus 5.5 (or Fable 5.1) · Effort: high · Depends on: 02, 09 (06 for arrivals) · Branch: `item/10-scenario-runtime`

## Goal

Build the engine the scenario bank runs on: load and validate scenarios, bind roles over the act log and world state, select per waking by the spec's algorithm, present through the briefing, resolve the chosen option into an act with `answers` set, log `shown` and `cites`. Plus the two tools every later bank passes through: a validator (schema, lint, satisfiability, forced-state) and a dry-run harness over seeds and bot policies. The 30 hand-written seed scenarios make it real.

## Notes for this tree (2026-11-07)

- Rules version is 11 (`sim/ages/record.h`), after items 08 and 09. Item 15 runs at the same time. Both of you keep the goldens unchanged (bank off for the golden suite, policies unchanged). If a golden has to change anyway, set `kRulesVersion` to 12 in a `golden:` commit and say why in the report; I regenerate once when merging the two.
- **The seed bank exists.** Thirty scenarios are in `docs/scenarios/seed-01-10.json`, `seed-11-20.json`, `seed-21-30.json`, with `docs/scenarios/vocab.json` and `docs/scenario.schema.json` (spec §10). Do not write 20 new ones (scope 10 below predates them). Move the bank to `data/scenarios/` (update the `text_lint_seeds` glob in `CMakeLists.txt` and every path that reads it) and make all 30 load, validate and fire. Fix a seed only where the runtime shows it is wrong, and list each change. The iteration gate: the 30 seed scenarios run.
- **The validator exists.** `tools/scenario_check.py` does steps 1 and 2 (schema, lint) and, with `--text`, the text lint and forced-state renders in every language (item 19, `tools/text/lint.py`, ctest `text_lint_seeds`). Extend it with steps 3 and 4; do not start a second validator.
- **Name clash.** `run_scenario` in `sim/ages/policy.cpp` is the policy runner (a "Scenario" there is a scripted policy). Put the scenario runtime in its own namespace or prefix so the two cannot be confused, and say which.
- **Text.** Scenario text: English in the scenario file, every language in `data/text/<lang>/scenarios.json` (spec §12, `docs/style-de.md`); the renderer in `sim/text/`. The sim emits codes and records; refusal reasons are `code|key=value` (`ages.h` `reason_code`). New UI strings need an `en` and a `de` line.
- **Item 09 facts you build on.** An act's id is its index in the act log; acts carry `source`, `answers`, `secondary` and a typed payload; all 40 schema acts are accepted by the API with gates in `data/gates.json` and reason codes. `ignore` is an act. `terraform(mountain)` is refused (only wasteland and lake exist), so a seed that needs it will fail its forced-state run: report it, do not add the kind. `tools/act_check.py` validates act logs. The policies `schemer` and `priest` exist in `data/policies/`; map the dry run's six bot policies to existing policy files (`raze_all`, `rest_only`, `protect_one`, `priest`, `schemer`, and a random one you add as a new file) and say how. `played.json` stays unchanged.
- **Item 08.** Slayer orders have named people with rank and traits; a scenario that binds a slayer reads them.
- **Chronicle and causes.** The because-line follows the chronicle level (item 05) and, since 05b, a cause shows only when the history is known.
- **People are hidden until met** (`sim/ages/person_seen.cpp`, item 04b). A scenario shown to the player that names a person makes that person learned of: add the route there.
- **Profile.** Seen counts across runs come from the profile (`sim/ages/profile.h`, item 04).
- New rolls draw from `rng_for(seed, tag, id)` (`sim/ages/rng.h`), never `world.rng`. Selection's tie and weight draw is keyed on (seed, waking index, bank hash) through it.
- Every new state-changing verb (`answer_scenario`, `lab_world` if it is reachable from the page) needs a wrapper in `sim/ages/recorded.cpp` and a case in `apply_input`, or replays break.
- Item 15 also touches `sim/wasm/api.cpp`, `recorded.cpp` and `CMakeLists.txt`. Keep your additions in your own files and small at those three places.
- Tests: add none. The golden `scenarios_seed_v1.hash` below is a test too; list it with the proposed tests and do not add it. Manu decides.
- `tools/scenario_check.py` and `tools/act_check.py` need `jsonschema` (`pip install --break-system-packages jsonschema`).
- The machine has 2 cores and a limited disk. Write run output under the scratchpad or `/tmp`, delete it when done, and never commit it. LFS uploads are refused: do not commit `web/sandbox-check.png` or other LFS files.
- Before you finish: `ctest --test-dir build -L fast`; the wasm build (`git lfs pull --include "games/hoard/assets/gltf/**"` first) and `node web/smoke.mjs --golden 1-60`; `node web/l10n-check.cjs --lang pseudo,de`; `node web/game-check.cjs`; `node web/check.cjs`. Push the branch; no pull request.

## Read first

- `docs/scenario-spec.md` entirely and `docs/scenario.schema.json`.
- `docs/research-storylets.md` sections 2, 3, 6 and 8 (why the algorithm is shaped this way; the Six Ages bulk tester; Yarn Spinner's read-only query rule).
- `docs/research-book-procedural-storytelling.md` sections 3.1 and 3.6.
- `docs/act-api.md` sections 1 and 3 (what the shell owes the sim per act; `answers`).
- `reports/09-new-acts.md` (the act record, gates, reason keys, coverage).
- `rules/card_match.cpp` and `cards.h` (reuse the predicate machinery where it fits; the scenario predicate grammar in spec section 4 is a string form of the same thing).
- The briefing code (17.5) and `tools/interest.py`.

## Scope

In:

1. **Loader** `sim/scenarios/bank.cpp`: reads `data/scenarios/*.json` (one file per cell or theme, an array each) and `data/scenarios/vocab.json` (closed lists: themes, paths, gate names from `data/arrivals.json`, tokens, chooser names). Fails loudly on an unknown id reference or vocabulary miss. The bank hash is part of the run's data hash.
2. **Predicates** `sim/scenarios/predicate.cpp`: parser for the grammar in spec section 4 to a small AST; evaluator over a `Context` that exposes the vocab paths (court.*, dragon.*, lair.*, world.*, role fields). `bound(role)`, `has_tag(role, tag)`, `not`, `and`. No `or`.
3. **Role binders** `sim/scenarios/bind.cpp`: act patterns over the log (type list, structure, payload_has, magnitude range, age in wakings or years, source, answers, target role, `after`/`before` another act role), faction/region/person/item/remnant binders with the `is` expression functions (`owner_of`, `neighbour_of`, `strongest_army`, `worst_relations`, `has_tag`), `remembers` (the court holds a memory of that act at a tier with stock ≥ 0.1), `choose` rules. Optional roles.
4. **Selection** `sim/scenarios/select.cpp` per waking exactly as spec section 6: eligibility, binding, score, slots, tier-first pick within 3 points by weight, quotas, pity, log. Pure function; writes after presentation. Seen counts for earlier runs come frozen from the profile file (item 04) at run start.
5. **Presentation**: the briefing gains a `scenarios[]` block: per slot the rendered headline, body (variant if its predicate holds), because-line (only when the chronicle level allows, item 05), options with labels, gates and the resolved act preview (which defences would strike, per core-loop §4 "numbers before acts"). Tokens `{role.field}` render from bound ids; an unbound token is a lint failure, never a runtime blank.
6. **Answering**: API `answer_scenario {scenario_id, option_id, magnitude?}` resolves the option's act template with the bound ids, validates against the act API, sets `answers` and `source: scenario`, applies it through `apply_acts`, runs the option's `outcomes` (suppress tag for N years, history line on a person via item 07's memory, `seek` queued for the next waking), and marks the scenario `shown.answered`. `ignore` is an act like any other.
7. **Validator** `tools/scenario_check.py` (Python, jsonschema): steps 1 to 4 of spec section 8. Step 4 (forced state) drives the headless runner through a `--lab` mode: build a minimal world from the roles (the runner gains `lab_world {courts, persons, acts}` in the API), play every option, assert acceptance and a rendered text with no unbound tokens. Reports per scenario and a vocabulary usage table.
8. **Dry run** `tools/scenario_dryrun.py`: steps 5 to 7. 300 seeds × 6 bot policies (burner, sleeper-hoarder, protector, priest, schemer, random; bots answer scenarios by a per-policy option preference), recording per scenario eligible rate, shown rate, waking positions, cites, and the per-cell depth metric; cut and flag lists per the thresholds; the bank metrics; a JSON summary the authoring pipeline (item 11) reads to steer generation.
9. **Interest tool**: `cited_player_act_share` (target ≥ 60 %), slot fill rate per slot, scenarios per waking, theme share, and the bank on/off delta on causal depth and agency.
10. **Seed bank**: the 30 existing seed scenarios (see the notes), moved to `data/scenarios/`. These are the exemplars item 11 pastes into its prompts: every one must pass the validator and fire in the dry run.

Out: LLM authoring (item 11), the page rendering (item 13; expose everything through `briefing.scenarios` and `answer_scenario`), hooks' conversion into scenarios (later; both coexist).

## Design notes

- Reuse `card_match` for the state side of predicates if it fits; otherwise the new evaluator must be the only predicate language for scenarios (one, not two, so item 11's lint has one grammar).
- Performance: selection runs once per waking (30 to 50 per run) over a bank of about 720; binding act patterns against a log of a few hundred acts is fine without indexes. Keep an index by act type anyway.
- Determinism: rng for ties and the weight draw is seeded from (seed, waking index, bank hash). Selection reads, never writes; `shown` is written after the briefing is built.

## Acceptance

- All 30 seed scenarios pass `scenario_check.py`; the dry run shows each at least 3 times over 1800 runs and none above the hog threshold.
- Under `played` with the seed bank: `cited_player_act_share ≥ 60 %` of shown scenarios; consequence slot filled on ≥ 40 % of wakings after the first act.
- `answer_scenario` round-trips: the chosen act appears in the log with `answers = scenario id`, the hook or scenario's outcomes applied.
- Goldens unchanged with an empty bank; a bank adds `scenario_shown` events, so the golden suite runs with the bank off.

## Proposed tests (confirm with the owner first)

1. `scenario_select_determinism`: same world, log and bank twice → identical shown list; a different bank hash → may differ.
2. `scenario_cites`: a `burn(library)` two wakings ago makes `burned_library_scribes` eligible and its `cites` contains that act id; after `burn` on that court's remaining memory (tier below written) it is not eligible.
3. Golden `scenarios_seed_v1.hash` (seed 42, played, seed bank on).

## Report

Standard report, plus the dry-run summary table for the seed bank, the vocabulary usage table, the list of seed changes, and the three rules in the spec that proved hardest to implement as written (with what you did instead).
