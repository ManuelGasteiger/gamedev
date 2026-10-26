---
name: unreal-editor
description: Drive and extend the Unreal Editor 5.8 through Epic's Unreal MCP plugin, build the C++ project from the command line, and avoid the known setup traps. Use before any Unreal editor, build or MCP-toolset work.
---

# Unreal editor

Source: Epic's "Unreal MCP in Unreal Editor" page for 5.8 (dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor). The plugin is Experimental; check the page again when something here stops matching.

## Connection

- Plugins: `Unreal MCP` and `All Toolsets` (both pull in `Toolset Registry`). Restart the editor after enabling.
- Editor Preferences > General > Model Context Protocol > Auto Start Server. Default endpoint `http://127.0.0.1:8000/mcp`, Streamable HTTP. The root `.mcp.json` already points there.
- Console: `ModelContextProtocol.StartServer [port]`, `ModelContextProtocol.StopServer`, `ModelContextProtocol.RefreshTools`.
- Loopback only, no authentication. Do not expose it.
- Start Claude Code after the editor is up, or reconnect the server once the editor is running.

## Calling tools

- Tool Search is on by default: the server lists `list_toolsets`, `describe_toolset` and `call_tool`. List toolsets, describe the one you need, then call.
- Calls run on the game thread one at a time. Never send overlapping calls, and only one agent uses the editor at a time.
- If a tool is missing or returns the wrong shape: Output Log category `LogModelContextProtocol` (`Log LogModelContextProtocol Verbose` raises it), or point the MCP Inspector (`npx @modelcontextprotocol/inspector`) at the endpoint.

## Adding project tools

- Python: a class deriving `unreal.ToolsetDefinition`, decorated `@unreal.uclass()`, in a plugin's `Content/Python/`. Each tool is a `@staticmethod` with `@toolset_registry.tool_call`, type hints and a Google-style docstring (`Args:`, `Returns:`); those become the schema.
- C++: derive `UToolsetDefinition`, `UCLASS(BlueprintType, Hidden)`, static `UFUNCTION(meta = (AICallable))`. Doc comments become the schema.
- After authoring run `ModelContextProtocol.RefreshTools` and reconnect the client. A new `UFUNCTION` needs an editor restart; Live Coding only picks up changed bodies.
- Prefer small tools with structured return types over free-form strings.

## Building C++ from the command line

```
"<UE_5.8>\Engine\Build\BatchFiles\Build.bat" HoardEditor Win64 Development -Project="<path>\Hoard.uproject" -WaitMutex
```

Live Coding blocks outside builds while the editor runs. Either compile in the editor (Ctrl+Alt+F11) or close the editor first.

Toolchain: the 5.8 release notes name Visual Studio 2022 17.14 with MSVC 14.44.35207 and Windows SDK 10.0.22621.0; Epic's 5.8 hardware page says to use Visual Studio 2026. This machine builds with the VS 2022 toolset (a forum report shows UnrealBuildTool asking for VS 2022 when only 2026 was installed).

## Known traps

- Niagara Fluids is a Beta plugin. New 5.8 projects start at Medium engine scalability, where Niagara Fluids does not render. Set scalability to Epic before judging a fluid.
- Blueprints are binary. Keep logic in C++ and read Blueprints only through editor tools.
