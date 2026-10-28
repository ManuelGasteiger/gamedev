// Death is final (docs/core-loop.md section 4): when apply_acts or a sleep reports the dragon slain, the screen goes dark
// and says who killed it and when, the highlights of its life, that the line of the dragon ends and that the save is gone.
// The only way on is a new world.
const DEATH = { el: null };
const DEATH_KINDS = new Set(['talent_taken', 'hook_resolved', 'dragon_moulted', 'oath_sworn', 'oath_breached_by_dragon', 'hostage_taken', 'raised_in_lair', 'dragon_raised_lineage',
  'slay_attempt_failed', 'faction_destroyed', 'slayers_founded', 'act_burn', 'dragon_slain']);

function deathOpen() { return !!DEATH.el && !DEATH.el.classList.contains('hidden'); }

// apply_acts' `slain`, or a sleep that ended in a slaying. dragon_slain names the court in `subject`; by the defences
// (cause `defences`) its object is the region and its tags the defences, by slayers its object is the band's leader
function deathFrom(res) {
  if (!res || play.dead) return false;
  const events = [...(res.events ?? []), ...(res.chronicle ?? [])];
  const slain = events.find((e) => e.kind === 'dragon_slain');
  if (!res.slain && res.wake_reason !== 'dragon_slain' && !slain) return false;
  const wounds = events.filter((e) => e.kind === 'dragon_wounded'), last = wounds[wounds.length - 1];
  const byDefences = !!res.slain || slain?.cause?.text_key === 'defences' || slain?.text_key === 'ev.dragon_slain_defences';
  showDeath({ year: slain?.year ?? model.year, court: slain?.subject || last?.subject || '', region: byDefences ? slain?.object || last?.object || '' : '',
    leader: byDefences ? '' : slain?.object || '', by: byDefences ? (slain?.tags?.length ? slain.tags : last?.tags ?? []) : [], asleep: !byDefences });
  return true;
}

function deathLine(info) {
  const court = info.court ? factionTitle(info.court) : 'the valley';
  const where = info.region && model.regions.has(info.region) ? ` over ${regionTitle(info.region)}` : '';
  const leader = info.leader ? nameOf(info.leader) : '';
  if (info.asleep) return `In the year ${info.year} the slayers of ${court}${leader ? `, led by ${leader},` : ''} came up the mountain while you slept, and did not come down until you were dead. Carts of gold rolled down into the valley for a season.`;
  const what = info.by.filter((t) => t !== 'fight').map((t) => DEF_WORD[t] ?? t.replace(/_/g, ' '));
  return `In the year ${info.year} the ${what.length ? what.join(' and ') : 'spears'} of ${court} brought you down${where}. They dragged you through the streets, and the bards were singing of it before the blood was dry.`;
}

function lifeHighlights() {
  const all = [...play.ages.flatMap((a) => [...a.acts, ...a.chronicle]), ...play.ageEvents];
  const mine = all.filter((e) => DEATH_KINDS.has(e.kind) && (e.kind !== 'act_burn' || e.magnitude >= 0.5));
  const seen = new Set(), out = [];
  for (const e of mine.sort((a, b) => a.year - b.year)) {
    const text = eventText(e);
    if (seen.has(text)) continue;
    seen.add(text);
    out.push({ year: e.year, text });
  }
  return out.length > 9 ? [...out.slice(0, 3), ...out.slice(-6)] : out;
}

function showDeath(info) {
  play.dead = true;
  try { localStorage.removeItem(SAVE_KEY); } catch (e) { /* storage unavailable */ }
  profileStore();   // item 04: the death report's rumors and a Myth's legend
  if (typeof closeGrowth === 'function') closeGrowth();
  $('lair').classList.add('hidden');
  $('wake').classList.add('hidden');
  keys.clear();
  releaseBurnHold();
  const d = model.dragon, g = d.growth;
  const life = g ? Object.entries(g).filter(([, v]) => v >= 0.5).map(([k, v]) => `${capFirst(k)} ${Math.floor(v)}`).join(', ') : `size ${fmt(d.size, 2)}`;
  const talents = (d.talents ?? []).map((id) => (typeof talentById === 'function' ? talentById(id)?.name : '') || id.replace(/_/g, ' '));
  const born = world.genesis?.hatched_year || play.ages[0]?.from || 0;
  const myth = sim({ cmd: 'world' }).myth;   // item 04: whether this death enters the next world's pre-history
  if (!DEATH.el) { DEATH.el = document.createElement('div'); DEATH.el.id = 'death'; DEATH.el.className = 'overlay hidden'; document.body.appendChild(DEATH.el); }
  DEATH.el.innerHTML = `<article class="tomb">
    <div class="kicker">year ${info.year} · age ${play.age} · ${Math.max(0, info.year - born)} years under the mountain</div>
    <h2>The line of the dragon ends</h2>
    <p class="how">${escapeHtml(deathLine(info))}</p>
    ${myth?.text ? `<p class="myth${myth.qualifies ? ' yes' : ''}">${escapeHtml(myth.text)}</p>` : ''}
    <p class="life">You grew to ${escapeHtml(life || 'nothing much')}${talents.length ? `; you had ${escapeHtml(talents.join(', '))}` : ''}.</p>
    ${biggestHits().length ? `<h3>The worst of it</h3><ol class="hl hits">${biggestHits().map((h) => `<li><span class="y">${h.year}</span>${escapeHtml(hitText(h))}</li>`).join('')}</ol>` : ''}
    <h3>Your life</h3><ol class="hl">${lifeHighlights().map((h) => `<li><span class="y">${h.year}</span>${escapeHtml(h.text)}</li>`).join('')}</ol>
    <p class="gone">The autosave is deleted. Death is final: there is no going back to this world.</p>
    <div class="row"><label class="muted">seed <input id="death-seed" type="number" value="${(hashString(String(play.seed) + ':' + info.year) % 9000) + 1}"></label><button id="death-new" class="primary">New world</button></div>
  </article>`;
  DEATH.el.classList.remove('hidden');
  $('dim').style.opacity = '0.85';
  $('death-new').addEventListener('click', () => newGame(Number($('death-seed').value) || 1, { genesis: true }));
  sfx('death');
  renderActions();
}

function closeDeath() { if (DEATH.el) DEATH.el.classList.add('hidden'); play.dead = false; }

addStyle(`
  #death { background: radial-gradient(ellipse at center, #1a0606f2, #030303fa 70%); z-index: 14; overflow-y: auto; align-items: flex-start; padding: 7vh 0; }
  #death .tomb { width: min(720px, 92vw); margin: 0 auto; color: #e6ddcc; font-family: Georgia, 'Palatino Linotype', 'DejaVu Serif', serif; text-align: center; }
  #death .kicker { font: 600 12px system-ui, sans-serif; letter-spacing: .14em; text-transform: uppercase; color: #9c6a5e; }
  #death h2 { font-weight: normal; font-size: 40px; margin: 14px 0 18px; color: #f0c2b4; letter-spacing: .02em; }
  #death p { font-size: 17px; line-height: 1.6; margin: 0 auto 14px; max-width: 34em; }
  #death p.how { color: #f4eee0; }
  #death p.life { color: #c9bfae; font-size: 15.5px; }
  #death p.myth { color: #9c8f7a; font-size: 15px; font-style: italic; }
  #death p.myth.yes { color: #e8c98a; }
  #death ol.hl { list-style: none; padding: 12px 0; margin: 10px auto 16px; max-width: 34em; text-align: left; border-top: 1px solid #3a2a26; border-bottom: 1px solid #3a2a26; font-size: 14.5px; line-height: 1.5; color: #b8ad9a; }
  #death ol.hl li { display: grid; grid-template-columns: 52px minmax(0, 1fr); margin-bottom: 4px; }
  #death ol.hl .y { color: #8a6a5e; font: 12px/1.9 ui-monospace, monospace; }
  #death h3 { font: 600 11px system-ui, sans-serif; letter-spacing: .14em; text-transform: uppercase; color: #9c6a5e; margin: 18px 0 0; }
  #death ol.hits { color: #e6c8bf; }
  #death p.gone { color: #d4594c; font: 13px system-ui, sans-serif; letter-spacing: .03em; }
  #death .row { justify-content: center; font: 13px system-ui, sans-serif; }
  #death .row input { width: 80px; }
  #death button.primary { font-size: 15px; padding: 7px 18px; }
`);
