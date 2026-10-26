// Courts and people: the court card for the selected region's owner (ruler, heir, heroes, sages, priests, and what the
// dragon may do to each), the lair's Known people tab (everyone met, alive or dead, and the hostages being raised), and
// the person verbs: take a hostage (apply_acts take_hostage with item = the person), kill (kill_person), release.
const COURT_ORDER = ['ruler', 'heir', 'hero', 'sage', 'priest', 'hostage_ruler'];

function courtFaction() {
  const r = selectedRegion && model.regions.get(selectedRegion);
  if (r?.owner) return r.owner;
  return play.focusFaction && model.factions.has(play.focusFaction) ? play.focusFaction : '';
}

function knownOf(id) { return (world.dragon.known_people ?? []).find((k) => k.id === id) ?? null; }

function relationHtml(id) {
  const k = knownOf(id);
  if (!k) return '<span class="rel muted">not met</span>';
  return `<span class="rel ${relationClass(k.relationship)}">you: ${relationWord(k.relationship)} (${signed(k.relationship)})</span>`;
}

function reignYears(p, f) {
  if (p.role !== 'ruler') return null;
  const crowned = [...(world.chronicle ?? [])].reverse().find((e) => e.kind === 'person_crowned' && e.subject === p.id);
  return world.year - (crowned ? crowned.year : p.born);
}

function personMeta(p, f) {
  const bits = [];
  const traits = traitWords(p.traits);
  if (traits.length) bits.push(traits.join(', '));
  bits.push(`age ${Math.max(0, world.year - p.born)}`);
  if (p.role === 'ruler') bits.push(`reigned ${yearsNum(reignYears(p, f))}`);
  const bet = p.tags.find((t) => t.startsWith('betrothed:'));
  if (bet) {
    const house = bet.slice(10), match = (world.people ?? []).find((x) => x.faction === house && x.role === 'heir' && x.id !== p.id);
    bits.push(match ? `betrothed to ${match.name} of ${factionTitle(house)}` : `betrothed into ${factionTitle(house)}`);
  }
  for (const t of ['usurper', 'claimant', 'slayer', 'captain', 'general', 'dragon_raised']) if (p.tags.includes(t)) bits.push(t === 'dragon_raised' ? 'raised in your lair' : t);
  return bits.join(' · ');
}

// what the dragon may do to this person, each with its cost and, when it cannot, why
function personVerbs(p, fid) {
  const f = model.factions.get(fid);
  const here = dragonRegion();
  const verbs = [];
  const stam = (c) => (play.stamina >= c ? '' : `needs ${c} stamina (${play.stamina} left)`);
  if (p.role === 'ruler' || p.role === 'heir') {
    const why = !f ? 'no court' : here !== f.capital ? `only at the capital, ${regionTitle(f.capital)}` : f.hostage_taken ? 'you already hold a hostage of this house' : stam(COST.hostage);
    verbs.push({ id: 'hostage', label: 'Take hostage', cost: `${COST.hostage} st`, why, title: `carry ${p.name} to the lair; ${factionTitle(fid)} grieves and fears you` });
  }
  const oath = oathBinds(fid);
  const killWhy = !f ? 'no court' : !f.regions.includes(here) ? `fly over ${factionTitle(fid)}'s land` : stam(COST.kill_person);
  verbs.push({ id: 'kill', label: 'Kill', cost: `${COST.kill_person} st`, why: killWhy, danger: true,
    title: `${factionTitle(fid)} remembers it as murder: fear +${CONSTANTS.offers?.envoy_kill_fear ?? 0.3}, cohesion drops${oath ? '; it breaks your oath with them (betrayal)' : ''}` });
  return verbs;
}

function personRowHtml(p, fid, opts = {}) {
  const verbs = opts.verbs === false ? [] : personVerbs(p, fid);
  return `<div class="person" data-person="${p.id}">
    <div class="who"><span class="role">${roleWord(p.role)}</span> <b>${escapeHtml(p.name)}</b></div>
    <div class="meta">${personMeta(p, fid)}</div><div class="meta">${relationHtml(p.id)}</div>
    ${verbs.length ? `<div class="verbs">${verbs.map((v) => `<button data-pverb="${v.id}" data-pid="${p.id}" class="${v.danger ? 'danger' : ''}" ${v.why ? 'disabled' : ''} title="${escapeHtml(v.why || v.title)}">${v.label} <small>${v.cost}</small></button>`).join('')}</div>
      ${verbs.filter((v) => v.why).map((v) => `<div class="why">${v.label.toLowerCase()}: ${escapeHtml(v.why)}</div>`).join('')}` : ''}
  </div>`;
}

function renderCourt() {
  const el = $('court');
  const fid = montage.active || play.landed ? '' : courtFaction();
  const f = fid ? world.factions.find((x) => x.id === fid) : null;
  if (!f) { el.classList.add('hidden'); el.dataset.faction = ''; return; }
  const people = (world.people ?? []).filter((p) => p.faction === fid);
  const atCourt = people.filter((p) => !p.tags.includes('hostage')).sort((a, b) => COURT_ORDER.indexOf(a.role) - COURT_ORDER.indexOf(b.role));
  const inLair = people.filter((p) => p.tags.includes('hostage'));
  const oaths = (world.oaths ?? []).filter((o) => o.status === 'active' && o.parties[1] === fid);
  el.dataset.faction = fid;
  el.innerHTML = `<h3><span class="sw" style="background:${factionCss(fid)}"></span>The court of ${factionTitle(fid)}</h3>
    <div class="meta">${f.dominant_race} · ${eraWord(f.era)} age · capital ${regionTitle(f.capital)} · fear ${fmt(f.fear)} · faith ${fmt(f.faith)}${oaths.length ? ` · <b class="up">sworn to you</b>` : ''}</div>
    ${armsCourtHtml(fid)}
    ${atCourt.map((p) => personRowHtml(p, fid)).join('') || '<div class="muted">no one of name</div>'}
    ${inLair.length ? `<div class="meta" style="margin-top:4px">in your lair: ${inLair.map((p) => `<b>${escapeHtml(p.name)}</b> (${roleWord(p.role)}, ${yearsNum(world.year - p.hostage_since)}${p.tags.includes('dragon_raised') ? ', raised as yours' : ''})`).join(', ')}
      <button data-pverb="release" data-pid="${inLair[0].id}" style="font-size:11px;padding:1px 7px;margin-left:4px" ${play.stamina >= COST.release ? '' : 'disabled'} title="send ${escapeHtml(inLair[0].name)} home">Release <small>${COST.release} st</small></button></div>` : ''}
    ${atCourt.length ? `<div class="cost-note">A kill costs ${COST.kill_person} stamina and is never forgotten: ${factionTitle(fid)} remembers the murder (fear +${CONSTANTS.offers?.envoy_kill_fear ?? 0.3}, cohesion drops)${oaths.length ? ', and you are sworn to them: it is a betrayal' : ''}.</div>` : ''}`;
  el.classList.remove('hidden');
  el.querySelectorAll('[data-pverb]').forEach((b) => b.addEventListener('click', () => personVerb(b.dataset.pverb, b.dataset.pid)));
}

function personVerb(verb, id) {
  if (verb === 'hostage') return takeHostagePerson(id);
  if (verb === 'kill') return killPerson(id);
  if (verb === 'release') return releaseHostage(id);
  return null;
}

function takeHostagePerson(id) {
  const p = personOf(id);
  const f = p && model.factions.get(p.faction);
  if (!f) return null;
  return act('take_hostage', { region: f.capital, target: f.id, item: id, person: p });
}

function killPerson(id, opts = {}) {
  if (montage.active) return null;
  const p = personOf(id);
  if (!p) return null;
  if (!spend(COST.kill_person)) return null;
  const before = narrationBefore();
  const res = sim({ cmd: 'kill_person', id });
  if (!res.ok) { play.stamina += COST.kill_person; toast(`cannot: ${res.error}`, true); return res; }
  pushEvents(res);
  const f = model.factions.get(p.faction);
  if (f?.capital && !play.landed) { flashTile(f.capital, new THREE.Color(0xff4040)); puff(new THREE.Vector3(...regionCentre(f.capital).flatMap((v, i) => (i ? [0, v] : [v]))), factionColor3(p.faction)); }
  refreshWorld();
  renderChronicle();
  narrateAct({ kind: 'kill_person', target: p.faction, person: p, envoy: !!opts.envoy }, res, before);
  actOutcome(res, f?.capital ?? '', { act: 'kill_person', shown: 0 });
  return res;
}

function releaseHostage(id) {
  const p = personOf(id);
  const f = p && model.factions.get(p.faction);
  if (!f || !f.regions.length) { toast('their house is gone; they stay in the lair', true); return null; }
  return act('release', { region: f.capital || f.regions[0], target: f.id, person: p });
}

// ---- the lair's Known people tab ------------------------------------------------------------------------------------
function personEvents(id) {
  return (world.chronicle ?? []).filter((e) => e.subject === id || e.object === id || (e.kind === 'act_kill' && (e.tags ?? []).includes(id)));
}

function personEventText(e) {
  if (e.kind === 'act_kill') return `You killed ${nameOf(e.tags[0])}.`;
  return eventSentence(e) ?? eventText(e);
}

function hostageRowsHtml() {
  const ids = world.dragon.hostages ?? [];
  if (!ids.length) return '<div class="muted">No one is held in the lair. Take an heir or a ruler at a court\'s capital, or accept an envoy who asks you to hide one.</div>';
  const firstOf = new Map();
  for (const id of ids) { const p = personOf(id); if (p && !firstOf.has(p.faction)) firstOf.set(p.faction, id); }
  const raise = CONSTANTS.people?.hostage_raise_years ?? 20;
  return ids.map((id) => {
    const p = personOf(id);
    if (!p) return '';
    const held = world.year - p.hostage_since;
    const raised = p.tags.includes('dragon_raised');
    const house = model.factions.get(p.faction);
    const why = !house ? 'their house is gone' : firstOf.get(p.faction) !== id ? `released after ${nameOf(firstOf.get(p.faction))}` : (play.stamina >= COST.release ? '' : `needs ${COST.release} stamina`);
    return `<div class="person hostage" data-person="${id}"><div class="who"><span class="role">${roleWord(p.role)} of ${factionTitle(p.faction) || 'a fallen house'}</span> <b>${escapeHtml(p.name)}</b></div>
      <div class="meta">${held} years in the lair · ${raised ? '<b class="up">raised as yours</b>: when they rule, the line is yours' : `grows up in the lair in ${Math.max(0, raise - held)} more years`} · ${relationHtml(id)}</div>
      <div class="verbs"><button data-pverb="release" data-pid="${id}" ${why ? 'disabled' : ''} title="${escapeHtml(why || 'send them home; the house is relieved')}">Release <small>${COST.release} st</small></button>${why ? `<span class="why">${escapeHtml(why)}</span>` : ''}</div></div>`;
  }).join('');
}

function knownRowsHtml() {
  const known = (world.dragon.known_people ?? []).slice();
  if (!known.length) return '<div class="muted">You have met no one yet. Show yourself at a court (Roar), take a hostage, answer an envoy, or defend a realm.</div>';
  const alive = (k) => !k.tags.includes('dead') && NAMES.people.has(k.id);
  known.sort((a, b) => (alive(b) - alive(a)) || (Math.abs(b.relationship) - Math.abs(a.relationship)));
  return known.map((k) => {
    const p = personOf(k.id);
    const dead = NAMES.dead.get(k.id);
    const status = alive(k) ? (p?.tags.includes('hostage') ? 'in your lair' : 'alive') : `died ${dead?.died ?? '?'}`;
    const evs = personEvents(k.id).slice(-4).reverse();
    return `<div class="person known ${alive(k) ? '' : 'dead'}" data-person="${k.id}">
      <div class="who"><span class="role">${p ? roleWord(p.role) + ' of ' + (factionTitle(p.faction) || 'a fallen house') : ''}</span> <b>${escapeHtml(p?.name ?? k.id)}</b> <span class="muted">· ${status}</span></div>
      <div class="meta">met ${k.met} (${k.how})${k.tags.filter((t) => t !== 'dead').length ? ' · ' + k.tags.filter((t) => t !== 'dead').join(', ') : ''} · <span class="rel ${relationClass(k.relationship)}">${relationWord(k.relationship)} (${signed(k.relationship)})</span>
        <span class="relbar"><i style="left:${50 + Math.min(0, k.relationship) * 50}%;width:${Math.abs(k.relationship) * 50}%" class="${k.relationship < 0 ? 'neg' : 'pos'}"></i></span></div>
      ${evs.length ? `<ul class="pev">${evs.map((e) => `<li><span class="y">${e.year}</span>${escapeHtml(personEventText(e))}</li>`).join('')}</ul>` : ''}</div>`;
  }).join('');
}

function knownTabHtml() {
  return `<div class="cols"><div><div class="box"><h3>In your lair</h3>${hostageRowsHtml()}</div></div>
    <div><div class="box"><h3>Known people</h3><div class="muted" style="margin-bottom:4px">Everyone you have met, alive or dead; the relationship moves with what you do to them and their court.</div>${knownRowsHtml()}</div></div></div>`;
}

function bindKnown(body) { body.querySelectorAll('[data-pverb]').forEach((b) => b.addEventListener('click', () => { personVerb(b.dataset.pverb, b.dataset.pid); renderLair(); })); }
