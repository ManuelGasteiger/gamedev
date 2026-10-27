# 06 · Access gating for visitors

Model: Sonnet 5.5 · Effort: medium · Depends on: 02 · Branch: `item/06-access-gating`

Corrected 2026-10-27 to the tree: item 02 is merged. Dread and awe are `dread_eff(f)` and `awe_eff(f)` (`sim/ages/memory_tiers.h`); `memory_tier(f, mem)` gives "", oral, written or monumental; the unlock thresholds are in `constants.json` "memory" (`pilgrims_awe` 0.15, `honest_awe` 0.30); the pilgrims hook already needs awe 0.15. Offers live in `sim/ages/offers.cpp` (not `oaths.cpp`); text keys in `data/text/en/narrator.json`, importances in `data/importance.json`, UI strings in `data/text/<lang>/ui.json`. Tests (Manu, 2026-10-27): `arrival_gating` and `arrival_cooldown`.

## Goal

Nobody arrives at the lair at random. Every visitor has a reason and a route, both written as predicates on court state, and the briefing prints the satisfied clauses as the "because" line. This item turns the offer generator (17.3) into an arrival table covering envoys, pilgrims, thieves, slayer bands, scholars, kin, refugees and prophets, adds `receive` and `hunt`, and gives item 10 the `arrival.needs` gate names.

## Read first

- `docs/research-social-sim.md` section 6 (the access table and its rules: per 10-year tick, one arrival per court per tick, cooldown 100 years for the same (court, kind, cause), most specific kind wins).
- `docs/story-and-endings.md` section 5.
- `docs/act-api.md` 2.2 (`receive`, `hunt`), 2.4 (`swear`), section 4 (witnesses).
- `docs/ages-sim.md` 9 and 17.3 (offers: kinds, motives, pretexts, lies), 18.3 (the treasure hunters card), `core-loop.md` section 5 (hooks: thief, slayers at the gate, pilgrims).
- `docs/scenario-spec.md` 2.1 `arrival` (what item 10 will read from this item).
- `offers.cpp`, `data/offers.json`, `rules/cards.cpp`, `hooks.cpp`, `data/hooks.json`.

## Scope

In:

1. **Arrival table** `data/arrivals.json`: one row per kind with `requires[]` (named predicates), `blocked_when[]`, `cooldown_years`, `specificity`, `person_role` (who comes), `text_key`. Start values from the research table:

| kind | requires | blocked when |
|---|---|---|
| pilgrim | `faith_share ≥ 0.05` (or faith ≥ 0.5 until item 12), `knows_lair`, `awe_eff ≥ 0.15`, `route_open` | `dread_eff > awe_eff + 0.3` (they leave offerings at a foothill shrine instead: faith pool trickle, no visit) |
| envoy | `memory_tier ≥ written` or `knows_lair_exact`, a motive (at war, collapsing, plague, ambitious ruler, offer row triggered), `cohesion ≥ 0.4` | last envoy killed within 2 generations; `dread_eff ≥ 0.6` with no `debt` |
| thief | gold rumour (a `take` or `show` near them within 150 years, or hoard above the lure threshold), era ≥ iron, `knows_lair_exact` | dragon awake at the lair; wards raise the Arcana needed |
| slayer band | order alive, grievance memory severity ≥ 3, bolt-throwers or better, band power ≥ lair threat | band power below threat (shown as "gathering") |
| scholar | a `teach` memory with that court, `awe_eff ≥ 0.15`, `dread_eff ≤ 0.4`, a library | no safe route |
| kin | a relative of a hostage or consort alive, ransom or grief motive, `knows_lair` | `dread_eff ≥ 0.7` |
| refugee | a court lost a region or collapsed within 2 decades, `awe_eff ≥ 0.1` in the loser or an oath with it | the lair region is wasteland |
| prophet | a court knows the prophecy (item 04) or a cult exists, `awe_eff ≥ 0.3`, piety > 0.5 | a false dragon reigns there |

2. **Rule** `rules/arrivals.cpp` after oaths: per court per decade, evaluate every kind; pick the most specific satisfied kind not blocked and not on cooldown; create an `Arrival {kind, court, person, year, because: [satisfied clause keys], motive, stated_reason, is_lie}`; at most one per court per decade. Envoy arrivals carry the existing offer draft; the lie mechanics stay. Arrivals wait at the lair until the dragon wakes; refugees and pilgrims wait at most one short window of the phase, then leave (an event).
3. **`knows_lair`** per court: set by any witnessed act within 2 regions of the lair, by a `show` at their capital, by a source mentioning the lair, by a returned hostage; decays with the oral tier unless written. `knows_lair_exact` needs a returned person or a thief who came back.
4. **Route** per court: `route_open` is false while an orc faction or wasteland sits on every path from the court to the lair region (graph check on the 12 regions); envoys and pilgrims need it, thieves and slayers do not.
5. **Acts.** `receive {outcome: receive | eat | turn_away}` on the waiting arrivals of a kind: `receive` pilgrims gives faith pool and Dominion and a `received` memory on the pilgrims' court; `eat` gives food and dread; `turn_away` cools faith. Refugees received settle the lair's slopes (hunting +) or are given a region through `give(region)`. `hunt` on the lair slopes or an ownerless region adds food without witnesses. The gains rows `receive`, `receive.eat`, `receive.turn_away` already exist in `data/reactions.json`.
6. **Briefing**: an "At the ledge" block listing arrivals with the because-line built from the satisfied clause keys ("Pilgrims from Aldmere, because the shrine's faith has held for 50 years and the road is open").
7. **Hooks**: `thief in the hoard`, `slayers at the gate`, `pilgrims` become arrivals with the same answers; keep the hook ids so the page's hook cards still render.

Out: the person-level response to Bargain etc. (item 07), the slayer order's internal roster (item 08), scenarios (item 10; this item only supplies gate names).

## Design notes

- Gate names are a closed list in `data/arrivals.json`; item 10's lint reads it for `arrival.needs`.
- The existing offer kinds (protect, burn_rival, return_hostage, hide_heir, teach, bless, judge, purge, ward, tribute, elope, rite) each map to an envoy motive row; nothing is lost.
- Determinism: the specificity tie-break is by kind order in the file, then `world.rng`.
- A new API verb that changes state (`receive`, `hunt`) gets a wrapper in `sim/ages/recorded.cpp` and a case in `apply_input`, or replays break.

## Acceptance

- Over 300 idle seeds × 3000 years no arrival occurs (nothing knows the lair); under `played`, envoys arrive only at courts with written memory or exact knowledge, pilgrims only where `awe_eff ≥ 0.15`.
- The because-line is non-empty for every arrival and names at least one clause.
- `interest.py`: arrivals per waking by kind under `played` (expect 0.5 to 1.5 total per waking after year 200); zero arrivals in a run is a flatness flag.
- The v0.4 bands that passed after item 02 still pass; `web/game-check.cjs` 90 of 90.
- Goldens: offers now fire through arrivals; event order may change. Separate golden commit.

## Tests (confirmed by the owner)

1. `arrival_gating`: a court with `awe_eff` 0.2 and `knows_lair` sends pilgrims; the same court with `dread_eff` 0.6 does not (foothill shrine event instead).
2. `arrival_cooldown`: the same (court, envoy, motive) does not repeat within 100 years.

## Report

Standard report, plus the arrivals-per-waking table by kind and the ten most common because-lines.
