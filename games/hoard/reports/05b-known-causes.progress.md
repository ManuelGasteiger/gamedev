# 05b · Known causes, folded tier-4 lines: progress

Branch `item/05b-known-causes` from `origin/main` (4659c29).

## Done

- `.bare` variants (en, de) for every pattern key that names its own cause; `sift_line_text(..., with_cause)` tries `<key>.bare` first along the usual fallback chain, and `pat.<id>.title.bare` for `{title}`.
- Briefing (`sift_block`, `tell`): pattern lines use the bare text below level 2; `cause_available` / `cause_shown` count a line's own cause.
- Sifter (`select.cpp`): tier-4 lines with the same fold key (`pat.<id>.fold[.outcome|.open]`, or `pat.fold.<template>` for raw events) in the same year fold into one line with `{names}`; params `fold_names`, `fold_lines`. Folds into the headline too. One budget slot. `sift_wake` marks every folded thread shown.
- Fold texts en, de; `pat.list.and`.

## Next

- Build, run fast tests, read the goldens that move, before/after examples, wasm checks, report.

## Blocked

- Nothing.
