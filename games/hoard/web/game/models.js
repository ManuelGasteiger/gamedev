// Models from the asset bundle (web/assets.json, embedded by web/inline.py). Parsed once before the scene is built,
// normalised to a target size with the base at y = 0, materials turned into flat Lambert to match the valley.
// Settlements place them with placeModel; flora instances their parts (web/game/terrain.js).

const MODEL_FIT = {   // [w = largest footprint side | h = height, size in world units]
  'house.0.a': ['w', 0.5], 'house.0.b': ['w', 0.55], 'house.1.a': ['w', 0.6], 'house.1.b': ['w', 0.55],
  'house.2.a': ['w', 0.55], 'house.2.b': ['w', 0.7], 'house.3.a': ['w', 0.65],
  'centre.0': ['w', 0.95], 'centre.1': ['w', 0.95], 'centre.2': ['w', 1.05],
  'tower.1': ['h', 1.0], 'tower.3': ['h', 1.1], wall: ['w', 1.0], gate: ['w', 1.15], temple: ['h', 1.35], mine: ['w', 0.85],
  'tree.a': ['h', 1.0], 'tree.b': ['h', 0.95], pine: ['h', 1.05], 'rock.a': ['w', 0.45], 'rock.b': ['w', 0.42], 'rock.c': ['w', 0.24],
};
const ACCENT_MATERIAL = 'Main';   // the RTS pack's red roofs and banners take the owner's colour

const prefabs = {};
let dragonGltf = null;

function assetBytes(key) {
  const b64 = ASSET_BUNDLE[key];
  if (!b64) return null;
  const bin = atob(b64), bytes = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i);
  return bytes;
}
const ASSET_BUNDLE = JSON.parse(document.getElementById('assets')?.textContent || '{}');

async function loadModels() {
  const loader = new THREE.GLTFLoader();
  for (const key of Object.keys(ASSET_BUNDLE)) {
    try {
      const gltf = await loader.parseAsync(assetBytes(key).buffer, '');
      if (key === 'dragon') dragonGltf = gltf;
      else prefabs[key] = prefab(gltf.scene, MODEL_FIT[key]);
    } catch (e) { console.error('model ' + key, e); }
  }
}

function prefab(root, fit) {
  root.updateMatrixWorld(true);
  const box = new THREE.Box3().setFromObject(root), size = box.getSize(new THREE.Vector3()), centre = box.getCenter(new THREE.Vector3());
  const k = fit ? fit[1] / (fit[0] === 'h' ? size.y : Math.max(size.x, size.z)) : 1;
  const norm = new THREE.Matrix4().makeScale(k, k, k).multiply(new THREE.Matrix4().makeTranslation(-centre.x, -box.min.y, -centre.z));
  const parts = [];
  root.traverse((o) => {
    if (!o.isMesh) return;
    const geometry = o.geometry.clone().applyMatrix4(new THREE.Matrix4().multiplyMatrices(norm, o.matrixWorld));
    const material = new THREE.MeshLambertMaterial({ color: o.material.color, flatShading: true });
    material.name = o.material.name;
    parts.push({ geometry, material });
  });
  return { parts, size: size.multiplyScalar(k) };
}

// one material per (source material, burn step, owner accent); burn darkens towards char like the old boxes did
const tintCache = new Map();
function tintedMaterial(mat, burn, accent) {
  const step = Math.round(Math.min(1, burn) * 20), acc = accent && mat.name === ACCENT_MATERIAL ? accent.getHexString() : '';
  const key = `${mat.uuid}|${step}|${acc}`;
  let m = tintCache.get(key);
  if (!m) {
    m = mat.clone();
    if (acc) m.color.copy(accent);
    m.color.lerp(new THREE.Color(PALETTE.char), Math.min(1, (step / 20) * 1.1));
    tintCache.set(key, m);
  }
  return m;
}

// a model standing on the old tile-top convention (y = 0.5 in a region group); null when the bundle lacks it
function placeModel(group, key, x, z, { rot = 0, scale = 1, burn = 0, accent = null } = {}) {
  const p = prefabs[key];
  if (!p) return null;
  const g = new THREE.Group();
  for (const part of p.parts) {
    const mesh = new THREE.Mesh(part.geometry, tintedMaterial(part.material, burn, accent));
    mesh.castShadow = true;
    g.add(mesh);
  }
  g.position.set(x, 0.5, z);
  g.rotation.y = rot;
  g.scale.setScalar(scale);
  group.add(g);
  return g;
}
