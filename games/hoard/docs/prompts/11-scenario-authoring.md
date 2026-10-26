# 11 · Scenario authoring pipeline (LLM, offline)

Model: Sonnet 5.5 for the script and the lint glue · Effort: medium · The generation calls themselves: Opus 5.5 (or Fable 5.1) · Depends on: 10 · Branch: `item/11-scenario-authoring`

## Goal

Fill the bank to the v1 targets (about 720 validated scenarios from 2,500 to 3,000 drafts) with an offline pipeline: a prompt per grid cell with closed enums and exemplars, a generation script against the Claude API, dedupe, the validator and dry run from item 10, and a per-cell report that steers the next round. The sim never calls a model; the bank is data.

## Read first

- `docs/scenario-spec.md` sections 2, 7, 8, 9 and `docs/scenario.schema.json`.
- `docs/research-storylets.md` section 7 (what fails in LLM-generated storylets: naming one thing two ways; the numbers that say expect a quarter to a third of drafts to fail mechanical checks).
- `docs/research-book-procedural-storytelling.md` ch 1 and ch 21 "For Hoard" (oatmeal: variety counted in distinct sim acts, not names; traits extreme and active; reactions over actions).
- `docs/story-and-endings.md` (tone and the endings scenarios may push), `docs/event-catalogue.md` (the house style for chronicle lines: short, concrete, one image), the 20 seed scenarios from item 10.
- `tools/scenario_check.py`, `tools/scenario_dryrun.py`, their JSON summaries.
- Anthropic SDK docs for the Messages API and the Batches API (use the `product-self-knowledge` skill or docs.claude.com; do not rely on memory for SDK details).

## Scope

In:

1. **Grid** `data/scenarios/grid.json`: cells = era × kind × (act type for consequences | arrival kind for audiences | theme for world, lair, omen) × race (or any). Each cell has a target count from spec section 7 and a list of exemplar ids from the seed bank. The dry-run summary's per-cell depth and dead/hog lists adjust targets between rounds.
2. **Prompt template** `tools/authoring/system.md` and `tools/authoring/cell.md`. A draft of the system prompt with `{{...}}` slots for the live schema, act API and vocab is in `docs/prompts/11-authoring-system.md`; start from it. The system prompt carries: the game in six sentences (from `prompts/00-preamble.md`), the house style (plain, concrete, short; no modern idiom; no real-world ethnic coding of any race; no fact the sim cannot know; one or two causes per because-line; traits shown through what people do), the full schema with field rules, the closed enums pasted verbatim from `vocab.json`, `arrivals.json` gate names and `act-api.schema.json` act types and payload keys, the predicate grammar, the token list, and the avoid list. The cell prompt carries: the cell's era, kind, act or arrival, race, two or three exemplars in full JSON, the dry-run notes for that cell (what was dead, what hogged), and the ask: 8 to 12 scenarios as a JSON array, each distinct in its causal template (not in names), with `provenance.gen = "llm"` and the cell id. Numbers (`weight`, `cooldown`, `salience.w`) are forbidden in the output; the script fills defaults by kind.
3. **Generation script** `tools/authoring/generate.py`: reads the grid, builds prompts, calls the API (Messages for small rounds, Batches for a full round; model from a flag, default the Opus-class model; `ANTHROPIC_API_KEY` from the environment), parses the JSON (tolerant of a fenced block), writes drafts to `data/scenarios/drafts/<cell>/<round>.json` with the raw response saved beside it, logs token use per cell. A `--dry` flag prints the prompts without calling. Retries once on a parse failure with the error appended to the prompt.
4. **Post-processing** `tools/authoring/sift.py`: fills numeric defaults; normalises ids (slug from headline plus cell); runs `scenario_check.py` steps 1 to 4 on the drafts; dedupes within a cell on (act roles' types, option act types, theme tags) first and on body-text n-gram overlap (≥ 0.5 Jaccard on 3-grams) second, keeping the one with more satisfied lint checks; moves survivors to `data/scenarios/bank/<cell>.json` with `reviewed: false`.
5. **Round report** `reports/11-round-<n>.md`: per cell drafts, passed lint, passed forced-state, deduped, kept; failure categories (unknown enum, unbound token, missing ignore, gate on every option, body too long, predicate parse error); the top five failure messages verbatim so the prompt can be fixed; the dry-run deltas after the round (bank metrics, dead and hog lists, cited-player-act share).
6. **Human read queue** `tools/authoring/review.py`: samples 10 % per cell plus every `must` and omen into `reports/11-review-<n>.md` as readable cards (headline, body, options, the because-line) with a checkbox each; a `--apply` pass sets `reviewed: true` for the checked ones and deletes the unchecked.
7. **Round plan**: round 1 is one cell per kind (5 cells, about 50 drafts) to debug the prompts; round 2 the full grid; round 3 the cells the dry run flags thin. Stop when the per-cell depth target holds and the cited-player-act share is ≥ 60 %.

Out: runtime use of any model; editing the sim; writing scenarios by hand beyond fixing an exemplar.

## Design notes

- Write the avoid list as positive instructions where possible ("name the court by its slot token", "give the envoy one trait shown in what he does"), and keep the negative ones short: no modern idiom, no real-world ethnic coding, no claims about the dragon's thoughts, no option that only differs in tone from another.
- Paste enums, never describe them. STORY2GAME's main failure mode was synonyms; the lint will catch them, but the prompt should prevent them.
- Keep the system prompt under about 4,000 words so cell prompts stay cheap to batch; the exemplars carry most of the style.
- Cost note for the report: tokens per kept scenario per round, so the owner can see the trend.

## Acceptance

- Round 1 (5 cells): ≥ 60 % of drafts pass lint and forced-state after at most two prompt revisions; the failures are categorised.
- Round 2: the bank reaches ≥ 70 % of every cell's target with `reviewed: false` scenarios that pass the dry run's dead/hog filters; cited-player-act share ≥ 60 % under `played`.
- Every kept scenario validates against both schemas and renders with no unbound token in the lab world.
- `generate.py --dry` reproduces the prompts byte-for-byte from the grid (so prompt changes are reviewable in git).

## Proposed tests (confirm with the owner first)

None in `ctest`. `sift.py --selftest` runs the pipeline on the 20 seed scenarios and asserts all 20 pass and none dedupe against each other.

## Report

Standard report, plus the round reports, the final system prompt (in full, as a file reference), and the three prompt changes that moved the pass rate most.
