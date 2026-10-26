// The narration strip: for the act just done, one sentence for what happened and one per court for how it took it.
// Uses the `lines` the sim returns with apply_acts when present ([{faction, text}]); until then the lines are built here
// from the before/after state, the act and the court's relationship to the dragon, the way acts.cpp picks a reaction
// (oath active -> betrayal, faithful -> disillusion, strong -> resolve, weak -> fear; witnesses at war with the victim
// approve, kin are dismayed).
const NARR = { lines: [], max: 6, group: 0 };
const NARR_EXTRA = ['offer_fulfilled', 'deceived_dragon', 'war_ended', 'coalition_ended', 'vassal_freed', 'person_crowned', 'race_returned', 'undead_raised', 'oath_breached_by_dragon', 'demand_refused'];
const HARM = ['burn', 'take', 'kill', 'kill_person'];

function oathBinds(fid) {
  return (world.oaths ?? []).some((o) => o.status === 'active' && (o.parties[0] === fid || o.parties[1] === fid) && o.clauses.some((c) => c.pred === 'no_harm' || c.pred === 'defend'));
}

function narrationBefore() {
  const factions = {};
  for (const f of world.factions) factions[f.id] = { fear: f.fear, faith: f.faith, wealth: f.wealth, power: f.power ?? 0, race: f.dominant_race ?? f.race, at_war_with: [...f.at_war_with], oath: oathBinds(f.id) };
  const known = new Map((world.dragon.known_people ?? []).map((k) => [k.id, k.relationship]));
  return { factions, known, threat: world.dragon.threat ?? 0 };
}

// a = { kind, target, region, against, person, envoy, spell, pred }
function narrateAct(a, res, before) {
  if (!res || res.ok === false) return;
  // the sim's own lines win: a line without a faction tells what happened, the others how each court took it
  const simLines = Array.isArray(res.lines) && res.lines.length ? res.lines : null;
  const simMain = simLines ? simLines.filter((l) => !l.faction).map((l) => cleanSimText(l.text)) : [];
  const main = simMain.length ? simMain : mainSentences(a, res);
  const reactions = simLines ? simLines.filter((l) => l.faction).map((l) => ({ faction: l.faction, who: l.person ? nameOf(l.person) : '', text: cleanSimText(l.text), mood: l.mood ?? '', source: 'sim' })) : reactionLines(a, before);
  pushNarration(main, reactions.slice(0, 4));
}

function mainSentences(a, res) {
  const ev = (kind) => (res.events ?? []).find((e) => e.kind === kind);
  const F = factionTitle(a.target);
  const out = [];
  const p = a.person;
  if (a.kind === 'take_hostage') {
    const h = ev('hostage_taken');
    const who = h ? personOf(h.subject) : p;
    out.push(who ? `You carried ${who.name}, ${roleWord(h?.tags?.find((t) => ROLE_TAGS.has(t)) ?? who.role)} of ${F}, off to your lair.` : `You took a hostage from the court of ${F}.`);
  } else if (a.kind === 'release') {
    const h = ev('hostage_released');
    out.push(h ? `${capFirst(nameOf(h.subject))} went home to ${F} after ${h.magnitude.toFixed(0)} years in your lair.` : `You let your hostage go home to ${F}.`);
  } else if (a.kind === 'kill_person') {
    out.push(a.envoy ? `You ate ${p.name}, envoy of ${F}, at the mouth of the lair.` : `You killed ${p.name}, ${roleWord(p.role)} of ${F}.`);
  } else if (a.kind === 'kill') {
    out.push(`You fell on the army of ${F} at ${nameOf(a.region)} and fed.`);
  } else if (a.kind === 'demand') {
    out.push(res.sworn ? `${F} swore ${(TEXT.words?.[a.pred] ?? a.pred).replace(/_/g, ' ')} under your shadow.` : `${F} refused your demand (${res.reason_text ?? res.reason}).`);
  } else if (a.kind === 'cast') {
    const c = ev('spell_cast') ?? ev('undead_raised');
    out.push(res.cast ? (c ? eventSentence(c) : `Your ${a.spell.replace(/_/g, ' ')} took hold.`) : `Your ${a.spell.replace(/_/g, ' ')} failed: ${res.reason_text ?? res.reason}.`);
  } else if (a.kind === 'terraform') {
    out.push(res.done ? `You reshaped ${nameOf(a.region)} into ${res.tag}.` : `The land would not change: ${res.reason_text ?? res.reason}.`);
  } else if (a.kind === 'accept_offer' || a.kind === 'decline_offer') {
    const e = ev(a.kind === 'accept_offer' ? 'oath_sworn' : 'offer_declined');
    out.push(e ? eventSentence(e) : `You answered the envoy of ${F}.`);
  } else if (a.kind === 'give') {
    out.push(`You gave ${Number(a.gold ?? 0).toFixed(1)} gold from the hoard to ${F}.`);
  } else {
    const act = (res.events ?? []).find((e) => e.kind.startsWith('act_'));
    const s = act && eventSentence(act);
    if (s) out.push(s);
  }
  for (const e of res.events ?? []) if (NARR_EXTRA.includes(e.kind) && !(a.kind === 'demand' && e.kind === 'demand_refused')) { const s = eventSentence(e); if (s) out.push(s); }
  return out;
}

function moodOf(a, f, b, before) {
  const isTarget = f.id === a.target;
  if (HARM.includes(a.kind)) {
    if (!isTarget) {
      if (a.kind !== 'burn') return 'shift';
      const t = before.factions[a.target];
      if (b.at_war_with.includes(a.target)) return 'approval';
      if (t && t.race === b.race) return 'kin';
      return 'unease';
    }
    if (a.kind === 'kill_person') return 'murder';
    if (b.oath) return 'betrayal';
    if (b.faith > (CONSTANTS.reaction_faith_high ?? 0.5)) return 'disillusion';
    return b.power >= (CONSTANTS.reaction_weak_ratio ?? 0.5) * before.threat ? 'resolve' : 'fear';
  }
  if (!isTarget) return a.kind === 'defend' && f.id === a.against ? 'loss' : 'shift';
  const moods = { show: 'awe', decline_offer: 'awe', accept_offer: 'shift', give: 'gratitude', accept_offering: 'devotion', destroy_altar: 'sacrilege', take_hostage: 'grief', release: 'relief', demand: a.sworn ? 'submission' : 'defiance' };
  if (a.kind === 'defend') return b.at_war_with.length && f.faith - b.faith > 0.2 ? 'saved' : 'helped';
  if (a.kind === 'miracle') return a.terror ? 'terror' : 'devotion';
  return moods[a.kind] ?? 'shift';
}

const MOOD_TEXT = {
  betrayal: (F) => `${F} trusted your oath and calls this betrayal; its faith in you is gone`,
  disillusion: (F) => `${F} prayed to you and now doubts you`,
  resolve: (F) => `${F} is strong enough to hate you for it, and talk of slayers grows`,
  fear: (F) => `${F} is too weak to answer and cowers`,
  approval: (F, a) => `${F}, at war with ${factionTitle(a.target)}, is glad of your fire`,
  kin: (F, a) => `${F} watched its kin in ${factionTitle(a.target)} burn`,
  unease: (F) => `${F} watched the smoke from across the border`,
  awe: (F) => `${F} saw you overhead`,
  saved: (F) => `${F} was losing; it calls you its saviour`,
  helped: (F) => `${F} is grateful for the help`,
  loss: (F) => `${F} lost its army and its spoils`,
  gratitude: (F) => `${F} takes your gold and thinks better of you`,
  devotion: (F) => `${F} sees a god in you`,
  terror: (F) => `${F} has learned to fear the sky`,
  sacrilege: (F) => `${F} saw its altar broken`,
  grief: (F, a) => `${F} lost ${a.person ? a.person.name : 'one of its own'} to your lair`,
  relief: (F) => `${F} has its own back`,
  murder: (F, a) => `${F} will remember the murder of ${a.envoy ? 'its envoy' : a.person ? a.person.name : 'one of its own'}`,
  submission: (F) => `${F} bent the knee`,
  defiance: (F) => `${F} refused you and steels itself against you`,
  shift: (F) => `${F} took note`,
};

function deltaText(d) {
  const parts = [];
  if (Math.abs(d.fear) >= 0.005) parts.push(`fear ${signed(d.fear)}`);
  if (Math.abs(d.faith) >= 0.005) parts.push(`faith ${signed(d.faith)}`);
  if (Math.abs(d.wealth) >= 0.4) parts.push(`wealth ${signed(d.wealth, 0)}`);
  return parts.length ? ` (${parts.join(', ')})` : '';
}

function reactionLines(a, before) {
  const lines = [];
  const forced = ['kill_person', 'take_hostage', 'release', 'demand'].includes(a.kind);
  for (const f of world.factions) {
    const b = before.factions[f.id];
    if (!b) continue;
    const d = { fear: f.fear - b.fear, faith: f.faith - b.faith, wealth: f.wealth - b.wealth };
    const moved = Math.abs(d.fear) >= 0.005 || Math.abs(d.faith) >= 0.005 || Math.abs(d.wealth) >= 0.4;
    if (!moved && !(forced && f.id === a.target)) continue;
    const mood = moodOf(a, f, b, before);
    lines.push({ faction: f.id, mood, text: MOOD_TEXT[mood](factionTitle(f.id), a) + deltaText(d) + '.', first: f.id === a.target });
  }
  lines.sort((x, y) => (y.first ? 1 : 0) - (x.first ? 1 : 0));
  for (const k of world.dragon.known_people ?? []) {
    const r0 = before.known.get(k.id);
    const dr = r0 === undefined ? k.relationship : k.relationship - r0;
    if (r0 !== undefined && Math.abs(dr) < 0.03) continue;
    if (r0 === undefined && k.tags.includes('dead')) continue;
    const p = personOf(k.id);
    if (!p || (a.person && a.person.id === k.id && k.tags.includes('dead'))) continue;
    const verb = r0 === undefined ? 'has met you' : dr < 0 ? 'thinks less of you' : 'thinks better of you';
    lines.push({ faction: p.faction, who: p.name, mood: r0 === undefined ? 'met' : dr < 0 ? 'colder' : 'warmer', text: `${p.name} ${verb}: ${relationWord(k.relationship)} (${signed(k.relationship)}).` });
  }
  return lines;
}

function pushNarration(main, reactions) {
  NARR.group += 1;
  for (const text of main) NARR.lines.push({ kind: 'main', text, group: NARR.group });
  for (const r of reactions) NARR.lines.push({ kind: 'react', text: r.text, faction: r.faction, who: r.who ?? '', mood: r.mood, group: NARR.group, source: r.source ?? '' });
  while (NARR.lines.length > NARR.max) NARR.lines.shift();
  renderNarration();
}

function clearNarration() { NARR.lines = []; renderNarration(); }

function narrationLineHtml(l) {
  if (l.kind === 'main') return escapeHtml(l.text);
  const col = l.faction ? factionCss(l.faction) : '#888';
  const name = l.who || factionTitle(l.faction);
  const text = l.text.startsWith(name) ? l.text.slice(name.length) : ' ' + l.text;
  return `<span class="sw" style="background:${col}"></span><b>${escapeHtml(name)}</b>${l.mood ? ` <em style="color:${col}">${l.mood}</em>` : ''}${escapeHtml(text)}`;
}

function renderNarration() {
  const el = $('narration');
  const n = NARR.lines.length;
  el.innerHTML = NARR.lines.map((l, i) => `<div class="nl ${l.kind}" style="opacity:${l.group === NARR.group ? 1 : Math.max(0.3, 0.85 - (n - 1 - i) * 0.12).toFixed(2)}">${narrationLineHtml(l)}</div>`).join('');
  el.classList.toggle('hidden', !n || montage.active || play.landed);
  if (play.landed) renderLairFoot();
  layoutOverlays();
}

function escapeHtml(s) { return String(s ?? '').replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' })[c]); }

// the strip sits above the action panel; the side column (tasks, court) stops above it too
function layoutOverlays() {
  const act = $('actions');
  const shown = !act.classList.contains('hidden');
  const top = shown ? act.getBoundingClientRect().top : innerHeight - 10;
  const nar = $('narration');
  nar.style.bottom = (innerHeight - top + 8) + 'px';
  nar.style.left = (shown ? act.getBoundingClientRect().left : 244) + 'px';
  $('side').style.maxHeight = Math.max(120, top - 46 - 10) + 'px';
}
