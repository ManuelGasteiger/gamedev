# 09 · New acts of the v1.1 API

Model: Opus 5.5 (or Fable 5.1) · Effort: high · Depends on: 02, 05, 07 (and 08 for `fight`) · Branch: `item/09-new-acts` (split into `item/09a-...` and `item/09b-...` if useful)

## Goal

Make every act in `act-api.schema.json` real in the sim and the wasm API: accepted when its gate holds, refused with a reason when not, logged with `source` and `answers`, reacted to by the dread/awe rows, narrated. The acts missing today: `fight`, `keep`, `dismiss`, `guise`, `broker`, `reveal`, `judge`, `haunt`, `commune`, `consecrate`, `receive`, `hunt`, `ignore`, `exit`, the `structure` argument on `burn`, person targets on `teach` and `corrupt`, `item` payload on `take`, `consent` on `take_hostage`, `pilgrimage`/`disband`/`tear_down`/`no_harm_to` clauses on `demand`, and `pick_talent`/`cast_before_sleep`/`sleep` as logged acts.

## Notes for this tree (2026-11-07)

- Rules version is 10 (`sim/ages/record.h`). Items 08 and 09 run in parallel; in your `golden:` commit set `kRulesVersion` to 11. The goldens are regenerated once more when the two are merged. Golden recipe: `reports/14-story-sifting.md`, plus the replays and the save check (see `reports/06b-rare-visitors.md`).
- Start with an inventory. `acts.cpp` (323 lines) has `burn`, `kill`, `take`, `defend`, `show`, `give`, `take_hostage`, `release`, `accept_offering`, `miracle`, `spell`, `wound`, `receive`. Several schema acts already exist as API verbs outside the act log:
  - `receive` and `hunt` (item 06, `arrival_acts.cpp`);
  - `keep_person` and `commune` (item 05, `chron_keep.cpp`, `chron_voice.cpp`);
  - `exit` (item 04, `exit.cpp`);
  - `demand` (`ages.cpp`);
  - the six moves, `teach`, `corrupt`, `bargain`, `threaten`, `court` and `release` (item 07, `moves.cpp`, `data/moves.json`);
  - `cast` (`spells.cpp`), `answer_hook` (`hooks.cpp`, `hook_resolve.cpp`), `accept_offer` and `decline_offer`, `pick_talent`, `sleep`.

  Map each to its schema act before writing new code; reuse what exists. If item 05's `keep_person` and the schema's `keep` should merge, keep the old verb working for saves and replays, and say how.
- Every new state-changing verb needs a wrapper in `sim/ages/recorded.cpp` and a case in `apply_input`, or replays break. The recorded verb names today are in `recorded.cpp`.
- `fight`: item 08 provides `resolve_fight` on branch `item/08-slayer-roster`, which runs at the same time. Do `fight` last. If that branch has pushed `resolve_fight` by then, merge it into yours and wire the act. If not, refuse `fight` with a reason code and list it as open.
- `docs/world-aging.md` is now in the repo (scope 3).
- New rolls draw from `rng_for(seed, tag, id)` (`sim/ages/rng.h`), never `world.rng`, so no existing draw moves. This replaces the design note below.
- Player text goes in `data/text/<lang>/*.json` with an `en` and a `de` line for every key, never in `en.json` (scope 6 predates the split). Refusal reasons are codes (`code|key=value`, `ages.h` `reason_code`).
- People are hidden until met (`person_seen.cpp`, item 04b). Acts that meet or learn of a person (guise, haunt, reveal, keep) must add a route there.
- Policies: add `data/policies/schemer.json` and `priest.json`; leave `played.json` unchanged.
- `tools/act_check.py` needs `jsonschema` (`pip install --break-system-packages jsonschema`).
- Tests: add none. List the proposed tests in the report; Manu decides.
- Before you finish: `ctest --test-dir build -L fast`; the wasm build (`git lfs pull --include "games/hoard/assets/gltf/**"` first) and `node web/smoke.mjs --golden 1-60`; `node web/l10n-check.cjs --lang pseudo,de`; `node web/game-check.cjs`; `node web/check.cjs`. Push the branch; no pull request.

## Read first

- `docs/act-api.md` entirely, `docs/act-api.schema.json`.
- `docs/research-social-sim.md` section 2 "the six moves" and the contract in `prompts/07-people-moves.md` (`resolve_move`, `read_release`), `prompts/08-slayer-roster.md` (`resolve_fight`), `prompts/05-chronicle.md` (`keep` historian, `commune`), `prompts/06-access-gating.md` (`receive`, `hunt`), `prompts/04-endings-prophecy.md` (`exit`).
- `docs/ages-sim.md` 3 (Act), 5 (applying the act log), 8 (verbs), the API notes in `docs/sandbox-game.md`.
- `acts.cpp`, `api.cpp`, `data/reactions.json`, `data/text/en/`.

## Scope

In:

1. **Act record** extended to the schema: `id`, `source`, `answers`, `secondary`, typed `payload`. A JSON validator in C++ is not required; the API rejects an act whose fields do not match the type's requirements with `{refused: true, reason}`. `tools/act_check.py` validates any act log against `act-api.schema.json` (Python, jsonschema) and is run by the tests on the policy logs.
2. **Acts**, one case each in `acts.cpp` (or one file per family under `sim/ages/acts/` if `acts.cpp` passes about 600 lines): each with gate check, immediate deltas, memory entries, reaction rows, chronicle event with cause, narrated `lines`.
   - `burn` with `structure`: library (written stocks ×0.1 for the owner, era_rate ×0.9 on neighbours for 10 decades, sources destroyed), temple (faith −0.3, cult tags removed, pious neighbours crusade card eligible), foundry (era progress −20, anti-dragon −0.1), walls (`walls` tag removed), altar (as `destroy_altar`), granary (wealth −10 %, plague chance ×1.5 for 2 decades), tower (one defence removed for the age).
   - `fight` → `resolve_fight` (item 08); `judge` → kill-with-framing on tagged persons, else `kill`.
   - `take` with `item` (named artifact moves to the hoard; `loot:item` tag), `take_hostage` with `consent` (awe row).
   - `receive`, `hunt` (item 06 defines; this item lands the API cases if 06 did not).
   - `demand` new clauses: `pilgrimage` (the court sends pilgrims every age), `disband` (slayer order dissolves; its grandmaster flees to a neighbour with a `grudge`), `tear_down` (a structure removed), `no_harm_to` (a third court protected; breach when they attack it).
   - `teach` on a person → `resolve_move(Teach)`; on a faction as today plus `subject` modifiers (`craft` era progress, `war` militarism and anti-dragon, `magic` world magic and arcane, `intrigue` cunning on the ruler, `dragon_lore` memory tier up).
   - `broker` (pacts: marriage → the marriage card's merge path, peace → war ends `brokered_by_dragon`, alliance → affinity +0.3 and the alliance rule, trade → both wealth growth ×1.1 for 10 decades, vassalage → the smaller swears to the larger); both courts witness; awe row.
   - `reveal` (secrets: `hidden_region` removes the `hidden` tag, `plot` resolves the barons'-plot card early, `heir_location` ends a pretender card, `slayer_plan` marks the order's traits known and gives the court a `forewarned` tag, `lie` exposes an envoy's lie: `deceived` tag flips to the court). The betrayed party gets `betrayal` if sworn to the dragon.
   - `haunt` (person gains a memory entry `src: dream`, message-specific tags; a prophet may found a cult per item 12's seed rule, a king may abdicate at 0.1, a slayer may desert at `0.2 × (1 − bold)`); no witnesses.
   - `commune` (item 05 defines; land the API case).
   - `consecrate` (region with temple or altar: `dragon_temple` monument object from item 02; faith floor; monumental deposit).
   - `curse` kinds, `corrupt` on a person → `resolve_move(Corrupt)` plus the secret; on a faction as today with `toward`.
   - `keep`, `dismiss` (lair roles; `historian` from item 05; `consort` founds a line: a `dragon_blood` person born after one phase window, tag `half_dragon`; `priest` keeps faith in their court from fading; `champion` fights the next slayer band before the dragon does; `ward` is a hostage raised with consent).
   - `guise` (gate Arcana 15 and the Mortal Guise talent; one per age; `purpose` effects per the API table; discovery roll `0.1 + 0.05 × sages + 0.1 × (court has forewarned)`, discovered → `show` 0.5 plus `deceived`).
   - `ignore` (log; the hook's "if ignored" branch; `ignored` tag on the hook or scenario id for later preconditions).
   - `exit` (item 04 defines; land the case).
   - `sleep`, `pick_talent`, `cast_before_sleep` logged as acts with `source: sleep | lair` so scenarios and the chronicle can cite them.
3. **World aging fields.** Acts write what the shells read later (`docs/world-aging.md` §3): `burn` sets the region's `last_burn_year` and `destroyed_share_at_last_burn`; a settlement that falls to zero value gets the `ruin` tag with the year; abandonment and resettlement rules set `abandoned_year` and `resettled_year` (coordinate with the migration and settle rules); the settle rule sets `farmland` when a region is farmed from a neighbouring capital rather than settled with its own town.
4. **Witnesses** derived per section 4 of the API: owner, neighbours, courts with a sage or envoy present; secret acts write nothing until `reveal`, a card or discovery.
5. **Gates and refusals** in `data/gates.json`: per act the growth axis, points, talent, phase, pool cost; `api.cpp` returns the reason key for the page to grey the action.
6. **Narration**: a `lines` row per new act and sub-kind in `en.json`; the reaction rows in `data/reactions.json` from `act-api.md` section 6.2 for every act not yet covered by item 02.
7. **Policies**: extend the scripted policies with a `schemer` (guise, corrupt, reveal, broker) and a `priest` (consecrate, receive, bless, miracle) so the interest tool and item 10's dry run can exercise the new acts.

Out: the scenario runtime (item 10), the page (item 13), the real-time raid mapping.

## Design notes

- Keep the act vocabulary closed: if an effect needs a new act, stop and report. New payload keys are fine when `act-api.md` lists them.
- A refused act is not logged. An accepted act always produces at least one chronicle event (`dragon_<type>`) with the act id as cause ref.
- Determinism: all rolls (`guise` discovery, `haunt` outcomes, `fight`) through `world.rng`. (Superseded: use `rng_for`, see the notes above.)

## Acceptance

- `tools/act_check.py` validates the `played`, `schemer` and `priest` policy logs against the schema with zero errors; a malformed act in a test log is refused by the API with a reason.
- Every act in the schema appears at least once in a 300-seed run of the three policies combined (coverage table in the report).
- The 15 scenarios in `act-api.md` section 5 can each be expressed as a sequence of accepted acts in a lab world (`load_world`): run them as a smoke script `tools/act_scenarios.py` and list any that cannot.
- Goldens: the played policy log changes only if the policy file changes; keep the existing `played.json` as is and add the new policies as new files.

## Proposed tests (confirm with the owner first)

1. `act_gates`: `guise` without the talent is refused with `gate.talent`; with it, accepted and logged once per age.
2. `burn_structure_library`: the library burn applies the three documented effects (written ×0.1, neighbours' era_rate modifier, sources gone).
3. `secret_acts_no_witness`: `corrupt` on a person writes no court memory; a later `reveal` writes it.

## Report

Standard report, plus the act coverage table (act → count over 300 seeds × 3 policies) and the list of API reason keys.
