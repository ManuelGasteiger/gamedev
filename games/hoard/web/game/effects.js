// Talent effects the page applies itself (docs/v04-plan.md, side P), read from dragon.effects with the contract's
// defaults: keys ending in _mult are 1, _add are 0, flags and levels are 0. Without dragon.effects nothing changes.
const CRUSH = { cost: 20, magnitude: 0.5, show: 0.3 };   // not in the contract: the page's starting numbers
const DRAGON_SPEED = 14;

function effect(key) {
  const v = model?.dragon?.effects?.[key];
  if (typeof v === 'number') return v;
  return key.endsWith('_mult') ? 1 : 0;
}

function burnMagnitude(m) { return clamp(m * effect('burn_mult'), 0.05, 1); }
function burnStamina(m) { return Math.max(1, Math.round(burnCost(m) * effect('burn_cost_mult'))); }
function killMagnitude(m) { return clamp(m * effect('kill_mult'), 0.05, 1); }
function killStamina() { return Math.max(1, Math.round(COST.kill * effect('kill_cost_mult'))); }

// what the burn row says about the breath: the range of the hold and the talents that shape it
function burnNote() {
  const notes = [];
  if (effect('burn_mult') !== 1) notes.push(`×${fmt(effect('burn_mult'))} per stamina`);
  if (effect('burn_cost_mult') !== 1) notes.push(`${Math.round((1 - effect('burn_cost_mult')) * 100)}% cheaper`);
  return notes.length ? ` (${notes.join(', ')})` : '';
}

function syncEffects() { dragon.speed = DRAGON_SPEED * effect('speed_mult'); }

// ---- Crush (Earth-shaker): kill the garrison where you land, and every neighbouring court sees it --------------------
function crushAction(r, owner) {
  if (!(effect('crush') > 0)) return null;
  const why = r.lair ? 'not your own lair' : !owner ? 'no settlement to crush here' : '';
  return { id: 'crush', name: 'Crush', cost: CRUSH.cost, desc: owner ? `land on ${factionTitle(owner.id)}: the garrison dies, every neighbour sees it` : '', why,
    controls: owner ? '<button data-run="crush">Crush</button>' : '' };
}

function crushActs(regionId, target) {
  const r = model.regions.get(regionId);
  const acts = [{ type: 'kill', target, region: regionId, magnitude: killMagnitude(CRUSH.magnitude), witnesses: witnessesFor(regionId, target), payload: { soldiers: 1 } }];
  const seen = new Set([target]);
  for (const n of r?.neighbours ?? []) {
    const owner = model.regions.get(n)?.owner;
    if (!owner || seen.has(owner)) continue;
    seen.add(owner);
    acts.push({ type: 'show', target: owner, region: n, magnitude: CRUSH.show, witnesses: [] });
  }
  return acts;
}

function crushEffect(regionId) {
  const t = tiles.get(regionId);
  if (!t) return;
  const ring = flatTorus(1, 0.14, 0xd8c7a0, 0.7);
  ring.material.transparent = true;
  t.group.add(ring);
  puff(t.group.position.clone().setY(t.group.position.y + 0.5), new THREE.Color(0x8a7a5a));
  effects.push({ life: 0.9, update(dt, e) { const k = 1 - e.life / 0.9; ring.scale.setScalar(1 + k * 3.2); ring.material.opacity = 1 - k; }, end() { t.group.remove(ring); } });
}
