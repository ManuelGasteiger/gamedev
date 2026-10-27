# German style sheet (draft)

Status: draft from item 19, before Manu's read of the 30 seeds. His corrections replace this page, the lexicon and the gold examples (`docs/style-de-gold.json`, empty until then). `tools/bank/translate.py` pastes this page into every German pass.

## Rules

- The player is du. Courts and people never address the dragon as Ihr unless a scenario says so.
- Briefing, scenario and option text in the Präsens; chronicle lines in the Präteritum ("verbrannte der Drache die Bibliothek").
- Quotes „…“, inner ‚…‘. No guillemets.
- Option labels are infinitive phrases without a full stop ("Den Gesandten fressen").
- Generated names stay as generated: no upper-casing, genitive only through `{x.name:gen}`; prefer "von X" in briefing text.
- Person nouns only through `{role.noun:...}`; items only through `{role.name:...}`, `{role.subj}`, `{role.obj}`.
- A cause clause is one unit: ", weil du … verbrannt hast" (verb last) or ", denn du hast … verbrannt" (verb second); never split.
- Numbers with a unit through the unit token: `vor {a.years_ago:year.dat}`.
- No modern idiom, no anglicisms where German has a plain word.

## Term base (seed, about 40 of the planned 100)

| English | German | note |
|---|---|---|
| hoard | Hort | also the lair as a place: "in deinem Hort" |
| lair | Hort / Höhle | Höhle for the cave itself |
| the long sleep | der lange Schlaf | |
| waking | Erwachen | plural Erwachen ("zwei Erwachen") |
| briefing | Bericht beim Erwachen | |
| court | Hof | the polity; "Feste" for a dwarf hold |
| hold (dwarf) | Feste | |
| envoy | Gesandter / Gesandte | adjectival noun |
| hostage | Geisel | feminine for every hostage |
| heir | Erbe / Erbin | |
| sage | Weiser / Weise | adjectival noun |
| elder | Ältester / Älteste | adjectival noun |
| kin | Verwandter / Verwandte | adjectival noun |
| slayer | Drachentöter / Drachentöterin | |
| slayer order | Drachentöterorden, der Orden | |
| priest | Priester / Priesterin | |
| prophet | Prophet / Prophetin | weak masculine |
| consort | Gefährte / Gefährtin | |
| ledge | Felsvorsprung | |
| pass | Pass | |
| hook | Aufhänger | |
| omen | Vorzeichen | |
| oath | Eid | |
| tribute | Tribut | |
| relic | Relikt | |
| artifact | Artefakt | |
| cannon | Kanone | |
| engine (siege) | Maschine | |
| bolt-thrower | Bolzengeschütz | |
| ward | Schutz | |
| curse | Fluch | |
| bless | segnen | |
| moult | Häutung | |
| hatchling | Schlüpfling | not yet used in text |
| wasteland | Ödland | |
| remnant | Versprengte | |
| cairn | Steinmal | |
| chronicle | Chronik | |
| Endings (page) | „Enden“ | decided 2026-10-26 |
| knowledge | Wissen | |
| magic | Magie | |
