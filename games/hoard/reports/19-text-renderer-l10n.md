# 19 · Text renderer in the core, language-aware schema, German for the seeds: report

Branch `item/19-text-renderer-l10n`, on main after items 17 and 01. Scope as decided: the narrator moves to the new files now and converts to the new grammar later; the page's chrome only; tests: `text_lint_seeds` alone.

## 1. What changed

Renderer (`sim/text/`, one job per file):
- `tokens` (the grammar of the research §2: `{role.field:mod.mod}`, selects, plural), `lexicon` (nouns by sex and case with determiner classes, articles, pronouns, possessive endings, units), `names` (genitive rule with apostrophe list and overrides), `numbers` (CLDR grouping and plural rules), `render` (sentence-start capitals, never upper-cases a name; dev builds keep a failed token as written, release builds fall back to English), `bundle` (pseudo made from English), `batch`, `reason`, `bundle_io` (native only).
- `hoard-sim --render-batch FILE`, `--text-bundle LANG`; wasm `lang`, `ui`, `render`, `render_batch`; results with a `reason` gain `reason_text` in the session's language.

Data:
- `data/text/en.json` → `data/text/en/narrator.json`; importances → `data/importance.json` (language-neutral, read by the sim; no hash moved).
- `data/text/{en,de}/`: `ui.json` (131 keys), `lexicon.json` (23 person nouns), `names.json`, `locale.json` (CLDR via Babel 2.18, `tools/text/cldr.py`), `scenarios.json` and `.meta.json` (246 keys), `patterns.json` (empty until item 14). All bundles are hashed into `data_hash` and passed to the wasm.

Sim:
- People and dead people carry `sex` (m or f), drawn from `rng_for(seed, "sex", id)` so no world draw moves; rules-1 saves load and get theirs from the seed; `kRulesVersion` 2 (own `golden:` commit).
- Refusal reasons are codes with arguments (`needs_knowledge|n=3`); spell and terraform failures tag their event with the code; 31 `reason.*` strings per language.

Scenarios and patterns (copied from the project first, verbatim, then changed in reviewable commits):
- Schema: `text.lang`, variant `id`, `noun` and `sex` on person roles, no `maxLength` (caps per language in `vocab.json` `text_caps_chars`); patterns lose `text_key` (keys `pat.<id>.*`).
- The 30 seeds: the four defects fixed (86 edits), nouns on all 39 person roles, tokens where German needs the role.
- German: the 163 draft strings under the new keys (6 changed to follow the fixed English), 83 new strings (seeds 21 to 30, six reveal lines). Unreviewed.
- `tools/scenario_check.py --text`: the lint and forced-state renders; `tools/text/{scenario_strings,lint,review}.py`; `tools/bank/translate.py`; `tools/check_literals.py`; `docs/style-de.md` (draft), `docs/style-de-gold.json` (empty).

Page: `web/game/ui.js` (`t`, `tf`, `?lang=`), `data-t` on the static chrome, the wake kicker, the flight panel; `web/l10n-check.cjs`.

Docs: `docs/scenario-spec.md` §12, prompts 11 and 11-authoring-system (token grammar, noun rule, two fixed seeds), README, CLAUDE.md "Text".

## 2. Numbers

- The five test strings plus the font string, 21 renders: all as the research expects, natively and in wasm, hash `e9c60a9de21bee87` on both (`hoard-sim --render-batch tests/text/five.json`, `node web/smoke.mjs --render-batch tests/text/five.json`).
- Forced-state renders of the seeds: 5,904 (246 keys × 2 languages × 12 states), 0 problems. The lint caught five problems on the first run (two English bare nouns, three German strings with a different role set), all fixed; a negative control (a `{x.pronoun}`, a stray brace, an English phrase in German) fails with 61 findings.
- `l10n-check.cjs` at 1280×800: en, pseudo and de pass on 12 screens each.
- Goldens: state hashes moved once (person sex); chronicle texts unchanged; `tools/check_literals.py` finds no two-word English left in `sim/`.

## 3. Decisions taken

- Person sex is stored (your answer), drawn from a keyed stream so runs keep their course.
- Language files carry a `locale.json` beside the seven files of the research, generated from CLDR and committed, so CI needs no Babel.
- `{a.size}` and other act fields arrive as finished words from whoever binds the role (item 10); the fixtures use "a third" and "ein Drittel".
- English uses tokens wherever German needs the role (`{crown.subj}` renders "It"), so both languages name the same roles; the lint enforces equal role sets per key.
- Years in UI strings are passed as text, not numbers, so "Jahr 1200" is not grouped.
- `tools/check_literals.py` is run by hand: you declined `text_no_literals` as a test.

## 4. Deviations from the brief

- The narrator keeps its old grammar and English only (your decision); its single glue words (the, of, an) stay in `briefing.cpp` until it converts. Hook and talent texts in `hooks.json` and `talents.json` move with that conversion.
- Page: chrome only (your decision). Narration, lair tabs' contents and the JS sentence building stay English.
- No German chronicle golden: with the narrator still English it would equal the English one. The five-string file is the German text golden for now.
- `translate.py` is written against the Python SDK page (docs, Messages example) but never called: the calls are paid and there is no key here. It puts the instructions in the user message; I could not confirm the `system` parameter from the page I read.
- `web/game-check.cjs` j.1024: the HUD may now take two rows (height under 80 instead of 60). At 1024 px the English HUD already overflowed and the growth chips were cut off; it now wraps instead, and the panels under it follow its height. This changes an acceptance threshold of yours; say if you want the old one back with another fix.

## 5. Found on the way

- The legend cut off its rows in English at 1280×800 (fixed: rows wrap).
- `princess_returns` was not the only hard-coded person: `the_gold_you_took` named the court's king.

## 6. Open

1. Your read of `reports/19-de-review.md` (all 30 seeds, German male and female, about 1,150 lines). Please note the minutes spent and defects per scenario; your corrections become the lexicon, `docs/style-de.md` and eight gold examples.
2. Names are sex-neutral: a female Aldric is possible. Per-sex name endings would go in `data/names.json` (a sim change, own golden).
3. The `Endings` page name in German („Enden“ or „Ausgänge“).

## 7. How to verify in five minutes

```
cd games/hoard
cmake -S . -B build && cmake --build build -j && ctest --test-dir build -L fast
./build/hoard-sim --data data --render-batch tests/text/five.json | grep hash
WASI_SDK=~/wasi-sdk bash web/build.sh && node web/smoke.mjs --render-batch tests/text/five.json
node web/l10n-check.cjs --lang pseudo,de
```
Open `web/hoard-game.html?lang=de` for the German chrome.
