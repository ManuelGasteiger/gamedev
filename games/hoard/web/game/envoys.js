// Envoy audiences in the lair: each offer is a named person of a court asking for something, with a reason that may be
// a lie; accept, decline, or eat the envoy (kill_person). Also "Oaths and tasks": what accepted offers still ask of the
// dragon, by when, and what they pay (side card during the raid, and the lair's Oaths tab).
const MOTIVE_SPEECH = {
  survival: 'We fear for our survival.', fear_of_conquest: 'They will swallow us if no one stops them.',
  revenge: 'They took our land. We want them to burn for it.', piety_rival: 'Their creed is an insult to ours.',
  succession: 'Our line needs its heir back.', protect_the_line: 'A usurper circles the throne. Keep the child safe.',
  catch_up: 'The valley has left us behind. Teach us.', hunger: 'Our people are hungry.',
  legitimacy: 'A pretender threatens the throne.', protect_the_altar: 'Guard the altar we raised to you.',
  awe: 'We stand in awe of you.', escape_marriage: 'I would rather burn than marry.', exit: 'We would open the way out for you.',
};

function quote(s) { return '“' + s.replace(/^"|"$/g, '').replace(/^\w/, (c) => c.toUpperCase()).replace(/([^.!?])$/, '$1.') + '”'; }

function statedReason(o) {
  const key = o.stated_reason || '';
  const fill = (t) => cleanSimText(t, null, { target: o.target_name || factionTitle(o.target) || 'them', envoy: o.envoy_name || 'I', faction: factionTitle(o.from_faction) });
  if (TEXT.pretexts?.[key]) return quote(fill(TEXT.pretexts[key]));
  if (MOTIVE_SPEECH[key]) return quote(MOTIVE_SPEECH[key]);
  if (TEXT.motives?.[key]) return quote(fill(TEXT.motives[key]));
  return key ? quote(key.replace(/_/g, ' ')) : '';
}

function offerAsk(o) {
  const ask = TEXT.asks?.[o.kind] ?? (o.ask ? o.ask.replace(/_/g, ' ') + (o.target_name ? ' ' + o.target_name : '') : o.kind);
  return cleanSimText(ask, null, { target: o.target_name || factionTitle(o.from_faction), price0: Number(o.price ?? 0).toFixed(0) });
}

function envoyPerson(o) { return o.envoy ? personOf(o.envoy) : null; }

function audienceHtml(o, i) {
  const fid = o.from_faction || o.oath.parties[1];
  const env = envoyPerson(o);
  const col = factionCss(fid);
  const hints = [];
  if (o.eyes_say_otherwise) hints.push(`${TEXT.misc?.eyes_say_otherwise?.trim() ?? "The envoy's eyes say otherwise."}${o.motive && TEXT.motives?.[o.motive] ? ` (the truth: ${cleanSimText(TEXT.motives[o.motive], null, { envoy: o.envoy_name, target: o.target_name, faction: factionTitle(o.from_faction) })})` : ''}`);
  if (o.marked) hints.push(TEXT.misc?.marked_envoy?.trim() ?? 'This court has lied to you before.');
  const inLair = env?.tags?.includes('hostage');
  const eatWhy = !o.envoy ? 'no envoy came in person' : play.stamina < COST.kill_person ? `needs ${COST.kill_person} stamina (${play.stamina} left)` : '';
  const oath = oathBinds(fid);
  const eatCost = `${COST.kill_person} stamina. ${factionTitle(fid)} remembers the murder of its envoy (fear +${CONSTANTS.offers?.envoy_kill_fear ?? 0.3}, cohesion drops), ${o.envoy_name || 'the envoy'} dies and the offer with them${oath ? '; you are sworn to this court, so it is a betrayal' : ''}.`;
  return `<div class="audience" style="--c:${col}" data-offer="${i}">
    <div class="speaker"><b>${escapeHtml(o.envoy_name || 'An envoy')}</b>${env ? `, ${roleWord(env.role)}` : ''} of <b style="color:${col}">${factionTitle(fid)}</b>${inLair ? ' <span class="muted">(speaking from your own lair, where you hold them)</span>' : ''} <span class="muted">· ${offerTitle(o.text_key)}</span></div>
    <div class="speech">${statedReason(o)}</div>
    <div class="ask">They ask ${escapeHtml(offerAsk(o))}.${o.price > 0 ? ` <span class="price">Price: ${fmt(o.price, 1)} gold${o.oath.clauses.some((c) => c.pred === 'tribute') ? ' a decade, as tribute' : ', paid when it is done'}.</span>` : ''}</div>
    ${hints.map((h) => `<div class="hint">${escapeHtml(h)}</div>`).join('')}
    <div class="clauses">${o.oath.clauses.map(clauseText).join('<br>') || (o.ask ? 'asks you to ' + o.ask.replace(/_/g, ' ') + (o.target_name ? ' ' + o.target_name : '') : offerRite(o.text_key))}<br>if you break it: ${stakeText(o.oath.stakes.breach_by_dragon)} · if they break it: ${stakeText(o.oath.stakes.breach_by_them)} · runs until year ${o.oath.expires}</div>
    <div class="row"><button class="primary" data-accept="${i}">Accept</button><button data-decline="${i}">Decline (they are shown the dragon, fear +)</button>
      <button class="danger eat" data-eat="${i}" ${eatWhy ? 'disabled' : ''} title="${escapeHtml(eatWhy || eatCost)}">Eat the envoy</button></div>
    <div class="cost-note">${eatWhy ? 'eat the envoy: ' + escapeHtml(eatWhy) : 'Eat the envoy: ' + escapeHtml(eatCost)}</div>
  </div>`;
}

function envoysTabHtml() {
  const offers = model.offers ?? [];
  if (!offers.length) return '<div class="muted">No envoys wait at the entrance. Courts send them when they are losing a war, fear a rival, want their hostage back, or worship you.</div>' + `<div class="box" style="margin-top:10px"><h3>Oaths and tasks</h3>${tasksHtml()}</div>`;
  return `<div class="muted">Envoys were sent when you woke. Each is a person of the court, and gives a reason, true or not.</div>` + offers.map(audienceHtml).join('');
}

function bindEnvoys(body) {
  body.querySelectorAll('[data-eat]').forEach((b) => b.addEventListener('click', () => eatEnvoy(Number(b.dataset.eat))));
}

function eatEnvoy(index) {
  const o = (model.offers ?? [])[index];
  if (!o?.envoy) return null;
  const res = killPerson(o.envoy, { envoy: true });
  if (res?.ok) { toast(`you ate ${o.envoy_name}, envoy of ${factionTitle(o.from_faction)}`, true); spawnEnvoysSettled(); }
  if (play.landed) renderLair();
  return res;
}

// ---- oaths and tasks ------------------------------------------------------------------------------------------------
const TASK_VERB = { defend: 'defend', burn: 'burn', release: 'release', teach: 'teach', bless: 'bless', ward: 'ward', kill: 'kill', take_hostage: 'take into the lair' };

function openTasks() {
  const out = [];
  for (const o of world.oaths ?? []) {
    if (o.status !== 'active') continue;
    const fid = o.parties[1], F = factionTitle(fid);
    const tribute = o.clauses.find((c) => c.pred === 'tribute' && c.who !== 'dragon');
    const pays = tribute ? `${F} pays ${fmt(tribute.params?.amount ?? 0, 1)} gold a decade` : o.price > 0 ? `${F} pays ${fmt(o.price, 0)} gold` : '';
    if (o.ask && !o.fulfilled) {
      const target = o.target ? nameOf(o.target) : F;
      const what = o.ask === 'defend' ? `defend ${F}${o.target ? ' against ' + nameOf(o.target) : ''}` : o.ask === 'release' ? `release ${target} to ${F}` : `${TASK_VERB[o.ask] ?? o.ask.replace(/_/g, ' ')} ${target}`;
      out.push({ faction: fid, kind: 'task', text: `${what} ${o.ask === 'defend' ? 'until' : 'before'} ${o.expires}${pays ? ` (${pays})` : ''}`, oath: o.id });
    } else if (o.clauses.some((c) => c.pred === 'no_harm' && c.who === 'dragon')) {
      out.push({ faction: fid, kind: 'vow', text: `do no harm to ${F} until ${o.expires}${pays ? ` (${pays})` : ''}`, oath: o.id });
    } else if (o.kind === 'demand') {
      out.push({ faction: fid, kind: 'owed', text: `${F} owes you ${o.clauses.map((c) => (TEXT.words?.[c.pred] ?? c.pred).replace(/_/g, ' ')).join(', ')} until ${o.expires}`, oath: o.id });
    }
  }
  return out;
}

function tasksHtml() {
  const tasks = openTasks();
  if (!tasks.length) return '<div class="muted">No open tasks. Accepting an envoy\'s offer swears an oath; what it asks of you appears here.</div>';
  return `<ul class="tasks">${tasks.map((t) => `<li class="${t.kind}"><span class="sw" style="background:${factionCss(t.faction)}"></span>${escapeHtml(t.text)}</li>`).join('')}</ul>`;
}

function renderTasks() {
  const el = $('tasks');
  const tasks = montage.active || play.landed ? [] : openTasks();
  el.classList.toggle('hidden', !tasks.length);
  if (tasks.length) el.innerHTML = `<h3>Oaths and tasks</h3>${tasksHtml()}`;
}
