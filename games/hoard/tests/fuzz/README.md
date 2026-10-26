# fuzz targets (next item's brief, no code yet)

libFuzzer with `-fsanitize=fuzzer,address,undefined` on Clang; 60 s per target per pull request, 15 to 30 minutes nightly (`docs/research-determinism-ci.md` §5).

1. **Act JSON**: bytes to `acts_from_json`, then `apply_acts` on a cached seed-42 world. Oracle: no crash or UB; the same input applied to two copies gives the same state hash.
2. **Input sequence**: bytes through `FuzzedDataProvider` into valid inputs of the recorded verbs (`sim/ages/recorded.cpp`) with live target ids, then `sleep`. Oracle: finite values, unique ids, non-negative wealth; `save_json`, `load_save`, `save_json` is idempotent; recording and replaying the sequence passes every checkpoint.
3. **Save and replay loader**: bytes to `load_save` and to `replay_lines`. Oracle: no crash, a clean refusal.
4. **Differential** (a script): replay the corpus on Clang, MSVC and wasm and compare hashes.
