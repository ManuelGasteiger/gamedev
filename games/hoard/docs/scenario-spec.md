# Hoard: scenario specification (v1)

Status 2026-10-23. One data format for everything the briefing offers the player that is not a bare hook: consequences of the dragon's own acts, audiences with visitors, world events that ask for an answer, lair events, omens. It merges the two proposals in `research-storylets.md` (section 8) and `research-book-procedural-storytelling.md` (section 3.1), written against `act-api.md` and `act-api.schema.json`. The machine-readable form is `scenario.schema.json`. Hooks (`core-loop.md` section 5) stay as they are; a hook can be re-expressed as a scenario later, and `ignore` already covers both.

Decisions this spec assumes: closed act vocabulary (`act-api.md`), 30 to 50 wakings per run, deterministic runtime (the LLM authors offline, the sim only matches and fills).

## 1. What a scenario is

A scenario is a unit of content with three parts:

1. **When it can fire**: typed role binders over the act log and the world state, plus predicates.
2. **What it shows**: a headline, a body, an optional "because" line (printed only when the chronicle level allows), text variants.
3. **What the player can do**: two to four options, each bound to exactly one act from the act API, plus `ignore`.

Everything the scenario does to the world happens through the act the player picks. A scenario may add `outcomes` (a memory line on a person, a suppressed theme, a `seek` for follow-ups), never a direct state write. That keeps the sim's rule 2 (two inputs only: the act log and the hoard) intact.

## 2. Schema

```json
{
  "id": "burned_library_scribes",
  "kind": "consequence",
  "stakes": "minor",
  "eras": ["iron", "castles", "gunpowder"],
  "weight": 100,
  "once": "run",
  "cooldown": null,
  "exclusive_group": null,
  "arrival": { "who": "envoy", "from": "$court", "needs": ["court.knows_lair", "court.dares"] },
  "roles": {
    "a":     { "act": { "type": "burn", "structure": "library", "magnitude": [0.2, 1.0], "age_wakings": [0, 2] } },
    "court": { "faction": { "is": "owner_of($a.target)", "remembers": "$a", "era_min": 2 } },
    "sage":  { "person": { "court": "$court", "tag": "sage", "choose": "by_score:renown" }, "optional": true },
    "envoy": { "person": { "court": "$court", "role": "envoy", "choose": "by_score:cunning" } }
  },
  "require": ["court.memory_tier >= written"],
  "forbid":  [ { "act": { "type": ["give", "teach"], "target": "$court", "after": "$a" } } ],
  "salience": [ { "if": "court.dread >= 0.3", "w": 3 }, { "if": "bound(sage)", "w": 1 } ],
  "tags": ["theme:memory", "theme:library", "faction:$court"],
  "text": {
    "headline": "The scribes of {court.name}",
    "body": "{envoy.name} stands at the ledge with a satchel of ash. {a.years_ago} years after you burned the library at {a.place}, {court.name} has no record of its own kings. The envoy asks for what you took, or for someone to teach them letters again.",
    "because": "The court lost its written memory at {a.place}; what it knows of you is now told, not read.",
    "variants": [ { "if": "bound(sage)", "body": "{sage.name}, the last sage of {court.name}, has sent {envoy.name} ..." } ]
  },
  "options": [
    { "id": "books",  "label": "Give back the books", "act": { "type": "give",  "target": "$court", "payload": { "books": "some" } } },
    { "id": "teach",  "label": "Teach them letters",  "act": { "type": "teach", "target": "$court", "payload": { "subject": "letters" } }, "gate": "lore >= 3" },
    { "id": "eat",    "label": "Eat the envoy",       "act": { "type": "kill",  "target": "$envoy", "magnitude": 0.05 } },
    { "id": "ignore", "label": "Send them away",      "act": { "type": "ignore", "target": "$self" } }
  ],
  "outcomes": {
    "ignore": [ { "suppress": { "tag": "theme:library", "years": 100 } } ],
    "teach":  [ { "history": { "person": "$envoy", "line": "taught_by_dragon", "severity": 2 } } ]
  },
  "followup": { "seek": ["theme:memory"], "after_wakings": 1 },
  "provenance": { "gen": "llm", "model": "", "cell": "iron/consequence/burn", "bank": "v1", "reviewed": false }
}
```

### 2.1 Fields

| field | rule |
|---|---|
| `id` | snake_case, unique in the bank |
| `kind` | `consequence` (an act role is required), `audience` (an `arrival` is required), `world`, `lair`, `omen` |
| `stakes` | `minor`, `major`, `must`. Scores compete only inside a tier. `must` needs both a `require` floor and a ceiling (Failbetter's min/max guard) and is reserved for telegraphed danger and ending thresholds; at most one per waking |
| `eras` | subset of bronze, iron, castles, gunpowder, arcane, judged on the strongest court; omit for any |
| `weight` | draw weight inside a 3-point score window: 1, 10, 20, 50, 100, 200, 500 (Fallen London's frequency classes); default 100. Rare scenarios may carry harsher or kinder options |
| `once` | `run` (default for anything with an act role or a named person), `world` (once per seed, even across saves), `never` (repeatable; then `cooldown` is required and at least 3 text variants) |
| `cooldown` | `{ "wakings": n, "years": y }`, both must pass |
| `exclusive_group` | answering one scenario closes its siblings for the run (the Mark of Cain) |
| `arrival` | `who` ∈ envoy, pilgrim, thief, refugee, hero, prophet, kin, scholar; `from` a role; `needs` a list of gate names from the access table (`research-social-sim.md` section 6). Nobody arrives without a reason and a route |
| `roles` | typed binders, section 3 |
| `require`, `forbid` | predicates (section 4) and negated act patterns; at least two distinct state elements read (Failbetter's two-quality minimum) |
| `salience` | `{if, w}` pairs; `w` ∈ 1, 3, 9 |
| `tags` | closed vocabulary: `theme:*` (about 30 themes, listed in `data/scenarios/vocab.json`), `faction:$role`, `race:*`, `ending:*` (pushes an ending), `tone:*` |
| `text` | headline ≤ 8 words, body ≤ 100 words, because ≤ 30 words, labels ≤ 12 words, result lines ≤ 40 words; tokens `{role.field}` only |
| `options` | 2 to 4 plus `ignore`; each binds one act template (section 5); at least one non-ignore option without a `gate` |
| `outcomes` | per option id: `suppress`, `history`, `seek`, `reveal_to_player` (a fact for the chronicle); never a state delta |
| `followup` | `seek` tags for next waking's selection, so no scenario needs another's id |
| `provenance` | who or what wrote it, which grid cell, bank version, human review flag |

## 3. Role binders

Each role has exactly one binder type. Binders resolve to ids; ties use the run rng seeded from (seed, waking index, scenario id).

| binder | fields | notes |
|---|---|---|
| `act` | `type` (one or a list), `structure`, `payload_has`, `magnitude` [min, max], `age_wakings` [min, max] or `age_years`, `source`, `answers` (scenario or hook id), `after` / `before` (another act role), `target` (a role) | a pattern over the act log, Felt-style; `forbid` uses the same shape negated |
| `faction` | `is` (expression: `owner_of($a.target)`, `neighbour_of($court)`, `strongest_army(neighbours($court))`, `worst_relations(dragon)`, `has_tag(slayers)`), `remembers` (an act role; the court must hold a memory of it at a tier that has not faded), `era_min`, `era_max`, `race`, `dread` [min, max], `awe` [min, max], `piety`, `openness`, `cohesion` [min, max], `at_war`, `has_tag[]`, `has_slayers`, `slayer_stage`, `has_temple`, `has_library`, `knows_prophecy`, `memory_tier_min`, `choose` | `remembers` is what makes forgetting filter scenarios without an extra rule |
| `region` | `owner` (a role or `none`), `terrain[]`, `tags[]`, `burn` [min, max], `has_structure`, `adjacent_to`, `choose` | |
| `person` | `court` (a role or `lair`), `role` (ruler, heir, hero, sage, priest, envoy, hostage, consort, historian, champion, ward), `tag`, `trait`, `alive` (default true), `knows_dragon`, `memory_has` (a line id), `is_lie`, `renown`, `cunning` [min, max], `age` [min, max], `choose` | the dragon's hostages and kept people are persons with `court = lair` |
| `item` | `in` (hoard, court, person), `agenda`, `kind` | artifacts with an agenda carry `commune` scenarios |
| `remnant` | `race`, `strength` [min, max] | |

`choose` values: `most_recent` (default for acts), `by_score:<field>` (renown, cunning, piety, wealth, dread, awe), `random`. `optional: true` lets the scenario fire without the role and swaps in a variant.

## 4. Predicate grammar

Predicates are strings so an LLM can write them and a lint step can parse them. Grammar:

```
pred     := clause (" and " clause)*
clause   := path op value | "bound(" role ")" | "has_tag(" role "," tag ")" | "not " clause
path     := role "." field ("." field)*
op       := ">=" | "<=" | ">" | "<" | "==" | "!="
value    := number | identifier | string
```

Allowed paths are listed in `data/scenarios/vocab.json` (court.dread, court.awe, court.memory_tier, court.era, court.cohesion, court.faith_share, court.at_war, dragon.might, dragon.lore, dragon.arcana, dragon.dominion, dragon.flame, dragon.phase, dragon.wounds_share, lair.has_historian, world.magic, world.year, world.wakings …). A predicate naming a path outside the list fails lint. `or` is deliberately absent: write two scenarios or two variants.

## 5. Option act templates

An option's `act` is an act from `act-api.schema.json` with role references in place of ids: `"target": "$court"`, `"secondary": "$attacker"`, `"payload": { "who": "$heir" }`. `$self` in `ignore` means this scenario. A `burn` aimed at a structure writes `"structure": "walls"` (an enum, not a role); the runtime maps it to `secondary: {kind: structure, id}`. `magnitude` may be a number or `"$slider"` (the shell asks). After binding, the runtime substitutes ids and validates the resolved act against the act API schema; an option whose act fails validation is dropped before the scenario is shown, and the dry run reports it.

`gate` is a predicate on the dragon (`lore >= 3`, `talent == white_fire`); gated options render greyed with the gate as the reason. Costs are the act's own (stamina, pools); the option carries no cost of its own.

## 6. Selection per waking

Pure function of (world state, act log, bank, seen counts frozen at run start, rng). Writes happen after presentation.

1. **Eligible**: eras match, every non-optional role binds, `require` holds, `forbid` matches nothing, `once` and `cooldown` clear, `exclusive_group` open, `arrival.needs` all satisfied.
2. **Bind** roles with their `choose` rule.
3. **Score** `S = 2·specificity + Σ salience w + 9·Σ_cited (m · 0.5^(age_wakings/2)) − 9·0.5^(wakings_since_shown/4) − 3·theme_hits_last_3_wakings − 3·min(9, seen_in_earlier_runs) + pity`. Specificity = number of roles + `require` + `forbid` entries.
4. **Slots**: one consequence, one audience (only if an arrival gate is open), one open slot (world, lair, omen, or a second consequence), plus at most one `must`. A waking with no eligible candidate for a slot leaves it empty; the briefing still leads with a hook or an event of medium importance (`core-loop.md`).
5. **Pick** per slot: highest stakes tier first; inside it, candidates within 3 points of the best, drawn by `weight`.
6. **Quotas**: at most one scenario per `faction:` tag and per `theme:` tag per waking; a theme above 15 % of the run's shown scenarios takes −9.
7. **Pity**: a witnessed act of magnitude ≥ 0.2 in the last 2 wakings with no consequence shown for 2 wakings forces the best eligible consequence. An arrival gate open for 4 wakings without an audience gains +3 per further waking. World scenarios get no pity.
8. **Log** `shown {scenario, waking, bound roles, cites: [act ids]}`. The chronicle draws "because" lines from `cites`; the interest tool counts them for causal depth and for the share of scenarios citing a player act (target ≥ 60 %).

Citation budget: an act may be cited by at most 2 scenarios per run and 1 per waking; acts held at monumental tier may be cited 4 times.

## 7. Bank targets (v1)

Assumes 30 to 50 wakings per run, about 1.5 scenarios shown per waking (45 to 75 per run), and a quarter of the eligible bank seen per run (Six Ages' ratio).

| era | consequence | audience | world | lair | omen | total |
|---|---|---|---|---|---|---|
| bronze | 50 | 30 | 20 | 10 | 5 | 115 |
| iron | 55 | 30 | 20 | 12 | 5 | 122 |
| castles | 60 | 35 | 20 | 12 | 6 | 133 |
| gunpowder | 55 | 30 | 20 | 12 | 6 | 123 |
| arcane | 45 | 25 | 15 | 10 | 8 | 103 |
| any | 60 | 30 | 10 | 15 | 10 | 125 |
| total | 325 | 180 | 105 | 71 | 40 | 721 |

Audiences by arrival: envoy 50, pilgrim 30, hero or slayer 30, thief 25, refugee 25, prophet 20. Consequences by act: burn, take, kill, take_hostage, demand at least 25 each; every other witnessed act type at least 5. Depth target from the dry run: when the dragon acted last waking, the consequence slot has a median of 6 or more eligible candidates and a 5th percentile of 2 or more. Drafts needed: 2,500 to 3,000 (expect a quarter to a third to fail mechanical checks, after STORY2GAME's rates).

## 8. Validation pipeline

1. **Schema**: `scenario.schema.json`; option acts checked against `act-api.schema.json` after a dummy binding.
2. **Lint**: closed vocabularies for tags, paths, roles, tokens with a usage report (Six Ages' compiler); every token bound; option targets are roles of the right kind; `ignore` and a gate-free option present; text caps; no predicate both required and forbidden; `must` has floor and ceiling; `once: never` has cooldown and 3 variants.
3. **Satisfiability**: interval check of predicates against the sim's value ranges; each act role's type is emittable in that era by some bot policy.
4. **Forced-state test** (Six Ages' bulk tester): build a minimal world that satisfies the roles, play every option, assert the sim accepts the act and changes state, the text renders with no unbound token, no referenced person is dead or absent.
5. **Dry run**: 300 seeds × 6 bot policies (burner, sleeper-hoarder, protector, priest, schemer using guise and corrupt, random). Record per scenario: eligible rate, shown rate, waking positions, cites.
6. **Cut and flag**: dead if shown in fewer than 3 of 1,800 runs; hog if shown in over 50 % of runs or over 2 % of all showings; too loose if eligible in over 60 % of its era's wakings; too tight if under 1 %. Dead and hog are cut or rewritten; loose and tight go back to the generator with the metric attached.
7. **Bank metrics**: per-cell depth (section 7), consequence share ≥ 60 %, no repeatable shown over 3 times per run, theme share ≤ 15 %, tags never matched, interest-tool deltas with the bank on and off (causal depth, agency).
8. **Near duplicates**: n-gram and embedding similarity on body text within each cell; keep the stronger of any pair over threshold.
9. **Human read**: 10 % per cell plus every `must` and every omen, for tone and wrong-person errors the tester cannot see.
10. **Freeze**: hash the bank; the bank version is part of the data that defines a run.

## 9. Authoring (offline, LLM)

Generate by grid cell (era × kind × act type or arrival × race) with two or three hand-written exemplars per cell, closed enums for every role, tag, path and act name pasted into the prompt, and a list of things to avoid (real-world ethnic coding in culture text, modern idiom, naming a fact the sim cannot know, more than one cause per "because"). Numbers stay out of the model's hands: `weight`, `cooldown`, `salience.w` use defaults by kind, the model chooses from enums. Ask for 8 to 12 scenarios per call, deduplicate inside the cell, then run steps 1 to 4 before any human reads one. The prompt template and the generation script are `prompts/11-scenario-authoring.md`.

## 10. The seed bank (2026-10-24)

Thirty hand-written scenarios live in `scenarios/seed-01-10.json`, `seed-11-20.json` and `seed-21-30.json`, with the closed vocabulary in `scenarios/vocab.json` and the first version of the validator (steps 1 and 2) in `scenarios/scenario_check.py`. All thirty pass. They cover 30 of the 40 acts as options, 28 of 34 themes, 14 consequences (one each for burn, burn of a library, take, kill, defend, demand, teach, judge, show, take_hostage, release, raise_undead, guise, and the hero challenge), 7 audiences, 5 world, 3 lair, 1 omen, and one `must` (the door in the temple). Writing them changed the spec in five places (the fifth: a `structure` field on burn templates):

- The binder schema was missing fields the scenarios needed at once (`piety`, `openness`, `knows_prophecy`, `has_slayers`, `has_structure`, `is_lie`, `owner: none`); they are in now. Expect the LLM rounds to ask for more; add to the schema and the vocab, never to free text.
- "At least one gate-free option besides `ignore`" bites on scenarios whose every answer is a late-game act (the Mountain, the false dragon). The fix was always a plain act (`burn`, `sleep`, `give`), which is also the better design: the player should never be shown a card with nothing they can do.
- A consequence must cite a player act. The theft of the cup is a card's doing, so it is a `world` scenario; what makes it feel like a consequence is `remembers` and the court's `robbed_the_dragon` tag, not the kind.
- Repeatables need three texts; the third variant tends to be the best one (the slayer who smiles at the hurt wing).

The seed bank is deliberately broad to serve as exemplars across kinds; its consequence share (47 %) is below the generated bank's target of 60 % (section 7), which item 11's grid enforces. Every consequence cell for the big five acts now has at least one exemplar; `crush`, `fight`, `broker`, `reveal`, `curse`, `corrupt` on a faction, `incite`, `bless`, `ward`, `miracle` and `consecrate` still have none and need one each before their cells are generated.

## 11. Example: a consequence that reads as memory

Precondition: `take_hostage` on a dwarf heir two wakings ago, the heir still in the lair, the dwarf court at written memory tier. Roles: `a` (the act), `court` (owner, remembers `a`), `heir` (person, court = lair, tag hostage, from `$court`), `envoy` (person, court, role envoy, by_score:cunning). Options: `release($heir)` with terms, `demand($court, tribute)`, `keep($heir, ward)`, `kill($envoy)`, `ignore`. Because-line: "Kell Hold has written the taking of {heir.name} into the Book; the dwarves do not forget what is written." Tags: theme:hostage, theme:grudge, faction:$court, race:dwarf. Follow-up: `seek: ["theme:grudge"]`.
