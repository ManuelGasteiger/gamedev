# Hoard: design references

Status 2026-10-18. Mechanics from other games, collected for `docs/core-loop.md` (v0.4). Numbers in the mechanics bullets come from the sources at the end, cited as [n]; "uncertain" marks values that sources contradict or that only one weak source gives. Numbers in the "For Hoard" bullets are proposed start values for the data files, not sourced.

## 1. RimWorld storytellers

- Cassandra Classic opens with one mad animal, then a single raider on day 5; major threats start on day 11. Then a cycle of 4.6 days on and 6 off: each on-phase brings 1 or 2 major threats (50/50) at least 1.9 days apart, about 8.5 per in-game year. The wiki's summary: ramping difficulty "with breaks every 1-2 raids" [1].
- Phoebe Chillax: 8 days on, 8 off, exactly one major threat per on-phase, the first on day 13, so 8 to 24 days between majors at Cassandra's raid size. The wiki warns that long calm makes players overrate their defences, so she can hit unexpectedly hard [2].
- Randy Random: an event every 1.35 days on average from weighted categories (Misc 3.5, FactionArrival 2.4, ThreatBig 1.4, OrbitalVisitor 1.1, ThreatSmall 0.6, ShipChunkDrop 0.22). After 13 days without a major threat the next event is one (a major every 7.14 days on average). Raid points get a random ×0.5 to ×1.5, so back-to-back big raids and long calms both occur [3].
- Threat points = (wealth points + colonist points) × difficulty × starting factor × adaptation, clamped to 35..10,000. Wealth points are 0 up to 14,000 wealth (buildings count half), 2,400 at 400k, 3,600 at 700k, 4,200 from 1M. Each colonist adds 15 points at low wealth, 140 at 400k, 200 at 1M. Starting factor 0.7 until day 10, 1.0 from day 40. Difficulty scale 0.1 to 2.2 [4].
- Adaptation is the drama lever: factor 0.4 at -30 adapt days, 0.8 at 0, 1.0 at 30, 1.2 at 60, 1.47 at the cap of 100. Adapt days grow 1 per day (0.5 above 60, 3 while negative) after a 30-day grace. A colonist death removes 30 (20 in a colony of 20), a downed colonist 3.5 to 8. Losses shrink the next raids, a winning streak grows them [4].
- Population intent: the chance of a wanderer joining is scaled by a curve over colonist count (8.0 at 0, 2.0 at 1, 1.0 at 5, 0.4 at 9, 0 at 12, -1 at 20) times a ramp over the first 10 days, floored at 0.05 (0.20 for Randy) [5].

For Hoard:
- Count the storyteller in wakings and use years as pressure. A major waking carries one major hook (50 % chance of a second, in another region) and is followed by at least one quiet waking with minor hooks only; after three wakings without a major the next one forces it (Randy's 13-day rule). The chance of a storyteller event on a waking is 0.15 + 0.25 × (quiet years / moult window), capped at 0.8 and reset by a major. Measuring in moult windows gives a hatchling and an ancient dragon the same pressure, and a long sleep after a quiet age is the likeliest to wake into a big event.
- Size slayer bands and sieges from a budget: (hoard points + Might points) × era scale × start factor × adaptation, clamped. Hoard points are 0 up to the starting hoard, then concave. Start factor 0.7 on wakings 1 and 2, 1.0 from waking 5. Adaptation in wakings: +1 per waking without a wound over 30 % of health (+3 while negative), -3 after a raid that ends below 30 % health; factor 0.8 + 0.1 × adaptation, clamped to 0.4..1.6.
- Ally intent: weight pilgrim and plea-for-help hooks by a curve over faithful or allied courts (×4 at none, ×1 at one, 0 from three), so a lone dragon is courted and a dominant one is left alone. Leave Randy's size roll out of the default, since under permadeath a dice spike reads as unfair; offer it as a difficulty option.

## 2. Plague Inc

- Notice: the cure starts shortly after the plague is spotted, and spotting gets likelier as severity rises; on Mega Brutal global awareness can start progress before that [6]. Once research runs, severity raises funding and makes the plague harder to cure, and it also raises DNA income [7].
- The cure is money: 340 research facilities in the default world (10 each in China and the USA), each up to $1,146.56 a day. Spending depends on severity, lethality and how important the infected country is; deaths cut spending and a country whose government falls stops paying [6]. The wiki and the game manual disagree on whether lethality speeds or slows the cure (uncertain) [7].
- The cure bar is always on screen, with warning popups at 25, 50, 75, 95 and 100 % [6].
- DNA comes from popping bubbles, scaled by severity, and passively from new infections and deaths, scaled by severity and lethality. Every evolved trait raises the cost of every other trait by 1; devolving takes 1 back, each devolve costs 1 more than the last, and most plagues start with devolving as a refund [8].
- Counters are small and stack: Genetic Hardening 1/2 (research efficiency +0.1/+0.3, base cost 7/22) and Genetic ReShuffle 1/2/3 (cure requirement +0.25 each, base cost 17/21/25) [9]. The Cure page instead says ReShuffle reduces money already spent (uncertain which is current) [6].

For Hoard:
- Fear is the notice stat and cuts both ways, as severity does: it raises tribute and every court's anti-dragon research. Start: research per year = base × era × (0.5 + fear) × the court's share of valley wealth. Show each court's progress on the wake screen and put 25/50/75/95 % in the briefing with the defence it unlocks next.
- Burning a court's capital or scholars' seat removes part of its research and halts it while the court is broken (a fallen government stops paying), while the fear it spreads makes the witnesses research faster.
- Keep counters incremental: a cursed court loses 25 % of its research rate per curse, up to three stacks, each curse dearer than the last, in place of one Hex that halves it. Plague's +1 cost per purchase fits spells and pools; talents stay free picks.

## 3. Telegraphed danger and fair permadeath

- Into the Breach telegraphs every enemy attack a turn ahead and has no random chance in the player's attacks, so each turn plays as a puzzle. Justin Ma: the aim was "every death felt like your own fault", and on failure it should be "very clear how or why you failed" [10].
- An attack-order tooltip shows when each threat resolves (the environment usually before enemies). A mech's move can be undone until it attacks, and one turn per battle can be reset [11].
- The power grid is health shared across battles, and losing all of it ends the run. A battle ends after a set number of turns, so surviving is enough [11].
- A lost run abandons that timeline; one pilot carries into the next [12].
- Slay the Spire first hid enemy actions, which clashed with permadeath. A "Next Turn" preview gave way to intent icons; early icons had no damage numbers, and playtesters found "having the numbers available was more engaging" and could plan without memorising symbols [13].
- RimWorld's commitment mode keeps one save, written on quit, and the run ends only when the whole colony is dead; storyteller and difficulty can be changed mid-game without penalty [14].

For Hoard:
- Before every act, list each defence that will hit, its damage after fear, hero and armour, the order the hits land, and the health left afterwards. An act that would kill needs a second confirm. One reset per raid undoes the last act and its wounds.
- Slayer clock in fixed stages, each at least one waking apart: "slayers forming" (named, with strength), "slayers ready" (year of the attempt, odds at current wounds), then the attempt. Add to the interest tool: deaths with no warning line in the previous briefing, target 0.
- The death recap names the killer and lists the three largest hits with the numbers shown before each act. A light carry-over like the kept pilot: the next world's legends remember the dead dragon.

## 4. Incremental and idle pacing

- Costs grow exponentially while production grows linearly or polynomially, so each generator slows down until a reset [15]. cost_next = base × rate^owned: AdVenture Capitalist's Lemonade Stand uses 1.07 [15], Cookie Clicker 1.15 per building [16], Derivative Clicker 5 × 1.1^n [17]. One designer pairs cost ×1.15 with output ×1.1 per level [18]. On Idle Idol the cost overtook the reward around level 35, and a 0.01 change in the rate made "a huge difference" by the 20th upgrade [19].
- Milestones keep steps visible: AdVenture Capitalist doubles a generator at 25 and 50 owned [15], Clicker Heroes gives ×4 per 25 owned [20], and Cookie Clicker unlocks a building upgrade at 1, 5, 25, 50, 100 and then every 50 owned, 15 tiers that each double the building [16][21].
- Prestige currency grows sublinearly with earnings: AdVenture Capitalist's 150 × sqrt(lifetime / 10^15) needs about 3-4× the earnings to double; Cookie Clicker's cube root needs 8×; Egg, Inc.'s exponent 0.14 needs about 128× [22]. Pecorella's rule of thumb: reset when the gain would be +50 % to +200 % [20].
- A prestige cycle runs in phases: a burst of buys after the reset, steady buys (about one per login), a slowdown, then a crawl that prompts the next reset [23].
- Magic Archery (Barribob, Steam, 4 Oct 2024) calls itself a "short incremental game": raise stats, buy upgrades, infuse arrows with magic, earn wealth from quests [24]. Players report finishing in 41 to 66 minutes, moving through jobs for passive experience, fire, ice and portal arrows, and shop and mana-tree upgrades [25]. A player comment says it has no prestige [26], and the developer agreed it may feel "too linear" [27]. No pacing numbers are published.

For Hoard:
- Measure growth points per waking in the played policy and aim for a visible step (talent or rank) on most mid-game wakings. If points per waking rise over a run, grow the rank cost 7 to 15 % per rank (the 1.07 to 1.15 range above) in place of a flat 25, and tune it in data with the interest tool.
- Every 5th rank of an axis gives a doubled bonus and a larger look change, so long runs keep landmarks.
- The sleep screen previews growth for three sleep lengths, the way a prestige button shows the gain. Phase totals 25/90/250/600 rise 3.6×, 2.8×, 2.4×; idle prestige formulas keep a steady multiple, so keep later phases near one ratio.

## 5. Roguelite choice moments

- Hades: each god's boon offers three options [28]. Rarity raises the numbers (True Shot: 70 common, 80 rare, 90 epic) and Poms of Power add levels [28]. One god per Attack, Special, Cast, Dash and Call; Legendary boons need specific boons of that god; harder rooms (Erebus, mini-bosses, pricier Charon offers) give better rarity odds [29]. 28 Duo boons, each needing one of several listed boons from each of two gods [30]. Mirror talents add +1 % per rank to the Legendary or Duo chance (up to 10 %) and up to 4 rerolls of boon offers (early-access values, uncertain for 1.0) [31].
- Slay the Spire: take one of three cards or skip. Rare/uncommon/common odds are 3/37/60 % after normal fights, 10/40/50 % after elites, always rare after bosses. A pity offset starts at -5 %, rises 1 % per common shown up to +40 % and resets on a rare. Singing Bowl lets a pick become +2 max HP [32]. The team used how often a card was picked when offered and how often it sat in winning decks to find weak and strong cards [13].
- Vampire Survivors: one of three random options per level, sometimes four [33]. Luck (+10 % per rank, max +30 %) raises the four-option chance, and meta upgrades buy rerolls, skips and banishes [34]. Banish removes an option for the run, 10 uses at max rank, which the GMDQ piece calls a player-facing answer to a noisy pool [33]. Evolutions need a maxed weapon, its passive and a chest from a boss at or after 10:00; chests hold 1, 3 or 5 items by Luck [35][36].
- Diablo 3 Paragon 2.0: no level cap, one account-wide level [37]. Each level gives one point, rotating Core, Offense, Defense, Utility. Capped stats stop at 50 points and are all full at Paragon 800; after that points go only to Core, where the main stat (+5 per point) and Vitality have no cap [38].

For Hoard:
- Keep two authored options per node and add a skip that pays a small permanent bonus (+5 health), so a node never forces a dead pick; give one reroll per moult.
- Duo talents: when the dragon holds a listed talent in each of two axes, the next node of either axis shows the duo as a third option (Iron Scales with White Fire, say). Any option can roll a greater version (+50 % effect, its own look) with the Spire pity: -5 % start, +1 % per option shown without one, cap +40 %; a fight hook answered this age adds to it, as Hades' harder rooms do.
- After the last node, rotate ranks through the axis' capped side bonuses, then put every later rank into the uncapped main number, each with a visible step. Log offers and picks per talent in the interest tool and rework talents that are rarely taken when offered.

## 6. World history, adapting enemies, doom clocks, runaway leaders

- Dwarf Fortress runs 5, 125, 250, 550 or 1050 years of history before play [39]. Civilizations found sites and fight each other and megabeasts, and longer histories leave more sacked towns, artifacts and necromancer towers; Toady One calls it "a giant zero-player strategy game ... (but thousands of agents)" [40]. Legends mode lists figures, sites, artifacts and chronicles by age, only for a world with no active fortress. In play, history comes from talking to people, images on engravings, coins and statues, and books, and one setting hides all history until it is uncovered [41].
- Crusader Kings 3 fires events from on_actions tied to world actions or a regular pulse (the example is every five years), picking from weighted lists [42]. Options carry stress by personality: a Compassionate ruler executing a prisoner gains 42. A new stress level brings a mental break that can shed much of it and often grants a coping trait (+20 % stress loss); most rulers get one or two per life [43]. One guide puts the levels at every 100 stress (uncertain) [44]. No sourced CK3 ambition system was found (uncertain).
- Shadow of Mordor: Uruks who achieve something or survive Talion are promoted, the one who kills him gains power, and each captain has strengths and weaknesses learned through intel. The aim was to make nameless orcs "memorable and unique", with sports games as the model for stories that go on after a loss [45]. Each defeat lets time pass, and unchecked captains keep rising and plotting against each other [46]. Players recall the killer moving up a rank and burned orcs returning scarred and remembering it [47].
- XCOM 2: the Avatar Project fills a 12-block bar; a full bar starts a countdown to defeat, and any reduction halts it. Destroying a facility removes all its progress; story objectives remove 1 to 3 blocks [48]. The countdown starts "around 20 days" [49] and varies by difficulty (uncertain) [50]. Jake Solomon: "We wanted to eliminate the late-game grind"; "There's no way to permanently delay the end of the game" [51].
- Civilization VI (Rise and Fall): a nuclear strike, a holy war or a city-state seizure triggers an Emergency; non-allies of the victim may join, get goals with a time limit, and gain permanent bonuses if they succeed; if they fail, the target gains a benefit [52][53][54]. Civ V's Vox Populi mod doubles its AI's victory-dispute weight (+25/35/45, +3 per era) toward a player close to any victory [55].

For Hoard:
- Legends: show the ten pre-history legends at hatching and keep the rest hidden. Each book or royal taken reveals one or two more about its court; a hero's legends name his weakness.
- Slayer roster per court: each sleep resolves a slayer turn (training, deaths, promotions). A slayer who wounds the dragon and lives comes back promoted with a counter to the act that hurt him (burned: a ward-cloak that halves burns on him), and the briefing names the current nemesis and his scars.
- Dragonbane bar: courts' anti-dragon progress fills a valley-wide 12-block bar; at 12 a countdown of one moult window runs to a joint slaying, and any reduction stops it. Burning the court that holds the most blocks removes them. Razing a capital calls a league: courts not sworn to the dragon join with a goal and a deadline; if the league fails, each member owes tribute.

## Sources

1. RimWorld Wiki, Cassandra Classic: https://rimworldwiki.com/wiki/Cassandra
2. RimWorld Wiki, Phoebe Chillax: https://rimworldwiki.com/wiki/Phoebe_Chillax
3. RimWorld Wiki, Randy Random: https://rimworldwiki.com/wiki/Randy_Random
4. RimWorld Wiki, Raid points: https://rimworldwiki.com/wiki/Raid_points
5. RimWorld Wiki, AI Storytellers: https://rimworldwiki.com/wiki/AI_Storytellers
6. Plague Inc. Wiki, Cure: https://plagueinc.wiki.gg/wiki/Cure
7. Plague Inc. Wiki, Core stats (Severity): https://plagueinc.wiki.gg/wiki/Severity
8. Plague Inc. Wiki, DNA Points: https://plagueinc.wiki.gg/wiki/DNA
9. Plague Inc. Wiki, Abilities: https://plagueinc.wiki.gg/wiki/Ability
10. Game Developer, Road to the IGF: Subset Games' Into the Breach: https://gamedeveloper.com/game-platforms/road-to-the-igf-subset-games-i-into-the-breach-i-
11. Game Informer, 23 tips to help you dominate Into the Breach: https://gameinformer.com/b/features/archive/2018/02/27/23-tips-to-help-you-dominate-into-the-breach
12. PCGamesN, Into the Breach and permadeath: https://www.pcgamesn.com/into-the-breach/permadeath
13. Wikipedia, Slay the Spire: https://en.wikipedia.org/wiki/Slay_the_Spire
14. RimWorld Wiki, Permadeath (commitment mode, difficulty): https://mail.rimworldwiki.com/wiki/Permadeath
15. Anthony Pecorella, The Math of Idle Games, Part I: https://www.gamedeveloper.com/design/the-math-of-idle-games-part-i
16. Cookie Clicker Wiki, Building: https://cookieclicker.wiki.gg/wiki/Building
17. Anthony Pecorella, The Math of Idle Games, Part II: https://www.gamedeveloper.com/disciplines/the-math-of-idle-games-part-ii
18. Eric Guan, Idle Game Design Principles: https://ericguan.substack.com/p/idle-game-design-principles
19. Game Developer, Balancing tips: how we managed math on Idle Idol: https://gamedeveloper.com/design/balancing-tips-how-we-managed-math-on-idle-idol
20. Anthony Pecorella, Quest for Progress (GDC Europe 2016 slides): https://www.slideshare.net/slideshow/quest-for-progress-gdc-europe-2016/65405507
21. dinogame.gg, How Cookie Clicker's Progression Curve Works: https://dinogame.gg/blog/how-cookie-clicker-progression-works/
22. Anthony Pecorella, The Math of Idle Games, Part III: https://www.gamedeveloper.com/design/the-math-of-idle-games-part-iii
23. Idle Games don't have to be so Idle: https://jwittsf.design.blog/2020/03/29/idle-games-dont-have-to-be-so-idle
24. WASDland, Magic Archery (Steam store data): https://www.wasdland.com/game/magic-archery-2905170/
25. Steam Community, Magic Archery, "What was Everyone's Fastest time": https://steamcommunity.com/app/2905170/discussions/0/4843149226488388177
26. itch.io, Magic Archery by Barribob (page and comments): https://barribob.itch.io/magic-archery
27. itch.io, Barribob's reply on Magic Archery: https://itch.io/post/9790056
28. Prima Games, Hades guide: skills and boons from the gods of Olympus: https://primagames.com/gaming/hades-guide-olympian-god-boons-skills/2
29. Hades Wiki, Boons (revision 6594): https://hades.fandom.com/wiki/Boons?oldid=6594
30. Prima Games, Hades: what are Duo Boons: https://primagames.com/tips/hades-what-are-duo-boons-how-get-duo-boons
31. Prima Games, Hades guide: Mirror of Night upgrades: https://primagames.com/gaming/hades-guide-mirror-of-night-upgrades-fated-list-of-prophecies
32. Slay the Spire Wiki, Card Rewards: https://slay-the-spire.fandom.com/wiki/Card_Rewards
33. GMDQ, Vampire Survivors' banish mechanic: https://gmdq.substack.com/p/vampire-survivors-banish-mechanic
34. PCGamesN, Vampire Survivors power-ups: https://www.pcgamesn.com/vampire-survivors/power-ups-best
35. Vampire Survivors Wiki, Evolution: https://vampire-survivors.fandom.com/wiki/Evolution
36. Vampire Survivors Wiki, Treasure Chest: https://vampire.survivors.wiki/w/Chest
37. PC Gamer, Diablo 3 2.0.1 patch will remove Paragon level cap: https://www.pcgamer.com/diablo-3-paragon-reaper-souls/
38. Diablo Wiki, Paragon: https://diablowiki.net/Paragon
39. Dwarf Fortress Wiki, DF2014:World gen: https://dwarffortresswiki.org/index.php/DF2014:World_gen
40. Dwarf Fortress Wiki, World: https://www.dwarffortresswiki.org/index.php/World
41. Dwarf Fortress Wiki, Legend: https://dwarffortresswiki.org/index.php/Legend
42. Paradox, CK3 Dev Diary #30: Event Scripting: https://admin-forum.paradoxplaza.com/forum/developer-diary/crusader-kings-3-dev-diary-30-event-scripting.1397140/post-26642903
43. Paradox, CK3 Dev Diary #31: A Stressful Situation: https://admin-forum.paradoxplaza.com/forum/developer-diary/ck3-dev-diary-31-a-stressful-situation.1399764/page-2
44. PlayerAssist, Crusader Kings 3: What is Stress: https://playerassist.com/crusader-kings-3-stress/
45. Wikipedia, Middle-earth: Shadow of Mordor: https://en.wikipedia.org/wiki/Middle-earth:_Shadow_of_Mordor
46. Edge (via PressReader), Middle-earth: Shadow of Mordor, 23 Oct 2014: https://www.pressreader.com/australia/edge/20141023/281500749509938
47. Unity Discussions, How does Shadow of Mordor's Nemesis System work? (player recollections): https://discussions.unity.com/t/how-does-shadow-of-mordors-nemesis-system-work/663842
48. UFOpaedia, Avatar Project (XCOM2): https://ufopaedia.org/Avatar_Project_(XCOM2)
49. PC Gamer, 9 things you should know before starting XCOM 2: https://www.pcgamer.com/xcom-2-guide
50. Steam Community, XCOM 2 doom timer discussion: https://steamcommunity.com/app/268500/discussions/0/144512753467480955
51. Game Informer, XCOM 2's board game influences and the race against the alien clock: https://gameinformer.com/games/xcom_2/b/pc/archive/2015/12/10/xcom-2s-board-game-influences-and-the-race-against-the-alien-clock
52. Wikipedia, Civilization VI: Rise and Fall: https://en.wikipedia.org/wiki/Civilization_VI:_Rise_and_Fall
53. PC Gamer, Civilization 6: Rise and Fall trailer details Golden and Dark Ages, Emergencies and more: https://www.pcgamer.com/au/civilization-6-rise-and-fall-trailer-details-golden-and-dark-ages-emergencies-and-more
54. GameSpot, Civ 6 Rise and Fall expansion's new features detailed: https://www.gamespot.com/articles/civilization-6-rise-and-fall-expansions-new-featur/1100-6456131
55. CivFanatics, Diplomacy AI Development (Vox Populi), page 8: https://forums.civfanatics.com/threads/diplomacy-ai-development.655040/page-8
