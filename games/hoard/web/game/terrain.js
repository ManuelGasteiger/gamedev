// The valley as a seeded low-poly heightfield. Regions are warped Voronoi cells around their sites (the hand layout plus
// jitter); relief and colour follow each region's terrain; a mountain rim, a sea and one river frame them. The page asks
// it for heights (heightAt), membership (regionAt), picking (pick), outlines and burn tints. Nothing here touches the sim.

const VALLEY = { half: [36, 34], cell: 0.6, water: 0, rim: 6.8, plateau: [2.3, 3.4], lairPeak: [-1.6, -2.4], detail: 1 };   // detail < 1 thins flora on software GL
const CAMERA_DIR = [Math.SQRT1_2, Math.SQRT1_2];   // the iso camera looks from +x +z; the rim stays low on that side

const GROUND = {
  sand: 0xcdbb8a, seabed: 0x8e8060, rock: 0x7d7369, scarRock: 0x5e566a, basalt: 0x45434a, crater: 0x6a3426,
  alpine: 0x6f8a52, snow: 0xe9eef2, mud: 0x6e6a48, char: 0x1d1a17, ember: 0x8a3a1a, waste: 0x6b6b66,
};

const RELIEF = {
  vale: (n, x, z) => 0.45 + 0.35 * n.fbm(x * 0.16, z * 0.16),
  marches: (n, x, z) => 0.85 + 0.7 * n.fbm(x * 0.2 + 5, z * 0.2),
  wood: (n, x, z) => 0.75 + 0.55 * n.fbm(x * 0.18 - 3, z * 0.18),
  hills: (n, x, z) => 1.3 + 4.4 * Math.pow(n.ridged(x * 0.13, z * 0.13), 2.2),
  scar: (n, x, z) => 0.9 + 1.4 * n.ridged(x * 0.24, z * 0.24) - 1.5 * Math.max(0, 0.12 - Math.abs(n.noise(x * 0.11 + 40, z * 0.11))) / 0.12,
  fen: (n, x, z) => 0.12 + 0.3 * n.fbm(x * 0.3, z * 0.3) - 0.5 * Math.max(0, -n.noise(x * 0.35 + 9, z * 0.35) - 0.15),
  lair: (n, x, z) => 0.9 + 0.5 * n.fbm(x * 0.2, z * 0.2),
  lake: (n, x, z) => -0.6 + 0.3 * n.fbm(x * 0.2 + 17, z * 0.2),   // a basin under the water; a settlement's plateau stands in it as an island
};

// flora per terrain: [kind, instances per square unit, cluster threshold on noise (-1 = everywhere)]
const FLORA = {
  wood: [['tree', 1.0, -0.25], ['pine', 0.15, 0.2]],
  vale: [['tree', 0.5, 0.45], ['bush', 0.08, -1]],
  marches: [['bush', 0.3, -0.1], ['tree', 0.25, 0.4]],
  fen: [['reed', 0.45, -0.2], ['dead', 0.05, -1]],
  hills: [['pine', 0.45, -0.1], ['rock', 0.16, -1]],
  scar: [['rock', 0.3, -1], ['dead', 0.06, -1]],
  lair: [['rock', 0.14, -1], ['dead', 0.03, -1]],
  lake: [['reed', 0.12, 0.35]],
};

const smooth = (a, b, x) => { const t = Math.min(1, Math.max(0, (x - a) / (b - a))); return t * t * (3 - 2 * t); };
const lerp = (a, b, t) => a + (b - a) * t;

function makeNoise(seed) {
  const rnd = seededRandom('noise:' + seed);
  const p = [...Array(256).keys()];
  for (let i = 255; i > 0; i--) { const j = Math.floor(rnd() * (i + 1)); [p[i], p[j]] = [p[j], p[i]]; }
  const perm = new Uint8Array(512), ang = new Float32Array(256);
  for (let i = 0; i < 512; i++) perm[i] = p[i & 255];
  for (let i = 0; i < 256; i++) ang[i] = rnd() * Math.PI * 2;
  const fade = (t) => t * t * t * (t * (t * 6 - 15) + 10);
  function noise(x, z) {
    const ix = Math.floor(x), iz = Math.floor(z), fx = x - ix, fz = z - iz;
    const g = (i, k) => { const a = ang[perm[perm[(ix + i) & 255] + ((iz + k) & 255)]]; return Math.cos(a) * (fx - i) + Math.sin(a) * (fz - k); };
    const u = fade(fx), v = fade(fz);
    const a = lerp(g(0, 0), g(1, 0), u), b = lerp(g(0, 1), g(1, 1), u);
    return lerp(a, b, v) * 1.41;
  }
  const fbm = (x, z, oct = 4) => { let s = 0, a = 0.5, f = 1; for (let i = 0; i < oct; i++) { s += a * noise(x * f, z * f); f *= 2.03; a *= 0.5; } return s; };
  const ridged = (x, z, oct = 4) => { let s = 0, a = 0.5, f = 1; for (let i = 0; i < oct; i++) { s += a * (1 - Math.abs(noise(x * f + 17.3 * i, z * f - 9.1 * i))); f *= 2.1; a *= 0.5; } return s; };
  return { noise, fbm, ridged };
}

function buildValley(seed, regions) {
  const n = makeNoise(seed);
  const rnd = seededRandom('valley:' + seed);
  const sites = regions.map((r) => {
    const [x, z] = toWorld(...(REGION_LAYOUT[r.id] ?? [200, 180]));
    const j = r.lair ? 0.3 : 1.4;
    return { id: r.id, terrain: r.terrain, lair: !!r.lair, x: x + (rnd() - 0.5) * 2 * j, z: z + (rnd() - 0.5) * 2 * j, scale: r.lair ? 0.85 : 1 };
  });
  const lair = sites.find((s) => s.lair);
  const peak = lair ? [lair.x + VALLEY.lairPeak[0], lair.z + VALLEY.lairPeak[1]] : [0, 0];
  const centre = [sites.reduce((s, x) => s + x.x, 0) / sites.length, sites.reduce((s, x) => s + x.z, 0) / sites.length];
  const seaAngle = Math.atan2(0.6, -0.8) + (rnd() - 0.5) * 0.6;
  const seaDir = [Math.cos(seaAngle), Math.sin(seaAngle)];
  const S = sites.length;

  function distances(x, z, out) {
    const qx = x + 1.7 * n.noise(x * 0.09 + 31, z * 0.09), qz = z + 1.7 * n.noise(x * 0.09, z * 0.09 - 57);
    let i1 = 0, d1 = Infinity, d2 = Infinity;
    for (let i = 0; i < S; i++) {
      const d = Math.hypot(qx - sites[i].x, qz - sites[i].z) * sites[i].scale;
      out[i] = d;
      if (d < d1) { d2 = d1; d1 = d; i1 = i; } else if (d < d2) d2 = d;
    }
    return { i1, d1, d2, reach: VALLEY.rim + 1.5 * n.noise(x * 0.07 + 100, z * 0.07) };
  }

  function lairMountain(x, z) {
    const d = Math.hypot(x - peak[0], z - peak[1]), k = Math.max(0, 1 - d / 6.5);
    return 7.5 * Math.pow(k, 1.6) + 0.9 * k * n.ridged(x * 0.3, z * 0.3) - (d < 0.9 ? (0.9 - d) * 1.8 : 0);
  }

  function outsideHeight(x, z) {
    const dx = x - centre[0], dz = z - centre[1], len = Math.hypot(dx, dz) || 1;
    const sea = smooth(0.35, 0.8, (dx * seaDir[0] + dz * seaDir[1]) / len);
    const front = smooth(0.1, 0.8, (dx * CAMERA_DIR[0] + dz * CAMERA_DIR[1]) / len);
    const mountain = (1.2 + 6.5 * Math.pow(n.ridged(x * 0.09, z * 0.09), 1.8)) * (1 - 0.72 * front);
    return { h: lerp(mountain, -3, sea), sea };
  }

  // grid heights before the river and the plateaus
  const nx = Math.round((2 * VALLEY.half[0]) / VALLEY.cell) + 1, nz = Math.round((2 * VALLEY.half[1]) / VALLEY.cell) + 1;
  const N = nx * nz;
  const H = new Float32Array(N), RIM = new Float32Array(N), SEA = new Float32Array(N), NEAR = new Int8Array(N), D = new Float32Array(N * S);
  const vx = (i) => -VALLEY.half[0] + i * VALLEY.cell, vz = (k) => -VALLEY.half[1] + k * VALLEY.cell;
  const d = new Float32Array(S);
  for (let k = 0; k < nz; k++) for (let i = 0; i < nx; i++) {
    const x = vx(i), z = vz(k), v = k * nx + i;
    const m = distances(x, z, d);
    D.set(d, v * S);
    let wsum = 0, inside = 0;
    for (let s = 0; s < S; s++) {
      const w = Math.exp(-(d[s] - m.d1) / 0.9);
      if (w < 0.01) continue;
      wsum += w; inside += w * (RELIEF[sites[s].terrain] ?? RELIEF.vale)(n, x, z);
    }
    inside = inside / wsum + lairMountain(x, z);
    const rim = smooth(m.reach, m.reach + 3.2, m.d1), out = outsideHeight(x, z);
    H[v] = lerp(inside, out.h, rim);
    RIM[v] = rim; SEA[v] = out.sea * rim; NEAR[v] = rim > 0.6 ? -1 : m.i1;
  }

  const riverInfo = { path: [], reason: '' };
  carveRiver();
  for (const s of sites) flatten(s);

  // the river rises in the northern rim, runs through a lake on the sea side of the valley and reaches the sea;
  // each leg is the cheapest path over the terrain, kept away from settlements
  function carveRiver() {
    const toVertex = (x, z) => Math.round((z + VALLEY.half[1]) / VALLEY.cell) * nx + Math.round((x + VALLEY.half[0]) / VALLEY.cell);
    const back = -Math.PI / 2 + (rnd() - 0.5) * 0.8;
    let src = -1;
    for (let r = 4; r < 40 && src < 0; r += 0.3) {
      const x = centre[0] + Math.cos(back) * r, z = centre[1] + Math.sin(back) * r;
      if (Math.abs(x) > VALLEY.half[0] - 1 || Math.abs(z) > VALLEY.half[1] - 1) break;
      const v = toVertex(x, z);
      if (RIM[v] > 0.5 && SEA[v] < 0.1 && H[v] > 2) src = v;
    }
    if (src < 0) return (riverInfo.reason = 'no source');
    const side = (rnd() - 0.5) * 6;
    const lake = [centre[0] + seaDir[0] * 9 - seaDir[1] * side, centre[1] + seaDir[1] * 9 + seaDir[0] * side];
    const near = (v, r) => { const x = vx(v % nx), z = vz(Math.floor(v / nx)); return sites.some((s) => Math.hypot(x - s.x, z - s.z) < (s.lair ? r + 1.1 : r)); };
    const legA = cheapest(src, (v) => v === toVertex(...lake));
    const legB = legA.length ? cheapest(legA[0], (v) => H[v] < -1 && SEA[v] > 0.6) : [];
    if (!legB.length) return (riverInfo.reason = legA.length ? 'no mouth' : 'lake unreachable ' + lake.map((v) => v.toFixed(1)));
    const path = [...legA.reverse(), ...legB.reverse()];
    riverInfo.path = path.map((v) => [vx(v % nx), vz(Math.floor(v / nx))]);
    riverInfo.lake = lake;
    path.forEach((v) => dig(v, 1.3, -0.3));
    if (!near(toVertex(...lake), 4.2)) dig(toVertex(...lake), 2.2, -0.45);

    function cheapest(from, isEnd) {
      const cost = new Float64Array(N).fill(Infinity), prev = new Int32Array(N).fill(-1), heap = [[0, from]];
      cost[from] = 0;
      while (heap.length) {
        const [c, v] = heapPop(heap);
        if (c > cost[v]) continue;
        if (isEnd(v)) { const out = []; for (let u = v; u >= 0; u = prev[u]) out.push(u); return out; }
        const i = v % nx, k = Math.floor(v / nx);
        for (const [di, dk] of [[1, 0], [-1, 0], [0, 1], [0, -1], [1, 1], [1, -1], [-1, 1], [-1, -1]]) {
          const ii = i + di, kk = k + dk;
          if (ii < 0 || kk < 0 || ii >= nx || kk >= nz) continue;
          const u = kk * nx + ii;
          const step = Math.hypot(di, dk) * (1 + 6 * Math.pow(Math.max(0, H[u]), 1.3) + (near(u, 3.4) ? 40 : 0));
          if (c + step < cost[u]) { cost[u] = c + step; prev[u] = v; heapPush(heap, [c + step, u]); }
        }
      }
      return [];
    }
  }

  function dig(v, radius, depth) {
    const pi = v % nx, pk = Math.floor(v / nx), reach = Math.ceil(radius / VALLEY.cell);
    for (let dk = -reach; dk <= reach; dk++) for (let di = -reach; di <= reach; di++) {
      const ii = pi + di, kk = pk + dk, dist = Math.hypot(di, dk) * VALLEY.cell;
      if (ii < 0 || kk < 0 || ii >= nx || kk >= nz || dist > radius) continue;
      const u = kk * nx + ii;
      H[u] = Math.min(H[u], VALLEY.water + depth + (0.9 - depth) * (dist / radius) ** 2);
    }
  }

  function flatten(s) {
    const [r0, r1] = s.lair ? [1.2, 2.0] : VALLEY.plateau;
    const level = Math.max(VALLEY.water + 0.4, heightRaw(s.x, s.z));
    const i0 = Math.max(0, Math.floor((s.x - r1 + VALLEY.half[0]) / VALLEY.cell)), i1 = Math.min(nx - 1, Math.ceil((s.x + r1 + VALLEY.half[0]) / VALLEY.cell));
    const k0 = Math.max(0, Math.floor((s.z - r1 + VALLEY.half[1]) / VALLEY.cell)), k1 = Math.min(nz - 1, Math.ceil((s.z + r1 + VALLEY.half[1]) / VALLEY.cell));
    for (let k = k0; k <= k1; k++) for (let i = i0; i <= i1; i++) {
      const v = k * nx + i, t = 1 - smooth(r0, r1, Math.hypot(vx(i) - s.x, vz(k) - s.z));
      H[v] = lerp(H[v], level, t);
    }
  }

  // exact height of the triangulated surface; the diagonal alternates per cell
  function heightRaw(x, z) {
    const gx = Math.min(nx - 1.001, Math.max(0, (x + VALLEY.half[0]) / VALLEY.cell)), gz = Math.min(nz - 1.001, Math.max(0, (z + VALLEY.half[1]) / VALLEY.cell));
    const i = Math.floor(gx), k = Math.floor(gz), fx = gx - i, fz = gz - k;
    const h00 = H[k * nx + i], h10 = H[k * nx + i + 1], h01 = H[(k + 1) * nx + i], h11 = H[(k + 1) * nx + i + 1];
    if ((i + k) & 1) return fx > fz ? h00 + (h10 - h00) * fx + (h11 - h10) * fz : h00 + (h11 - h01) * fx + (h01 - h00) * fz;
    return fx + fz < 1 ? h00 + (h10 - h00) * fx + (h01 - h00) * fz : h11 + (h01 - h11) * (1 - fx) + (h10 - h11) * (1 - fz);
  }

  function regionAt(x, z) {
    if (Math.abs(x) > VALLEY.half[0] || Math.abs(z) > VALLEY.half[1]) return null;
    const m = distances(x, z, d);
    return m.d1 < m.reach + 1.0 ? sites[m.i1].id : null;
  }

  // faces: two per cell, one colour each
  const faces = (nx - 1) * (nz - 1) * 2;
  const pos = new Float32Array(faces * 9), col = new Float32Array(faces * 9);
  const faceRegion = new Int8Array(faces), faceBase = new Float32Array(faces * 3), faceMask = new Float32Array(faces), faceDist = new Float32Array(faces);
  const byRegion = sites.map(() => []);
  const c = new THREE.Color(), tmp = new THREE.Color();
  let f = 0;
  for (let k = 0; k < nz - 1; k++) for (let i = 0; i < nx - 1; i++) {
    const a = k * nx + i, b = a + 1, cc = a + nx, dd = cc + 1;
    const tris = (i + k) & 1 ? [[a, dd, b], [a, cc, dd]] : [[a, cc, b], [b, cc, dd]];
    for (const tri of tris) {
      let cx = 0, cz = 0, ch = 0, rimT = 0, seaT = 0;
      tri.forEach((v, j) => {
        const x = vx(v % nx), z = vz(Math.floor(v / nx));
        pos[f * 9 + j * 3] = x; pos[f * 9 + j * 3 + 1] = H[v]; pos[f * 9 + j * 3 + 2] = z;
        cx += x / 3; cz += z / 3; ch += H[v] / 3; rimT += RIM[v] / 3; seaT += SEA[v] / 3;
      });
      const ny = faceNormalY(f);
      const near = NEAR[tri[0]];
      faceColour(c, cx, cz, ch, ny, rimT, seaT, tri[0]);
      faceBase.set([c.r, c.g, c.b], f * 3);
      faceRegion[f] = near;
      if (near >= 0) {
        byRegion[near].push(f);
        faceMask[f] = n.fbm(cx * 0.45 + 70, cz * 0.45) * 0.5 + 0.5;
        faceDist[f] = Math.hypot(cx - sites[near].x, cz - sites[near].z) / 7;
      }
      for (let j = 0; j < 3; j++) col.set([c.r, c.g, c.b], f * 9 + j * 3);
      f++;
    }
  }

  function faceNormalY(f) {
    const o = f * 9;
    const ux = pos[o + 3] - pos[o], uy = pos[o + 4] - pos[o + 1], uz = pos[o + 5] - pos[o + 2];
    const wx = pos[o + 6] - pos[o], wy = pos[o + 7] - pos[o + 1], wz = pos[o + 8] - pos[o + 2];
    const nxv = uy * wz - uz * wy, nyv = uz * wx - ux * wz, nzv = ux * wy - uy * wx;
    return Math.abs(nyv) / (Math.hypot(nxv, nyv, nzv) || 1);
  }

  function faceColour(out, x, z, h, ny, rimT, seaT, v) {
    const jitter = 1 + 0.06 * n.noise(x * 1.7 + 11, z * 1.7);
    const dv = D.subarray(v * S, v * S + S);
    let d1 = Infinity;
    for (let s = 0; s < S; s++) d1 = Math.min(d1, dv[s]);
    out.setRGB(0, 0, 0);
    let wsum = 0;
    for (let s = 0; s < S; s++) {
      const w = Math.exp(-(dv[s] - d1) / 0.35);
      if (w < 0.02) continue;
      tmp.setHex(TERRAIN_COLOUR[sites[s].terrain] ?? 0x777777);
      out.r += tmp.r * w; out.g += tmp.g * w; out.b += tmp.b * w; wsum += w;
    }
    out.multiplyScalar(1 / wsum);
    if (rimT > 0.5) out.setHex(GROUND.alpine);
    const lairD = Math.hypot(x - peak[0], z - peak[1]);
    const rocky = ny < 0.72 || (rimT > 0.5 && h > 3.4 + n.noise(x * 0.4, z * 0.4));
    if (rocky) out.setHex(NEAR[v] >= 0 && sites[NEAR[v]].terrain === 'scar' ? GROUND.scarRock : GROUND.rock);
    if (lairD < 5.5 && h > 1.6) out.setHex(GROUND.basalt);
    if (lairD < 1.3 && h > 5) out.setHex(GROUND.crater);
    if (h > 5.4 + 0.8 * n.noise(x * 0.5, z * 0.5) && lairD > 3 && ny > 0.55) out.setHex(GROUND.snow);
    if (h < VALLEY.water + 0.14) out.setHex(h < VALLEY.water - 0.15 ? GROUND.seabed : seaT > 0.05 || rimT > 0.3 ? GROUND.sand : GROUND.mud);
    out.multiplyScalar(jitter);
  }

  const geo = new THREE.BufferGeometry();
  geo.setAttribute('position', new THREE.BufferAttribute(pos, 3));
  geo.setAttribute('color', new THREE.BufferAttribute(col, 3));
  geo.computeVertexNormals();
  const mesh = new THREE.Mesh(geo, new THREE.MeshLambertMaterial({ vertexColors: true, flatShading: true }));
  mesh.castShadow = mesh.receiveShadow = true;

  // burn spreads from the site outward with a noisy front; embers glow on the front while the region burns
  const charC = new THREE.Color(GROUND.char), emberC = new THREE.Color(GROUND.ember), wasteC = new THREE.Color(GROUND.waste);
  function tintRegion(id, burn, waste) {
    const s = sites.findIndex((x) => x.id === id);
    if (s < 0) return;
    for (const fi of byRegion[s]) {
      c.setRGB(faceBase[fi * 3], faceBase[fi * 3 + 1], faceBase[fi * 3 + 2]);
      if (waste) c.lerp(wasteC, 0.8 + 0.2 * faceMask[fi]);
      const ch = Math.min(1, Math.max(0, (burn * 1.6 - faceDist[fi] * 0.9 - faceMask[fi] * 0.6 + 0.15) * 3));
      if (ch > 0) c.lerp(burn > 0.3 && ch < 0.6 ? emberC : charC, ch * 0.9);
      for (let j = 0; j < 3; j++) col.set([c.r, c.g, c.b], fi * 9 + j * 3);
    }
    geo.attributes.color.needsUpdate = true;
  }

  // outline of a region as a ribbon just inside its border (marching squares on the cell's distance margin)
  function outlineGeometry(id, origin, inset = 0.3, width = 0.15) {
    const s = sites.findIndex((x) => x.id === id);
    const field = new Float32Array(N);
    for (let v = 0; v < N; v++) {
      const dv = D.subarray(v * S, v * S + S);
      let other = Infinity;
      for (let j = 0; j < S; j++) if (j !== s) other = Math.min(other, dv[j]);
      const x = vx(v % nx), z = vz(Math.floor(v / nx));
      field[v] = Math.min(other - dv[s], VALLEY.rim + 1.5 * n.noise(x * 0.07 + 100, z * 0.07) + 1.0 - dv[s]) - inset;
    }
    const verts = [];
    const lift = 0.07;
    const edge = (va, vb) => { const t = field[va] / (field[va] - field[vb]); return [lerp(vx(va % nx), vx(vb % nx), t), lerp(vz(Math.floor(va / nx)), vz(Math.floor(vb / nx)), t)]; };
    for (let k = 0; k < nz - 1; k++) for (let i = 0; i < nx - 1; i++) {
      const a = k * nx + i, b = a + 1, cc = a + nx, dd = cc + 1;
      const code = (field[a] > 0 ? 1 : 0) | (field[b] > 0 ? 2 : 0) | (field[dd] > 0 ? 4 : 0) | (field[cc] > 0 ? 8 : 0);
      if (code === 0 || code === 15) continue;
      const e = { ab: () => edge(a, b), bd: () => edge(b, dd), dc: () => edge(dd, cc), ca: () => edge(cc, a) };
      const segs = { 1: [['ca', 'ab']], 2: [['ab', 'bd']], 3: [['ca', 'bd']], 4: [['bd', 'dc']], 5: [['ca', 'ab'], ['bd', 'dc']], 6: [['ab', 'dc']], 7: [['ca', 'dc']],
        8: [['dc', 'ca']], 9: [['dc', 'ab']], 10: [['ab', 'bd'], ['dc', 'ca']], 11: [['dc', 'bd']], 12: [['bd', 'ca']], 13: [['bd', 'ab']], 14: [['ab', 'ca']] }[code];
      for (const [p, q] of segs) {
        const [x0, z0] = e[p](), [x1, z1] = e[q]();
        const len = Math.hypot(x1 - x0, z1 - z0) || 1, ox = (-(z1 - z0) / len) * width / 2, oz = ((x1 - x0) / len) * width / 2;
        const pts = [[x0 - ox, z0 - oz], [x0 + ox, z0 + oz], [x1 + ox, z1 + oz], [x1 - ox, z1 - oz]].map(([x, z]) => [x - origin.x, Math.max(VALLEY.water, heightRaw(x, z)) + lift - origin.y, z - origin.z]);
        verts.push(...pts[0], ...pts[1], ...pts[2], ...pts[0], ...pts[2], ...pts[3]);
      }
    }
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.Float32BufferAttribute(verts, 3));
    return g;
  }

  // the camera is orthographic, so a pick is a straight march along the view ray down to the surface
  function pick(origin, dir) {
    const o = origin.clone(), step = dir.clone().multiplyScalar(0.25);
    const t0 = (12 - o.y) / dir.y;
    if (t0 > 0) o.addScaledVector(dir, t0);
    for (let i = 0; i < 400; i++) {
      const h = Math.max(VALLEY.water, heightRaw(o.x, o.z));
      if (o.y <= h) {
        const lo = o.clone().sub(step), hi = o.clone();
        for (let j = 0; j < 8; j++) { const mid = lo.clone().add(hi).multiplyScalar(0.5); if (mid.y <= Math.max(VALLEY.water, heightRaw(mid.x, mid.z))) hi.copy(mid); else lo.copy(mid); }
        return hi;
      }
      o.add(step);
    }
    return null;
  }

  // random spots in a region away from its settlement, above water and not too steep
  function spots(id, count, accept) {
    const s = sites.find((x) => x.id === id), r = seededRandom(`spots:${seed}:${id}`), out = [];
    for (let tries = 0; tries < count * 12 && out.length < count; tries++) {
      const x = s.x + (r() * 2 - 1) * 9, z = s.z + (r() * 2 - 1) * 9;
      if (regionAt(x, z) !== id) continue;
      const h = heightRaw(x, z);
      if (h < VALLEY.water + 0.22 || Math.hypot(x - s.x, z - s.z) < VALLEY.plateau[0] + 0.3) continue;
      const slope = Math.abs(heightRaw(x + 0.3, z) - h) + Math.abs(heightRaw(x, z + 0.3) - h);
      if (slope > 0.55 || (accept && !accept(x, z, h, r))) continue;
      out.push([x, h, z, r()]);
    }
    return out;
  }

  function rimSpots(count) {
    const r = seededRandom('rim:' + seed), out = [];
    for (let tries = 0; tries < count * 10 && out.length < count; tries++) {
      const x = (r() * 2 - 1) * VALLEY.half[0], z = (r() * 2 - 1) * VALLEY.half[1];
      if (regionAt(x, z)) continue;
      const h = heightRaw(x, z);
      if (h < 0.5 || h > 4.6 || Math.abs(heightRaw(x + 0.3, z) - h) > 0.35) continue;
      out.push([x, h, z, r()]);
    }
    return out;
  }

  const area = (id) => byRegion[sites.findIndex((x) => x.id === id)].length * VALLEY.cell * VALLEY.cell / 2;
  const site = (id) => { const s = sites.find((x) => x.id === id); return s ? [s.x, s.z] : [0, 0]; };
  return { seed, river: riverInfo, mesh, heightAt: (x, z) => Math.max(VALLEY.water, heightRaw(x, z)), heightRaw, regionAt, site, peak, area, tintRegion, outlineGeometry, pick, spots, rimSpots, noise: n };
}

function heapPush(h, item) { h.push(item); let i = h.length - 1; while (i > 0) { const p = (i - 1) >> 1; if (h[p][0] <= h[i][0]) break; [h[p], h[i]] = [h[i], h[p]]; i = p; } }
function heapPop(h) {
  const top = h[0], last = h.pop();
  if (h.length) { h[0] = last; let i = 0; for (;;) { const l = 2 * i + 1, r = l + 1; let m = i; if (l < h.length && h[l][0] < h[m][0]) m = l; if (r < h.length && h[r][0] < h[m][0]) m = r; if (m === i) break; [h[m], h[i]] = [h[i], h[m]]; i = m; } }
  return top;
}

// flora: one set of instanced meshes per region so burn and wasteland can tint a region's trees together
const FLORA_GEO = {};
function floraGeometry(kind) {
  if (FLORA_GEO[kind]) return FLORA_GEO[kind];
  const g = {
    tree: [new THREE.CylinderGeometry(0.05, 0.07, 0.35, 5, 1, true).translate(0, 0.17, 0), new THREE.IcosahedronGeometry(0.34, 0).scale(1, 1.15, 1).translate(0, 0.62, 0)],
    pine: [new THREE.CylinderGeometry(0.04, 0.06, 0.25, 4, 1, true).translate(0, 0.12, 0), new THREE.ConeGeometry(0.3, 0.95, 6, 1, true).translate(0, 0.66, 0)],
    bush: [null, new THREE.IcosahedronGeometry(0.2, 0).scale(1.2, 0.8, 1.2).translate(0, 0.12, 0)],
    reed: [null, new THREE.ConeGeometry(0.06, 0.42, 4, 1, true).translate(0, 0.2, 0)],
    dead: [new THREE.CylinderGeometry(0.025, 0.06, 0.75, 4, 1, true).translate(0, 0.37, 0), null],
    rock: [null, new THREE.DodecahedronGeometry(0.24, 0).scale(1, 0.7, 1).translate(0, 0.08, 0)],
  }[kind];
  FLORA_GEO[kind] = g;
  return g;
}
const FLORA_COLOUR = { tree: 0x3f6e34, pine: 0x2c5232, bush: 0x557a3a, reed: 0x8a9a5a, rock: 0x86807a, trunk: 0x5a4330, dead: 0x5e5248 };

// flora kinds that come from the RTS models when the asset bundle has them (web/game/models.js); primitives otherwise
const FLORA_MODELS = { tree: ['tree.a', 'tree.b'], pine: ['pine'], rock: ['rock.a', 'rock.b', 'rock.c'] };

// variants of a kind, each a list of parts { geometry, leaf, colour }; leaf parts blacken and drop when the region burns
function floraVariants(kind) {
  const keys = (FLORA_MODELS[kind] ?? []).filter((k) => prefabs[k]);
  if (keys.length) return keys.map((k) => prefabs[k].parts.map((p) => ({ geometry: p.geometry, leaf: p.material.name === 'Green', colour: p.material.color.getHex() })));
  const [trunk, leaf] = floraGeometry(kind), parts = [];
  if (trunk) parts.push({ geometry: trunk, leaf: false, colour: kind === 'dead' ? FLORA_COLOUR.dead : FLORA_COLOUR.trunk });
  if (leaf) parts.push({ geometry: leaf, leaf: kind !== 'rock', colour: FLORA_COLOUR[kind] });
  return [parts];
}

function instanceFlora(group, variants, spots, materials, scaleOf) {
  const m = new THREE.Matrix4(), q = new THREE.Quaternion(), up = new THREE.Vector3(0, 1, 0), c = new THREE.Color();
  variants.forEach((parts, vi) => {
    const mine = spots.filter((s) => Math.floor(s[3] * 7919) % variants.length === vi);
    if (!mine.length) return;
    for (const part of parts) {
      const inst = new THREE.InstancedMesh(part.geometry, part.leaf ? materials.leaf : materials.trunk, mine.length);
      mine.forEach(([x, h, z, k], i) => {
        const s = scaleOf(k);
        q.setFromAxisAngle(up, k * 40);
        m.compose(new THREE.Vector3(x, h - 0.03, z), q, new THREE.Vector3(s, s, s));
        inst.setMatrixAt(i, m);
        inst.setColorAt(i, c.setHex(part.colour).multiplyScalar(0.85 + ((k * 7.31) % 1) * 0.3));
      });
      inst.castShadow = true;
      inst.userData.leaf = part.leaf;
      group.add(inst);
    }
  });
}

function buildFlora(valley, r) {
  const group = new THREE.Group();
  const trunkMat = new THREE.MeshLambertMaterial({ color: 0xffffff, flatShading: true }), leafMat = new THREE.MeshLambertMaterial({ color: 0xffffff, flatShading: true });
  const area = valley.area(r.id), n = valley.noise;
  for (const [kind, density, cluster] of FLORA[r.terrain] ?? []) {
    const spots = valley.spots(r.id, Math.round(area * density * VALLEY.detail), (x, z) => cluster < -0.99 || n.fbm(x * 0.12 + 300, z * 0.12) > cluster * 0.5);
    if (!spots.length) continue;
    const base = kind === 'tree' && r.terrain === 'wood' ? 1.1 : 0.8;
    instanceFlora(group, floraVariants(kind), spots, { trunk: trunkMat, leaf: leafMat }, (k) => base + k * 0.6);
  }
  return { group, trunkMat, leafMat };
}

function tintFlora(flora, burn, waste) {
  const char = Math.min(1, burn * 1.3);
  flora.leafMat.color.setRGB(1, 1, 1).lerp(new THREE.Color(0x2a2522), char);
  flora.trunkMat.color.setRGB(1, 1, 1).lerp(new THREE.Color(waste ? 0x77736c : 0x262220), waste ? 0.7 : char);
  for (const inst of flora.group.children) if (inst.userData.leaf) inst.visible = !waste && burn < 0.75;
}

function buildRimFlora(valley) {
  const spots = valley.rimSpots(Math.round(420 * VALLEY.detail));
  const group = new THREE.Group(), mat = new THREE.MeshLambertMaterial({ color: 0xffffff, flatShading: true });
  if (spots.length) instanceFlora(group, floraVariants('pine'), spots, { trunk: mat, leaf: mat }, (k) => 0.9 + k * 0.7);
  return group;
}

function buildWater() {
  const geo = new THREE.PlaneGeometry(170, 170, 48, 48);
  const p = geo.attributes.position, r = seededRandom('water');
  for (let i = 0; i < p.count; i++) p.setZ(i, (r() - 0.5) * 0.08);
  geo.computeVertexNormals();
  const mesh = new THREE.Mesh(geo, new THREE.MeshPhongMaterial({ color: 0x3a74ad, transparent: true, opacity: 0.84, flatShading: true, shininess: 60, specular: 0x334d66 }));
  mesh.rotation.x = -Math.PI / 2;
  mesh.position.y = VALLEY.water - 0.02;
  mesh.receiveShadow = true;
  const floor = new THREE.Mesh(new THREE.PlaneGeometry(170, 170), new THREE.MeshLambertMaterial({ color: GROUND.seabed }));
  floor.rotation.x = -Math.PI / 2;
  floor.position.y = -3.05;
  const group = new THREE.Group();
  group.add(floor, mesh);
  return group;
}
