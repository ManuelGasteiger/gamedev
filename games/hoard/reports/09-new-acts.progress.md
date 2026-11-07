# 09 · New acts: progress

Branch `item/09-new-acts` from main 1478254. Build with `-j2`. Item 08 pushed `resolve_fight` on `item/08-slayer-roster` (6a71ed4); merged into this branch.

## Inventory (schema act → what the tree has)

| act | before 09 | now |
|---|---|---|
| burn, kill, take, defend, show, give, take_hostage, release, accept_offering, miracle | `acts.cpp` | same path; burn structures, take item, take_hostage consent added |
| destroy_altar, crush | reaction row only | `acts_force.cpp` |
| receive, hunt, exit, commune, demand, swear, terraform, sleep, pick_talent | API verbs | the act calls the verb (`acts_routed.cpp`); the verb logs the act |
| keep | `keep_person` (historian) | keep(historian) calls it; other roles `acts_lair.cpp`; `keep_person` stays for saves and replays |
| teach, corrupt (court), curse, bless, ward, incite, raise_undead | `cast` | the act casts; `cast` logs the act |
| teach, corrupt (person) | `resolve_move` only | `acts_speech.cpp`, `acts_arcana.cpp` |
| sponsor | spell act | `acts_lair.cpp` |
| fight | none | `resolve_fight` (item 08) from `acts_force.cpp` |
| broker, reveal, judge, haunt, consecrate, guise, dismiss, build, ignore | none | new |

## Done

- act record (source, answers, target kind, args), schema act log (`act_log.json`), world-aging fields
- `data/gates.json`, gate check, refusals (`apply_acts` out param, wasm `refused`, `act_gates` command)
- routing of the old verbs, logging of sleep, pick_talent, cast, swear, commune, keep, terraform
- new acts by family, demand clauses, sworn pilgrims arrival, Mortal Guise (arcana node 3)
- reaction rows, narrator lines, reason keys en and de, importance
- policies `schemer.json`, `priest.json`; `tools/act_check.py`; `tools/act_scenarios.py` with `hoard-sim --act-script`
- world-aging from abandon, collapse, fading, settle
- merge of item 08 (6a71ed4, then 2fe68cd), fight wired
- golden commits (rules 11), save expectation, pages; ctest fast 24/24, smoke 1-60 equal, l10n pass, game-check 90/90, check.cjs ok
- README, report draft

## Next

1. fill the report's numbers (coverage 300 x 3, interest after), commit, push

## Notes for a replacement

- Never `git add -A games/hoard`: the LFS-pulled assets show as modified. Add paths.
- The disk is nearly full (shared machine); delete run outputs after use.
- Push with plain `git push`; a merge that staged assets was rejected once and redone.

## Baselines

- interest main (idle, played, 300 seeds): scratchpad `main/games/interest_main.txt`
- played chronicle hashes equal to main on seeds 1 to 60 before the 08 merge

## Blocked

- nothing
