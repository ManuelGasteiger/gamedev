# 07 · People: traits, memory, social moves

Branch `item/07-people-moves` from 7d8c372. Nothing paid, no external calls.

## What changed

- `sim/ages/state.h` (own commits): `PersonTraits.list` (up to 3 of 12, strongest first), `Person.virtue`, `vice`, `vice_hidden`, `mem`, `permanent`, `rel`, `flips`; `PersonMemory`, `PersonRel`; `World.secrets` (`Secret` with `exposed` and the purge's `discontent`).
- `sim/ages/person_json.{h,cpp}`: the new person fields and secrets in saves; empty parts are left out. `serialise.cpp` calls it (four lines).
- `data/moves.json`, `sim/ages/move_data.{h,cpp}`: traits (base list, virtues, vices, `trait_on`), acquired tags and cap, memory constants, per-act memory rows, flips, the court cap, rel weights, the five scored moves, release readings, secret and inquisition numbers. Plumbed through `data.h`, `data.cpp`, `data_io.cpp`, the data hash, `api.cpp`, `build.sh`, `build.ps1`, `smoke.mjs`.
- `sim/ages/person_record.{h,cpp}`: traits at birth from `rng_for(seed, "traits", id)` (no world draw moves), the court cap of full records, `axes` (dread, awe, debt), acquired tags, memory entries, `witness_act` (owner court: witness; witness courts: told), inheritance at death.
- `sim/ages/moves.{h,cpp}`: `resolve_move`, `move_score`, `read_release`, `release_terms`.
- `sim/ages/rules/person_memory.cpp`, `secrets.cpp`, `inquisition.cpp`: after `rule_people` in `step.cpp`.
- Hooks into existing code: `names.cpp` (traits at birth), `people.cpp` (inheritance in `bury`, parent rel on the heir, `fall_person`; `retire` takes `Params`), `hostages.cpp` (hostage_of rel, release reading), `acts.cpp` (one call to `witness_act`), `memory_tiers.cpp` (a lettered ruler keeps records).
- Cards: branch `boost` (`cards.h`, `card_data.cpp`, `cards.cpp`), effect `fall` (`card_effects.cpp`); `data/events/hostage_came_home.json` reads schooling, `data/events/intrigue.json` (inquisition, purge).
- Text: narrator lines (en) for the moves, flips, secrets, inquisition, purge, the agent homecoming and terms; importance rows and tiers; `reason.move_*` in en and de.
- Runner: the wakings log carries `people` (tracked people, with memory, the move census). `tools/interest.py` prints them.
- Tests (the owner's list): `move_scores`, `princess_chain`, both fast.

## Numbers

`tools/interest.py`, played, 300 generated seeds, 3000 years, before (main 7d8c372) and after (this branch):

| measure | before | after |
|---|---|---|
| outcome entropy (bits) | 3.96 | 4.06 |
| reversals per 1000 y | 7.71 | 7.58 |
| causal depth | 0.080 | 0.079 |
| pacing CV | 0.384 | 0.388 |
| persistence | 0.387 | 0.382 |
| agency (Jaccard vs idle) | 0.905 | 0.903 |
| events per run | 5666 | 5775 |
| dragon slain | 42 | 38 |
| majors per 10 wakings | 33.6 | 35.5 |
| briefings naming a person from an earlier one (band 30 to 60 %) | 7 % | 16 % |
| arrivals per waking after year 200 | 1.89 | 1.91 |

Acceptance rate per move (census: every move scored on every tracked person at every waking of the 300 runs; access and skill are not checked, so it measures the weight table against the people the runs made):

| move | threshold | accepted | band 20 to 80 % |
|---|---|---|---|
| teach | 2 | 37 % | met |
| corrupt | 3 | 31 % | met |
| bargain | 2 | 67 % | met |
| threaten | 1 | 60 % | met |
| court | 3 (research 4) | 30 % | met; 13 % at threshold 4 |
| release | no roll | reading by the dominant trait | |

Trait flips: 26 416 over 411 026 finished lives, 0.064 per lifetime (target at most 1).

People with at least one dragon memory per court after year 300: 1.65 (expected 2 to 5). Courts hold only about 2.6 tracked people on average (ruler, heir, a sage or priest; heroes are rare), so 1.65 is about two thirds of them. The cap of 8 is never reached.

Corrupted-princess chain (`princess_chain`, fixed start world seed 1, the elf court): Threaten the craven king 7 against 1 (he gives up his heir), Teach the curious and ambitious heir 5 against 2 (`lettered`), Corrupt 5 against 3 (`corrupted:dragon`, a secret on her court), Release reads `mercy` (debt 3, awe 3). Fifty years later she is queen and her court's memory tier is written. The secret surfaces 20 years after the release at 0.05 x (piety + sages x 0.02) per decade, and the inquisition card fires a decade later.

## Decisions taken

- New draws come from keyed streams (`rng_for` with "traits", "flip", "secret"), never `world.rng`, so genesis names, roles and the four trait numbers are unchanged (the brief's notes override "ties by world.rng").
- The four existing traits count in the list when their number is at least `trait_on` 0.75, strongest first, up to 3. A virtue and a vice are drawn from the eight new traits when missing (in drawn order), a third trait with chance 0.5. Virtues: curious, honest, loyal, devout, bold, pious; vices: proud, greedy, ambitious, craven, vengeful, cunning (not in the docs).
- `ax` is the research's gain divided by sev, so s x sev x ax gives its numbers (taught, sev 2, is awe 1, debt 0.5). Taken literally with the research gains as ax, the chain would end at debt 4, awe 5 instead of 3 and 3.
- Bold, pious, cunning and loyal weigh 0 in every move (no row in the research table).
- A score at or above the threshold accepts, no roll: there is one narration per outcome, so no tie between narrations arises.
- A failed precondition returns a reason code and writes nothing. Access: a hostage, a visitor at the ledge, or `ctx.guise`. Lore per teach subject (letters 3, craft 3, war 5, intrigue 8, dragon lore 8, magic 8 plus Arcana 3), corrupt Arcana 3, court Lore 8 or guise.
- Threaten with `demand: hostage` hands over the heir: the giver remembers `taken_hostage` (sev 3), the hostage `taken` (sev 2). On a proud or vengeful target it writes a hidden `humiliated` entry whatever the answer.
- Corrupt writes `corrupted:<toward>` but not `dragon_touched` (the cap of 2 is full with `lettered`). `dragon_touched` is written when a witnessed sev 3 entry becomes permanent. Nothing writes `scarred` yet (item 08).
- Permanence is judged before the step's decay, at the strength an entry was written with; permanent entries do not decay. A flip removes the first trait of the row for the entry's dominant axis (dread takes bold, proud, curious; awe takes craven, vengeful; debt takes vengeful, proud). A flipped virtue or vice leaves that field empty. The four trait numbers stay, so slayer odds and epithets are untouched.
- Dragon acts write person memory: the owner court's tracked people as witnesses, the witness courts' tracked people as told. Sev from magnitude (2 at 0.3, 3 at 0.6). A generation is the race's `generation`.
- Inheritance happens at death, to the people whose `parent` rel names the dead (50 % for entries whose dread outweighs awe plus debt, 25 % otherwise, source told).
- Release reads the first trait of the list. `read_release` writes the entry itself and `release_hostage_person` calls it, so item 09 must not call both.
- Terms: `release_terms` stores the draft oath with status `draft`; at the person's return the court swears it when the person's dread plus debt is at least 2 (`terms_accept`), else refuses.
- While a lettered ruler reigns the court's memory is written (written = max(written, oral)), and the ruler counts in `keeps_records`.
- Inquisition: the card tags the court `inquisition:<person>` for 10 decades and stops anti-dragon research (`anti_dragon_rate` x 0) for 20 years. Purge members are tracked devout or pious people with no positive parent, heir, kin, mentor or consort bond to the person. While they make up 80 % of the court's tracked people, discontent rises 0.25 per decade; at 1 the ultimatum fires and the `purge` card decides: a one-region court brings the person down, otherwise civil war (split) or the fall at even odds. Hooks held on the person are not modelled (people hold no hooks).
- The agent branch of `hostage_came_home` tags the person `dragon_agent` for 100 years; the free reveal per waking is for item 09 to read.
- Court threshold 3 instead of 4: at 4 only 13 % accept.
- A trait flip has importance 0.4 (tier 2): at 0.5 it counted as a major event and raised majors per 10 wakings from 33.6 to 52.3.

## Deviations

- `read_release` and `resolve_move` take `Params` (contract: `read_release(World&, PersonId)`), like item 14's interfaces. `MoveCtx` adds `subject`, `toward`, `demand`, `hostage`.
- The lab uses `load_data` (the fixed start world) and the elf court: a human heir old enough to be taught cannot outlive the reign counter for 60 years, and her secret dies with her.
- `acquired[]` is not a field: acquired tags live in `Person.tags` (item 05 reads `lettered` with `has_tag`) and count against the cap of 2 by the list in moves.json.
- Dread, awe and debt are computed on demand (`axes`), not stored per tick.
- The vice carries `vice_hidden`; nothing reveals it yet and `people_json` sends it as stored. The shell must respect the flag until a source, the historian (item 05) or Lore 15 reveals it.
- No changes to `docs/`, and the README file list is not updated.

## Goldens

Changed, rules 7 to 8, in three `golden:` commits: chronicles, chronicle hashes, `sift_bank_v1.hash`, replays and save expectation; `state_hashes.txt` (300 seeds); the wasm and both pages. The idle chronicle hash `ages_v0.hash` is unchanged. In the three chronicle texts the first change is at the first waking after the hatching (seed 1 at 670, seed 2 at 510, seed 3 at 610): `trait_flipped` lines for people who saw the first burns. In seed 1 the flip also makes the wake guarantee unnecessary (an event above `importance_min`), so the storyteller's festival is not drawn and the world stream moves from there. The wasm agrees with native on seeds 1 to 60.

## Practices (sketch only, not implemented)

```json
{"practices": [
  {"id": "hostage", "roles": ["taker", "held", "kin"],
   "actions": [{"if": ["held.at_lair", "taker.awake"], "then": ["taker may teach held", "taker may corrupt held"]},
               {"if": ["held.years >= hostage_raise_years"], "then": ["held acquires dragon_raised"]},
               {"if": ["taker releases held"], "then": ["read_release(held)", "kin answer the terms"]}]},
  {"id": "oath", "roles": ["swearer", "holder"], "expiry": "years",
   "actions": [{"if": ["clause broken by swearer"], "then": ["holder remembers betrayal sev 3", "spawn vendetta(holder, swearer)"]}]},
  {"id": "courtship", "roles": ["dragon", "consort"],
   "actions": [{"if": ["court accepted", "consort.at_lair"], "then": ["keep consort", "rel consort_of 60"]}]},
  {"id": "vendetta", "roles": ["avenger", "target"],
   "actions": [{"if": ["avenger.alive", "target.known"], "then": ["avenger sworn to the memory", "heir inherits the vendetta"]}]}
]}
```

## Open questions

1. People with a dragon memory per court is 1.65, below the expected 2 to 5, because a court tracks about 2.6 people. More heroes, sages and priests (item 08's roster) would raise it; or the expectation becomes a share of the tracked people.
2. Court threshold 3 or the research's 4?
3. Should a flip replace a trait with an opposite instead of removing it? The docs name no opposites.
4. The purge needs devout people and rarely forms in a court of two or three tracked people; is 80 % of them the right base?
5. The narrator lines are English only, like the rest of `narrator.json`.

## How to verify in five minutes

```
cmake --build build -j2 && ctest --test-dir build -L fast      # 20 of 20, move_scores and princess_chain among them
./build/test_princess_chain                                     # the chain, with its years
seq 1 60 | xargs -P 2 -I{} ./build/hoard-sim --data data --golden {} | sort -n | diff - <(head -60 tests/golden/state_hashes.txt)
node web/smoke.mjs --golden 1-60
python3 tools/interest.py --bin build/hoard-sim --data data --seeds 1-30 --policies played --genesis --jobs 2   # "people" and "move census" lines
```

Checks run: `ctest -L fast` 20 of 20; `node web/smoke.mjs --golden 1-60` matches; `node web/l10n-check.cjs --lang pseudo,de` passed; `node web/game-check.cjs` 90 of 90, no console errors.
