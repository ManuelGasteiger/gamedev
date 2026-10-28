# Faith, cults and culture drift: research for Hoard

Excerpt for item 04 (and item 12 later): the "For Hoard" parts of sections 1, 2, 3, 5 and 6, verbatim. The findings and 61 sources are in the Project (`claude/research-faith-and-culture.md`). Every number is a proposal for the headless runner to tune.

## 1. Crusader Kings 3: fervor, heresy, hostility

**For Hoard**

- Give each cult a fervor `f` (0 to 1) separate from its membership share. Per decade: `f += 0.03*(0.5-f)`, plus 0.05*m for each witnessed act matching one of the cult's two virtues, minus 0.08*m for each sin, minus 0.02 when nothing was witnessed and no priest is present (0.005 with a temple), minus 0.1*max(0, share-0.5). The size term copies the lower fervor of large faiths.
- Three tenets per cult from an authored list of about 20. Each tenet names one virtue and one sin from the act vocabulary: a Flame tenet prizes `burn` on the unworthy, a Keeper tenet prizes `defend` and `give` and sins on `burn`. The player's behaviour then steers fervor directly.
- Schism check each decade when `f <= 0.40`, share >= 0.05 and a `heterodox` sage exists: `p = min(0.5, 0.05 + 2*(0.40-f))`. The splinter takes 20 to 50 percent of members, inherits two tenets and swaps one, both cults gain +0.15 fervor, and converts cannot switch again for 15 years. This yields the Order of the Flame against the Reformed in the paper playtest.
- Hostility from tenet distance: 0 differing tenets Righteous, 1 Astray, 2 Hostile, 3 Evil; +1 with a `zealot` tenet, -1 with `syncretic`. Holy war needs Hostile. A declared holy war costs the declarer 0.03 fervor, and a defender who loses land gains 0.05.

## 2. Dwarf Fortress: religions, cults, necromancers, memory

**For Hoard**

- Cult stage ladder by share `s` of a court: seed (prophet exists, `A_eff >= 0.3`), shrine at `s >= 0.02`, priest at 0.05, temple at 0.10 plus a wealth spend, high priest at 0.15 with a temple, holy city once two temples exist in different courts. The priest and high priest are roles the `keep` act already knows.
- Fear cult: a court with `D_eff >= 0.5` and `A_eff < 0.2` grows a propitiation cult capped at 0.05 share. It pays tribute and has no doctrine. If awe later passes 0.3 it becomes a real cult whose founding story is the burning. This turns DF's useless megabeast worship into a seed.
- Tag each cult with the dragon's two most frequent act families as its spheres (burn gives fire, defend gives shelter, take gives hoard). Names and titles follow the spheres.
- Necromancer origin: a person with `mortality_fixation` (over 80 percent of lifespan, Lore >= 6) plus a death-sphere patron (an artifact with agenda `lichdom`, an undead court, or a dragon with the death sphere). The patron grants the slab. Three apprentices found a tower. Zombie cap 50 plus 10 per apprentice, overflow becomes raiders, one region blights per 100 years. Courts exile long-lived scholars at 5 percent per decade, which seeds necromancers elsewhere.

## 3. Caves of Qud: generated history that leaves things in the world

**For Hoard**

- Memory tiers as stocks per court, kept separately for dread and awe. Effective value `E = 1 - prod(1 - x_k)` over tiers. (Built in item 02; see `reports/02-dread-awe-memory.md` for the shipped numbers.)
- Every monument stores one `act_id` and a bias flag, the way Qud shrines show one random gospel. A temple of a dragon cult converts half of the dread in what it remembers into awe, which is the flattering-inscription rule. An anti-dragon memorial does the reverse. The two accounts in waking 3 of the playtest then come from one log with different bias.
- A `dragon_temple` gets a faith floor `0.1 + 0.2*min(1, age/300)`, so older temples hold more.

## 5. Civ VI, god games, and belief that feels earned

**For Hoard**

*Unlocks* from effective values (built in item 02):

| level | effect |
|---|---|
| `A >= 0.15` | pilgrims may reach the lair |
| `A >= 0.30` | cults can found; honest envoys; oaths ×1.5 stable |
| `A >= 0.50` | shrines become temples; `consecrate`; faith floor 0.3 |
| `D >= 0.30` | demands accepted; tribute proportional to `D` |
| `D >= 0.50` | slayer orders can form; research rate ×(1 + 2*D*(1-0.7*A)) |
| `D >= 0.70` | garrisons flee; defence value ×(1-D) on raids |

Oath stability is `1 - 0.8*D*(1-A)`. Slayer pressure is `D*(1-0.7*A)`. Institution strength is `I = sqrt(A*D)`, and a cult becomes a temple religion at `I >= 0.35`. Awe alone stays a cult and dread alone feeds slayers, as the brief wants.

*Ascension check*, evaluated each decade and shown as a progress line at wake. Every item must hold continuously for 150 years, and a lapse over 30 years resets the timer, so one long sleep cannot farm it:

1. At least 4 courts spanning at least 3 races with `A_eff >= 0.6`, dragon-cult share >= 0.4 (>= 0.5 in two of them) and cult fervor >= 0.5.
2. A consecrated `dragon_temple` in each, at least three older than 100 years.
3. World magic >= 0.6, or one court in the arcane era.
4. A living high priest recognised by courts covering at least 3 races. The office must outlive one incumbent.
5. Mean `D` across those courts <= 0.5, and none of them persecuting the cult.
6. Era >= castles in at least 3 of them, and some court with slayers ready or cannons. The strong mortals that make the ending possible also make the dragon killable.
7. The dragon alive with Dominion >= 30 and at least 4 `consecrate` acts. The next waking offers `ascend`.

## 6. Historical cult-to-state-religion patterns

**For Hoard: cult life cycle**

1. *Founding.* Per decade, per court with `A_eff >= 0.3` and no dragon cult: `p = 0.01 + 0.06*A_eff`, ×3 if a prophecy text sits in its library, ×5 after a `guise preach` or `haunt` prophecy in the last 20 years. Seed share 0.002. False-dragon cults come from the card and fear cults from section 2.
2. *Growth.* `ds = r*s*(1-s)*(1 + 0.5*P)` per decade, `r = 0.34 * clamp(A_eff/0.5, 0.3, 1.5) * (0.5 + 0.25*devotion_level)`. `P` sums `s_j` over adjacent courts times 1, 2 (temple) or 4 (holy city), capped at 3. An isolated cult at `A = 0.5` and devotion level 2 needs about 115 years to reach 10 percent and about 180 to a majority, which suits the sleep tiers.
3. *Ruler conversion.* Per decade `p = 0.02 + 0.5*s`, capped at 0.25, from `s >= 0.05`; ×3 after a `defend`, a `miracle` or a royal illness. Patronage then adds 0.05 share per decade. A state decree follows 40 to 80 years after conversion and a ban on the old cult 10 to 15 years after that.
4. *Fragility.* A top-down conversion at `s < 0.15` reverses within 10 years of the ruler's death with probability `clamp(1 - s/0.4, 0.1, 0.9)`. Imposing a cult over the established state gods at `s < 0.15` costs 0.2 cohesion and carries a 5 percent yearly coup risk.
5. *Schism.* Section 1.
6. *Persecution and inquisition.* Opens when a state church faces a minority cult with `s >= 0.05` at Hostile or worse, or when the `inquisition` card fires. Violent form: minority growth ×0.5 for 20 years, with a 30 percent chance per wave of a grudge hero. Economic form: the minority loses about 1.5 percent of relative share per decade for 150 years or more. A violent campaign over 10 years that leaves `s >= 0.02` ends in a toleration card with probability 0.3 per decade.
7. *Dissolution.* Share below 0.002 for 30 years, or no priest and no monument for 60 years. If the dragon dies the cult loses 0.5 fervor. It survives as a memory religion if a temple stands, and enters the next world's pre-history.
