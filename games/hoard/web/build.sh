#!/usr/bin/env bash
# Builds web/ages.wasm with wasi-sdk, web/ages-sandbox.html from sandbox.template.html and web/hoard-game.html from game.template.html.
# `build.sh --html` skips the wasm compile and only re-embeds the existing web/ages.wasm into both pages; `--wasm` only compiles.
set -euo pipefail
cd "$(dirname "$0")/.."
WASI_SDK="${WASI_SDK:-/tmp/wasi}"
CXX="$WASI_SDK/bin/clang++"
SYSROOT="$WASI_SDK/share/wasi-sysroot"

if [ "${1:-}" != "--html" ]; then
  SOURCES=$(ls sim/core/*.cpp sim/ages/*.cpp sim/ages/rules/*.cpp sim/sift/*.cpp sim/text/*.cpp | grep -v -e data_io.cpp -e bundle_io.cpp)
  "$CXX" --target=wasm32-wasi --sysroot="$SYSROOT" -std=c++17 -O2 -ffp-contract=off -fno-fast-math -DHOARD_BUILD="\"$(git rev-parse --short HEAD 2>/dev/null || echo dev)\"" \
    -fno-exceptions -DJSON_NOEXCEPTION -mexec-model=reactor \
    -Isim \
    -Wl,--export=ages_cmd -Wl,--export=ages_alloc -Wl,--export=ages_free \
    -Wl,--strip-all \
    $SOURCES sim/wasm/api.cpp sim/wasm/text_api.cpp -o web/ages.wasm
fi
[ "${1:-}" = "--wasm" ] && exit 0

python3 - <<'EOF'
import base64, json, pathlib
root = pathlib.Path('.')
wasm = base64.b64encode((root / 'web/ages.wasm').read_bytes()).decode()
data = {k: (root / f'data/{k}.json').read_text() for k in ['constants', 'races', 'reactions', 'regions', 'factions', 'shocks', 'names', 'offers']}
data.update({k: (root / f'data/{k}.json').read_text() for k in ['talents', 'defences']})   # v0.4 (S1)
data['hooks'] = (root / 'data/hooks.json').read_text()   # v0.4 hooks (S3)
data['arrivals'] = (root / 'data/arrivals.json').read_text()   # item 06
data['moves'] = (root / 'data/moves.json').read_text()   # item 07
data['text'] = (root / 'data/text/en/narrator.json').read_text()
data['importance'] = (root / 'data/importance.json').read_text()
# item 19: one bundle per language as sim/text/bundle_io.cpp merges it
data['text_bundles'] = {d.name: json.dumps({'lang': d.name, **{f.name[:-5]: json.loads(f.read_text()) for f in sorted(d.glob('*.json'))}}, ensure_ascii=False)
                        for d in sorted((root / 'data/text').iterdir()) if d.is_dir()}
# the event cards, joined in file-name order into one JSON array as data_io.cpp does
data['events'] = '[' + ',\n'.join(p.read_text() for p in sorted((root / 'data/events').glob('*.json'))) + ']'
data['genesis'] = (root / 'data/genesis.json').read_text()   # v0.4 world (S2)
# item 14: the pattern bank joined as data_io.cpp does, and the dry-run base rates
data['patterns'] = '[' + ',\n'.join(p.read_text() for p in sorted((root / 'data/patterns').glob('*.json')) if p.name != 'base_rates.json') + ']'
data['base_rates'] = (root / 'data/patterns/base_rates.json').read_text()
print('web/ages.wasm', (root / 'web/ages.wasm').stat().st_size, 'bytes')
for template, out in [('web/sandbox.template.html', 'web/ages-sandbox.html'), ('web/game.template.html', 'web/hoard-game.html')]:
    html = (root / template).read_text()
    html = html.replace('%%WASM_BASE64%%', wasm).replace('%%DATA_JSON%%', json.dumps(data))
    html = html.replace('%%THREE_JS%%', (root / 'web/vendor/three.r158.min.js').read_text().replace('</script', '<\\/script'))
    (root / out).write_text(html)
    print(out, len(html), 'bytes')
EOF
python3 web/inline.py web/hoard-game.html
