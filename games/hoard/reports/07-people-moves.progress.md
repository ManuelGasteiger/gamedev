# 07 people moves: progress

Branch `item/07-people-moves` from origin/main 7d8c372, pushed.

## Done

- State, serialise, moves.json and loader, person record, moves, rules (person memory, secrets, inquisition), cards, text, tests `move_scores` and `princess_chain`, interest metrics.
- Tuning: witness courts hear of acts (told), court threshold 3, trait flip importance 0.4 (not major).
- Golden commit 0cef692 (rules 8): chronicles, chronicle hashes, sift_bank_v1, replays, save expectation.
- Baseline interest (main, played, 300 seeds) in scratchpad `interest-before.txt`; branch run before tuning `i07-after.txt`.

## Next

- `state_hashes.txt` (300 seeds) in its own golden commit.
- wasm build (`WASI_SDK=/root/wasi-sdk bash web/build.sh`, LFS gltf pulled), page golden commit.
- Checks: ctest -L fast, smoke --golden 1-60, l10n-check pseudo,de, game-check.
- Final 300-seed interest run, report `reports/07-people-moves.md`.
