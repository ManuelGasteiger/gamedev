# Hoard: story, chronicle, endings (v0.1)

Status 2026-10-22. Brainstorm after the v0.4 review. Decisions taken: divine ascension is the hardest ending and is planted early as a prophecy; several endings with twists; the game should take multiple runs to finish; fear splits into dread and awe; civilisations get longer memory through historians and books; how real-time actions map to sim acts is deferred on purpose.

## 1. The chronicle: how the player learns what happened

The sim already has the causal graph (act log → consequence). The design only decides how much of it the player can see. Understanding is a progression:

| level | source | shows | reliability |
|---|---|---|---|
| 0 | dreams, what you saw awake | headlines only ("Dorun burned") | fragments, no causes |
| 1 | the hoard: books, held royals, artifacts | the history of the court each came from | biased by origin; two sources on one event cancel each other's lies (Keen Eyes shows a lie in one) |
| 2 | a kept historian in the lair | the "because" lines between events | depends on who writes |
| 3 | high Lore | counterfactuals ("had the granaries stood, the plague would have stayed in the fens") | the sim's truth |

The historian is earned, not given: a mortal you take (scribe, monk, court chronicler). He dies during your first long sleep; you wake to a half-finished chronicle and his grandson, or nobody. Keeping one across sleeps is a puzzle with several answers: a long-lived elf, a kobold line (Kobold Servants), an undead scribe (never dies, every entry is about death), a mortal who sleeps in the hoard and ages slowly.

A second voice, separate from the historian: an artifact in the hoard with an agenda (crown, mirror, skull). It tells the truth selectively and pushes one ending. The lich-crown wants you undead, the sun-mirror wants a god. It carries the prophecy.

Ties to the interest tool: track the share of hooks caused by the player's own acts versus storyteller events, and the played-versus-idle history delta, so "the world reacts to me" is measured, not hoped for.

## 2. Prophecy

One prophecy per seed at hatching, in riddle form, encoding the conditions of one ending (usually ascension). Mortals know it too: a cult forms to make it come true, a king founds slayers to prevent it, a church schisms over whether the dragon-god is the prophecy or its heresy. The prophecy is a public object the sim reacts to. More prophecies are found in books; some are wrong.

## 3. Endings

All endings are condition checks on world state, never scripted sequences, so strange combinations can fire (religious undead, pacifist scholarly orcs). Culture must be a drifting vector (aggression, piety, scholarship, order) so any race can end up anywhere, unlikely but possible.

1. **Divine ascension** (hardest): faith in several courts across at least two races, temples standing for three ages, an age of magic, a living high priest. Variants by the mix: the Protector (awe), the Devourer (dread; worship out of terror), the Bound God (a corrupt cult ascends you and owns the church).
2. **Lichdom**: an undead court or your own necromancy produces undead priests who offer unlife. Costs Might (the body rots), immune to slayers, the living unite. Either an ending or a mode change into a second act.
3. **The Portal**: a faction leaving the world (elves, a cult) opens a way out. Needs an oath held unbroken for ages. They only take you if you have become something they want to bring.
4. **The Mountain**: size past a threshold plus a thousand-year sleep; the world builds on your back. Epilogue: the civilisations living on you.
5. **The Brood**: find or make an egg (a lost egg in a dwarf vault, a sorcery, a transformed consort). You pass the hoard on; the next run starts in that world with you as its legend. Seed of multiplayer.
6. **Dragon-emperor**: every court sworn for N ages, a dragon-raised dynasty, a capital at the lair.
7. **The Last Flame**: every race gone. Make the burn-everything policy a reachable, bleak ending for the monster build.
8. **The Scholar**: Arcana and Lore reveal why dragons are rare and where the others went; exit into their realm.
9. **The Myth**: death while a legend. Killed after a long reign with enough memory in the world, you enter the next world's pre-history as a god. Permadeath's payoff; dying well becomes a goal.
10. **Mortality**: a witch or god offers to make you mortal. Cheapest to build.
11. **The Sun-eater**: cult, artifact, comet. You fly up and end the world.

### Why it is hard

Every good ending needs mortals to be strong (temples, magic, scholars, long-held oaths), and strong mortals are what kills you. Keeping them weak is safe and never ends. Multiple runs come from hidden prerequisites discovered only through the chronicle, and from carrying knowledge between runs (legends), never power.

## 4. Dread, awe, memory

- **Dread**: from burns, kills, hostages. Garrisons flee, fear-tribute flows; research and slayers rise, oaths wobble, peoples migrate away, fewer envoys dare the road. Decays fast unless renewed.
- **Awe**: from being seen without burning, protecting, giving, winning fights in the open, size, art. Faith, pilgrims, envoys, stable oaths. Scholars study you, feeding counters slowly. Decays slowly once recorded.
- Both: a temple religion. Dread alone: slayers. Awe alone: a cult.
- **Memory** as a court stat in three tiers: oral (fades in a generation), written (books, a historian: slow), monumental (statues, temples: near-permanent). Burning a library resets their memory of you, including recorded slayer tactics. Long Memory stops being a talent and becomes something you cause or destroy in the world.

## 5. Interacting with mortals

Nobody arrives at random. Access is gated by reason and route: pilgrims need faith, envoys need a court that knows where you are and dares, refugees need a disaster, sorcerers need an artifact to covet, thieves need a rumour of gold, sacrifices need a cult. The chronicle later shows why each came.

Dragon skills on Lore and Arcana: speech (Old Tongues), reading, writing, art, and **Mortal Guise** (take a human form, walk into a city). Guise answers romance, intrigue and in-person corruption without cutscenes. Example: hostage princess → Teach (intrigue) → Corrupt (dark magic) → Release; she returns as an agent with traits and a memory of you. The court may detect her (inquisition event), which also targets its own mages and slows the guild.

Romance as bonds, not a dating sim: a consort who ages while you sleep, a dragon-blood line, a lover who asks for lichdom too.

Characters are data: traits, relations, a memory of what you did to them (Shadow of Mordor's captains). No cutscenes.

## 6. LLM-authored scenarios (deterministic at runtime)

Idea: an LLM writes scenarios the player reacts to, where each offered reaction maps to a sim act. Keep the sim deterministic by generating hundreds of scenarios in advance and matching them at runtime.

A scenario is data:

- **preconditions** on sim state, referencing the act log where possible ("the dragon burned court X's capital within the last two wakings", "court Y holds a hostage lineage", "awe ≥ 0.5 and dread ≤ 0.2 at Z")
- **slots** filled at runtime with names, places, lineages
- **text** for the briefing and the audience
- **options**, each an act from the sim's API with its arguments bound to slots, plus visible cost and expected effect
- **tags**: tone, race, era, ending it pushes, things it must not contain

Pipeline: LLM authors against the act API schema and a list of things to avoid → schema validation → sim dry-run per scenario (does the precondition ever fire over 300 seeds? does every option resolve to a legal act?) → bank. Runtime picks by preconditions and weights, fills slots, logs offer and pick for the interest tool. Scenarios whose preconditions never fire or whose options are never picked get rewritten.

Why it feels reactive: the matching, not the prose. A scenario whose precondition names the player's own act reads as a consequence. The act-caused-hook metric in section 1 measures it.

Runtime LLM, if ever: flavour text only, never state.

## 7. Not decided

Cutscenes (leaning no), romance depth, which endings ship first, how real-time raid actions map to sim acts (deferred until the acts are stable).
