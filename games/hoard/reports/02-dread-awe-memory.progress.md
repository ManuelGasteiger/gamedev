# 02 progress

Tests (Manu, 2026-10-26): dread_awe_update, memory_tiers, extend reactions.

Docs copied: brief, act-api.md, talents-v2.md, pacing-model.md. research-faith-and-culture, research-social-sim, story-and-endings stay in the Project (numbers are in act-api §6).

## Model

- Truth: `Faction.dread` and `Faction.awe` as `{oral, written}` stocks plus monuments holding their own `{dread, awe}`; `monumental` = clamp01(sum over the court's monuments). Effective `1 − Π(1 − x)`.
- Memory entries stay the record: one per (act, court, axis) with `tier` (highest reached), `strength`, `act` (index in `w.acts`); weight decays at that tier's rate. Faith entries keep v0.4's decay (culture memory_halflife).
- Witnesses derived by the sim: owner 1.0, neighbours 0.5 (Glamour halves neighbours). Shell-supplied witnesses are ignored.
- Old saves: hash verified on the saved JSON, then fear → dread.oral, faith × 0.5 → awe.oral.

## Steps

- [ ] state, reactions.json, deposit, decay, saves
- [ ] every fear read mapped
- [ ] research spend, unlocks
- [ ] monuments and cards
- [ ] talents v2 (nodes 0 to 3) and flight block
- [ ] forecast, briefing, wasm export, narrator rows
- [ ] page and game-check fields
- [ ] tests
- [ ] tuning, bands, sweep
- [ ] goldens
