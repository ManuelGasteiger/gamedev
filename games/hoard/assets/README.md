# assets

Third-party packs, sorted by format first, then by what the object is. Blender sources live in `../art/`.

| folder | for | notes |
|---|---|---|
| `gltf/` | browser shell (three.js), Unreal via Interchange | every folder is self-contained (`.gltf` + `.bin` + textures, or `.glb`) |
| `fbx/` | Unreal FBX import | textures in a `textures/` folder next to the FBX files where the pack has them |
| `previews/` | looking things up | pack preview images and per-object renders (fantasy-rts) |
| `licenses/` | attribution | one file per pack |
| `_unused/` | nothing | OBJ/MTL copies and Unity-specific exports; safe to delete (about 330 MB) |

## Used by the valley sandbox

`web/assets.json` lists the models the browser sandbox embeds (game key → file). Add a line there and a fit in `web/game/models.js` to bring in another one.

## Packs

| pack | where | licence | use |
|---|---|---|---|
| Quaternius Ultimate Fantasy RTS (Aug 2022) | `*/settlements/fantasy-rts`, `*/nature/fantasy-rts` (Resource_, Mountain, Rock) | CC0 | settlements per era: FirstAge = eras 0-1, SecondAge = eras 2-3, Level1-3 = wealth; Temple = altar, WallTowers = dug_in, Mine = dwarf holds |
| Quaternius Medieval Village MegaKit (Standard) | `*/settlements/medieval-village` | CC0 | modular walls and roofs for a hero village later |
| Quaternius Stylized Nature MegaKit (Standard, 68 of 116) | `*/nature/stylized-nature` | CC0 | trees, dead trees, pines, rocks, grass |
| Quaternius Ultimate Monsters | `*/creatures/ultimate-monsters/{big,blob,flying}` | CC0 | Orc, Yeti, Demon, Ghost; `flying/Dragon.gltf` is a cute alternative dragon (Fast_Flying, Flying_Idle) |
| Quaternius Modular Character Outfits Fantasy (Standard) | `gltf/characters/modular-outfits-fantasy` | CC0 | peasant and ranger outfits; needs the Universal Base Characters pack for bodies and heads (not downloaded) |
| Quaternius Universal Animation Library 1 and 2 (Standard) | `gltf/animations/*` | CC0 | humanoid clips; `_RM` files have root motion baked in; UAL2 includes a female mannequin |
| MattBas Simple 3D Dragon | `gltf/creatures/dragon-placeholder`, `../art/dragon-placeholder` | CC BY-SA 4.0 | sandbox placeholder dragon (Flying-loop, Idle-loop, Run-loop, Walk-loop); share-alike, credit required, never in a shipped build |
| redragon.blend | `../art/redragon` | unknown | licence not recorded; note the source before using it anywhere public |
