// Hooks (docs/core-loop.md section 5): this age's situations from world.hooks. The wake screen leads with them (what
// happened, where, what answers it, what it pays, what follows if you let it be, a go that flies there); each has a pin
// over its region and a row in the HUD list; an answered hook says so at once, an ignored one on the next waking.
const HOOKS = { pins: new Map(), layer: null, known: new Map(), answered: [] };
const HOOK_VERB = { take: 'take', kill: 'kill the army', burn: 'burn', defend: 'defend', take_hostage: 'take a hostage', kill_person: 'kill', show: 'roar', give: 'give', crush: 'crush' };

function hasHooks() { return Array.isArray(world?.hooks); }
function currentHooks() { return world?.hooks ?? []; }
const HOOK_KILL = { slayers: 'fight the slayers', herd: 'hunt the herd', horde: 'scatter the horde', thief: 'catch the thief' };
function hookVerb(h, a) { return a === 'kill' && HOOK_KILL[h.kind] ? HOOK_KILL[h.kind] : HOOK_VERB[a] ?? a.replace(/_/g, ' '); }
function hookRegion(h) { return h.resolve?.region || h.region || ''; }
function hookAnswer(h) {
  const who = h.resolve?.person || h.person;
  const acts = (h.resolve?.acts ?? []).map((a) => (a === 'kill_person' && who ? `kill ${nameOf(who)}` : hookVerb(h, a)));
  return capFirst((acts.length ? acts.join(' or ') : 'go') + (hookRegion(h) ? ` at ${regionTitle(hookRegion(h))}` : ''));
}
function remember(list) { for (const h of list) HOOKS.known.set(h.id, h); }

// ---- the HUD list ---------------------------------------------------------------------------------------------------
function renderHooksHud() {
  const el = $('hooks');
  if (!el) return;
  const list = currentHooks();
  remember(list);
  const show = hasHooks() && !montage.active && !play.landed && (list.length || HOOKS.answered.length);
  el.classList.toggle('hidden', !show);
  if (!show) return;
  el.innerHTML = `<h3>This age</h3><ul class="hooklist">${list.map((h, i) => `<li><b>${escapeHtml(h.title)}</b>${h.fight ? ' <span class="fight">a fight</span>' : ''}<br><span class="muted">${escapeHtml(hookAnswer(h))}</span> <button class="go" data-hook="${i}">go</button></li>`).join('')}
    ${HOOKS.answered.map((h) => `<li class="done">✓ ${escapeHtml(h.title)} <span class="muted">answered</span></li>`).join('')}</ul>`;
  el.querySelectorAll('[data-hook]').forEach((b) => b.addEventListener('click', () => goToHook(list[Number(b.dataset.hook)])));
}

function goToHook(h) {
  if (!h) return;
  if (wakeOpen()) closeWake(true);
  const region = hookRegion(h);
  if (!model.regions.has(region)) return;
  play.focusFaction = h.faction || model.regions.get(region)?.owner || '';
  flyTo(region);
  renderActions();
  toast(`flying to ${regionTitle(region)}: ${escapeHtml(h.title)}`);
}

// ---- what an act did to the hooks (apply_acts' hooks_resolved) -------------------------------------------------------
function hooksFrom(res) {
  const ids = (res?.hooks_resolved ?? []).map((x) => (typeof x === 'object' ? x.id : x));
  for (const id of ids) {
    const got = (res.hooks_resolved ?? []).find((x) => x?.id === id);
    const h = HOOKS.known.get(id) ?? { id, title: 'a call', reward_text: got?.reward_text ?? '' };
    if (HOOKS.answered.some((x) => x.id === id)) continue;
    HOOKS.answered.push(h);
    toast(`Answered: ${escapeHtml(h.title)}${h.reward_text ? ` (${escapeHtml(h.reward_text)})` : ''}`);
    sfx('hook');
    const region = hookRegion(h);
    if (tiles.has(region)) flashTile(region, new THREE.Color(PALETTE.crown));
  }
  if (ids.length) { syncHookPins(); renderHooksHud(); }
}

// ---- answering where the hook waits: a row per hook of this region in the action panel -------------------------------
const HOOK_TAKE = { vein: 'artifacts', library: 'books', herd: 'food', fat_city: 'art' };
const HOOK_ARMED = { id: '', until: 0 };

function hookActionsHtml(regionId) {
  const list = currentHooks().filter((h) => hookRegion(h) === regionId);
  if (!list.length) return '';
  return `<div class="hookacts">${list.map((h) => {
    const choices = h.resolve?.choices ?? [];
    const acts = h.resolve?.acts ?? [];
    const fight = h.fight_damage > 0 ? `<span class="fight">a fight</span> about ${Math.round(h.fight_damage)} wounds` : '';
    const buttons = choices.length
      ? choices.map((c) => `<button data-hookans="${h.id}" data-choice="${escapeHtml(c)}">${escapeHtml(capFirst(c))}</button>`).join(' ')
      : acts.map((a) => `<button data-hookans="${h.id}" data-act="${escapeHtml(a)}">${escapeHtml(capFirst(a === 'kill_person' ? `kill ${nameOf(h.resolve?.person || h.person)}` : hookVerb(h, a)))}</button>`).join(' ');
    return `<div class="hookact"><div><b>◆ ${escapeHtml(h.title)}</b> ${fight}</div>
      <div class="muted">${h.reward_text ? 'pays ' + escapeHtml(h.reward_text) : ''}</div><div class="ctl">${buttons}</div></div>`;
  }).join('')}</div>`;
}

function bindHookActions(panel) {
  panel.querySelectorAll('[data-hookans]').forEach((b) => b.addEventListener('click', () => answerHook(b.dataset.hookans, b.dataset.choice ?? '', b.dataset.act ?? '')));
}

// a fight that could kill asks twice, as a lethal act does
function hookFightConfirmed(h) {
  const left = (world.dragon?.health_max ?? 0) - (world.dragon?.wounds ?? 0);
  if (!(h.fight_damage > 0) || h.fight_damage < left) return true;
  const now = performance.now();
  if (HOOK_ARMED.id === h.id && now < HOOK_ARMED.until) { HOOK_ARMED.id = ''; return true; }
  HOOK_ARMED.id = h.id; HOOK_ARMED.until = now + 4000;
  toast(`this fight could kill you (${Math.round(h.fight_damage)} wounds, ${Math.round(left)} health left): press again to go ahead`, true);
  return false;
}

function answerHook(id, choice = '', actName = '') {
  const h = currentHooks().find((x) => x.id === id);
  if (!h || montage.active || play.dead || !hookFightConfirmed(h)) return null;
  const region = hookRegion(h);
  if (choice) {
    const before = snapshot();
    const res = sim({ cmd: 'answer_hook', id, choice });
    if (!res.ok) { toast(`cannot: ${res.error ?? res.reason ?? 'refused'}`, true); return res; }
    pushEvents(res);
    refreshWorld();
    showDeltas(before, region);
    renderChronicle();
    actOutcome(res, region, { act: 'hook', shown: h.fight_damage ?? 0 });
    return res;
  }
  const a = actName || h.resolve?.acts?.[0];
  if (a === 'kill_person') return killPerson(h.resolve?.person || h.person);
  if (a === 'kill') return act('kill', { region, attacker: h.faction });
  if (a === 'defend') return act('defend', { region, target: h.resolve?.target || h.other, against: h.resolve?.against || h.faction });
  if (a === 'take') return act('take', { region, target: h.faction, category: HOOK_TAKE[h.kind] ?? 'gold', magnitude: 0.3 });
  if (a === 'burn') return act('burn', { region, target: h.faction, magnitude: 0.4 });
  if (a === 'take_hostage') return act('take_hostage', { region, target: h.faction });
  return act(a, { region, target: h.faction });
}

// a new age: what was answered and what was let go, for the wake screen, then a clean slate
function closeHookAge(chronicle) {
  const ignored = (chronicle ?? []).filter((e) => e.kind === 'hook_ignored').map((e) => {
    const h = [...HOOKS.known.values()].find((x) => hookRegion(x) === e.object && (x.kind === e.tags?.[0] || !e.tags?.length)) ?? HOOKS.known.get(e.subject);
    return h ?? { title: `the call at ${regionTitle(e.object)}`, ignore_text: '' };
  });
  const last = { answered: HOOKS.answered.slice(), ignored };
  HOOKS.answered = [];
  HOOKS.known.clear();
  return last;
}

// ---- the wake screen's first section --------------------------------------------------------------------------------
function hooksWakeHtml(w) {
  if (!hasHooks()) return '';
  const list = currentHooks();
  remember(list);
  const last = w.hooksLast ?? { answered: [], ignored: [] };
  const lastLine = [
    ...last.answered.map((h) => `You answered <b>${escapeHtml(h.title)}</b>.`),
    ...last.ignored.map((h) => `You let <b>${escapeHtml(h.title)}</b> go${h.ignore_text ? `: ${escapeHtml(h.ignore_text)}` : ''}.`),
  ].join(' ');
  const cards = list.map((h, i) => `<div class="hookcard">
      <div class="ht">${escapeHtml(h.title)}${h.fight ? ' <span class="fight">a fight</span>' : ''}</div>
      <p class="hx">${escapeHtml(h.text ?? '')}</p>
      <dl><dt>where</dt><dd>${escapeHtml(regionTitle(hookRegion(h)))}${h.faction ? `, ${escapeHtml(factionTitle(h.faction))}` : ''}</dd>
        <dt>answer</dt><dd>${escapeHtml(hookAnswer(h))}</dd>
        ${h.reward_text ? `<dt>pays</dt><dd>${escapeHtml(h.reward_text)}</dd>` : ''}
        ${h.ignore_text ? `<dt>if ignored</dt><dd>${escapeHtml(h.ignore_text)}</dd>` : ''}</dl>
      <button class="go" data-hookgo="${i}">go</button></div>`).join('');
  return `<section class="hooks"><div class="row2"><div class="margin">this age</div><div>
      ${list.length ? `<div class="hookcards">${cards}</div>` : '<p class="quiet">Nothing calls for you this age.</p>'}
      ${lastLine ? `<p class="lastage">${lastLine}</p>` : ''}</div></div></section>`;
}

function bindHooksWake(el) { el.querySelectorAll('[data-hookgo]').forEach((b) => b.addEventListener('click', () => goToHook(currentHooks()[Number(b.dataset.hookgo)]))); }

// the line under a region's label
function hookLabelHtml(regionId) {
  const list = currentHooks().filter((h) => hookRegion(h) === regionId);
  return list.length ? `<div class="hk">${list.map((h) => '◆ ' + escapeHtml(h.title)).join('<br>')}</div>` : '';
}

// ---- pins over the hooks' regions -----------------------------------------------------------------------------------
function syncHookPins() {
  if (!HOOKS.layer) { HOOKS.layer = new THREE.Group(); scene.add(HOOKS.layer); }
  const wanted = new Set(currentHooks().map(hookRegion).filter((r) => tiles.has(r)));
  for (const [rid, g] of HOOKS.pins) if (!wanted.has(rid)) { HOOKS.layer.remove(g); HOOKS.pins.delete(rid); }
  for (const rid of wanted) {
    if (HOOKS.pins.has(rid)) continue;
    const [cx, cz] = regionCentre(rid), x = cx - 1.7, z = cz + 1.5, y = groundAt(x, z);
    const g = new THREE.Group();
    const gold = new THREE.MeshBasicMaterial({ color: 0xffc940 });
    const pole = new THREE.Mesh(new THREE.CylinderGeometry(0.035, 0.035, 2.6, 5), new THREE.MeshBasicMaterial({ color: 0xc9a040 }));
    pole.position.y = 1.3;
    const gem = new THREE.Mesh(new THREE.OctahedronGeometry(0.42, 0), gold);
    gem.position.y = 3.0;
    gem.scale.y = 1.4;
    const ring = new THREE.Mesh(new THREE.TorusGeometry(0.55, 0.05, 6, 24), gold);
    ring.rotation.x = Math.PI / 2;
    ring.position.y = 0.12;
    g.add(pole, gem, ring);
    g.position.set(x, y, z);
    g.userData.gem = gem;
    HOOKS.layer.add(g);
    HOOKS.pins.set(rid, g);
  }
}

function updateHookPins() {
  const t = performance.now() / 1000;
  for (const g of HOOKS.pins.values()) { g.userData.gem.position.y = 3.0 + Math.sin(t * 2.2) * 0.18; g.userData.gem.rotation.y = t * 1.4; }
}

addStyle(`
  #side #hooks { border-color: #8a7230; }
  #side #hooks h3 { color: var(--accent); }
  ul.hooklist { list-style: none; margin: 0; padding: 0; }
  ul.hooklist li { padding: 3px 0 4px; border-bottom: 1px dotted var(--line); line-height: 1.35; }
  ul.hooklist li:last-child { border-bottom: 0; }
  ul.hooklist li.done { color: var(--good); }
  ul.hooklist button.go, #wake .hookcard button.go { font-size: 11px; padding: 0 8px; border-color: var(--accent); color: var(--accent); background: #2a2516; }
  .fight { font: 600 10px system-ui, sans-serif; color: #fff; background: #8e2a22; border-radius: 2px; padding: 0 4px; letter-spacing: .04em; text-transform: uppercase; vertical-align: 1px; }
  .label .hk { font-size: 11px; font-weight: 600; color: #ffd36a; }
  #wake .hookcards { display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 10px; }
  #wake .hookcard { border: 1px solid #6d5f3d; border-top: 3px solid var(--accent); border-radius: 3px; padding: 9px 11px 10px; background: #211f1a; display: flex; flex-direction: column; gap: 5px; }
  #wake .hookcard .ht { font-size: 18px; color: #f4eee0; }
  #wake .hookcard p.hx { font-size: 14.5px; line-height: 1.45; color: #d9d2c2; }
  #wake .hookcard dl { font: 12px/1.45 system-ui, sans-serif; gap: 1px 8px; }
  #wake .hookcard dt { color: #bba679; font-variant: small-caps; letter-spacing: .04em; }
  #wake .hookcard dd { color: #e9e3d5; }
  #wake .hookcard button.go { align-self: flex-start; margin-top: 2px; }
  .hookacts { margin: 6px 0 4px; display: flex; flex-direction: column; gap: 5px; }
  .hookact { border: 1px solid #6d5f3d; border-left: 3px solid var(--accent); background: #211f1a; padding: 5px 8px; border-radius: 3px; font-size: 12.5px; }
  .hookact .ctl { margin-top: 4px; display: flex; gap: 6px; flex-wrap: wrap; }
  .hookact button { border-color: var(--accent); color: var(--accent); background: #2a2516; }
  #wake p.lastage { margin-top: 10px; font-size: 15px; color: #cfc6b3; }
`);
