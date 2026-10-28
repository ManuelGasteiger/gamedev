# 07 · People: traits, memory, social moves

Model: Opus 5.5 · Effort: high · Depends on: 02, 06 · Branch: `item/07-people-moves`

## Goal

Characters become data the dragon can act on and that remembers what the dragon did. Each named person gets a small trait set, a memory of the dragon with source and strength, and derived dread, awe and debt. Six social moves (Teach, Corrupt, Bargain, Threaten, Court, Release) resolve by a weight table against traits and memory, write memory on both sides, and give the corrupted-princess chain its rules. This is the responder side of the acts item 09 plumbs; coordinate by the interface in section "Contract".

## Notes for this tree (2026-10-28)

- Keep `bold, pious, greedy, cunning` as today so genesis output is unchanged; the eight new traits are added beside them.
- `hostage_came_home.json` and `rules/hostages.cpp` exist; extend them for the schooling branches.
- Item 05 runs in parallel and reads `lettered` for the historian. Add the tag with that exact name.
- Rules version is 7 (`sim/ages/record.h`). The golden recipe deletes `tests/golden/ages_v0*.hash` and `sift_bank_v1.hash`, runs `build/test_golden` twice, then regenerates `state_hashes.txt` (300 seeds), the replays and the save check; see `reports/14-story-sifting.md` for the commands.
- Rules live in `sim/ages/rules/`; the act code in `sim/ages/acts.cpp`; people in `rules/people.cpp` and `rules/people_rise.cpp`. `Person` (`sim/ages/state.h`) has `PersonTraits{bold, pious, greedy, cunning}` as doubles and `memory_of_dragon`.
- Player text is in `data/text/<lang>/*.json` (`narrator.json`, `ui.json`, `patterns.json`, ...), never in `en.json`; every new key needs a `de` line too (`node web/l10n-check.cjs --lang pseudo,de`).
- New rules draw from `rng_for(seed, tag, id)` (`sim/ages/rng.h`), never `world.rng`, so no existing draw moves.
- A new state-changing verb needs a wrapper in `sim/ages/recorded.cpp` and a case in `apply_input`.
- Item 14 built the sifter in `sim/sift/` (`briefing_lines`, `make_source`, `sources_conflict`, `because_chain`; all take `Params`). Item 06 built arrivals (`data/arrivals.json`, the `receive` and `hunt` verbs); `broker` and `keep` do not exist yet (item 09).
- Items 04, 05 and 07 run in parallel on separate branches. Touch shared files (`state.h`, `serialise.cpp`, `briefing.cpp`) in small, separate commits so merges stay simple.

## Read first

- `docs/research-social-sim.md` sections 1, 2 "For Hoard" (the character record, the six-move weight table, practices as plain data), 4 (CK3 inheritance numbers), and the corrupted-princess chain at the end of section 6.
- `docs/research-book-procedural-storytelling.md` sections 3.3 (two-layer memory with transformation) and 3.4 (one virtue, one vice, reactions over actions).
- `docs/act-api.md` 2.3 `release`, 2.4 `teach`, 2.5 `corrupt`, 2.6 `keep`, section 4 (secret acts).
- `docs/ages-sim.md` 17.2 (people as built), 18.1 (ages), 18.4 (keepers).
- `data/names.json`, the people code, `acts.cpp`.

## Scope

In:

1. **Person record** extended: `traits[]` (max 3 from a base list of 12: curious, proud, greedy, ambitious, devout, honest, craven, vengeful, bold, pious, cunning, loyal; keep the existing four as members of this list), `acquired[]` (max 2: `dragon_touched`, `lettered`, `schooled:<subject>`, `corrupted:<toward>`, `scarred`), `virtue` and `vice` (one each; the vice `hidden: true` until revealed by a source, the historian or Lore ≥ 15), `mem[]` entries `{ev, year, sev 1..3, src: witness | read | told, tier, s (strength 0..1), ax: {dread, awe, debt}}`, `rel[] {to, kind: parent | heir | rival | mentor | kin | hostage_of | consort_of, w −100..100}`. Derived per tick: `dread`, `awe`, `debt` as clamped sums of `s × sev × ax` over `mem` (range −5..5).
2. **Memory rules**: source scales strength (witness 1.0, read 0.8, told 0.6); decay per generation by tier as the court-level tiers (oral halves, written −10 %, monumental floor 0.3); a `recent` entry whose `sev × s ≥ 2` moves to `permanent[]` and may flip one trait with probability `0.3 × s` (writes a chronicle line: "Maelis, once craven, is craven no more"); heirs inherit 50 % of negative and 25 % of positive strength from a parent's entries about the dragon.
3. **Six moves** as data `data/moves.json`: per move the intent, hard preconditions (access: the person is at the lair or the dragon is in guise at their court; dragon skill: `teach` needs Lore per subject, `court` needs Lore 8 or Guise), the trait weight row (start values from the research table), threshold, shared modifiers (awe counts for Teach and Court cap +4, dread for Threaten and Bargain cap +4, debt for all cap +3), memory terms (`mercy` +2, `betrayal` −3, earlier success of the same move +1, clamp ±6). Score ≥ threshold accepts; margin under 2 picks the reluctant narration; ties by `world.rng`. Effects write a memory entry on the person and a court-level deposit (item 02) when witnessed.
4. **Release reading**: no roll; the person's dominant trait reads it: default `mercy` (debt +2, awe +1), proud `insult` (sev 2 grudge), vengeful `weakness` (no gratitude), craven `reprieve` (dread +1). `release` with `terms` attaches an oath draft the court must answer on the person's return.
5. **Secrets**: `corrupt` on a person adds `corrupted:<toward>` and a `Secret {holder: court, about: person, discovery_per_decade: 0.05 × (court piety + sages × 0.02)}`; discovery fires the `inquisition` card (item 04 adds the prophecy cards; this item adds `inquisition` in `data/events/intrigue.json`: devout nobles form a purge faction, those holding a hook or bond with the person cannot join, ultimatum at 100 % discontent per the CK3 pattern, outcome civil war or the person's fall; the court's mages' research halts for 20 years as a side effect). A `schooled:intrigue` person doubles discovery resistance.
6. **Hostage schooling**: a kept hostage receives `teach` moves while the dragon is awake; subjects write `schooled:<subject>`; the `hostage who came home` card reads them (rules: `schooled:intrigue` raises the betray branch, `lettered` raises the rule branch, `corrupted` adds the agent branch: the person acts as the dragon's agent, +1 `reveal` secret per waking about their court).
7. **Cap**: at most 8 named persons per court tracked with full records (ruler, heir, 2 heroes, 2 sages, 2 priests); others are counted, not simulated.

Out: the acts' plumbing in the API (item 09: `teach` and `corrupt` on persons, `keep`, `guise`, `reveal`; this item exposes `resolve_move(person, move, ctx) → outcome` and `read_release(person)` for 09 to call), the slayer roster (item 08 reads `mem` and `traits`), practices (Hostage, Oath, Courtship, Vendetta as data: sketch the JSON shape in the report, do not implement).

## Contract with item 09

`moves.h`: `MoveOutcome resolve_move(World&, PersonId, MoveId, const MoveCtx&)` where `MoveCtx` carries the dragon's growth points, talents, guise flag and the act id; returns `{accepted, margin, memory_written[], chronicle_line_key}`. `read_release(World&, PersonId) → ReleaseReading`. Both deterministic from `world.rng`.

## Acceptance

- The corrupted-princess chain from the research doc runs end to end in a lab world (`load_world`): Threaten the craven king accepts (score 7 vs 1), Teach the curious heir accepts, Corrupt accepts after Teach raised debt, Release reads `mercy`, 60 years later she rules and the court's memory tier is written, the secret surfaces at the documented rate and the inquisition card fires.
- Over 300 played seeds: move acceptance rates per move between 20 % and 80 % (none always or never accepted); trait flips ≤ 1 per person-lifetime on average.
- `interest.py`: persons with ≥ 1 dragon memory per court under `played` (expect 2 to 5 after year 300).
- Goldens: person records change genesis output? Only if names or roles change. Keep the base list's first four traits identical to today's so existing persons are unchanged; separate golden commit if the hash moves.

## Tests (confirmed by the owner, 2026-10-28)

1. `move_scores`: fixed persons with known traits give the table's scores for each move (Teach on curious+ambitious = 5, Threaten on craven with dread 3 = 7).
2. `princess_chain`: the lab scenario above, asserting each step's state.

## Report

Standard report, plus the acceptance-rate table per move and the practices JSON sketch.
