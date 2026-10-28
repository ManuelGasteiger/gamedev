# Storylets and quality-based narrative: a model for Hoard's scenario bank

Excerpt for items 04 and 05: the "For Hoard" parts of sections 3 and 4, verbatim. The full text with section 8 (the scenario schema, selection and validation) and the sources is in the Project (`claude/research-storylets.md`); item 10 copies it whole.

## 3. King of Dragon Pass and Six Ages

**For Hoard**
- 400 to 600 scenes with a quarter seen per run set the order of magnitude.
- Role binders as named functions: `strongest_army(neighbours(court))`, `worst_relations(dragon)`, `most_recent_act(type, court)`.
- A `suppress: {tag, years}` outcome and a closed tag vocabulary checked by the validator.
- Briefing voices (historian, hostages, consort, priest) follow Six Ages' advice rules: a priority line tied to this scenario or the speaker's court, two fallback levels, no two voices making the same point, insight gated by skill (the historian's Lore gates the "because" line; a hostage knows only her own court). Bias comes from the speaker's court and memory, and the recommendation must match the argument. The artifact with an agenda may leave things out, though it never states anything the sim contradicts.
- Oracle analog: an optional "consult" on any scenario, paid from the knowledge pool, showing the sign of each option's reaction row and who would witness it.

## 4. Wildermyth: casting, history, legacy

**For Hoard**
- Tier decides precedence, `weight` decides odds inside the tier.
- Roles bind by tags with `forbidden` and `relative_to` (a person whose memory includes act `a`); `choose` names a score function.
- A person's memory of the dragon is Hoard's history line: outcomes write it, later scenarios read its tags.
- Across runs only knowledge carries: legend aspects unlocked per profile that change the next world's pre-history and which scenarios exist. Never power.
- Tight against loose casting becomes a dry-run metric with a target band.
