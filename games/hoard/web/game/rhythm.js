// Phases and the sleep rhythm: the HUD phase chip with progress to the next phase (growth points against
// phase.next_phase_points), and the Sleep tab's two primary choices from the phase's windows (rest = the short window,
// moult = the long one), each with its bet (bet.js) and forecast.
const PHASE_SUBJECT = ['A hatchling', 'A young dragon', 'A grown dragon', 'An ancient dragon', 'An elder dragon'];

function phaseInfo() {
  const ph = world.phase ?? {};
  const i = clamp((ph.phase ?? 1) - 1, 0, 4);
  return { index: i, phase: ph.phase ?? 1, name: TEXT.misc?.phase_names?.[i] ?? `phase ${i + 1}`, next: TEXT.misc?.phase_names?.[i + 1] ?? '',
    short: ph.short_sleep ?? 1, long: ph.long_sleep ?? 10, ready: !!ph.ready_to_moult, score: ph.next_phase_score ?? 0, stiff: !!ph.stiff,
    points: typeof ph.points === 'number' ? ph.points : null, nextPoints: typeof ph.next_phase_points === 'number' ? ph.next_phase_points : null };
}

// what the next phase is measured in: growth points (v0.4), else size, knowledge and magic
function phaseProgressText(p) {
  return p.nextPoints ? `${pointsText(p.points)} of the ${p.nextPoints} growth points` : `${Math.round(clamp(p.score, 0, 1) * 100)}% of the size, knowledge and magic`;
}

function renderPhaseHud() {
  const p = phaseInfo();
  $('h-phase').textContent = p.name;
  $('h-phase-bar').style.width = (p.index >= 4 ? 100 : clamp(p.score, 0, 1) * 100) + '%';
  const stat = $('h-phase-stat');
  stat.classList.toggle('ready', p.ready);
  stat.title = p.index >= 4 ? 'elder: the last phase' : p.ready ? `ready to moult: sleep ${p.long} years to become ${p.next}` : `${phaseProgressText(p)} it takes to become ${p.next}`;
  $('h-phase-note').textContent = p.ready ? 'moult!' : p.stiff ? 'stiff' : '';
}

// a sleep as long as the phase's rest window restores full strength; a shorter one restores its share
function restedStamina(left, years) {
  const full = staminaBudget();
  const share = Math.min(1, (years || 0) / Math.max(1, phaseInfo().short));
  return Math.min(full, Math.round((left || 0) + full * share));
}

const PHASE_NOUN = ['a hatchling', 'a young dragon', 'a grown dragon', 'an ancient dragon', 'an elder dragon'];

function rhythmLine(p) {
  const head = `${PHASE_SUBJECT[p.index]} needs ${yearsWord(p.short)} of rest to regain its strength (less restores less);`;
  if (p.index >= 4) return `${head} its moults of ${yearsWord(p.long)} lead to no further phase.`;
  const tail = p.ready ? 'You are ready: moult now.' : `You have ${phaseProgressText(p)} it takes.`;
  return `${head} a moult of ${yearsWord(p.long)} lets it grow into ${PHASE_NOUN[p.index + 1]}. ${tail}`;
}

function forecastSummary(years, f = sim({ cmd: 'forecast', years })) {
  if (!f.ok) return '';
  return `wake <b>${f.wake_reason.replace(/_/g, ' ')}</b> after ~${f.years} y · size ~${fmt(f.size, 3)} · knowledge ~${fmt(f.knowledge, 2)} · magic ~${fmt(f.magic, 2)} · hunger ~${fmt(f.hunger)} · strongest ${f.strongest ? escapeHtml(factionTitle(f.strongest)) : '-'}`;
}

function sleepOptionHtml(years, note = '') {
  const f = sim({ cmd: 'forecast', years });
  return `${sleepBetHtml(years, f)}<div class="muted fc">${note}${forecastSummary(years, f)}</div>`;
}

function sleepTabHtml() {
  const p = phaseInfo();
  const d = model.dragon;
  return `${hasArms() ? slayersHtml() : ''}<div class="box rhythm"><h3>Phase ${p.phase}: ${p.name}</h3><div class="rhythm-line">${rhythmLine(p)}</div>
      ${p.stiff ? `<div class="muted">You woke stiff from the moult: this raid's stamina is halved.</div>` : ''}
      <div class="sleep-choice">
        <div><button id="sleep-rest" class="primary big" data-years="${p.short}">Rest (${yearsNum(p.short)})</button>${sleepOptionHtml(p.short)}</div>
        <div class="${p.ready ? 'ready' : ''}"><button id="sleep-moult" class="${p.ready ? 'primary' : ''} big" data-years="${p.long}">Moult (${yearsNum(p.long)})</button>
          ${sleepOptionHtml(p.long, p.ready ? '<b class="up">ready to moult</b> · digestion doubles · ' : 'not ready: a long sleep without the moult · ')}</div>
      </div></div>
    <div class="box"><h3>Any length</h3>
      <div class="row"><input id="sleep-years" type="number" value="${play.sleepYears ?? 100}" step="1" min="1" style="width:90px"><button id="sleep-go">Sleep</button>
        <label class="muted"><input id="skip-lapse" type="checkbox" ${play.skipLapse ? 'checked' : ''}> skip the time-lapse, wake at once</label></div>
      <div id="forecast" class="muted" style="margin-top:8px"></div></div>
    <div class="box"><h3>Before you sleep</h3><div class="muted">${(model.offers ?? []).length} envoys waiting · ${openTasks().length} open oaths and tasks · ${(world.dragon.hostages ?? []).length} hostages in the lair · ${play.ageEvents.length} events this age · hoard gold ${fmt(d.hoard.gold, 1)} · hunger ${fmt(d.hunger)} (need ${fmt(d.hunger_need)} per decade)</div></div>`;
}

function bindSleepTab(body) {
  for (const id of ['sleep-rest', 'sleep-moult']) $(id)?.addEventListener('click', (e) => sleep(Number(e.currentTarget.dataset.years)));
  $('skip-lapse')?.addEventListener('change', (e) => { play.skipLapse = e.target.checked; });
}
