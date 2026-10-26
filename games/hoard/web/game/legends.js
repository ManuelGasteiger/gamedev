// The legends of the valley: before the first flight of a generated world (world.genesis, docs/core-loop.md section 6) a
// page of the chronicle tells what happened before the egg hatched (the `legends` command, years absolute: the egg
// hatches at genesis.hatched_year) and how the valley stands now. It uses the wake screen's frame: Enter, Esc or
// "Take wing" closes it. A classic world carries an empty genesis (no template) and has no legends.
const GENESIS_TITLE = {
  rival_kingdoms: 'The valley of rival crowns', dwarf_empire: 'The valley of the old dwarf empire', elven_realm: 'The valley of the elves and their guests',
  orc_horde: 'The valley under the horde', free_cities: 'The valley of free cities', fallen_kingdom: 'The valley of ruins',
  the_exodus: 'The valley of the newcomers', the_undying_queen: 'The valley of the undying throne', the_empty_valley: 'The empty valley',
};

function hasLegends() { return !!world?.genesis?.template; }

function openLegends() {
  const g = world?.genesis;
  if (!g?.template) return false;
  const res = sim({ cmd: 'legends' });
  const events = res?.ok ? (res.events ?? []).slice().sort((a, b) => a.year - b.year) : [];
  const el = $('wake');
  el.dataset.kind = 'legends';
  el.innerHTML = legendsHtml(g, events);
  el.classList.remove('hidden');
  el.scrollTop = 0;
  for (const id of ['legends-go', 'legends-skip']) $(id).addEventListener('click', () => closeWake(true));
  renderActions();
  return true;
}

function legendsHtml(g, events) {
  const hatched = g.hatched_year ?? model.year;
  const ago = (y) => (hatched - y <= 0 ? '<span class="ago">now</span>' : `<span class="ago">${hatched - y}</span><span class="range">years before</span>`);
  const lines = events.map((e) => `<div class="row2"><div class="margin">${ago(e.year)}</div>
      <p class="${(e.importance ?? 0) >= 0.9 ? 'great' : ''}">${escapeHtml(cleanSimText(e.text))}</p></div>`).join('');
  const courts = world.factions.filter((f) => f.regions.length).map((f) => `<li><span class="sw" style="background:${factionCss(f.id)}"></span><b>${escapeHtml(factionTitle(f.id))}</b>,
      ${racePlural(f.dominant_race ?? f.race)} of the ${eraWord(f.era)} age at ${escapeHtml(regionTitle(f.capital))}, ${f.regions.length} region${f.regions.length === 1 ? '' : 's'}</li>`).join('');
  const lead = GENESIS_TITLE[g.template] ?? capFirst(String(g.template).replace(/_/g, ' '));
  return `<article class="page legends">
    <div class="kicker"><span>The legends of the valley</span><span class="sep">·</span><span>${g.prehistory_years ?? '?'} years before you</span><span class="spacer"></span><button id="legends-skip" class="ghost" title="Enter or Esc">take wing ⏎</button></div>
    <h2 class="headline">${escapeHtml(lead)}</h2>
    <div class="dragonline">You hatch in the year ${hatched}, in the ash of the crater of ${escapeHtml(regionTitle('lair'))}. Everyone below has forgotten that dragons were real.</div>
    <section class="slept">${lines || '<div class="row2"><div class="margin"></div><p class="quiet">No one remembers what came before.</p></div>'}</section>
    <section><div class="row2"><div class="margin">the courts</div><ul>${courts}</ul></div></section>
    <footer><button id="legends-go" class="primary">Take wing</button><span class="muted">Enter or Esc</span></footer>
  </article>`;
}

addStyle(`
  #wake .legends p.great { color: #f4eee0; }
  #wake .legends .margin .ago { display: block; font-size: 15px; letter-spacing: .02em; color: #d8c6a0; }
  #wake .legends .margin .range { margin-top: -2px; }
`);
