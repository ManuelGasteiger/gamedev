// What the dragon looks like: every look key of docs/v04-plan.md (plates, fire, wings, spikes, horns, runes, crest, eyes,
// veins, shadow, aura, hoard_glitter, banners) and the steps the sim adds to the look numbers for ranks and milestones
// (scale, beam, aura, eyes, hoard_glitter; look.rank_step counts the milestones per axis). Parts are built once on the
// rest pose and pinned to bones; syncLook switches them, updateLook animates, lookFlourish plays when a talent is taken.
const FIRE_COLOUR = { orange: 0xff8a2a, gold: 0xffc53d, white: 0xfff3dc, blue: 0x5ab6ff };
// look values are levels (a later talent raises them); a word per level for the talent cards
const LOOK_WORDS = {
  plates: { dark: 'darker plates', metal: 'a metal sheen on your scales' }, fire: { gold: 'gold fire', white: 'white fire', blue: 'blue fire' },
  wings: ['bigger wings'], spikes: ['spikes along your tail', 'spikes along your back and tail'], horns: ['horns'], runes: ['glowing runes on your scales', 'runes all over your scales'],
  crest: ['a golden crest', 'a taller golden crest', 'a great golden crest'], eyes: ['glowing eyes', 'eyes like coals'], veins: ['red veins that glow'], shadow: ['shadowed wings'],
  aura: ['an aura around you', 'a strong aura around you'], hoard_glitter: ['a glittering hoard in your lair', 'a hoard that blazes with light'],
  banners: ['a banner of the faithful at your lair', 'banners of the faithful at your lair', 'a forest of banners at your lair'],
};
const LOOK = { parts: null, wings: [], look: {}, rage: false, flash: 0, flashColour: new THREE.Color(), hit: 0, zoomPulse: null, lair: null, lairKey: '', textures: {} };

function lookOf() { return model?.dragon?.look ?? {}; }
function lookLevel(key) { const v = lookOf()[key]; return typeof v === 'number' ? v : typeof v === 'string' && v ? 1 : 0; }
function lookScale() { return 1 + lookLevel('scale'); }
function fireColour() { return FIRE_COLOUR[lookOf().fire] ?? FIRE_COLOUR.orange; }
function beamWidth() { return 1 + lookLevel('beam') + (lookOf().fire ? 0.2 : 0); }
function fireTexture() { return lookOf().fire ? lookTexture('flame', [[0, '255,255,255,1'], [0.35, '255,255,255,0.85'], [1, '255,255,255,0']]) : flameTexture; }

// what a talent will show, for its card: its look keys in words, else what its effect changes on screen
const EFFECT_SEE = {
  stamina_add: 'a longer stamina bar', speed_mult: 'faster flight', armor_mult: 'smaller wound numbers', arcane_armor_mult: 'smaller tower damage', health_add: 'a longer health bar',
  heal_mult: 'more healing in the sleep panel', burn_cost_mult: 'cheaper burns in the action panel', burn_mult: 'bigger burns for the same stamina', kill_mult: 'harder kills',
  witness_mult: 'neighbours who take less notice', sight: 'every range ring and its damage', lie_sight: 'the lie in an envoy\'s eyes', premonition: 'a warning before slayers come',
  forecast_mult: 'a sharper forecast', crush: 'a Crush row in the action panel', burn_spread: 'fire leaping to the neighbours', tower_breaker: 'towers falling where you burn',
  ward_breaker: 'wards that no longer halve your fire', molten_gold: 'more gold from burning towns', dread_mult: 'bigger dread numbers', burn_dread_mult: 'bigger dread from your fire', protect_awe_mult: 'more awe when you protect', awe_gain_mult: 'bigger awe numbers', tribute_mult: 'bigger tribute', bed_mult: 'more Might from each sleep',
  digest_mult: 'more growth from each sleep', teach_mult: 'teaching that lasts', demand_mult: 'Demand opening sooner', thief_guard: 'no more thieves in the hoard', hunt_mult: 'more food from the hunt',
  cult: 'a court that keeps its faith', brood: 'dragon-raised courts that stay yours', oath_steadfast: 'oaths that hold', slay_mult: 'a safer forecast', lair_ward: 'a ward over your lair',
  spell_cost_mult: 'cheaper spells', undead_cost_mult: 'cheaper undead', hex: 'curses that slow their weapons', oral_dread_decay_mult: 'dread that lasts', burn_tier_up: 'burns they write down', carry_add: 'a bigger carry gauge', gold_weight_mult: 'gold that weighs less', chronicle_books: 'a clearer chronicle', faith_mult: 'bigger faith numbers', ember_mult: 'more embers',
  burn_linger: 'char that lingers', hostage_lore_mult: 'more Lore from hostages',
};
function lookSeeText(t) {
  const word = (k, v) => (Array.isArray(LOOK_WORDS[k]) ? LOOK_WORDS[k][clamp(Math.round(v) - 1, 0, LOOK_WORDS[k].length - 1)] : LOOK_WORDS[k]?.[v]);
  const words = Object.entries(t?.look ?? {}).map(([k, v]) => word(k, v)).filter(Boolean);
  if (!words.length) for (const k of Object.keys(t?.effects ?? {})) if (EFFECT_SEE[k]) { words.push(EFFECT_SEE[k]); break; }
  return words.join(' and ');
}

function lookTexture(key, stops) {
  if (!LOOK.textures[key]) {
    const c = document.createElement('canvas');
    c.width = c.height = 64;
    const g = c.getContext('2d'), grad = g.createRadialGradient(32, 32, 1, 32, 32, 32);
    for (const [at, rgba] of stops) grad.addColorStop(at, `rgba(${rgba})`);
    g.fillStyle = grad;
    g.fillRect(0, 0, 64, 64);
    LOOK.textures[key] = new THREE.CanvasTexture(c);
  }
  return LOOK.textures[key];
}
function glowSprite(colour, scale, opacity = 1, additive = true) {
  const s = new THREE.Sprite(new THREE.SpriteMaterial({ map: lookTexture('glow', [[0, '255,255,255,1'], [0.3, '255,255,255,0.55'], [1, '255,255,255,0']]), color: colour, transparent: true, opacity, depthWrite: false, blending: additive ? THREE.AdditiveBlending : THREE.NormalBlending }));
  s.scale.setScalar(scale);
  return s;
}
function basic(colour, opacity = 1) { return new THREE.MeshBasicMaterial({ color: colour, transparent: opacity < 1, opacity, depthWrite: opacity >= 1 }); }

// a part at a point of the dragon's own frame (rest pose, scale 1), oriented in that frame, riding on a bone from then on
function lookPin(root, obj, boneName, at, rot = [0, 0, 0]) {
  const b = root.getObjectByName(boneName);
  if (!b) return null;
  const pos = dragon.group.localToWorld(new THREE.Vector3(...at));
  b.add(obj);
  obj.position.copy(b.worldToLocal(pos));
  const want = dragon.group.getWorldQuaternion(new THREE.Quaternion()).multiply(new THREE.Quaternion().setFromEuler(new THREE.Euler(...rot)));
  obj.quaternion.copy(b.getWorldQuaternion(new THREE.Quaternion()).invert().multiply(want));
  obj.scale.setScalar(dragon.group.scale.x / b.getWorldScale(new THREE.Vector3()).x);
  obj.visible = false;
  return obj;
}

// called by loadDragonModel while the model still stands in its rest pose
function buildLookParts(root) {
  const p = { horns: [], eyes: [], crest: [], spikes: [], runes: [], shadow: [], aura: null, motes: [] };
  const ivory = new THREE.MeshPhongMaterial({ color: 0xe9dcc0, flatShading: true, shininess: 0 });
  for (const s of [1, -1]) {
    const horn = new THREE.Group();
    const base = new THREE.Mesh(new THREE.ConeGeometry(0.075, 0.3, 5), ivory);
    base.position.y = 0.15;
    const tip = new THREE.Mesh(new THREE.ConeGeometry(0.045, 0.24, 5), ivory);
    tip.position.set(0, 0.36, -0.06);
    tip.rotation.x = -0.6;
    horn.add(base, tip);
    p.horns.push(lookPin(root, horn, 'Head', [s * 0.11, 1.15, 1.36], [-1.05, 0, -s * 0.4]));
    const eye = new THREE.Group();
    eye.add(new THREE.Mesh(new THREE.SphereGeometry(0.04, 8, 6), basic(0xfff07a)), glowSprite(0xffd23a, 0.42, 0.9));
    p.eyes.push(lookPin(root, eye, 'Head', [s * 0.09, 1.09, 1.74]));
  }
  const gold = new THREE.MeshPhongMaterial({ color: 0xf0c040, emissive: 0x6a4a00, flatShading: true, shininess: 0 });
  [[1.66, 0.2], [1.53, 0.27], [1.4, 0.24], [1.28, 0.17]].forEach(([z, h], i) => p.crest.push(lookPin(root, new THREE.Mesh(new THREE.ConeGeometry(0.05, h, 4), gold), 'Head', [0, 1.17 + h / 2 - 0.02 * i, z], [-0.35, 0, 0])));
  const bone = new THREE.MeshPhongMaterial({ color: 0xd8cba8, flatShading: true, shininess: 0 });
  [['UpperBack', 1.06, 0.5, 0.3], ['UpperBack', 1.05, 0.3, 0.32], ['LowerBack', 1.03, 0.08, 0.32], ['Tail0', 1.0, -0.3, 0.28], ['Tail1', 0.98, -0.72, 0.25], ['Tail2', 0.95, -1.12, 0.22], ['Tail3', 0.96, -1.52, 0.19], ['Tail4', 0.96, -1.9, 0.16]]
    .forEach(([b, y, z, h]) => p.spikes.push(lookPin(root, new THREE.Mesh(new THREE.ConeGeometry(0.07, h, 4), bone), b, [0, y + h / 2, z], [-0.55, 0, 0])));
  const rune = basic(0xc893ff, 0.95);
  [['Neck0', 0, 1.0, 0.86], ['UpperBack', 0.15, 0.94, 0.42], ['UpperBack', -0.15, 0.94, 0.42], ['LowerBack', 0.16, 0.92, 0.12], ['LowerBack', -0.16, 0.92, 0.12], ['Tail0', 0, 0.95, -0.38],
    ['Tail1', 0.08, 0.93, -0.8], ['Tail1', -0.08, 0.93, -0.8], ['Tail2', 0, 0.92, -1.2], ['Neck2', 0, 1.07, 1.18]].forEach(([b, x, y, z]) => {
    const m = new THREE.Mesh(new THREE.OctahedronGeometry(0.05, 0), rune);
    m.scale.y = 0.35;
    const g = new THREE.Group();
    g.add(m, glowSprite(0xa060ff, 0.3, 0.8));
    p.runes.push(lookPin(root, g, b, [x, y, z]));
  });
  for (const s of [1, -1]) for (const [b, at] of [['WingFingerOutMid', [1.85, 1.03, 0.86]], ['WingFingerMidMid', [1.5, 1.03, 0.6]], ['WingFingerInMid', [1.05, 1.03, 0.5]], ['WingFingerOutBottom', [2.4, 1.04, 0.5]]]) {
    const smoke = glowSprite(0x140c1e, 1.05, 0.7, false);
    smoke.material.map = lookTexture('smoke', [[0, '255,255,255,1'], [0.55, '255,255,255,0.7'], [1, '255,255,255,0']]);
    p.shadow.push(lookPin(root, smoke, (s > 0 ? 'Left' : 'Right') + b, [s * at[0], at[1], at[2]]));
  }
  p.aura = glowSprite(0xa070ff, 4.6, 0.45);
  p.aura.position.set(0, 1.0, 0.2);
  p.aura.visible = false;
  dragon.group.add(p.aura);
  for (let i = 0; i < 8; i++) { const m = new THREE.Mesh(new THREE.OctahedronGeometry(0.07, 0), basic(0xd2b0ff)); m.visible = false; dragon.group.add(m); p.motes.push(m); }
  LOOK.parts = p;
  LOOK.light = new THREE.PointLight(0xfff0dc, 0, 16, 0);
  scene.add(LOOK.light);
  LOOK.wings = ['LeftWingTop', 'RightWingTop'].map((n) => root.getObjectByName(n)).filter(Boolean);
}

function syncLook(rage = false) {
  const look = lookOf(), p = LOOK.parts;
  LOOK.look = look;
  LOOK.rage = rage;
  const plate = look.plates === 'metal' ? 0xc0c8d8 : look.plates === 'dark' ? 0x9890b2 : 0xffffff;
  const colour = new THREE.Color(plate);
  if (rage) colour.multiply(new THREE.Color(0xff8f80));
  for (const m of dragon.parts.materials) {
    m.color.copy(colour);
    if ('metalness' in m) { m.metalness = look.plates === 'metal' ? 0.55 : 0; m.roughness = look.plates === 'metal' ? 0.32 : 1; }
  }
  dragon.beam.material.color.setHex(fireColour());
  if (p) {
    const show = (list, on) => list.forEach((o) => o && (o.visible = on));
    const level = (k) => lookLevel(k);
    show(p.horns, level('horns') > 0);
    show(p.eyes, level('eyes') > 0);
    p.crest.forEach((o, i) => o && (o.visible = level('crest') > 0 && i < 1 + Math.min(3, Math.round(level('crest')))));
    p.spikes.forEach((o, i) => o && (o.visible = level('spikes') >= 2 || (level('spikes') > 0 && i >= 3)));
    p.runes.forEach((o, i) => o && (o.visible = level('runes') > 0 && (level('runes') >= 2 || i < 6)));
    show(p.shadow, level('shadow') > 0);
    p.aura.visible = level('aura') > 0;
    show(p.motes, level('aura') >= 1);
  }
  syncLairLook(look);
}

// the hoard heap with its glitter and the banners of the faithful, on the lair's ledge
function syncLairLook(look) {
  const glitter = lookLevel('hoard_glitter'), banners = Math.min(3, Math.round(lookLevel('banners')));
  const faithful = model ? [...model.factions.values()].filter((f) => f.faith > 0.3).sort((a, b) => b.faith - a.faith).slice(0, 3).map((f) => f.id) : [];
  const key = [glitter ? Math.round(Math.min(60, model?.dragon?.hoard?.gold ?? 0) / 10) : -1, glitter.toFixed(1), banners, banners ? faithful.join(',') : '-', valley?.seed].join('|');
  if (!valley || key === LOOK.lairKey) return;
  LOOK.lairKey = key;
  if (LOOK.lair) scene.remove(LOOK.lair);
  const g = new THREE.Group();
  const [cx, cz] = regionCentre('lair');
  g.position.set(cx, groundAt(cx, cz) - 0.5, cz);
  LOOK.lair = g;
  LOOK.glitter = [];
  if (glitter > 0) {
    const gold = new THREE.MeshPhongMaterial({ color: 0xe8b53c, emissive: 0x4a3200, flatShading: true, shininess: 0 });
    const size = 0.55 + Math.min(0.5, (model?.dragon?.hoard?.gold ?? 0) / 80);
    const heap = new THREE.Mesh(new THREE.ConeGeometry(size, size * 0.55, 7), gold);
    heap.position.set(0.2, 0.5 + size * 0.27, -0.25);
    g.add(heap);
    const rnd = seededRandom('glitter');
    for (let i = 0; i < Math.round(4 + 6 * glitter); i++) {
      const s = glowSprite(0xfff2b0, 0.32, 0);
      const a = rnd() * Math.PI * 2, d = rnd() * size * 0.8;
      s.position.set(0.2 + Math.cos(a) * d, 0.55 + rnd() * size * 0.5, -0.25 + Math.sin(a) * d);
      s.userData.phase = rnd() * 7;
      g.add(s);
      LOOK.glitter.push(s);
    }
  }
  if (banners > 0) {
    const colours = faithful.length ? faithful.map((id) => factionColor3(id)) : [new THREE.Color(PALETTE.crown)];
    for (let i = 0; i < banners; i++) {
      const a = -0.4 + i * 0.75, x = 1.1 + Math.cos(a) * 1.6, z = 0.9 + Math.sin(a) * 1.6;
      g.add(box(0.07, 1.7, 0.07, PALETTE.post, x, 1.35, z));
      const flag = box(0.55, 0.36, 0.04, 0x000000, x + 0.3, 1.95, z);
      flag.material.color.copy(colours[i % colours.length]);
      g.add(flag);
    }
  }
  scene.add(g);
  drape(g);
}

// after the clip has posed the bones: bigger wings. The mixer writes a bone only when its clip value changes and the
// wings' scale tracks are constant, so the scale is set from the clip's value captured once, never multiplied in place
function applyLookPose() {
  const k = typeof LOOK.look.wings === 'number' && LOOK.look.wings > 0 ? LOOK.look.wings : 1;
  for (const b of LOOK.wings) {
    if (!b.userData.clipScale) b.userData.clipScale = b.scale.clone();
    b.scale.copy(b.userData.clipScale).multiplyScalar(k);
  }
}

function updateLook(dt) {
  const look = LOOK.look, p = LOOK.parts, t = performance.now() / 1000;
  LOOK.flash = Math.max(0, LOOK.flash - dt * 1.1);
  LOOK.hit = Math.max(0, LOOK.hit - dt * 5);
  const vein = look.veins ? 0.28 + 0.18 * Math.sin(t * 3.1) : 0;
  const e = new THREE.Color(0x000000);
  if (vein) e.add(new THREE.Color(0xc01010).multiplyScalar(vein));
  if (LOOK.flash) e.add(LOOK.flashColour.clone().multiplyScalar(LOOK.flash * 0.9));
  if (LOOK.hit) e.add(new THREE.Color(0xff2a1a).multiplyScalar(LOOK.hit));
  if (showcase()) e.add(new THREE.Color(0x4a4a5c));
  for (const m of dragon.parts.materials) if (m.emissive) m.emissive.copy(e);
  if (p) {
    const glow = 0.75 + 0.25 * Math.sin(t * 2.4);
    p.runes.forEach((o, i) => { if (o?.visible) o.children[1].material.opacity = 0.5 + 0.4 * Math.sin(t * 2 + i); });
    const eyes = lookLevel('eyes'), aura = lookLevel('aura');
    p.eyes.forEach((o) => { if (o?.visible) { o.children[1].material.opacity = Math.min(1, glow * (0.6 + 0.25 * eyes)); o.children[1].scale.setScalar(0.3 + 0.14 * eyes); } });
    if (p.aura.visible) { p.aura.material.opacity = Math.min(0.85, 0.2 + 0.26 * aura) * glow; p.aura.scale.setScalar(3.8 + 0.8 * aura); }
    p.motes.forEach((m, i) => { if (!m.visible) return; const a = t * 0.9 + (i / p.motes.length) * Math.PI * 2; m.position.set(Math.cos(a) * 1.9, 1.0 + Math.sin(t * 1.7 + i) * 0.25, 0.2 + Math.sin(a) * 1.9); m.rotation.y = t * 2; });
  }
  // while the growth screen shows the dragon close up, a soft light from the camera's side brings out the details
  if (LOOK.light) {
    const want = showcase() ? 1.5 : 0;
    LOOK.light.intensity += (want - LOOK.light.intensity) * Math.min(1, dt * 4);
    LOOK.light.position.copy(dragon.group.position).add(new THREE.Vector3(Math.sin(cam.yaw) * 6, 7, Math.cos(cam.yaw) * 6));
  }
  for (const s of LOOK.glitter ?? []) s.material.opacity = Math.max(0, Math.sin(t * 3 + s.userData.phase) * 1.4 - 0.4);
  if (LOOK.zoomPulse) {
    const z = LOOK.zoomPulse;
    z.t += dt;
    const k = Math.min(1, z.t / z.dur);
    cam.zoom = z.from * (1 + 0.16 * Math.sin(Math.PI * k));
    resize();
    if (k >= 1) { cam.zoom = z.from; resize(); LOOK.zoomPulse = null; }
  }
}

function lookHit() { LOOK.hit = 1; }
function showcase() { return typeof growthUi !== 'undefined' && growthUi.open; }

// taking a talent: the camera leans in, the dragon flashes in the axis colour, a ring and sparks burst, a chime plays
function lookFlourish(axis) {
  const colour = new THREE.Color(typeof axisColour === 'function' ? axisColour(axis) : '#ffd25a');
  LOOK.flash = 1;
  LOOK.flashColour.copy(colour);
  sfx('talent');
  if (!LOOK.zoomPulse) LOOK.zoomPulse = { from: cam.zoom, t: 0, dur: 1.1 };
  if (!(typeof growthUi !== 'undefined' && growthUi.open)) { cam.follow = true; cam.target.set(dragon.pos.x, 0, dragon.pos.z); }
  const ring = new THREE.Mesh(new THREE.TorusGeometry(1, 0.05, 6, 48), new THREE.MeshBasicMaterial({ color: colour, transparent: true, depthWrite: false }));
  ring.rotation.x = Math.PI / 2;
  const sparks = [];
  for (let i = 0; i < 14; i++) { const s = glowSprite(colour, 0.5, 1); const a = (i / 14) * Math.PI * 2; s.userData.v = new THREE.Vector3(Math.cos(a) * 3, 1.5 + (i % 3), Math.sin(a) * 3); sparks.push(s); }
  sceneLayers.effects.add(ring, ...sparks);
  const at = dragon.group.position.clone();
  sparks.forEach((s) => s.position.copy(at).add(new THREE.Vector3(0, 0.8 * dragon.group.scale.x, 0)));
  effects.push({ life: 1.1, update(dt, e) {
    const k = 1 - e.life / 1.1;
    ring.position.copy(dragon.group.position).add(new THREE.Vector3(0, 0.6 * dragon.group.scale.x, 0));
    ring.scale.setScalar((0.6 + k * 4) * dragon.group.scale.x);
    ring.material.opacity = 1 - k;
    for (const s of sparks) { s.position.addScaledVector(s.userData.v, dt); s.userData.v.y -= dt * 4; s.material.opacity = 1 - k; }
  }, end() { sceneLayers.effects.remove(ring, ...sparks); } });
}
