// Growth (v0.4): the five axes as HUD chips with progress to the next talent, the "talent ready" badge, and the growth
// screen over a close-up of the dragon (also the lair's Growth tab): per-axis bars, then the open pick as two big cards
// (roguelite rewards: few options, big text, an instant visible result), or the trees of taken, locked and future talents
// when nothing waits. Everything here waits for dragon.growth in the world view.
const AXES04 = [
  { id: 'might', name: 'Might', colour: '#d0845a' },
  { id: 'flame', name: 'Flame', colour: '#ff9433' },
  { id: 'arcana', name: 'Arcana', colour: '#b48cff' },
  { id: 'lore', name: 'Lore', colour: '#e0c060' },
  { id: 'dominion', name: 'Dominion', colour: '#7fb0ff' },
];
const FED_BY = { might: 'the gold you sleep on', flame: 'what you burned this age', arcana: 'artifacts', lore: 'books and royals', dominion: 'art, tribute and worship' };
const POOL_OF = { might: ['size', 'size'], arcana: ['magic', 'magic'], lore: ['knowledge', 'knowledge'], dominion: ['faith_pool', 'faith pool'] };
const NODES_DEFAULT = [3, 8, 15, 25];
const POOL_HUD = ['h-size', 'h-knowledge', 'h-magic', 'h-faith', 'h-threat'];
const growthUi = { catalogue: null, open: false, saved: null, el: null, pick: 0, busy: false, afterWake: false };

function hasGrowth() { return !!model?.dragon?.growth; }

function talentCatalogue() {
  if (!growthUi.catalogue && hasGrowth()) {
    const res = sim({ cmd: 'talents' });
    if (res?.ok) growthUi.catalogue = res;
  }
  return growthUi.catalogue;
}
function talentById(id) { return talentCatalogue()?.talents?.find((t) => t.id === id) ?? null; }
function fedText(f) { return Array.isArray(f) ? f.join(', ').replace(/_/g, ' ') : typeof f === 'string' && f ? f.replace(/_/g, ' ') : null; }
function axisMeta(id) {
  const base = AXES04.find((a) => a.id === id) ?? { id, name: cap(id), colour: '#c8c8c8' };
  const cat = talentCatalogue()?.axes?.find((a) => a.id === id);
  return { ...base, name: cat?.name ?? base.name, fed: fedText(cat?.fed_by) ?? FED_BY[id] ?? '', rankEvery: cat?.rank_every ?? 25, rankGrowth: Math.max(1, cat?.rank_growth ?? 1),
    rankText: cat?.rank?.text ?? '', milestoneEvery: cat?.milestone?.every ?? 0 };
}
function axisColour(id) { return (AXES04.find((a) => a.id === id) ?? {}).colour ?? '#c8c8c8'; }
function pointsText(v) { return v >= 10 || Math.abs(v - Math.round(v)) < 0.05 ? String(Math.floor(v + 0.05)) : v.toFixed(1); }
function phaseNow() { return world?.phase?.phase ?? 1; }
function pendingPicks() { return model?.dragon?.picks ?? []; }
function pickOpen(p) { return p.open ?? !(p.needs_phase > phaseNow()); }
function openPicks() { return pendingPicks().filter(pickOpen); }

// a talent's `node` is an index into the catalogue's `nodes` (the points it opens at)
function nodePoints(i) { return (talentCatalogue()?.nodes ?? NODES_DEFAULT)[i] ?? 0; }
function authoredNodes(axis) { return [...new Set((talentCatalogue()?.talents ?? []).filter((t) => t.axis === axis).map((t) => t.node))].sort((a, b) => a - b); }

// where the axis' current rank began (talents.cpp rank_state): the first rank costs rank_every past the last authored
// node, each next one rank_growth times the one before
function rankFloor(id, pts) {
  const points = authoredNodes(id).map(nodePoints), a = axisMeta(id);
  if (!points.length || !(a.rankEvery > 0)) return null;
  let at = Math.max(...points), cost = a.rankEvery;
  while (at + cost <= pts) { at += cost; cost *= a.rankGrowth; }
  return at;
}

// progress from the node below to the next one (or from the last rank to the next)
function axisProgress(id) {
  const d = model.dragon, pts = d.growth?.[id] ?? 0, next = d.next_node?.[id];
  if (typeof next !== 'number') return { pts, next: null, share: 1 };
  const points = authoredNodes(id).map(nodePoints), last = points.length ? Math.max(...points) : 0;
  const below = points.filter((n) => n < next);
  const from = next > last ? rankFloor(id, pts) ?? next - axisMeta(id).rankEvery : below.length ? Math.max(...below) : 0;
  return { pts, next, share: clamp((pts - from) / Math.max(0.001, next - from), 0, 1) };
}

// ---- HUD ------------------------------------------------------------------------------------------------------------
function renderGrowthHud() {
  const on = hasGrowth();
  $('hud').classList.toggle('v04', on);
  for (const id of POOL_HUD) $(id)?.parentElement.classList.toggle('hidden', on);
  $('h-growth').classList.toggle('hidden', !on);
  $('lair-tab-growth').classList.toggle('hidden', !on);
  const n = on ? openPicks().length : 0;
  $('h-picks').classList.toggle('hidden', !n);
  if (!on) return;
  $('h-picks').textContent = n === 1 ? 'talent ready' : `${n} talents ready`;
  $('h-growth').innerHTML = AXES04.map((a) => chipHtml(a.id)).join('');
}

function chipHtml(id) {
  const a = axisMeta(id), p = axisProgress(id), d = model.dragon;
  const waiting = openPicks().some((x) => x.axis === id);
  const pool = POOL_OF[id] ? `; ${POOL_OF[id][1]} ${fmt(d[POOL_OF[id][0]] ?? 0)}` : d.hoard?.embers !== undefined ? `; embers ${fmt(d.hoard.embers, 1)}` : '';
  const title = `${a.name} ${pointsText(p.pts)}${p.next !== null ? `, next talent at ${p.next}` : ''}${waiting ? ', a choice is waiting' : ''}. Fed by ${a.fed}${pool}.`;
  return `<button class="chip${waiting ? ' ready' : ''}" data-axis="${id}" style="--c:${a.colour}" title="${escapeHtml(title)}"><span>${a.name} <b>${pointsText(p.pts)}</b></span><i class="prog"><i style="width:${Math.round(p.share * 100)}%"></i></i></button>`;
}

// ---- the parts of the screen ----------------------------------------------------------------------------------------
function axesHtml() {
  return `<div class="gaxes">${AXES04.map((x) => {
    const a = axisMeta(x.id), p = axisProgress(x.id), r = model.dragon.ranks?.[x.id] ?? 0;
    return `<div class="gax" style="--c:${a.colour}"><div class="gtop"><span class="an">${a.name}</span><span class="ap">${pointsText(p.pts)}</span></div>
      <div class="gmeter"><i style="width:${Math.round(p.share * 100)}%"></i></div>
      <div class="gnext">${p.next !== null ? `next at ${p.next}` : ''}${r ? ` · rank ${r}` : ''}</div><div class="gfed">from ${escapeHtml(a.fed)}</div></div>`;
  }).join('')}</div>`;
}

function bigCard(t, key) {
  const see = lookSeeText(t);
  return `<button class="gcard" data-pick="${t.id}" style="--c:${axisColour(t.axis)}"><span class="key">${key}</span><b class="tn">${escapeHtml(t.name)}</b>
    <span class="tx">${escapeHtml(t.text)}</span>${see ? `<span class="see">You will see ${escapeHtml(see)}.</span>` : ''}</button>`;
}

function stageHtml() {
  const open = openPicks();
  if (!open.length) return '';
  growthUi.pick = clamp(growthUi.pick, 0, open.length - 1);
  const p = open[growthUi.pick], a = axisMeta(p.axis);
  const options = (p.options ?? []).map(talentById).filter(Boolean);
  const at = (x) => x.points ?? nodePoints(x.node);
  const queue = open.map((x, i) => (i === growthUi.pick ? '' : `<button class="gq" data-queue="${i}" style="--c:${axisColour(x.axis)}">${axisMeta(x.axis).name} · ${at(x)}</button>`)).join('');
  const locked = pendingPicks().filter((x) => !pickOpen(x)).map((x) => `<span class="gq locked" title="a moult opens it">${axisMeta(x.axis).name} · ${at(x)}: needs phase ${x.needs_phase}</span>`).join('');
  return `<div class="gstage" style="--c:${a.colour}">
    <div class="gpickhead"><span class="ga">${a.name} reached ${at(p)}</span><span>Choose one. The other is gone for good.</span></div>
    <div class="gcards">${options.map((t, i) => bigCard(t, i + 1)).join('<span class="or">or</span>')}</div>
    ${queue || locked ? `<div class="gqueue">${queue ? `<span class="muted">also waiting</span>${queue}` : ''}${locked}</div>` : ''}</div>`;
}

function nodeRows(id) {
  const cat = talentCatalogue();
  const phases = cat?.node_phase ?? [1, 1, 1, 2];
  const taken = new Set(model.dragon.talents ?? []);
  return authoredNodes(id).map((i) => {
    const options = (cat?.talents ?? []).filter((t) => t.axis === id && t.node === i);
    const pick = pendingPicks().find((p) => p.axis === id && p.node === i);
    const chosen = options.find((t) => taken.has(t.id));
    const state = chosen ? 'taken' : pick ? (pickOpen(pick) ? 'open' : 'locked') : 'future';
    return { n: nodePoints(i), options, chosen, state, needs: pick?.needs_phase ?? phases[i] ?? 1 };
  });
}

function nodeHtml(row) {
  const names = row.options.map((t) => escapeHtml(t.name)).join(' or ');
  const tips = row.options.map((t) => `${t.name}: ${t.text}`).join('\n');
  let body;
  if (row.state === 'taken') {
    const other = row.options.find((t) => t !== row.chosen);
    body = `<b class="tn">${escapeHtml(row.chosen.name)}</b><span class="tx">${escapeHtml(row.chosen.text)}</span>${other ? `<span class="closed" title="${escapeHtml(other.text)}">${escapeHtml(other.name)}</span>` : ''}`;
  } else if (row.state === 'open') body = `<span class="ask">ready: ${names}</span>`;
  else if (row.state === 'locked') body = `<span class="lock">needs phase ${row.needs}: a moult</span><span class="tx" title="${escapeHtml(tips)}">${names}</span>`;
  else body = `<span class="tx" title="${escapeHtml(tips)}">${names}</span>`;
  return `<li class="gn ${row.state}"><span class="at">${row.n}</span><div class="gb">${body}</div></li>`;
}

function treesHtml() {
  return `<div class="gcols">${AXES04.map((x) => {
    const a = axisMeta(x.id);
    const r = model.dragon.ranks?.[x.id] ?? 0;
    const dearer = a.rankGrowth > 1 ? `, each next one ${Math.round((a.rankGrowth - 1) * 100)}% dearer` : '';
    const step = a.milestoneEvery > 1 ? ` Every ${ordinal(a.milestoneEvery)} rank is a bigger step.` : '';
    return `<section class="gcol" style="--c:${a.colour}"><header>${a.name}</header><ol class="gnodes">${nodeRows(x.id).map(nodeHtml).join('')}</ol>
      ${a.rankText ? `<div class="granks">${r ? `<b>Rank ${r}.</b> ` : ''}Past the last talent, a rank ${a.rankEvery} points on${dearer}: ${escapeHtml(a.rankText)}.${step}</div>` : ''}</section>`;
  }).join('')}</div>`;
}

function growthSummary() {
  const open = openPicks().length, locked = pendingPicks().length - open;
  const num = (n) => capFirst(TEXT.misc?.numbers?.[n] ?? String(n));
  const lead = open ? `${num(open)} ${open === 1 ? 'talent is' : 'talents are'} ready.` : 'No talent is ready. Bring loot home and sleep on it to grow.';
  return lead + (locked ? ` ${num(locked)} more ${locked === 1 ? 'waits' : 'wait'} for the next phase.` : '');
}

function growthBodyHtml(overlay) {
  const stage = stageHtml();
  return `<header class="ghead"><h2>Growth</h2><p>${escapeHtml(growthSummary())}</p><span class="spacer"></span>${overlay ? `<button data-close>${stage ? 'Later' : 'Close'}</button>` : ''}</header>
    ${axesHtml()}${stage || treesHtml()}`;
}

function growthTabHtml() { return talentCatalogue() ? `<div class="gtab">${growthBodyHtml(false)}${openPicks().length ? treesHtml() : ''}</div>` : '<div class="muted">The talent list is not available.</div>'; }

function bindGrowth(root) {
  root.querySelectorAll('[data-pick]').forEach((b) => b.addEventListener('click', () => chooseTalent(b.dataset.pick, root)));
  root.querySelectorAll('[data-queue]').forEach((b) => b.addEventListener('click', () => { growthUi.pick = Number(b.dataset.queue); rerenderGrowth(); }));
  root.querySelectorAll('[data-close]').forEach((b) => b.addEventListener('click', closeGrowth));
}

function renderGrowth() {
  growthUi.el.innerHTML = `<div class="gshade" data-close title="close"></div><section class="gsheet">${growthBodyHtml(true)}</section>`;
  bindGrowth(growthUi.el);
}
function rerenderGrowth() { if (growthUi.open) renderGrowth(); if (play.landed && lairTab === 'growth') renderLair(); }

function openGrowth(axis = null) {
  if (!hasGrowth() || montage.active || play.dead) return;
  if (!talentCatalogue()) { toast('the talent list is not available', true); return; }
  if (wakeOpen()) closeWake(true);
  if (play.landed) takeOff();
  if (!growthUi.open) growthUi.saved = { zoom: cam.zoom };
  growthUi.open = true;
  const open = openPicks();
  growthUi.pick = Math.max(0, open.findIndex((p) => p.axis === axis));
  keys.clear();
  releaseBurnHold();
  dragon.target = null;
  dragon.heading = cam.yaw + Math.PI / 2 - 0.5;   // a three-quarter profile: horns, crest, spikes and wings show
  play.landOnArrival = false;
  cam.follow = false;
  cam.zoom = clamp(4.6 / dragon.group.scale.x, 1.5, 3.4);
  resize();
  renderGrowth();
  growthUi.el.classList.remove('hidden');
  frameDragon();
  renderActions();
}

function closeGrowth() {
  if (!growthUi.open) return;
  growthUi.open = false;
  growthUi.el.classList.add('hidden');
  cam.zoom = growthUi.saved?.zoom ?? 1;
  cam.follow = true;
  growthUi.saved = null;
  LOOK.zoomPulse = null;
  resize();
  renderActions();
}

function growthKey(e) {
  if (e.key === 'Escape' || (e.key === 'Enter' && !openPicks().length)) { e.preventDefault(); closeGrowth(); return; }
  const card = growthUi.el.querySelectorAll('.gcard')[Number(e.key) - 1];
  if (card && !growthUi.busy) chooseTalent(card.dataset.pick, growthUi.el);
}

// the dragon's body in the middle of the band between the HUD and the sheet
function frameDragon() {
  updateCamera();
  const up = new THREE.Vector3(0, 1, 0).applyQuaternion(camera.quaternion);
  const sheet = growthUi.el.querySelector('.gsheet');
  const top = $('hud').getBoundingClientRect().bottom / innerHeight, bottom = sheet ? sheet.getBoundingClientRect().top / innerHeight : 0.45;
  const body = dragon.group.position.clone().add(new THREE.Vector3(0, 0.9 * dragon.group.scale.x, 0));
  cam.target.copy(body).addScaledVector(up, ((top + bottom) / 2 - 0.5) * 2 * (23.5 / cam.zoom));
}

// the chosen card rises, the other is struck out, the dragon changes with a flash; then the next choice
function chooseTalent(id, root) {
  if (growthUi.busy) return;
  const cards = [...root.querySelectorAll('.gcard')];
  const res = pickTalent(id);
  if (!res?.ok) return;
  growthUi.busy = true;
  cards.forEach((c) => c.classList.add(c.dataset.pick === id ? 'chosen' : 'gone'));
  setTimeout(() => { growthUi.busy = false; growthUi.pick = 0; rerenderGrowth(); if (growthUi.open) frameDragon(); }, 1150);
}

function pickTalent(id) {
  const t = talentById(id);
  const res = sim({ cmd: 'pick_talent', id });
  if (!res?.ok) { toast(`${escapeHtml(t?.name ?? id)} cannot be taken: ${escapeHtml(res?.error ?? 'that choice is not open')}`, true); return res; }
  pushEvents(res);
  const stamina = effect('stamina_add');
  refreshWorld();
  play.stamina += Math.round(effect('stamina_add') - stamina);   // more stamina is felt at once, not at the next waking
  renderHud();
  const see = lookSeeText(t);
  toast(`You took <b>${escapeHtml(t?.name ?? id)}</b>${see ? `: ${escapeHtml(see)}` : ''}.`);
  lookFlourish(t?.axis);
  renderChronicle();
  return res;
}

// ---- the wake screen's growth section ---------------------------------------------------------------------------------
function growthWakeHtml(w) {
  if (!hasGrowth()) return '';
  const gained = Object.entries(w.growth_gained ?? {}).filter(([, v]) => v >= 0.05).sort((a, b) => b[1] - a[1]);
  const chips = gained.map(([k, v]) => `<span class="wgain" style="--c:${axisColour(k)}">${axisMeta(k).name} +${pointsText(v)}</span>`).join(' ');
  const healed = w.healed > 0.05 ? ` Sleep closed ${hasHealth() && health().wounds < 0.05 ? 'all ' : ''}${numText(w.healed)} wounds.` : '';
  const ranks = (w.ranks ?? []).map((e) => ` ${eventSentence(e) ?? `Your ${axisMeta(e.object).name} rose to rank ${e.magnitude}.`}`).join('');
  const open = openPicks().length;
  const lead = gained.length ? `You grew in your sleep: ${chips}.${escapeHtml(ranks)}` : 'Nothing in the hoard fed your growth this time.';
  const pick = open ? `<div class="wpick">${open === 1 ? 'A talent is' : `${capFirst(TEXT.misc?.numbers?.[open] ?? String(open))} talents are`} ready to choose. <button class="primary" data-growth-open>Choose</button></div>` : '';
  return `<section class="grew"><div class="row2"><div class="margin">growth</div><div><p>${lead}${healed}</p>${pick}</div></div></section>`;
}

growthUi.el = document.createElement('div');
growthUi.el.id = 'growth';
growthUi.el.className = 'hidden';
document.body.appendChild(growthUi.el);
$('h-growth').addEventListener('click', (e) => { const b = e.target.closest('[data-axis]'); if (b) openGrowth(b.dataset.axis); });
$('h-picks').addEventListener('click', () => openGrowth());

addStyle(`
  #hud .chips { display: inline-flex; gap: 3px; min-width: 0; overflow: hidden; flex: 0 1 auto; }
  #hud .chip { display: inline-flex; flex-direction: column; gap: 2px; padding: 2px 6px 3px; background: #15181f; border: 1px solid var(--line); border-radius: 3px; color: var(--dim); line-height: 1.1; white-space: nowrap; }
  #hud .chip b { color: var(--fg); font-weight: 600; }
  #hud .chip .prog { display: block; height: 2px; background: #2a2e38; border-radius: 1px; overflow: hidden; }
  #hud .chip .prog i { display: block; height: 100%; background: var(--c); }
  #hud .chip.ready { border-color: var(--c); }
  #hud .chip.ready span { color: var(--fg); }
  #hud .chip:hover { border-color: var(--c); }
  #hud button.picks { background: var(--accent); color: #15171c; border-color: var(--accent); font-weight: 600; animation: pickpulse 1.6s ease-in-out infinite; }
  @keyframes pickpulse { 50% { box-shadow: 0 0 0 4px #e0b24a55; } }
  @media (max-width: 1500px) { #hud .chip { padding: 2px 4px 3px; } #hud.v04 { gap: 5px; } #hud.v04 h1, #hud.v04 #h-hunger-stat .bar { display: none; } }
  @media (max-width: 1180px) { #hud .chips { gap: 2px; } #hud .chip { padding: 1px 2px 2px; } #hud.v04 { gap: 3px; } }

  #growth { position: fixed; inset: 0; z-index: 12; display: flex; flex-direction: column; }
  #growth .gshade { flex: 1; min-height: 150px; cursor: zoom-out; background: linear-gradient(#0b0d1200 60%, #0b0d12aa); }
  #growth .gsheet { max-height: 62vh; display: flex; flex-direction: column; background: var(--panel-solid); border-top: 1px solid var(--line); box-shadow: 0 -16px 40px #000a; overflow: auto; }
  .ghead { display: flex; align-items: baseline; gap: 14px; padding: 12px 18px 8px; }
  .ghead h2 { margin: 0; font: normal 23px/1.2 Georgia, 'Palatino Linotype', 'DejaVu Serif', serif; color: #f4eee0; }
  .ghead p { margin: 0; color: #c9c2b2; font: 14.5px/1.4 Georgia, 'Palatino Linotype', 'DejaVu Serif', serif; }
  .ghead .spacer { flex: 1; }
  .gaxes { display: grid; grid-template-columns: repeat(5, minmax(0, 1fr)); gap: 10px; padding: 0 18px 10px; border-bottom: 1px solid var(--line); }
  .gax .gtop { display: flex; justify-content: space-between; align-items: baseline; }
  .gax .an { font-weight: 600; color: var(--c); letter-spacing: .03em; }
  .gax .ap { font-size: 19px; font-weight: 600; font-variant-numeric: tabular-nums; }
  .gmeter { height: 5px; background: #11131a; border-radius: 3px; overflow: hidden; margin: 3px 0 2px; }
  .gmeter i { display: block; height: 100%; background: var(--c); }
  .gax .gnext { color: var(--fg); font-size: 11px; }
  .gax .gfed { color: var(--dim); font-size: 10.5px; line-height: 1.25; }
  .gstage { padding: 12px 18px 16px; background: linear-gradient(color-mix(in srgb, var(--c) 10%, transparent), transparent 70%); }
  .gpickhead { display: flex; gap: 12px; align-items: baseline; margin-bottom: 10px; font: 15px Georgia, 'Palatino Linotype', 'DejaVu Serif', serif; color: #c9c2b2; }
  .gpickhead .ga { font: 600 13px system-ui, sans-serif; letter-spacing: .08em; text-transform: uppercase; color: var(--c); }
  .gcards { display: flex; align-items: stretch; justify-content: center; gap: 14px; }
  .gcards .or { align-self: center; font: italic 15px Georgia, serif; color: var(--dim); }
  .gcard { position: relative; flex: 0 1 360px; min-height: 150px; text-align: left; display: flex; flex-direction: column; gap: 8px; padding: 16px 18px 16px; border: 1px solid color-mix(in srgb, var(--c) 70%, var(--line)); border-radius: 6px;
    background: linear-gradient(160deg, color-mix(in srgb, var(--c) 16%, #232733), #1c1f28 60%); color: var(--fg); transition: transform .18s, box-shadow .18s, opacity .35s; font: inherit; cursor: pointer; }
  .gcard:hover, .gcard:focus-visible { transform: translateY(-4px); box-shadow: 0 10px 28px #000a, 0 0 0 1px var(--c); outline: none; }
  .gcard .key { position: absolute; top: 10px; right: 12px; font: 600 11px ui-monospace, monospace; color: var(--c); border: 1px solid var(--c); border-radius: 3px; padding: 0 5px; }
  .gcard .tn { font: normal 26px/1.15 Georgia, 'Palatino Linotype', 'DejaVu Serif', serif; color: #f6f0e2; padding-right: 24px; }
  .gcard .tx { font-size: 15.5px; line-height: 1.4; color: #e2e4ea; }
  .gcard .see { margin-top: auto; font: italic 14px/1.35 Georgia, 'Palatino Linotype', 'DejaVu Serif', serif; color: var(--c); }
  .gcard.chosen { transform: translateY(-8px) scale(1.04); box-shadow: 0 0 0 2px var(--c), 0 0 40px color-mix(in srgb, var(--c) 60%, transparent); }
  .gcard.gone { opacity: .25; transform: scale(.96); }
  .gcard.gone .tn { text-decoration: line-through; }
  .gqueue { display: flex; gap: 6px; align-items: center; justify-content: center; margin-top: 12px; flex-wrap: wrap; font-size: 12px; }
  .gq { border-color: var(--c); color: var(--c); background: #1a1d25; font-size: 12px; padding: 2px 9px; }
  .gq.locked { border: 1px dashed var(--line); color: #a88d58; border-radius: 3px; padding: 2px 9px; }
  .gcols { display: grid; grid-template-columns: repeat(5, minmax(0, 1fr)); }
  .gcol { padding: 8px 12px 12px; border-left: 1px solid var(--line); min-width: 0; }
  .gcol:first-child { border-left: 0; }
  .gcol header { font-weight: 600; color: var(--c); letter-spacing: .03em; margin-bottom: 2px; }
  ol.gnodes { list-style: none; margin: 4px 0 0; padding: 0; position: relative; }
  ol.gnodes::before { content: ''; position: absolute; left: 10px; top: 10px; bottom: 10px; width: 1px; background: var(--line); }
  .gn { display: grid; grid-template-columns: 21px minmax(0, 1fr); gap: 8px; padding: 5px 0; position: relative; }
  .gn .at { width: 21px; height: 21px; border-radius: 50%; display: grid; place-items: center; font-size: 10.5px; font-weight: 600; background: var(--panel-solid); border: 1px solid var(--line); color: var(--dim); z-index: 1; }
  .gn.taken .at { background: var(--c); border-color: var(--c); color: #15171c; }
  .gn.open .at { border-color: var(--c); color: var(--c); box-shadow: 0 0 0 3px color-mix(in srgb, var(--c) 25%, transparent); }
  .gn .gb { display: flex; flex-direction: column; gap: 2px; min-width: 0; }
  .gn .tn { font: 600 14px/1.25 Georgia, 'Palatino Linotype', 'DejaVu Serif', serif; color: #f2ecdf; }
  .gn .tx { font-size: 11.5px; line-height: 1.35; color: var(--dim); }
  .gn.future .tx, .gn.locked .tx { color: #6f7483; }
  .gn .closed { font-size: 11px; color: #6a6f7c; text-decoration: line-through; }
  .gn .lock { font-size: 11px; color: #c8a96a; }
  .gn .ask { font-size: 11.5px; color: var(--c); font-weight: 600; }
  .granks { color: #6f7483; font-size: 10.5px; margin-top: 6px; line-height: 1.35; }
  .gtab .ghead { padding: 0 0 8px; }
  .gtab .gaxes { padding: 0 0 10px; }
  .gtab .gstage { padding: 12px 0 14px; }
  .gtab .gcols { margin-top: 8px; border-top: 1px solid var(--line); }
  #wake .wgain { color: var(--c); white-space: nowrap; font-weight: 600; }
  #wake .wpick { margin-top: 8px; font: 15px/1.5 Georgia, 'Palatino Linotype', 'DejaVu Serif', serif; }
  #wake .wpick button { font: 600 12px system-ui, sans-serif; margin-left: 8px; }
`);
