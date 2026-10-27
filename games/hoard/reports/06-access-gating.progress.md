# Item 06, access gating for visitors: progress

Branch `item/06-access-gating`, from 3836e41.

## Done

- data/arrivals.json, the arrival rule (rules/arrivals.cpp), lair knowledge and routes (arrival_gates.cpp).
- Verbs `receive` and `hunt`: recorded wrappers, `apply_input` cases, wasm commands.
- Hooks thief, slayers and pilgrims now read the arrival table (`arrival` key in data/hooks.json).
- Briefing block `ledge`, narrator lines, importance rows, ui lines in en and de.
- Tests `arrival_gating` and `arrival_cooldown` (fast, pass).
- interest.py prints arrivals per waking by kind and the ten most common because-lines.

## Left

- Measure idle and played over 300 seeds, tune the gates if arrivals per waking are outside 0.5 to 1.5.
- game-check.cjs and l10n-check.cjs.
- README file list and constants.
- Golden commit with the rules version bump, regenerated goldens, replays, save expect, wasm and html.
- Final report.
