// The arms race made visible (docs/design-references.md 2 and 3): each court's progress towards the next defence it
// unlocks (factions[i].anti_dragon_next {at, defence}, measured from the last unlock it passed as anti_dragon.cpp does) in
// the court card and the wake page, the 25/50/75/95 % steps it crossed while you slept (anti_dragon_progress events), and
// the slayer clock from factions[i].slayers {stage, strength, odds}: forming, ready (they come in your next sleep), at
// your door after a premonition (world tag slayers_coming:<court>), then the attempt.
const ARMS_STEPS = [[0.25, 'a quarter of the way to'], [0.5, 'halfway to'], [0.75, 'three quarters of the way to'], [0.95, 'on the brink of']];
const ARMS_WHO = { bolt_throwers: 'engineers', cannons: 'gunsmiths', wards: 'mages', levies: 'captains' };
const ARMS_WHAT = { bolt_throwers: 'bolt-throwers', slayer_band: 'a slayer order', cannons: 'cannons', wards: 'wards', levies: 'levies' };
const DEFENCE_BOOK = (() => { try { return JSON.parse(DATA.defences || '{}'); } catch (e) { return {}; } })();

function hasArms(view = world) { return (view?.factions ?? []).some((f) => f.anti_dragon_next !== undefined); }

// the highest unlock the court has passed: its progress bar starts there
function passedLevel(f) {
  let best = 0;
  const race = f.dominant_race ?? f.race, slayersAt = CONSTANTS.slayers_at ?? 1;
  for (const d of DEFENCE_BOOK.defences ?? []) {
    if (f.era < (d.min_era ?? 0) || (d.races_any?.length && !d.races_any.includes(race))) continue;
    if (d.min_anti_dragon > 0 && d.min_anti_dragon <= f.anti_dragon) best = Math.max(best, d.min_anti_dragon);
    if ((d.tags_any ?? []).includes('slayers') && slayersAt <= f.anti_dragon) best = Math.max(best, slayersAt);
  }
  return best;
}

// {id, at, progress} of the court's next unlock, or null
function armsOf(fid, view = world) {
  const f = (view?.factions ?? []).find((x) => x.id === fid), n = f?.anti_dragon_next;
  if (!n || !(n.at > 0)) return null;
  const from = Math.min(passedLevel(f), n.at);
  return { id: n.defence, at: n.at, progress: n.at > from ? clamp(((f.anti_dragon ?? 0) - from) / (n.at - from), 0, 1) : 0 };
}
function armsLabel(id) { return ARMS_WHAT[id] ?? String(id ?? '').replace(/_/g, ' '); }
function armsStep(p) { let s = null; for (const step of ARMS_STEPS) if (p >= step[0] - 1e-9) s = step; return s; }

// ---- the slayer clock -----------------------------------------------------------------------------------------------
function slayerCourts(stage, view = world) { return (view?.factions ?? []).filter((f) => f.regions?.length && f.slayers?.stage === stage); }
function slayersReady(view = world) { return slayerCourts('ready', view).map((f) => f.id); }
// after a premonition the band waits at the lair and strikes in the next sleep without another warning
function slayersAtDoor(view = world) { return (view?.tags ?? []).filter((t) => t.startsWith('slayers_coming:')).map((t) => t.slice(15)); }
function oddsText(f) { return typeof f?.slayers?.odds === 'number' ? `${Math.round(f.slayers.odds * 100)}%` : '?'; }

function slayersHtml(view = world) {
  const door = new Set(slayersAtDoor(view));
  const wounds = hasHealth() && health().wounds > 0.5 ? ` The ${numText(health().wounds)} wounds you carry are in those odds; sleep heals them only after the attempt.` : '';
  const ready = slayerCourts('ready', view).map((f) => `<div class="slayers-ready">${door.has(f.id) ? `The slayers of ${escapeHtml(factionTitle(f.id))} wait at the foot of your mountain.` : `${escapeHtml(factionTitle(f.id))}'s slayer band is ready.`} They will come up the mountain in your next sleep: odds ${oddsText(f)} of killing you.${wounds}</div>`);
  const forming = slayerCourts('forming', view).map((f) => `<div class="slayers-forming">${escapeHtml(factionTitle(f.id))} is gathering a slayer band (strength ${fmt(f.slayers.strength, 1)}). It can be ready when you next wake, and strike in the sleep after.</div>`);
  return ready.join('') + forming.join('');
}

function armsCourtHtml(fid) {
  const f = world.factions.find((x) => x.id === fid), n = armsOf(fid), stage = f?.slayers?.stage ?? '';
  const slay = stage === 'ready' ? `<b class="down">slayer band ready: they strike in your next sleep (odds ${oddsText(f)})</b>` : stage === 'forming' ? `<b class="warn">slayer band forming (strength ${fmt(f.slayers.strength, 1)})</b>` : '';
  const race = n && (n.progress > 0 || f.fear > 0) ? `arms race: ${Math.round(n.progress * 100)}% of the way to ${escapeHtml(armsLabel(n.id))} <span class="armsbar"><i style="width:${n.progress * 100}%"></i></span>` : '';
  if (!slay && !race) return '';
  return `<div class="arms">${slay}${slay && race ? '<br>' : ''}${race}<span class="cost-note">Fear feeds their work: the more they dread you, the faster it goes.</span></div>`;
}

// ---- the wake page --------------------------------------------------------------------------------------------------
// what the courts achieved while you slept: the steps the sim told (anti_dragon_progress) and the defences now standing
function armsChanges(before, after, chronicle = []) {
  if (!hasArms(after)) return [];
  const lines = [];
  for (const f of after.factions ?? []) {
    if (!f.regions?.length) continue;
    const was = armsOf(f.id, before), now = armsOf(f.id, after);
    if (was && (!now || now.id !== was.id) && (after.factions.find((x) => x.id === f.id)?.anti_dragon ?? 0) >= was.at) lines.push({ faction: f.id, text: `${factionTitle(f.id)} now has ${armsLabel(was.id).replace(/^a /, 'its ')}.`, done: true });
  }
  const latest = new Map();
  for (const e of chronicle) if (e.kind === 'anti_dragon_progress') latest.set(e.subject + '|' + e.object, e);
  for (const e of latest.values()) {
    if (!(after.factions ?? []).some((f) => f.id === e.subject && f.regions?.length)) continue;
    const step = armsStep(e.magnitude)?.[1] ?? `${Math.round(e.magnitude * 100)}% of the way to`;
    const text = e.object === 'slayer_band' ? `${factionTitle(e.subject)} is ${step} founding a slayer order.` : `${factionTitle(e.subject)}'s ${ARMS_WHO[e.object] ?? 'smiths'} are ${step} ${armsLabel(e.object)}.`;
    lines.push({ faction: e.subject, text, step: e.magnitude });
  }
  return lines;
}

// each court's progress to its next unlock, kept with the wake page so its bars can show what the sleep added
function armsLevels(view) { return Object.fromEntries((view?.factions ?? []).map((f) => [f.id, armsOf(f.id, view)]).filter(([, n]) => n)); }

function armsWakeHtml(w) {
  if (!hasArms()) return '';
  const lines = w.arms ?? [];
  const bars = world.factions.filter((f) => f.regions?.length && (armsOf(f.id)?.progress ?? 0) > 0).map((f) => {
    const n = armsOf(f.id), was = w.armsWas?.[f.id];
    const gain = was && was.id === n.id ? Math.round((n.progress - was.progress) * 100) : null;
    return `<li><span class="sw" style="background:${factionCss(f.id)}"></span>${escapeHtml(factionTitle(f.id))} <span class="armsbar"><i style="width:${n.progress * 100}%"></i></span> <span class="muted">${Math.round(n.progress * 100)}%${gain ? ` (${gain > 0 ? '+' : '−'}${Math.abs(gain)})` : ''} to ${escapeHtml(armsLabel(n.id))}</span></li>`;
  }).join('');
  const clock = slayersHtml();
  return `<section class="armsrace"><div class="row2"><div class="margin">arms race</div><div>${clock}
    ${lines.length ? `<ul class="armslines">${lines.map((l) => `<li class="${l.done ? 'done' : ''}">${escapeHtml(l.text)}</li>`).join('')}</ul>` : clock || bars ? '' : '<p class="quiet">No court works on a weapon against you.</p>'}
    ${bars ? `<ul class="armsbars">${bars}</ul>` : ''}</div></div></section>`;
}

addStyle(`
  .slayers-ready, .slayers-forming { border-left: 4px solid var(--bad); background: #3a1210; color: #ffd6cf; padding: 6px 10px; margin: 0 0 8px; font: 15.5px/1.45 Georgia, 'Palatino Linotype', 'DejaVu Serif', serif; border-radius: 2px; }
  .slayers-forming { border-left-color: #d49a4c; background: #2e2312; color: #f3dfbf; }
  .armsbar { display: inline-block; width: 70px; height: 6px; background: #11131a; border: 1px solid var(--line); border-radius: 3px; vertical-align: 0; overflow: hidden; }
  .armsbar i { display: block; height: 100%; background: linear-gradient(90deg, #b8392e, #e8705f); }
  #side .arms { margin: 4px 0 2px; font-size: 11.5px; color: #e8b9b0; }
  #side .arms .cost-note { display: block; }
  #side .arms .warn { color: #e8b56a; }
  #wake ul.armslines li { font-size: 15.5px; }
  #wake ul.armslines li.done { color: #ffb3a6; }
  #wake ul.armsbars { font: 12px/1.7 system-ui, sans-serif; margin-top: 6px; }
  #wake ul.armsbars li { margin: 0; }
`);
