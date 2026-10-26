# 19 · Text renderer in the core, language-aware schema, German for the seeds

Model: Opus 5.5 (renderer, schema, lint) · Sonnet 5.5 for the German pass script · Effort: high · Depends on: 17 (goldens, CI) · Must merge before item 11 generates round 1 of the bank · Branch: `item/19-text-renderer-l10n`

## Goal

All player-facing text is rendered by the C++ core from per-language files, so the wasm and Unreal shells show the same finished strings and German needs no change to any scenario. The token grammar is frozen before the bank exists, because 720 scenarios at about 8 strings each are about 5,900 strings per language, and every one written against today's `{x.pronoun}` would need rework.

## Read first

- `docs/research-localization.md` entirely; §2 "template feature set" and "the first five test strings", §3 "file layout", §4 "schema change", the lint and the review process are the spec.
- `docs/scenarios/localization-seed-de-draft.json` (163 seed strings, English and an unreviewed German draft in the proposed notation).
- `docs/scenario-spec.md`, `docs/scenario.schema.json`, `docs/scenarios/*.json`, `scenario_check.py`; `docs/patterns/` (pattern text follows the same split, keys `pat.<id>.<field>`).
- `data/text/en.json` and every place the sim builds a string that reaches the briefing (grep for string literals in `sim/` that reach `briefing`, `lines`, `chronicle`).

## Scope

1. **Renderer** `sim/text/` (one job per file: `lexicon`, `names`, `tokens`, `render`, `bundle`): token form `{role.field:mod.mod}` with fields `name`, `noun`, `subj`, `obj`, `poss`, `refl`, numeric fields; modifiers determiner (`def`, `indef`, `poss2`), case (`nom`, `acc`, `dat`, `gen`), noun-key override, unit (`year`); selects `{role.gender?m=…|f=…|n=…}`; plural `{n:plural(one=…,other=…)}`; whole cause clauses per language from `narrator.json`. Roles declare a kind; person roles a closed `noun` key and an optional `sex`. Names: per-language `names.json` with the genitive rule and overrides; syllable tables stay language-neutral. No U+2068/2069 isolation marks. Number formatting from build-time CLDR data (German "1.200", English "1,200"). Dev builds show a failed lookup as its key; release builds fall back to English rendered with English rules.
2. **Files** per the research §3: `data/text/<lang>/{ui,lexicon,names,narrator,scenarios,scenarios.meta,patterns}.json`, bundled by the build into `text.<lang>.json` and hashed into `data_hash`. The browser shells pass the bundle to the wasm core; the API takes a `lang` and returns finished strings. Every existing string in `data/text/en.json` and every sim literal that reaches the player moves into these files.
3. **Schema change** (`scenario.schema.json`, `pattern.schema.json`, linters): scenario files keep logic plus the English authoring text with `text.lang: "en"`; required `id` on every variant; `noun` and `sex` on person roles; `subj/obj/poss/refl` replace `pronoun`; the key scheme `scn.<id>.<field>` (`scn.<id>.variant.<vid>.body`, `scn.<id>.result.<option>`); provenance per key (`src` hash, `by`, `rev`) in `scenarios.meta.json`; length caps per language outside the schema (German body 910, cause line 286, label and headline 120; English 700, 220, 80, 80).
4. **Fix the 30 seed scenarios**: the four English defects the research lists (bare person nouns where a role is bound, caseless pronoun, `{a.years_ago} years` at 1, the hard-coded queen in `princess_returns`) and `noun` keys on every person role. `scenario_check.py` gains the lint: no bare person noun where a person role is bound, identical role-token sets in every language, no leftover English in a non-English file, caps, lexicon-only terms.
5. **German for the seeds**: import the draft into `data/text/de/`, render every key under forced states (each person role m and f, each item gender m, f, n) and lint the renders; write the German pass script (`tools/bank/translate.py`, Sonnet) for item 11 to call per batch: translate with adaptation, keep every role token, render-and-repair once. Manu reads all 30 rendered in both sexes and his corrections become the lexicon, a one-page style sheet (`docs/style-de.md`: du for the player, Präsens in the briefing, Präteritum in the chronicle, „…“ quotes, a term base of about 100 words) and eight gold examples for the pass.
6. **Pseudo-locale**: a `pseudo` language generated at build (accented letters, 35 % expansion, brackets). `web/l10n-check.cjs` loads each browser screen with `?lang=pseudo` and fails when an element's `scrollWidth` exceeds its `clientWidth` or a bracket is missing.
7. **Update `prompts/11-scenario-authoring.md` and `11-authoring-system.md`** to the new token grammar and the `noun` rule, with two of the fixed seeds as examples.

Out: UE `FText` and the Localization Dashboard (shell chrome may read the bundle through `FText::FromString` later), languages beyond English and German, Crowdin or Weblate, CJK fonts.

## Design notes

- Language logic is data plus a small code hook (RimWorld's worker pattern); German needs only lookups, selects, capitalisation and number format. No German grammar engine.
- The sim's state and hashes never depend on language: render after the sim, from records. The chronicle text golden stays English; add one German text golden for the three text seeds.
- Upper-casing generated names is banned in the renderer (Weiß would become WEISS); shells must not upper-case either.
- Fonts: the shells' UI font must cover ä ö ü ß ẞ „ “ ‚ ‘; render `Weiß ẞ „Äpfel“ »Öl«` in the check.

## Acceptance

- The five test strings of the research §2 render to the expected German natively and in wasm with equal hashes.
- All 30 seeds lint clean in English and German; 0 mechanical failures (stray braces, double spaces, missing tokens) across the forced-state renders.
- No player-facing string literal left in `sim/` (a grep gate in `check_banned.sh`).
- `l10n-check.cjs` passes on every browser screen with `pseudo` and `de`.
- Goldens: the English chronicle text hash may change only through the moved strings, in a `golden:` commit with the diff.

## Proposed tests (confirm with the owner first)

`text_render_de` (the five strings), `text_lint_seeds` (the lint on all seeds), `text_no_literals` (the grep gate). Labelled `fast`.

## Report

Standard report, plus: the list of moved literals, the forced-state render counts, the lint findings and fixes, the hours Manu spent reading the German seeds and the defects found per scenario (this calibrates the sample size for the bank: 60 scenarios with zero defects bounds the rate under 5 %).
