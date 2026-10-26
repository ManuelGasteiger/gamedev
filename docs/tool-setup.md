# Tool setup

Status 2026-10-12, from `setup-report.txt` (run `tools/check-setup.ps1` again after installing anything).

## This machine

| Item | State |
|---|---|
| CPU, RAM | Intel i7-6700T (4 cores, 8 threads, low-power part), 32 GB |
| GPU | NVIDIA GeForce GTX 980, 4 GB VRAM, driver 582.28 |
| Disks | C: 27 GB free, D: 492 GB free. Everything big goes on D: |
| Unreal | 5.8 at `D:\EpicGames\UE_5.8`, Epic Games Launcher installed. The registry also lists an old "4.0" entry; ignore it |
| Visual Studio | 2022 Community 17.14 and 2026 Community 18.7, both without any C++ toolset |
| Windows SDK | 10.0.26100.0 |
| Blender | 5.1.2, MCP add-on running in the Claude desktop app |
| Git, Git LFS, uv, Python 3.10, Node 24, ffmpeg | installed |
| Claude Code | 2.1.89 (`claude update` brings it current) |
| Repository | `D:\gamedev` is a git repo, no commits yet |

## The GPU is below Unreal 5.8's spec for Lumen

Epic's 5.8 hardware page lists Lumen (global illumination, reflections, MegaLights) for NVIDIA RTX 2000 series or newer, AMD RX 6000 or newer, Intel Arc; it recommends 8 GB of VRAM or more. The GTX 980 is a 2014 Maxwell card with 4 GB. Nanite and Virtual Shadow Maps need DirectX 12 with Shader Model 6.6 atomics; whether the GTX 980 meets that is not confirmed. NVIDIA's 580 driver branch is the last one for Maxwell, Pascal and Volta.

What it means for Hoard: the editor runs, C++ work and the fire-grid port are fine, but the fire lighting the village (the second judgement of the bake-off) cannot be evaluated on this card. Decide on a GPU upgrade or a rented GPU machine for captures before the bake-off.

## Still to install

1. **C++ toolchain.** Visual Studio Installer > Visual Studio 2022 Community > Modify: workload "Game development with C++" with "IDE support for Unreal Engine", plus the individual component "MSVC v143 - VS 2022 C++ x64/x86 build tools (v14.44-17.14)". This installs on C: (27 GB free is enough for one workload, not much more).
   Epic's 5.8 page now says to use Visual Studio 2026 for general development, while the 5.8 release notes name VS 2022 17.14 with MSVC 14.44.35207, and one forum report shows UnrealBuildTool asking for VS 2022 with only 2026 installed. The VS 2022 toolset is the safe choice; VS 2026 can stay as the editor.
2. `claude update`.
3. Later, by milestone: EmberGen (bake-off), Wwise (fire milestone), ZibraVDB (set pieces), Auto-Rig Pro (dragon). Generation keys go in `.env`.

## Connecting Unreal to Claude

### Unreal side, once per project

1. Create the project: Unreal 5.8 > Games > Blank, location `D:\gamedev\games\hoard`, name `Hoard`, no starter content. Blueprint is fine until the C++ toolchain is in; Tools > New C++ Class turns it into a C++ project later.
2. Edit > Plugins: enable **Unreal MCP**, **All Toolsets** (Toolset Registry comes with them) and **Python Editor Script Plugin**. Restart.
3. Edit > Editor Preferences > General > Model Context Protocol: tick **Auto Start Server**. Restart, or type `ModelContextProtocol.StartServer` in the console.
4. Output Log, filter `LogModelContextProtocol`: the server reports `127.0.0.1:8000/mcp`. If port 8000 is taken, change it in the same preferences panel and in `.mcp.json`.

### Claude Code on this PC (main route)

```
cd D:\gamedev
claude
```

Approve the two servers from `.mcp.json` when asked, check `/mcp` (unreal-mcp connected), then ask it to list the Unreal toolsets. Start the editor before Claude Code, or reconnect from `/mcp` afterwards. This route can also edit the C++ files and run builds.

### The Claude app (optional)

The Claude desktop app forwards its local MCP servers into chats, the way Blender already shows up. Unreal's server only speaks HTTP, so it needs the `mcp-remote` bridge. Settings > Developer > Edit Config, then add inside the existing `mcpServers` object:

```json
"unreal": {
  "command": "npx",
  "args": ["-y", "mcp-remote", "http://127.0.0.1:8000/mcp", "--allow-http", "--transport", "http-only"]
}
```

Restart the desktop app with the editor already running. Untested here: mcp-remote's documentation does not say how it behaves with a server that has no authentication. Use one client at a time; Unreal runs tool calls one by one on the game thread.

`blender` in `.mcp.json` runs Blender Lab's MCP through uv (`projects.blender.org/lab/blender_mcp`, subdirectory `mcp`), not the unrelated PyPI package. The desktop app already runs one; whether the add-on accepts two clients at once is untested.
