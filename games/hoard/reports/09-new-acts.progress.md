# 09 · New acts: progress

Branch `item/09-new-acts` from main 1478254. Build with `-j2`. Item 08 had pushed nothing at the start (no `item/08-slayer-roster` on origin).

## Inventory (schema act → what the tree has)

| act | today | plan |
|---|---|---|
| burn, kill, take, defend, show, give, take_hostage, release, accept_offering, miracle | `acts.cpp` | keep; add structure, item, consent |
| destroy_altar, crush | reaction row only (generic path) | handler |
| receive, hunt | `receive`, `hunt` verbs (`arrival_acts.cpp`); receive writes an act through `apply_acts` | route the act to the verb |
| exit | `exit` verb (`exit.cpp`), writes an act | route |
| commune | `commune` verb (`chron_voice.cpp`), no act | route, log |
| keep | `keep_person` verb, historian only (`chron_keep.cpp`), no act | keep(historian) calls it; other roles new; old verb stays for replays |
| demand | `demand` verb (`ages.cpp`), tribute/no_walls/no_settle, writes an act | route; new clauses |
| swear | `accept_offer` verb | route by offer id |
| teach (faction), curse, bless, ward, incite, raise_undead, corrupt (faction, disabled) | `cast` verb (`spells.cpp`), no act | route; log the act |
| teach, corrupt (person) | `resolve_move` (`moves.cpp`), no API verb in the tree | call from the act |
| terraform | `terraform` verb, no act | route, log |
| sponsor | `sponsor_remnant` (spell act) | route |
| sleep, pick_talent | verbs, no act | log as acts |
| fight, broker, reveal, judge, haunt, consecrate, guise, dismiss, build, ignore | none | new |

The brief lists the six moves as API verbs; the tree has `resolve_move` only (item 07 left the plumbing to 09).

## Done

- branch, inventory

## Next

1. Act record (source, answers, target kind, string args), schema act log export, schema-shaped input.
2. `data/gates.json` and the gate check, refusals out of `apply_acts`.
3. Route the existing verbs, log sleep, pick_talent, cast.
4. New acts by family.
5. World-aging fields, reactions, narration, reason keys.
6. Policies schemer, priest; `tools/act_check.py`, `tools/act_scenarios.py`.
7. fight (merge 08 if `resolve_fight` is there).
8. Goldens (rules 11), report.

## Blocked

- nothing
