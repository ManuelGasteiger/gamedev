# Procedural Storytelling in Game Design (Short & Adams, 2019), mapped to Hoard

Excerpt for items 04, 05 and 07: the per-chapter "For Hoard" notes the briefs cite and the synthesis of section 3, verbatim. The chapter table, findings and 47 sources are in the Project (`claude/research-book-procedural-storytelling.md`).

## 2. Chapter notes the briefs cite

**Ch 13, Adams (Dwarf Fortress).** Strong events enter a short-term memory where they can be re-felt, then move to permanent memory, which can change personality, and the change is logged.
For Hoard: Give each named person a memory of the dragon in two layers (section 3.3). Treat monuments as engravings: each depicts one dragon act, stands in a region, and can be visited or burned.

**Ch 14, Rouse (Church in the Darkness).** Each run re-rolls the cult leaders' stance, from pacifist to militant, and how apocalyptic they are; setting, doctrine and voices stay authored.
For Hoard: The hatching prophecy already works like the leader re-roll, one per-seed variable that reframes the same bank. Add a second per-seed variable, the dragon cult's doctrinal temper (mercy to apocalypse), that scenarios can use as a precondition.

**Ch 15, Grinblat (Caves of Qud).** The event comes first; its stated cause is then picked from the sultan's properties, and if nothing fits, the system adds a property that makes it fit. Conflicting gospels told from different perspectives were listed as future work.
For Hoard: Hoard simulates real causes, so retro-rationalisation belongs in the biased sources, which is the "different perspectives" work Grinblat left open. See section 3.2. Give each dynasty one to three domains that recur in its texts, and have every artifact carry the event that produced it.

**Ch 16, Dias (Improv).** Phrases carry hierarchical tags; a mismatch filter drops any phrase whose tag conflicts with a recorded tag in the same category; salience filters choose among the rest.
For Hoard: Use the same three mechanisms for briefing text (section 3.7).

**Ch 17, Lazer-Walker.** An algorithm people cannot follow reads to them as random.
For Hoard: Limit each chronicle "because" to one or two causes drawn from a small, stable vocabulary, so players can learn to read it.

**Ch 21, Short (personalities).** Make the causal direction clear, push traits past realistic averages, prefer active traits, support after-the-fact investigation, reactions beat actions, use transformation sparingly.
For Hoard: The basic briefing beat is one dragon act plus the contrasting reactions of two or three named people. Traits should be extreme and active (a Vengeful heir acts, a Craven one flees). The historian and Lore are the investigation tools. Transformations come from permanent memory (section 3.3).

**Ch 24, Kim (The Shrouded Isle).** Each villager rolls one virtue and one vice; a hidden trait turns an accusation into a clue that pays off later.
For Hoard: Give each named mortal exactly one virtue and one vice. Each modifies one sim stat and gates scenario casting. The vice stays hidden until a source, the historian or Lore reveals it.

## 3. Synthesis: the ideas that matter most for Hoard

**3.2 Perspective rationalisation for sources (ch 15, 2).** The sim records the true cause. Each court writing about an event picks its stated cause from its own properties, and may invent one, following Grinblat's add-a-property fallback.

```json
{"event_id": 4412, "author_court": "elves_vael", "year": 1210,
 "true_cause": "dragon_burned_harbor", "stated_cause": "dwarven_treachery",
 "cause_from": "court.grudges[0]", "medium": "written", "lost_if": "library_burned"}
```

Pipeline step: generate the source when the event resolves (deterministic from seed and event id), file it in the author court's library, and destroy it with the library. Looted sources enter the hoard as Lore. The historian's job is to compare true_cause with stated_cause.

**3.3 Two-layer memory with transformation (ch 13, 21).** Per named person: `recent[]` holds entries {act, intensity, year} and decays one tier per generation (oral). `permanent[]` takes entries whose intensity reaches a threshold (proposal: 60 of 100) and passes to heirs when the court has written or monumental memory. A new permanent entry may flip one trait (proposal: probability scaled by intensity). The flip writes a chronicle line, which is Adams's logged personality change and Short's transformation in one.

**3.4 Reactions over actions (ch 21, 24).** The core briefing beat is one dragon act plus two or three named reactions, chosen so the reactors' virtues and vices differ. Each person carries one virtue and one vice, both extreme and active, each tied to one sim stat and to scenario casting.

**3.5 Two meters and escalation ladders (ch 19).** Dread and awe are independent meters per court, and both can be high. Demands are ordered ladders (tribute → oath → hostage → heir as consort), and each rung unlocks only after the previous one held for some span of years. Every hard turn is preceded by at least two warning lines in earlier briefings.

**3.6 Count master stories, and test at scale (ch 1, 6, 7, 28).** Players carry knowledge between runs, which is exactly the condition that wore out Zinderneuf. Proposed bank target: distinct causal templates at least twice the number a player sees across the expected runs to finish. The 300-seed dry run should report, per template, its fire rate and the share of seeds where it never fires.

**3.7 Tagged chronicle text with contradiction filtering (ch 16, 17).** Tag every chronicle line in a hierarchy (court/race/era/mood). Drop any line whose tag conflicts with current state or with a line already in this briefing, raise salience for aspects not yet mentioned, and limit "because" clauses to one or two causes from a fixed vocabulary.

**3.8 Monuments as engravings (ch 13).** Monumental memory is a set of world objects, each depicting one dragon act in a region. Finding one reveals how that court remembers the dragon. Burning one lowers the court's memory tier, matching the library rule.
