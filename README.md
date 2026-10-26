# gamedev

Manu's game workspace. Shared skills, tools and notes at the root, one folder per game under `games/`.

| Path | What lives there |
|---|---|
| `CLAUDE.md` | Rules for agents working in this repo |
| `.claude/skills/` | Process skills: 17 from Stefan 3D AI's Mr. Mak pack (MIT, see `LICENSE-mr-mak.txt`) plus `unreal-editor` |
| `.mcp.json` | Editor connections for Claude Code: Unreal MCP (HTTP, port 8000) and Blender |
| `docs/` | Suite notes: tool setup, the Stefan 3D AI pipeline, studies of other runs |
| `tools/` | Shared scripts. `check-setup.ps1` checks this machine, `video-watch/` samples frames from a clip |
| `reference/` | Read-only copies of `mr-mak-workspace` and `zer0-g`. Not committed |
| `scratch/` | Throwaway output (video frames, test renders). Not committed |
| `games/hoard/` | Hoard: docs, art, generation sources, the browser prototype, later the Unreal project |

Start Claude Code from this folder so it picks up `CLAUDE.md`, the skills and `.mcp.json`.

## First-time setup

1. `git init`, `git lfs install`, first commit (see `docs/tool-setup.md`).
2. `powershell -ExecutionPolicy Bypass -File tools\check-setup.ps1` writes `setup-report.txt`.
3. Install what the report and `docs/tool-setup.md` say is missing.
