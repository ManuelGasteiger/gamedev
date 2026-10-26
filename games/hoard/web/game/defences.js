// Defences in the valley (regions[i].defences, docs/core-loop.md section 4): levies as spears around the houses,
// bolt-throwers as towers with a range ring, the slayer band as a banner camp, wards as a blue dome, cannons as squat
// smoking towers. Rings show when you come near (always with the sight effect) and every label carries the numbers;
// flying inside a ring wounds you every second, sent as `wound` acts in small batches. Nothing hurts before it is shown.
const DEF = { layer: null, groups: new Map(), pending: new Map(), since: 0, warned: new Set(), smoke: 0, warnEl: null };
const DEF_RING = { margin: 5, flush: 1, batch: 3, ward: 2.9 };
const DEF_WORD = { levies: 'levies', bolt_throwers: 'bolt-throwers', slayer_band: 'slayer band', wards: 'wards', cannons: 'cannons' };

function hasDefences() { return (world?.regions ?? []).some((r) => Array.isArray(r.defences)); }
function regionDefences(id) { return (world?.regions ?? []).find((r) => r.id === id)?.defences ?? []; }
function liveDefences(id) { return regionDefences(id).filter((d) => !d.wrecked); }
function rangedDefences(id) { return liveDefences(id).filter((d) => d.radius > 0 && d.passive > 0); }
// radii come in map units (the 400 × 360 layout of REGION_LAYOUT); the valley is WORLD_SCALE of that
function defRadius(d) { return (d.radius ?? 0) * WORLD_SCALE; }
function defName(d) { return (d.label || DEF_WORD[d.id] || d.id.replace(/_/g, ' ')).toLowerCase(); }
// as the sim's armor_against: the talents' armor, and arcane armor against everything that strikes from range
function defenceArmor(d) { return effect('armor_mult') * (d.radius > 0 ? effect('arcane_armor_mult') : 1); }
function round1(v) { return Math.round(v * 10) / 10; }
function numText(v) { return Math.abs(v - Math.round(v)) < 0.05 ? String(Math.round(v)) : v.toFixed(1); }

// the wound a harmful act costs here: per_act after fear and hero (from the sim) × armor from the talents
function actWound(regionId) { return round1(liveDefences(regionId).reduce((s, d) => s + d.per_act * defenceArmor(d), 0)); }
function passiveHere(regionId) { return round1(rangedDefences(regionId).reduce((s, d) => s + d.passive * defenceArmor(d), 0)); }
// as the sim's burn_mult_at: wards halve your burns unless your fire breaks them
function wardBurnMult(regionId) { return effect('ward_breaker') > 0 ? 1 : liveDefences(regionId).reduce((m, d) => m * (d.burn_mult ?? 1), 1); }

function defenceText(d) {
  const name = defName(d);
  if (d.wrecked) return `${name} (wrecked)`;
  const bits = [];
  if (d.per_act > 0) bits.push(`${numText(d.per_act * defenceArmor(d))} per act`);
  if (d.passive > 0 && d.radius > 0) bits.push(`${numText(d.passive * defenceArmor(d))} a second within the ring`);
  if ((d.burn_mult ?? 1) < 1) bits.push(effect('ward_breaker') > 0 ? 'your fire breaks them' : `your burns ×${d.burn_mult}`);
  return bits.length ? `${name}: ${bits.join(', ')}` : name;
}

// the line under a region's label and the facts in the action panel: numbers, not icons
function defenceLabelHtml(regionId) {
  const list = regionDefences(regionId);
  if (!list.length) return '';
  const w = actWound(regionId), p = passiveHere(regionId), ward = wardBurnMult(regionId);
  const bits = [w > 0 ? `${numText(w)} wounds per act` : '', p > 0 ? `${numText(p)} a second in range` : '', ward < 1 ? `burns ×${ward}` : ''].filter(Boolean);
  return bits.length ? `<div class="df">${bits.join(' · ')}</div>` : '';
}

// the hits a harmful act here takes, in the order the defences strike (the sim's list order)
function strikeList(regionId) { return liveDefences(regionId).filter((d) => d.per_act > 0).map((d) => ({ id: d.id, label: defName(d), hit: round1(d.per_act * defenceArmor(d)) })); }

function defenceFactsHtml(regionId) {
  const list = regionDefences(regionId);
  if (!list.length) return '';
  const armor = effect('armor_mult');
  return `<div class="defs"><b>Defences</b> ${list.map((d) => `<span class="def${d.wrecked ? ' wrecked' : ''}">${escapeHtml(defenceText(d))}</span>`).join('')}${armor !== 1 ? `<span class="muted">after your armor ×${fmt(armor)}</span>` : ''}</div>${strikeLineHtml(regionId)}`;
}

function strikeLineHtml(regionId) {
  const strikes = strikeList(regionId), total = actWound(regionId);
  if (!strikes.length) return '';
  const after = hasHealth() ? Math.max(0, health().left - total) : null;
  const order = strikes.map((s, i) => `${i ? 'then ' : ''}${escapeHtml(s.label)} ${numText(s.hit)}`).join(', ');
  return `<div class="defs strike${after !== null && after <= 0 ? ' lethal' : ''}"><b>If you act here</b><span>${order}: ${numText(total)} wounds${after !== null ? `, leaving ${Math.ceil(after)} of ${Math.round(health().max)} health${after <= 0 ? ': you would die' : ''}` : ''}</span></div>`;
}

// ---- the scene ------------------------------------------------------------------------------------------------------
function defenceKey(list) { return list.map((d) => `${d.id}:${d.radius}:${d.wrecked ? 1 : 0}`).join(','); }

function syncDefences() {
  if (!DEF.layer) { DEF.layer = new THREE.Group(); scene.add(DEF.layer); }
  const seen = new Set();
  for (const r of world?.regions ?? []) {
    const list = r.defences ?? [];
    if (!list.length) continue;
    seen.add(r.id);
    const key = defenceKey(list) + '|' + valley.seed;
    const had = DEF.groups.get(r.id);
    if (had?.key === key) continue;
    if (had) DEF.layer.remove(had.group, ...had.rings);
    DEF.groups.set(r.id, buildDefences(r.id, list, key));
  }
  for (const [id, g] of DEF.groups) if (!seen.has(id)) { DEF.layer.remove(g.group, ...g.rings); DEF.groups.delete(id); }
}

function buildDefences(regionId, list, key) {
  const [cx, cz] = regionCentre(regionId);
  const group = new THREE.Group();
  group.position.set(cx, groundAt(cx, cz) - 0.5, cz);
  const rings = [], smokers = [];
  for (const d of list) {
    const build = DEF_BUILD[d.id] ?? DEF_BUILD.other;
    build(group, d, smokers);
    if (d.radius > 0 && d.passive > 0 && !d.wrecked) rings.push(rangeRing(cx, cz, defRadius(d)));
  }
  drape(group);
  DEF.layer.add(group, ...rings);
  return { key, group, rings, smokers };
}

const defMat = (c) => new THREE.MeshLambertMaterial({ color: c, flatShading: true });
function defBox(g, w, h, d, c, x, y, z) { const m = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), defMat(c)); m.position.set(x, y, z); g.add(m); return m; }
function defCyl(g, r0, r1, h, c, x, y, z, seg = 7) { const m = new THREE.Mesh(new THREE.CylinderGeometry(r0, r1, h, seg), defMat(c)); m.position.set(x, y, z); g.add(m); return m; }
function wreck(obj, d) { if (!d.wrecked) return obj; obj.rotation.z = 0.7; obj.traverse((o) => { if (o.isMesh) { o.material = o.material.clone(); o.material.color.lerp(new THREE.Color(PALETTE.char), 0.7); } }); return obj; }
function atAngle(a, dist) { return [Math.cos(a) * dist, Math.sin(a) * dist]; }

const DEF_BUILD = {
  levies(g, d) {
    for (let i = 0; i < 12; i++) {
      const [x, z] = atAngle((i / 12) * Math.PI * 2 + 0.13, 2.5);
      const spear = new THREE.Group();
      defCyl(spear, 0.022, 0.022, 0.75, 0x5c4630, 0, 0.37, 0, 4);
      defCyl(spear, 0, 0.05, 0.16, 0xc8ccd4, 0, 0.82, 0, 4);
      spear.position.set(x, 0.5, z);
      spear.rotation.set(Math.sin(i) * 0.15, 0, Math.cos(i * 1.7) * 0.15);
      g.add(spear);
    }
  },
  bolt_throwers(g, d) {
    for (const a of [2.25, 5.35]) {
      const [x, z] = atAngle(a, 2.95);
      const t = new THREE.Group();
      defBox(t, 0.62, 1.15, 0.62, 0x7a6248, 0, 0.58, 0);
      defBox(t, 0.78, 0.1, 0.78, 0x5c4630, 0, 1.2, 0);
      const arm = defBox(t, 0.95, 0.07, 0.1, 0x3e2e20, 0, 1.38, 0.1);
      arm.rotation.y = a;
      defCyl(t, 0.03, 0.03, 0.75, 0xd0d0d8, 0, 1.4, 0, 4).rotation.x = Math.PI / 2;
      t.position.set(x, 0.5, z);
      g.add(wreck(t, d));
    }
  },
  slayer_band(g, d) {
    const [x, z] = atAngle(3.35, 3.4);
    const camp = new THREE.Group();
    for (const [tx, tz] of [[-0.35, 0.2], [0.35, -0.15]]) { const tent = new THREE.Mesh(new THREE.ConeGeometry(0.34, 0.5, 4), defMat(0x6b2f2a)); tent.position.set(tx, 0.25, tz); camp.add(tent); }
    for (const [bx, bz] of [[0.05, 0.55], [-0.6, -0.35]]) {
      defBox(camp, 0.06, 1.45, 0.06, 0x3a2a20, bx, 0.72, bz);
      defBox(camp, 0.42, 0.55, 0.03, 0x8e1c1c, bx + 0.23, 1.15, bz);
      defBox(camp, 0.08, 0.32, 0.035, 0xe8e0d0, bx + 0.23, 1.15, bz);
    }
    const fire = new THREE.Mesh(new THREE.SphereGeometry(0.08, 6, 4), new THREE.MeshBasicMaterial({ color: 0xff9a3a }));
    fire.position.set(0, 0.08, 0);
    camp.add(fire);
    camp.position.set(x, 0.5, z);
    g.add(camp);
  },
  wards(g, d) {
    const dome = new THREE.Mesh(new THREE.SphereGeometry(defRadius(d) || DEF_RING.ward, 20, 10, 0, Math.PI * 2, 0, Math.PI / 2),
      new THREE.MeshBasicMaterial({ color: 0x5ea0ff, transparent: true, opacity: 0.13, depthWrite: false, side: THREE.DoubleSide }));
    const grid = new THREE.Mesh(dome.geometry, new THREE.MeshBasicMaterial({ color: 0x9cc8ff, wireframe: true, transparent: true, opacity: 0.22, depthWrite: false }));
    dome.position.y = grid.position.y = 0.45;
    dome.userData.spin = grid.userData.spin = true;
    g.add(dome, grid);
  },
  cannons(g, d, smokers) {
    for (const a of [0.85, 3.95]) {
      const [x, z] = atAngle(a, 3.05);
      const t = new THREE.Group();
      defCyl(t, 0.5, 0.58, 0.62, 0x8a8790, 0, 0.31, 0, 8);
      for (let i = 0; i < 6; i++) { const [mx, mz] = atAngle((i / 6) * Math.PI * 2, 0.46); defBox(t, 0.16, 0.16, 0.16, 0x9a97a0, mx, 0.7, mz); }
      for (const s of [-1, 1]) { const barrel = defCyl(t, 0.07, 0.09, 0.62, 0x2c2c32, s * 0.16, 0.74, 0.3, 6); barrel.rotation.x = Math.PI / 2 - 0.25; }
      t.position.set(x, 0.5, z);
      g.add(wreck(t, d));
      if (!d.wrecked) smokers.push(t);
    }
  },
  other(g, d) { defCyl(g, 0.3, 0.36, 0.9, 0x8a8790, 2.9, 0.95, 0, 6); },
};

// a ribbon and a faint disc draped over the ground: the area where the towers reach you
function rangeRing(cx, cz, radius) {
  const n = 72, pos = [], fill = [];
  const at = (a, r) => { const x = cx + Math.cos(a) * r, z = cz + Math.sin(a) * r; return [x, groundAt(x, z) + 0.16, z]; };
  for (let i = 0; i < n; i++) {
    const a0 = (i / n) * Math.PI * 2, a1 = ((i + 1) / n) * Math.PI * 2;
    const p = [at(a0, radius - 0.1), at(a0, radius + 0.1), at(a1, radius + 0.1), at(a1, radius - 0.1)];
    pos.push(...p[0], ...p[1], ...p[2], ...p[0], ...p[2], ...p[3]);
    for (const [r0, r1] of [[0, radius / 3], [radius / 3, (2 * radius) / 3], [(2 * radius) / 3, radius - 0.1]]) {
      const q = [at(a0, r0), at(a0, r1), at(a1, r1), at(a1, r0)];
      fill.push(...q[0], ...q[1], ...q[2], ...q[0], ...q[2], ...q[3]);
    }
  }
  const mesh = (verts, opacity) => {
    const geo = new THREE.BufferGeometry();
    geo.setAttribute('position', new THREE.Float32BufferAttribute(verts, 3));
    return new THREE.Mesh(geo, new THREE.MeshBasicMaterial({ color: 0xff4a32, transparent: true, opacity, depthWrite: false, side: THREE.DoubleSide, polygonOffset: true, polygonOffsetFactor: -3, polygonOffsetUnits: -3 }));
  };
  const g = new THREE.Group();
  g.add(mesh(pos, 0.75), mesh(fill, 0.045));
  g.userData = { radius, cx, cz };
  g.visible = false;
  return g;
}

// ---- in range: ring visibility, passive wounds, the first-time warning ---------------------------------------------
function defencesPaused() { return play.landed || montage.active || play.dead || wakeOpen() || (typeof growthUi !== 'undefined' && growthUi.open); }

function updateDefences(dt) {
  if (!DEF.layer) return;
  const sight = effect('sight') > 0, t = performance.now() / 1000, paused = defencesPaused();
  DEF.smoke -= dt;
  for (const [id, g] of DEF.groups) {
    const [cx, cz] = regionCentre(id);
    const dist = Math.hypot(dragon.pos.x - cx, dragon.pos.z - cz);
    for (const ring of g.rings) {
      const show = sight || dist < ring.userData.radius + DEF_RING.margin;
      ring.visible = show;
      if (show) { const inside = dist <= ring.userData.radius; ring.children[0].material.opacity = inside ? 0.6 + 0.35 * Math.sin(t * 8) : 0.75; ring.children[1].material.opacity = inside ? 0.12 : 0.045; }
    }
    g.group.children.forEach((c) => { if (c.userData.spin) c.rotation.y = t * 0.15; });
    if (DEF.smoke <= 0) for (const s of g.smokers) smokePuff(s);
    if (paused) continue;
    for (const d of rangedDefences(id)) {
      const key = id + '|' + d.id;
      if (dist > defRadius(d)) continue;
      if (!DEF.warned.has(key)) { DEF.warned.add(key); rangeWarning(id, d); }
      DEF.pending.set(key, (DEF.pending.get(key) ?? 0) + d.passive * dt);
      if (!DEF.since) DEF.since = t;
    }
  }
  if (DEF.smoke <= 0) DEF.smoke = 0.9;
  const total = [...DEF.pending.values()].reduce((s, v) => s + v, 0);
  if (DEF.since && (total >= DEF_RING.batch || t - DEF.since >= DEF_RING.flush)) flushPassiveWounds();
}

function flushPassiveWounds() {
  const acts = [];
  let rate = 0;
  for (const [key, amount] of DEF.pending) {
    const [region, item] = key.split('|');
    const target = model.regions.get(region)?.owner;
    const d = regionDefences(region).find((x) => x.id === item);
    if (!target || amount < 0.05) continue;
    acts.push({ type: 'wound', target, region, magnitude: round1(amount), item, witnesses: [] });
    rate += d?.passive ?? 0;
  }
  DEF.pending.clear();
  DEF.since = 0;
  if (!acts.length || play.dead) return null;
  const res = applyActs(acts);
  if (!res?.ok) return res;
  pushEvents(res);
  const raw = acts.reduce((s, a) => s + a.magnitude, 0);
  actOutcome(res, acts[0].region, { act: 'passive', rate: passiveHere(acts[0].region), seconds: rate > 0 ? raw / rate : 0 });
  renderChronicle();
  return res;
}

function rangeWarning(regionId, d) {
  const owner = model.regions.get(regionId)?.owner;
  const text = `In range of the ${defName(d)} of ${factionTitle(owner)} at ${regionTitle(regionId)}: ${numText(d.passive * defenceArmor(d))} wounds a second while you stay inside the ring, ${numText(d.per_act * defenceArmor(d))} for every act here.`;
  if (!DEF.warnEl) { DEF.warnEl = document.createElement('div'); DEF.warnEl.id = 'range-warn'; document.body.appendChild(DEF.warnEl); }
  DEF.warnEl.textContent = text;
  DEF.warnEl.classList.remove('show');
  void DEF.warnEl.offsetWidth;
  DEF.warnEl.classList.add('show');
  sfx('warn');
}

function smokePuff(tower) {
  const s = new THREE.Sprite(new THREE.SpriteMaterial({ map: flameTexture, color: 0x8c8c8c, transparent: true, opacity: 0.5, depthWrite: false }));
  const p = tower.getWorldPosition(new THREE.Vector3());
  s.position.set(p.x + (Math.random() - 0.5) * 0.3, p.y + 1.0, p.z + (Math.random() - 0.5) * 0.3);
  s.scale.setScalar(0.4);
  sceneLayers.effects.add(s);
  effects.push({ life: 2.2, update(dt, e) { s.position.y += dt * 0.6; s.position.x += dt * 0.25; s.scale.setScalar(0.4 + (2.2 - e.life) * 0.45); s.material.opacity = 0.45 * (e.life / 2.2); }, end() { sceneLayers.effects.remove(s); } });
}

// a new age: towers warn again
function resetDefenceWarnings() { DEF.warned.clear(); DEF.pending.clear(); DEF.since = 0; }

addStyle(`
  .label .df { font-size: 10px; color: #ff9c8a; }
  #actions .defs { display: flex; flex-wrap: wrap; gap: 2px 10px; margin: -2px 0 6px; color: #ffb3a6; font-size: 11.5px; }
  #actions .defs b { color: #ff8a78; font-weight: 600; text-transform: uppercase; font-size: 10.5px; letter-spacing: .05em; }
  #actions .defs .wrecked { color: var(--dim); text-decoration: line-through; }
  #actions .defs.strike { color: #f3d2cb; }
  #actions .defs.strike.lethal span { color: #fff; background: #8e2a22; padding: 0 4px; border-radius: 2px; }
  #range-warn { position: fixed; top: 84px; left: 50%; transform: translateX(-50%); max-width: min(720px, 90vw); z-index: 9; pointer-events: none; background: #2a0f0dee; border: 1px solid #d4594c;
    border-radius: 4px; padding: 8px 14px; color: #ffd9d2; font: 15px/1.4 Georgia, 'Palatino Linotype', 'DejaVu Serif', serif; text-align: center; opacity: 0; box-shadow: 0 6px 24px #000a; }
  #range-warn.show { animation: rangewarn 4.5s forwards; }
  @keyframes rangewarn { 0% { opacity: 0; transform: translate(-50%, -8px); } 6% { opacity: 1; transform: translate(-50%, 0); } 80% { opacity: 1; } 100% { opacity: 0; } }
`);
