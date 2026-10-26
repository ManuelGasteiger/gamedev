// The wake screen, "While you slept": the briefing the sim returns with every sleep, set as a page of a chronicle. It
// leads with this age's hooks (v0.4), then the headline and the century's paragraphs with their cause clauses, what you
// grew, people, threads, opportunities with a go button. Esc / Enter close it (and open the growth screen once when a
// talent waits); the HUD's "Last sleep" reopens the last one.
const THREAD_WORD = { arc: 'ongoing', tag: 'mark', oath: 'oath', spell: 'spell', remnant: 'remnant' };

function wakeOpen() { return !$('wake').classList.contains('hidden'); }

function growthHtml(g) {
  if (!g) return '';
  const row = (k, label, digits, badUp = false) => {
    const [b, a] = g[k] ?? [0, 0];
    const v = a - b, good = badUp ? v < 0 : v > 0;
    return `${label} ${fmt(a, digits)} <span class="${Math.abs(v) < 0.5 * 10 ** -digits ? '' : good ? 'up' : 'down'}">(${v >= 0 ? '+' : '−'}${Math.abs(v).toFixed(digits)})</span>`;
  };
  return [row('size', 'size', 3), row('knowledge', 'knowledge', 2), row('magic', 'magic', 2), row('faith_pool', 'faith', 2), row('hunger', 'hunger', 2, true)].join(' · ');
}

function wakeListHtml(items, empty) {
  if (!items.length) return `<p class="quiet">${empty}</p>`;
  return `<ul>${items.map((t) => `<li>${t}</li>`).join('')}</ul>`;
}

// a century of lines reads as several short paragraphs: a new one after four lines, where the year turns
function wakeChunks(lines) {
  const chunks = [];
  for (const l of lines) {
    const cur = chunks[chunks.length - 1];
    if (!cur || (cur.length >= 4 && l.year !== cur[cur.length - 1].year)) chunks.push([l]); else cur.push(l);
  }
  return chunks;
}

function wakeHtml(w) {
  const b = w.briefing ?? {};
  const reason = (w.wake_reason ?? '').replace(/_/g, ' ');
  const groups = (b.while_you_slept ?? []).map((g) => wakeChunks(g.lines ?? []).map((chunk, i) => `<div class="row2${i ? ' cont' : ''}"><div class="margin">${i === 0 ? `<span class="range">${g.from}${g.to > g.from ? '–' + g.to : ''}</span>` : ''}${chunk[0].year}</div>
      <p>${chunk.map((l) => `<span class="ln${l.event !== undefined && l.event === b.headline?.event ? ' lead' : ''}" title="year ${l.year}${l.events ? ' · ' + l.events + ' events' : ''}">${escapeHtml(l.text.replace(/ \.\.\. /g, ' … '))}</span>`).join(' ')}</p></div>`).join('')).join('');
  // people: what happened to those you know, then the hostages nobody mentioned yet; one line per person and text
  const told = new Set(), seenText = new Set();
  const people = [];
  for (const p of [...(b.people ?? []), ...(b.threads ?? []).filter((t) => t.kind === 'hostage').map((t) => ({ ...t, person: t.id }))]) {
    if (seenText.has(p.text) || (p.kind === 'hostage' && told.has(p.person))) continue;
    seenText.add(p.text);
    if (p.person) told.add(p.person);
    people.push(escapeHtml(p.text));
  }
  const threads = (b.threads ?? []).filter((t) => t.kind !== 'hostage').map((t) => `<span class="tk">${THREAD_WORD[t.kind] ?? t.kind}</span> ${escapeHtml(t.text)}`);
  const opps = (b.opportunities ?? []).map((o, i) => `${escapeHtml(o.text)} <button class="go" data-go="${i}" title="${o.kind === 'offer' ? t('wake.go.envoy', 'land at the lair and hear the envoy') : t('wake.go.fly', 'fly there')}">${t('wake.go', 'go')}</button>`);
  const hungry = w.wake_reason === 'hunger' ? ' Starving: the dragon wakes smaller and weaker.' : '';
  return `<article class="page">
    <div class="kicker"><span>${t('wake.slept', 'While you slept')}</span><span class="sep">·</span><span>${tf('wake.span', 'year {w.from} to {w.to}', { roles: { w: { fields: { from: String(w.from), to: String(w.to) } } } })}</span><span class="sep">·</span><span>${t('wake.woke', 'woke:')} ${reason}</span><span class="spacer"></span><button id="wake-skip" class="ghost" title="${t('wake.begin.title', 'Enter or Esc')}">${t('wake.begin', 'begin ⏎')}</button></div>
    ${hooksWakeHtml(w)}
    <h2 class="headline">${escapeHtml(b.headline?.text ?? TEXT.misc?.headline_none ?? 'A quiet age.')}</h2>
    <div class="dragonline">You slept ${yearsNum(w.years_slept)}.${hungry}${w.atrophy ? ` Atrophy −${fmt(w.atrophy, 3)} size.` : ''} ${growthHtml(w.growth)}<br>${escapeHtml(rhythmLine(phaseInfo()))}${phaseInfo().stiff ? ' You woke stiff from the moult: this raid\'s stamina is halved.' : ''}</div>
    <section class="slept">${groups || `<div class="row2"><div class="margin">${w.from}–${w.to}</div><p class="quiet">${escapeHtml(TEXT.misc?.quiet ?? 'Nothing the chroniclers thought worth a line.')}</p></div>`}</section>
    ${armsWakeHtml(w)}
    ${growthWakeHtml(w)}
    <section><div class="row2"><div class="margin">${t('wake.people', 'people')}</div>${wakeListHtml(people, 'No one you know died, rose or was crowned.')}</div></section>
    <section><div class="row2"><div class="margin">${t('wake.threads', 'threads')}</div>${wakeListHtml(threads, 'No oaths, spells or remnants are running.')}</div></section>
    <section><div class="row2"><div class="margin">opportunities</div>${wakeListHtml(opps, 'Nothing calls for you.')}</div></section>
    <footer><button id="wake-ok" class="primary">Begin the raid</button><span class="muted">Enter or Esc · “Last sleep” in the bar reopens this page · ${w.offers ?? 0} envoys on their way · stamina ${w.stamina ?? play.stamina}</span></footer>
  </article>`;
}

function openWake(w, fresh = false) {
  if (!w) { toast('no sleep yet'); return; }
  play.lastWake = w;
  const el = $('wake');
  el.dataset.kind = 'wake';
  el.innerHTML = wakeHtml(w);
  el.classList.remove('hidden');
  el.scrollTop = 0;
  growthUi.afterWake = fresh && hasGrowth() && openPicks().length > 0;
  $('wake-ok').addEventListener('click', () => closeWake());
  $('wake-skip').addEventListener('click', () => closeWake());
  el.querySelectorAll('[data-go]').forEach((b) => b.addEventListener('click', () => goTo((w.briefing?.opportunities ?? [])[Number(b.dataset.go)])));
  el.querySelectorAll('[data-growth-open]').forEach((b) => b.addEventListener('click', () => { closeWake(true); openGrowth(); }));
  bindHooksWake(el);
  renderActions();
}

// closing the first page after a sleep opens the growth screen when a talent waits (picks can wait: "Later" closes it)
function closeWake(skipGrowth = false) {
  $('wake').classList.add('hidden');
  if (document.activeElement && document.activeElement !== document.body) document.activeElement.blur();
  const growth = growthUi.afterWake && !skipGrowth && openPicks().length > 0;
  growthUi.afterWake = false;
  renderActions();
  if (growth) openGrowth();
}

// a "go" closes the page, selects the court and flies the dragon to it (to the lair for an envoy)
function goTo(o) {
  if (!o) return;
  closeWake(true);
  if (o.kind === 'offer') {
    play.focusFaction = o.faction || '';
    if (dragonRegion() === 'lair') { landAtLair(); setLairTabTo('envoys'); } else { flyTo('lair'); play.landOnArrival = true; toast('flying to the lair, where the envoy waits'); }
    return;
  }
  const f = model.factions.get(o.faction);
  if (!f) { toast(`${factionTitle(o.faction)} is gone`, true); return; }
  let region = f.capital || f.regions[0];
  if (o.kind === 'war') { const a = model.armies.find((x) => x.defender === f.id && f.at_war_with.includes(x.attacker)); if (a) region = a.region; }
  play.focusFaction = f.id;
  play.goTarget = region;
  flyTo(region);
  renderActions();
  toast(`flying to ${regionTitle(region)} (${factionTitle(f.id)})`);
}

function setLairTabTo(t) { lairTab = t; if (play.landed) renderLair(); }
