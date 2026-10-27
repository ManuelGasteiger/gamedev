// Health and wounds (docs/core-loop.md section 4): the HUD bar, what a hit looks and sounds like (the dragon flashes,
// the screen edge reddens, the number rises), the wound a harmful act will cost with the second press a lethal one needs,
// and the log of hits the death recap reads.
const HARMFUL = new Set(['burn', 'take', 'kill', 'take_hostage', 'crush']);
const WOUND = { arm: null, armMs: 4000, low: 0.3, vignette: null };

function hasHealth() { return typeof model?.dragon?.health_max === 'number'; }
function health() {
  const d = model.dragon, max = d.health_max ?? 0, wounds = clamp(d.wounds ?? 0, 0, max);
  return { max, wounds, left: Math.max(0, max - wounds) };
}

function renderHealthHud() {
  const on = hasHealth(), stat = $('h-health-stat');
  stat.classList.toggle('hidden', !on);
  if (!on) return;
  const h = health(), share = h.left / Math.max(1, h.max);
  $('h-health').textContent = `${Math.round(h.left)}/${Math.round(h.max)}`;
  $('h-health-bar').style.width = (share * 100) + '%';
  stat.classList.toggle('warn', share < WOUND.low);
  stat.title = `${numText(h.wounds)} wounds against ${Math.round(h.max)} health. Wounds stay until you sleep: a rest as long as your phase's rest window heals them all.`;
  vignette().classList.toggle('low', share < WOUND.low);
}

function vignette() {
  if (!WOUND.vignette) { WOUND.vignette = document.createElement('div'); WOUND.vignette.id = 'vignette'; document.body.appendChild(WOUND.vignette); }
  return WOUND.vignette;
}

function woundCost(name, regionId) { return HARMFUL.has(name) && hasHealth() ? actWound(regionId) : 0; }
function isLethal(w) { return hasHealth() && w > 0 && w >= health().left; }
function lethalArmed(name, regionId) { return !!WOUND.arm && WOUND.arm.key === name + '|' + regionId && performance.now() - WOUND.arm.at < WOUND.armMs; }

// the cost text of an action row: stamina, then the wound it costs here and the health left after; a lethal one says so
function woundCostText(name, regionId) {
  const w = woundCost(name, regionId);
  if (!(w > 0)) return '';
  return isLethal(w) ? ` · <b class="lethal">${numText(w)} wounds: lethal${lethalArmed(name, regionId) ? ', press again' : ''}</b>`
    : ` · <span class="wcost">${numText(w)} wounds → ${Math.ceil(health().left - w)} left</span>`;
}

// a lethal act runs only on a second press within a few seconds
function confirmLethal(name, regionId) {
  const w = woundCost(name, regionId);
  if (!isLethal(w) || lethalArmed(name, regionId)) { WOUND.arm = null; return true; }
  WOUND.arm = { key: name + '|' + regionId, at: performance.now() };
  toast(`This costs ${numText(w)} wounds and you have ${Math.ceil(health().left)} health left: it would kill you, and death is final. Press again to do it anyway.`, true);
  sfx('warn');
  renderActions();
  return false;
}

// what an act or a passive wound did to the dragon (the `wounds` of apply_acts); ctx = { region, act, shown, rate, seconds }
function woundsFrom(res, ctx = {}) {
  const w = res?.wounds;
  if (!w || !hasHealth()) return;
  model.dragon.wounds = w.total;
  if (world?.dragon) world.dragon.wounds = w.total;
  if (typeof w.max === 'number') model.dragon.health_max = w.max;
  if (w.taken > 0) { hitFeedback(w.taken); logHit(ctx, w.taken); }
  renderHealthHud();
  refreshWoundTexts();
}

// after a hit: the open panel's wound costs and health left, in place, so its controls keep their values
function refreshWoundTexts() {
  const panel = $('actions'), id = selectedRegion;
  if (!id || panel.classList.contains('hidden')) return;
  const strike = panel.querySelector('.defs.strike');
  if (strike) strike.outerHTML = strikeLineHtml(id) || '<span></span>';
  panel.querySelectorAll('.action[data-id]').forEach((row) => {
    const cost = row.querySelector('.cost'), verb = row.dataset.id === 'hostage' ? 'take_hostage' : row.dataset.id;
    if (!cost || row.classList.contains('off') || !HARMFUL.has(verb)) return;
    cost.innerHTML = (cost.dataset.base ?? '') + woundCostText(verb, id);
    row.classList.toggle('lethal', isLethal(woundCost(verb, id)));
  });
}

// every hit of the dragon's life with the number shown before it, for the death recap; it travels in the save
function logHit(ctx, taken) {
  play.hits = play.hits ?? [];
  const last = play.hits[play.hits.length - 1];
  if (ctx.act === 'passive' && last?.act === 'passive' && last.region === ctx.region && last.age === play.age) { last.taken = round1(last.taken + taken); last.seconds += ctx.seconds ?? 0; return; }
  play.hits.push({ year: model.year, age: play.age, act: ctx.act ?? 'act', region: ctx.region ?? '', court: model.regions.get(ctx.region)?.owner ?? '', taken: round1(taken), shown: ctx.shown ?? null, rate: ctx.rate ?? null, seconds: ctx.seconds ?? 0 });
  if (play.hits.length > 40) { play.hits.sort((a, b) => b.taken - a.taken); play.hits.length = 30; play.hits.sort((a, b) => a.year - b.year); }
}

function biggestHits(n = 3) { return (play.hits ?? []).slice().sort((a, b) => b.taken - a.taken).slice(0, n); }

function hitText(h) {
  const where = `${regionTitle(h.region)}${h.court ? ` (${factionTitle(h.court)})` : ''}`;
  if (h.act === 'passive') return `${Math.max(1, Math.round(h.seconds))} seconds inside the rings at ${where}: ${numText(h.taken)} wounds. The rings showed ${numText(h.rate ?? 0)} a second.`;
  const verb = { burn: 'Burning', take: 'Taking from', kill: 'Killing the army at', take_hostage: 'Taking a hostage at', crush: 'Crushing', kill_person: 'The fight at' }[h.act] ?? capFirst(h.act);
  return `${verb} ${where}: ${numText(h.taken)} wounds. The panel showed ${h.shown ? numText(h.shown) : 'none'} before the act.`;
}

function hitFeedback(amount) {
  lookHit();
  sfx('hit');
  const v = vignette();
  v.classList.remove('hit');
  void v.offsetWidth;
  v.classList.add('hit');
  const el = document.createElement('div');
  el.className = 'dmg';
  el.textContent = '−' + numText(amount);
  $('floats').appendChild(el);
  const start = performance.now(), dx = (Math.random() - 0.5) * 40;
  effects.push({ life: 1.4, update() {
    const p = dragon.group.position.clone().add(new THREE.Vector3(0, 1.4 * dragon.group.scale.x, 0)).project(camera);
    const k = (performance.now() - start) / 1400;
    el.style.left = ((p.x + 1) / 2 * innerWidth + dx) + 'px';
    el.style.top = ((1 - p.y) / 2 * innerHeight - 30 - k * 60) + 'px';
    el.style.opacity = String(1 - k * k);
  }, end() { el.remove(); } });
}

addStyle(`
  #hud .bar.health i { background: linear-gradient(90deg, #b8392e, #d9634f); }
  #hud #h-health-stat.warn b { color: var(--bad); }
  #hud #h-health-stat.warn .bar { border-color: var(--bad); }
  .action .cost .wcost { color: #ff9c8a; }
  .action .cost .lethal { color: #fff; background: var(--bad); border-radius: 2px; padding: 0 3px; font-weight: 600; }
  .action.lethal { background: #3a1512; }
  .float.dmg, .dmg { position: absolute; transform: translate(-50%, -50%); font: 700 20px system-ui, sans-serif; color: #ff5a46; text-shadow: 0 1px 3px #000, 0 0 8px #000; white-space: nowrap; }
  #vignette { position: fixed; inset: 0; pointer-events: none; z-index: 4; opacity: 0; background: radial-gradient(ellipse at center, transparent 48%, rgba(200, 16, 10, 0.62) 100%); }
  #vignette.low { opacity: 0.45; }
  #vignette.hit { animation: vhit 0.7s ease-out; }
  @keyframes vhit { 0% { opacity: 1; } 100% { opacity: 0; } }
  #vignette.low.hit { animation: vhitlow 0.7s ease-out; }
  @keyframes vhitlow { 0% { opacity: 1; } 100% { opacity: 0.45; } }
`);
