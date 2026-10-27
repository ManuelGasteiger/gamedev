# 14 story sifting: progress

Branch `item/14-story-sifting`, lead's own item.

- [x] bank: `data/patterns/bank.json` (25 patterns over the real kinds), schema and vocab updated, lint passes; `docs/patterns/seed.json` dropped (its kinds did not exist in the sim)
- [x] acceptors `sim/sift/acceptor.cpp` at the end of every step; compounds kept in `World.sift`, out of the chronicle
- [x] thread records, score, headline, tiers (`importance.json` "tiers"), budget, ages block, callbacks, because chain, sources
- [x] briefing: `lines`, `sources`, headline and "while you slept" from the sifter once the bank has run
- [x] goldens unchanged with the bank off (`--golden` and the wasm `golden` command clear the bank before genesis); checked 300 seeds
- [x] tests `sift_acceptor`, `sift_budget`; `sift_bank_v1.hash` added to `test_golden` (written at the end)
- [ ] base rates dry run (`tools/sift_rates.py`), 300-seed metrics (`interest.py`), tuning
- [ ] acceptor cost table, seed 42 acceptance, wasm and page checks, report
