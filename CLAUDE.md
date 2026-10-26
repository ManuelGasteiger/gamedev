# gamedev

Workspace for Manu's games. Shared skills and tools here, one folder per game under `games/`. Each game has its own `CLAUDE.md`; read it before working on that game.

## Layout

- `.claude/skills/` process skills. Read the matching one before asset, VFX, animation, audio or visual review work.
- `docs/` suite notes. `tool-setup.md` says what is installed and how.
- `tools/` shared scripts, one job per file.
- `reference/` read-only copies of other people's repos. Never edit, never import from it at runtime. Copy what you need into `tools/` and adapt it there.
- `scratch/` throwaway output. Not committed.

## Rules

- Never guess. Find the documentation or say plainly that you are not sure.
- Paid generation: ask before the first paid job of a batch and state the expected cost. Save the job receipt (endpoint, request id, inputs) under the game's `sources/` before polling. Never resubmit a paid job after a timeout; resume it.
- Approvals are separate steps: concept, mesh, export, engine integration. A successful API call or a nice render is not a game asset.
- One agent drives an editor at a time. Unreal MCP runs tool calls serially on the game thread; never send overlapping calls.
- Verify visual changes in the engine with comparable OFF/ON captures (`gameplay-visual-review`). Keep the captures with the game's docs.
- Code is for programmers: no file-header essays, few comments, one job per file.
- Tests: Manu defines the test list. Do not add tests he did not ask for.
- Docs and notes: plain and brief.
