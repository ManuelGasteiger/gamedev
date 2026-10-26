# Asset licences

One row per pack or file that enters the repo. Licence texts as shipped with each pack are in `licenses/`. Add a row the moment a file is added.

| asset | where | licence | obligation | status |
|---|---|---|---|---|
| Quaternius Ultimate Fantasy RTS | `*/settlements/fantasy-rts`, `*/nature/fantasy-rts`, `art/packs/fantasy-rts` | CC0 1.0 (`licenses/fantasy-rts.txt`) | none | ok |
| Quaternius Medieval Village MegaKit (Standard) | `*/settlements/medieval-village` | CC0 1.0 | none | ok |
| Quaternius Stylized Nature MegaKit (Standard) | `*/nature/stylized-nature` | CC0 1.0 | none | ok |
| Quaternius Ultimate Monsters | `*/creatures/ultimate-monsters`, `art/packs/ultimate-monsters` | CC0 1.0 | none | ok |
| Quaternius Modular Character Outfits Fantasy (Standard) | `gltf/characters/modular-outfits-fantasy` | CC0 1.0 | none | ok |
| Quaternius Universal Animation Library 1 and 2 (Standard) | `gltf/animations/*`, `art/packs/universal-animation-library-2` | CC0 1.0 | none | ok |
| MattBas Simple 3D Dragon | `gltf/creatures/dragon-placeholder`, `art/dragon-placeholder` | CC BY-SA 4.0 (`licenses/dragon-placeholder.txt`, source https://dragon.best/3d/) | credit, change notice, modified files under BY-SA, no technological measures | **placeholder only, never in an outside build**; replace with the CC0 Quaternius dragon before any tester build; never use as input for the hero dragon |
| redragon.blend | `art/redragon` | unknown | unknown | **licence unknown**: record the source before any use |
| three.js r158 | `web/vendor/three.r158.min.js`, `gltf-loader.r158.js` | MIT (`web/vendor/three.LICENSE`) | notice in shipped web builds | ok |
| nlohmann/json 3.12.0 | `sim/ages/json.h` | MIT | notice in every shipped build | ok |

Quaternius pack pages read CC0 on 2026-10-24; later releases use the Quaternius Asset License (`research-licensing-naming.md` §1). Save each pack page as a dated PDF in `_licence-proof/` when convenient.
