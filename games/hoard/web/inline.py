"""Makes web/hoard-game.html one self-contained file: every <script src="game/NAME.js"></script> or
<script src="vendor/NAME.js"></script> becomes an inline <script> with that file's contents, and
<script type="application/json" id="assets" data-manifest="assets.json"></script> gets {key: base64 GLB} for every
model the manifest lists (paths relative to the page; .gltf files are packed into GLB on the way).
Usage (after build.sh has written the page): python3 web/inline.py web/hoard-game.html"""
import base64
import json
import pathlib
import re
import struct
import sys

page = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else 'web/hoard-game.html')
base = page.parent
html = page.read_text()


def inline(m):
    code = (base / m.group(1)).read_text().replace('</script', '<\\/script')
    return f'<script>\n/* {m.group(1)} */\n{code}\n</script>'


def glb(path):
    if path.suffix == '.glb':
        return path.read_bytes()
    gltf = json.loads(path.read_text(encoding='utf-8'))
    if any('uri' in i for i in gltf.get('images', [])):
        raise SystemExit(f'{path}: images by uri are not packed')
    data, offsets = b'', []
    for b in gltf['buffers']:
        uri = b['uri']
        blob = base64.b64decode(uri.split(',', 1)[1]) if uri.startswith('data:') else (path.parent / uri).read_bytes()
        data += b'\0' * (-len(data) % 4)
        offsets.append(len(data))
        data += blob
    for v in gltf['bufferViews']:
        v['byteOffset'] = v.get('byteOffset', 0) + offsets[v['buffer']]
        v['buffer'] = 0
    gltf['buffers'] = [{'byteLength': len(data)}]
    data += b'\0' * (-len(data) % 4)
    js = json.dumps(gltf, separators=(',', ':')).encode()
    js += b' ' * (-len(js) % 4)
    head = struct.pack('<III', 0x46546C67, 2, 28 + len(js) + len(data))
    return head + struct.pack('<II', len(js), 0x4E4F534A) + js + struct.pack('<II', len(data), 0x004E4942) + data


def embed(m):
    manifest = json.loads((base / m.group(1)).read_text())
    bundle = {key: base64.b64encode(glb(base / rel)).decode() for key, rel in manifest.items()}
    return f'<script type="application/json" id="assets">{json.dumps(bundle)}</script>'


html, n = re.subn(r'<script src="((?:game|vendor)/[\w.-]+\.js)"></script>', inline, html)
html, k = re.subn(r'<script type="application/json" id="assets" data-manifest="([\w.-]+)"></script>', embed, html)
page.write_text(html)
print(page, 'inlined', n, 'scripts, embedded', k, 'asset bundle,', len(html), 'bytes')
