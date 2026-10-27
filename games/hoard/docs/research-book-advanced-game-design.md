# Advanced Game Design (Sellers, 2017), mapped to Hoard (excerpt)

Excerpt for item 03: sections 3.4, 3.7 and 3.8. The whole document with sources stays in the Project (`claude/research-book-advanced-game-design.md`).

### 3.4 The dragon's growth engine

A boosting engine with converters: raids fill the hoard, sleep length decides, digestion converts each hoard type into its stat, stats raise raid yield. If digestion leaves the hoard intact, one long sleep on a big hoard dominates and late raids become optional; if it consumes a fraction per century, raids stay necessary. The dragon has no braking engine of its own.

### 3.7 Loops already in the design

| ID | Loop | Sign | Path |
|---|---|---|---|
| R1 | Hoard engine | Reinforcing | hoard → stats → raid yield → hoard |
| R2 | Mortal growth | Reinforcing | time → wealth → era → wealth |
| R3 | Escalation (fear feeds research) | Reinforcing across sides | burns, kills → dread → research, slayers → threat → harder raids → dread |
| R4 | Awe and faith | Reinforcing | awe → faith → tribute, art → Dominion, size → awe |
| R5 | Grudge | Reinforcing | kill named person → heir grudge → slayer order → kill |
| B1 | Hoard lure | Balancing | hoard size → rumour → thieves, slayers → loss, risk |
| B2 | Coalition | Balancing | hegemon share → coalition → share falls |
| B3 | Collapse | Balancing | expansion → cohesion falls → successor states |
| B4 | Forgetting | Balancing | years asleep → memory decay → dread falls → research motive falls |
| B5 | Burn sink | Balancing | burns → wealth falls → research capacity falls |
| D | Sleep bet | Decider | sets the time step for R1 (digestion), R2 and B4 (years) |

R3 and B5 start from the same act. A burn raises dread (research motive) and lowers wealth (research capacity). With research proportional to dread times wealth, a burn on a poor faction nets out balancing and on a rich one reinforcing. That product is the most important single formula in the arms race and should live in data, visible and tunable.

### 3.8 Missing or one-sided loops

1. **No brake on the dragon.** Proposal: digestion yield per stat follows a logistic curve in the hoard mass of that type, so specialisation saturates and new loot types matter; or a Might upkeep that eats gold per century.
2. **Awe only reinforces.** Proposal: faith creates expectations (miracles, protection); expectations unmet during long sleeps turn into heresy or a rival god, capping R4.
3. **Research is free for mortals.** Proposal: research spends wealth that would otherwise compound. High-dread factions arm and stagnate, so the dragon has reason to keep some neighbours afraid and others calm.
4. **Gold is not conserved.** Proposal: one valley-gold stock; hoard share = hoard gold / total gold; rumour strength scales with share; mines and mints add slowly.
5. **Oaths only decay.** Proposal: a dragon visit renews an oath. This maintenance loop is the cleanest counterweight to always sleeping 3000 years.
6. **The dragon sits outside the coalition balance.** Proposal: count the dragon's ally toward hegemon share, and let the dragon itself trigger a coalition as a common threat.
7. **Memory is not coupled to research.** Proposal: research persists only as far as the memory tier allows (oral research resets in a generation, written persists, a burned library sets it back).
8. **Dread has no dose-response curve.** Proposal: low dread does nothing, middle dread drives research and slayers, extreme dread drives tribute and flight. This makes the context's existing dread effects one curve instead of parallel effects.
