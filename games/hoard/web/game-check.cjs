// node web/game-check.cjs — drives web/hoard-game.html headless through the playtest checklist (web/GAME.md) and writes
// one screenshot per item to web/shots/ (v02-*.png for the v0.2 items: wake screen, go, narration, court, hostage and
// known people, envoy audience and eat-the-envoy, rest and moult). Exit 0 only when every item passes and the console
// stayed clean.
// Env: PLAYWRIGHT_BROWSERS_PATH, NODE_PATH (for playwright). The page is self-contained (three.js and web/game/*.js are
// inlined at build time: bash web/build.sh --html && python3 web/inline.py web/hoard-game.html); the route below only
// serves an older build that still loaded three.js from cdnjs (THREE_LOCAL, default web/vendor/three.r158.min.js).
const path = require('path');
const fs = require('fs');
const { chromium } = require('playwright');

const SEED = 39;   // chosen so that age 1 ends with wars, armies and a protect offer (see GAME.md); was 77 before v0.2's people and offers
const ONLY_V04 = process.argv.includes('--v04');   // `node web/game-check.cjs --v04` runs the v0.4 screens alone
const shots = path.resolve(__dirname, 'shots');
const results = [];
let page;

function check(id, ok, detail) { results.push({ id, ok: !!ok, detail }); console.log((ok ? 'PASS ' : 'FAIL ') + id + (detail ? '  ' + detail : '')); return ok; }
async function shot(name) { await page.screenshot({ path: path.join(shots, name + '.png'), timeout: 180000 }); }   // swiftshader on a busy machine is slow
const state = () => page.evaluate(() => game.state());
// item 06: an envoy climbs only to a lair its court has seen, so the lab lets every court see it before a sleep
async function seeLair() {
  await page.evaluate(() => {
    const w = game.world();
    for (const f of w.factions) { f.lair_known = w.year; f.lair_exact = true; }
    game.loadWorld(w);
  });
  await page.waitForTimeout(100);
}
const oathStatus = (st) => ({ active: 'active', expired: 'expired', breached_by_dragon: 'broken by you', breached_by_them: 'broken by them' })[st] ?? st;
const actions = () => page.evaluate(() => game.actions());
const fly = (id) => page.evaluate((id) => game.flyTo(id), id);
const act = (name, params) => page.evaluate(([n, p]) => game.act(n, p), [name, params]);
const panelText = () => page.$eval('#actions', (e) => e.innerText);
// headless swiftshader renders a few frames per second; a held key only moves the dragon on frames, so hold until n frames ran
async function holdKey(key, frames = 4) { const f0 = (await state()).frame; await page.keyboard.down(key); await page.waitForFunction((n) => game.state().frame >= n, f0 + frames); await page.keyboard.up(key); }
async function frames(n = 2) { const f0 = (await state()).frame; await page.waitForFunction((k) => game.state().frame >= k, f0 + n); }

async function sleepThrough(years, opts = {}) {
  await page.evaluate(() => { if (!game.state().landed) game.land(); });
  await page.evaluate((t) => game.setLairTab(t), 'sleep');
  await page.$eval('#sleep-years', (el, y) => { el.value = String(y); el.dispatchEvent(new Event('change')); }, years);
  const forecast = await page.$eval('#forecast', (e) => e.textContent);
  if (opts.beforeSleep) await opts.beforeSleep(forecast);
  await page.click('#sleep-go');
  await page.evaluate(() => game.setSpeed(2000));
  return forecast;
}

// since S1 a talent that waits opens the growth screen as the wake page closes; the v0.3 items let it wait
async function dismissGrowth() { if (await page.evaluate(() => game.v04().growthOpen)) await page.keyboard.press('Escape'); }
async function beginRaid() { await page.click('#wake-ok'); await dismissGrowth(); }

async function finishSleep() {
  await page.evaluate(() => game.finishMontage());
  await page.waitForFunction(() => !game.state().montage);
}

(async () => {
  fs.mkdirSync(shots, { recursive: true });
  for (const f of fs.readdirSync(shots)) if (f.endsWith('.png') && !f.startsWith('v04real-') && (!ONLY_V04 || f.startsWith('v04-'))) fs.unlinkSync(path.join(shots, f));   // v04real-*: the hand playthrough's shots
  // one self-contained file: no script or stylesheet is fetched, the split game/*.js are inlined (web/inline.py)
  const built = fs.readFileSync(path.resolve(__dirname, 'hoard-game.html'), 'utf8');
  const external = built.match(/<script[^>]*\ssrc=|<link[^>]*\shref=|\ssrc="https?:|url\(["']?https?:/g) ?? [];
  const split = fs.readdirSync(path.resolve(__dirname, 'game')).filter((f) => f.endsWith('.js')).map((f) => f.slice(0, -3));
  const inlined = split.filter((n) => built.includes(`/* game/${n}.js */`));
  check('v02.selfcontained', external.length === 0 && inlined.length === split.length, `${(built.length / 1e6).toFixed(2)} MB, ${external.length} external references, ${inlined.length}/${split.length} split scripts inlined`);
  const browser = await chromium.launch({ args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });
  page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
  const errors = [];
  page.on('console', (m) => { if (m.type() === 'error' || (m.type() === 'warning' && !/deprecated with r150/.test(m.text()))) errors.push(m.type() + ': ' + m.text()); });
  page.on('pageerror', (e) => errors.push('pageerror: ' + e.message));
  if (ONLY_V04) { await checksV04(); return finish(browser, errors); }
  const local = process.env.THREE_LOCAL || path.resolve(__dirname, 'vendor/three.r158.min.js');
  if (fs.existsSync(local)) await page.route('**/three.js/r158/three.min.js', (route) => route.fulfill({ path: local, contentType: 'application/javascript' }));
  await page.goto('file://' + path.resolve(__dirname, 'hoard-game.html'));
  await page.waitForSelector('body[data-ready="1"]', { timeout: 30000 });
  await page.evaluate((seed) => { try { localStorage.clear(); } catch (e) { /* ignore */ } game.newGame(seed); }, SEED);
  await page.waitForTimeout(300);
  await shot('00-start');

  // ---- a. every region type: the panel lists every verb, each either usable or with a reason ----------------------
  const VERBS = ['burn', 'take', 'kill', 'defend', 'roar', 'hostage', 'release', 'give', 'altar', 'miracle', 'demand', 'spell-ward', 'spell-curse', 'spell-bless', 'spell-incite', 'spell-raise_undead', 'spell-teach', 'spell-corrupt', 'sponsor', 'terraform'];
  const regions = await page.evaluate(() => game.world().regions.map((r) => ({ id: r.id, terrain: r.terrain, owner: r.owner, capital: r.capital })));
  const seenTerrain = new Set();
  let panelOk = true, shotIndex = 0;
  for (const r of regions) {
    await fly(r.id);
    await page.waitForTimeout(60);
    const list = await actions();
    const ids = list.map((a) => a.id);
    const missing = VERBS.filter((v) => !ids.includes(v));
    const stubbed = list.filter((a) => !a.ok && /not in this build/.test(a.why) && a.id !== 'spell-corrupt');
    const silent = list.filter((a) => !a.ok && !a.why);
    const text = await panelText();
    const flat = text.replace(/\s+/g, ' ');
    const hasFacts = /terrain/.test(flat) && (r.owner ? /race .* era .* wealth .* dread .* awe .* faith/.test(flat) : true);
    if (missing.length || stubbed.length || silent.length || !hasFacts) { panelOk = false; console.log('  panel problem at', r.id, { missing, stubbed: stubbed.map((a) => a.id), silent: silent.map((a) => a.id), hasFacts }); }
    if (!seenTerrain.has(r.terrain)) { seenTerrain.add(r.terrain); await shot(`a${shotIndex++}-panel-${r.terrain}`); }
  }
  check('a.panel', panelOk && seenTerrain.size === 7, `${seenTerrain.size} terrain types, ${VERBS.length} verbs each with a reason when unavailable`);

  // a. usable verbs now: burn, take (every category offered), roar, hostage, give (after take), miracle/altar/spells gated with reasons
  await fly('east_vale');
  const s0 = await state();
  const aldBefore = s0.factions.find((f) => f.id === 'aldmere');
  const cats = await page.$$eval('#take-cat option', (o) => o.map((x) => x.value));
  const r1 = await act('burn', { region: 'east_vale', magnitude: 0.4 });
  await page.waitForTimeout(400);
  await shot('a-burn');
  const s1 = await state();
  const aldAfter = s1.factions.find((f) => f.id === 'aldmere');
  check('a.burn', r1 && r1.ok && aldAfter.wealth < aldBefore.wealth && aldAfter.dread > aldBefore.dread, `aldmere wealth ${aldBefore.wealth.toFixed(1)} -> ${aldAfter.wealth.toFixed(1)}, dread ${aldAfter.dread.toFixed(2)}`);
  const rTake = await act('take', { region: 'east_vale', category: 'gold', magnitude: 0.3 });
  const rBooks = (await fly('deep_wood'), await act('take', { region: 'deep_wood', category: 'books', magnitude: 0.3 }));
  const rArt = (await fly('hill_hold'), await act('take', { region: 'hill_hold', category: 'artifacts', magnitude: 0.6 }));
  const hoard = (await state()).dragon.hoard;
  check('a.take', rTake?.ok && rBooks?.ok && rArt?.ok && hoard.gold > 0 && hoard.books > 0 && hoard.artifacts > 0 && cats.includes('royals') && cats.includes('food') && cats.includes('gold'), `east_vale (human, era 0) offers ${cats.join('/')}; elves give books, dwarves artifacts; hoard gold ${hoard.gold.toFixed(1)} books ${hoard.books.toFixed(1)} artifacts ${hoard.artifacts.toFixed(1)}`);
  const dorunDread0 = (await state()).factions.find((f) => f.id === 'dorun').dread;
  const rRoar = await act('show', { region: 'hill_hold' });
  const dorunDread1 = (await state()).factions.find((f) => f.id === 'dorun').dread;
  check('a.roar', rRoar?.ok && dorunDread1 > dorunDread0, `dorun dread ${dorunDread0.toFixed(2)} -> ${dorunDread1.toFixed(2)}`);
  const giveBefore = await state();
  const rGive = await act('give', { region: 'hill_hold', gold: 2 });
  const giveAfter = await state();
  const dw = giveAfter.factions.find((f) => f.id === 'dorun').wealth - giveBefore.factions.find((f) => f.id === 'dorun').wealth;
  const dg = giveBefore.dragon.hoard.gold - giveAfter.dragon.hoard.gold;
  check('a.give', rGive?.ok && Math.abs(dw - 2) < 0.01 && Math.abs(dg - 2) < 0.01 && rGive.events.some((e) => e.kind === 'act_give'), `dorun wealth +${dw.toFixed(1)}, hoard gold -${dg.toFixed(1)} (the faith memory +0.04 is offset by the earlier take, so faith stays 0)`);
  await fly('west_vale');
  const rHost = await act('take_hostage', { region: 'west_vale' });
  const lin = (await state()).dragon.lineages;
  const relOk = (await actions()).find((a) => a.id === 'release')?.ok;
  await shot('a-hostage');
  check('a.hostage', rHost?.ok && lin.includes('vessary') && relOk, `lineages ${lin.join(',')}; release now offered: ${relOk}`);
  const rRel = await act('release', { region: 'west_vale' });
  const hostAgain = (await actions()).find((a) => a.id === 'hostage');
  const linAfter = (await state()).dragon.lineages;
  check('a.release', rRel?.ok && !/already holding/.test(hostAgain.why) && !linAfter.includes('vessary'), `heir returned; lineages now [${linAfter.join(',')}]; hostage row: ${hostAgain.ok ? 'available' : hostAgain.why}`);
  const gated = await actions();
  const g = (id) => gated.find((a) => a.id === id);
  check('a.gates', !g('miracle').ok && /faith pool/.test(g('miracle').why) && !g('altar').ok && /faith/.test(g('altar').why) && !g('spell-ward').ok && /magic/.test(g('spell-ward').why) && !g('terraform').ok && /size/.test(g('terraform').why) && !g('spell-corrupt').ok && /not in this build/.test(g('spell-corrupt').why) && !g('demand').ok && /knowledge/.test(g('demand').why) && !g('spell-teach').ok && /knowledge/.test(g('spell-teach').why),
    `miracle: ${g('miracle').why} | altar: ${g('altar').why} | ward: ${g('spell-ward').why} | terraform: ${g('terraform').why} | demand: ${g('demand').why}`);
  await fly('north_scar');
  const scar = await actions();
  const sc = (id) => scar.find((a) => a.id === id);
  check('a.unclaimed', !sc('burn').ok && /nothing here/.test(sc('burn').why) && !sc('sponsor').ok && /remnant/.test(sc('sponsor').why) && !sc('spell-raise_undead').ok && /magic/.test(sc('spell-raise_undead').why), `burn: ${sc('burn').why} | sponsor: ${sc('sponsor').why} | undead: ${sc('spell-raise_undead').why}`);

  // ---- b. land, sleep 100: act log matches, deltas shown, chronicle first lines are the acts --------------------
  const acts1 = (await state()).ageEvents.filter((k) => k.startsWith('act_'));   // v0.2 adds hostage_taken/released person events to the raid log
  await page.evaluate(() => game.flyTo('lair'));
  await page.keyboard.press('l');
  await page.waitForTimeout(100);
  const landed = (await state()).landed;
  await shot('b-lair');
  await page.evaluate(() => game.setLairTab('chronicle'));
  const chronText = await page.$eval('#lair-chron', (e) => e.innerText);
  const firstLines = chronText.split('\n').filter((l) => /^\d+\S/.test(l)).slice(0, 8).join(' | ');
  const expectedActs = ['act_burn', 'act_take', 'act_take', 'act_take', 'act_show', 'act_give', 'act_take_hostage', 'act_release'];
  const lines = chronText.split('\n').filter((l) => /^\d+dragon:/.test(l));
  check('b.actlog', landed && JSON.stringify(acts1) === JSON.stringify(expectedActs) && /release on Vessary/.test(lines[0]) && /burn on Aldmere at East Vale \(m 0.40\)/.test(lines[7]) && lines.length === 8, `acts: ${acts1.join(',')} | chronicle newest first: ${firstLines.slice(0, 200)}`);
  await shot('b-chronicle');
  await page.evaluate(() => game.setLairTab('hoard'));
  await shot('b-hoard');
  const forecast1 = await sleepThrough(100);
  await page.waitForTimeout(200);
  await shot('b-montage');
  await finishSleep();
  await page.waitForTimeout(300);
  await shot('b-wake');
  const s2 = await state();
  const wakeText = await page.$eval('#wake', (e) => e.innerText);
  check('b.sleep', s2.year === 100 && s2.age === 2 && s2.ages[0].acts >= expectedActs.length && s2.ages[0].events > 0 && /slept 100 years/.test(wakeText) && /knowledge/.test(wakeText), `year ${s2.year}, age ${s2.age}, ${s2.ages[0].events} events, acts logged ${s2.ages[0].acts}`);
  check('g.forecast1', /Forecast for 100 years/.test(forecast1) && /noise ±50%/.test(forecast1), forecast1.slice(0, 120));

  // ---- d. envoys after the sleep, accept a defence offer, breach it ----------------------------------------------
  await beginRaid();
  await page.waitForTimeout(100);
  await frames(2);
  const readOffers = () => page.evaluate(() => game.world().offers.map((o) => ({ key: o.text_key, party: o.oath.parties[1], clauses: o.oath.clauses.map((c) => c.pred) })));
  let s3 = await state();
  let offers = await readOffers();
  // a protect offer from a court that an army is marching on (the defend check below needs that army)
  const besieged = () => page.evaluate(() => {
    const w = game.world();
    return w.offers.findIndex((o) => /protect/.test(o.text_key) && (w.armies ?? []).some((a) => a.defender === o.oath.parties[1] && (w.factions.find((f) => f.id === a.defender)?.at_war_with ?? []).includes(a.attacker)));
  });
  let defOffer = await besieged();
  // the history depends on the sim's data; sleep on (at most 12 centuries) until a court at war asks for protection
  for (let extra = 0; extra < 12 && (defOffer < 0 || s3.armies === 0); extra++) {
    await seeLair();
    await sleepThrough(100);
    await finishSleep();
    await page.waitForTimeout(100);
    await beginRaid();
    await page.waitForTimeout(100);
    await frames(2);
    s3 = await state();
    offers = await readOffers();
    defOffer = await besieged();
  }
  check('d.envoys', s3.offers > 0 && s3.envoys === s3.offers && defOffer >= 0 && s3.armies > 0, `${s3.offers} offers (${offers.map((o) => o.key.slice(6) + ':' + o.party).join(', ')}), ${s3.envoys} envoy figures, ${s3.armies} armies, ${s3.arrows} war arrows`);
  await page.waitForTimeout(800);
  await shot('d-envoys-walking');
  await page.evaluate(() => game.land());
  await page.evaluate(() => game.setLairTab('envoys'));
  const envoyText = await page.$eval('#lair-body', (e) => e.innerText);
  await shot('d-envoys-tab');
  check('d.clauses', /pay tribute of .* wealth per decade/.test(envoyText) && /you will defend them/.test(envoyText) && /if you break it: betrayal/.test(envoyText), envoyText.split('\n').slice(0, 4).join(' | ').slice(0, 200));
  const party = offers[defOffer].party;
  await page.click(`[data-accept="${defOffer}"]`);
  await page.waitForTimeout(100);
  await page.evaluate(() => game.setLairTab('oaths'));
  const oathText = await page.$eval('#lair-body', (e) => e.innerText);
  const s4 = await state();
  await shot('d-oaths');
  check('d.accept', s4.oaths.some((o) => o.party === party && o.status === 'active') && new RegExp(party).test(oathText) && /active/.test(oathText) && s4.ageEvents.includes('oath_sworn'), `oath with ${party}: ${s4.oaths.map((o) => o.id + ' ' + o.status).join(', ')}`);

  // ---- a. kill and defend with the armies that now stand on the map ---------------------------------------------
  await page.evaluate(() => game.takeOff());
  await frames(2);
  const tasksD = await page.evaluate(() => game.tasks());
  const tasksCardD = await page.$eval('#tasks', (e) => !e.classList.contains('hidden') && e.innerText);
  const partyName = party[0].toUpperCase() + party.slice(1);
  await shot('v02-tasks');
  check('v02.tasks', tasksD.some((t) => t.faction === party) && tasksCardD && new RegExp(`defend ${partyName}.* until \\d+ \\(${partyName} pays [\\d.]+ gold a decade\\)`).test(tasksCardD),
    `Oaths and tasks: ${(tasksCardD || '').replace(/\n/g, ' | ')}`);
  const armies = await page.evaluate(() => game.world().armies);
  const partyFaction = s4.factions.find((f) => f.id === party);
  const defendable = armies.find((a) => a.defender === party && partyFaction.at_war_with.includes(a.attacker));
  await fly(armies[0].region);
  const killOk = (await actions()).find((a) => a.id === 'kill')?.ok;
  const hunger0 = (await state()).dragon.hunger;
  const rKill = await act('kill', { region: armies[0].region, attacker: armies[0].attacker });
  const hunger1 = (await state()).dragon.hunger;
  await shot('a-kill');
  check('a.kill', killOk && rKill?.ok && hunger1 <= hunger0, `${armies[0].attacker} at ${armies[0].region}; hunger ${hunger0.toFixed(2)} -> ${hunger1.toFixed(2)}`);
  await fly(defendable.region);
  const defOk = (await actions()).find((a) => a.id === 'defend')?.ok;
  const rDef = await act('defend', { region: defendable.region, against: defendable.attacker });
  const warEnded = rDef?.events?.some((e) => e.kind === 'war_ended' || e.kind === 'coalition_ended');
  await page.waitForTimeout(300);
  await shot('a-defend');
  check('a.defend', defOk && rDef?.ok && rDef.events.some((e) => e.kind === 'act_defend'), `defended ${party} at ${defendable.region} against ${defendable.attacker}${warEnded ? ', war ended' : ''}`);

  // ---- a. demand and teach (knowledge from the books), then breach the oath by burning the party -----------------
  const kn = (await state()).dragon.knowledge;
  const courts = (await state()).factions.filter((f) => f.capital).sort((a, b) => b.wealth - a.wealth);
  const rich = courts[0];
  // the richest court the panel lets you make a demand of (a court sworn to every clause already, or not cowed, says why)
  let dmCourt = rich, dmAction = null, dmPred = 'tribute';
  const dmRefusals = [];
  for (const f of courts) {
    await fly(f.capital);
    const a = (await actions()).find((x) => x.id === 'demand');
    if (a.ok) { dmCourt = f; dmAction = a; dmPred = await page.$eval('#demand-pred', (e) => e.value); break; }
    dmRefusals.push(`${f.id}: ${a.why}`);
  }
  const rDm = await act('demand', { region: dmCourt.capital, pred: dmPred });
  await shot('a-demand');
  check('a.demand', kn >= 0.5 && dmAction?.ok && rDm?.ok && rDm.sworn && (await state()).oaths.some((o) => o.id.startsWith('demand_')), `knowledge ${kn.toFixed(2)}; ${dmRefusals.length ? 'panel refuses ' + dmRefusals.join(' | ') + '; ' : ''}${dmCourt.id} ${rDm?.sworn ? 'swears ' + dmPred.replace('_', ' ') : 'refuses: ' + rDm?.reason}`);
  await fly('hill_hold');
  const teachOk = (await actions()).find((a) => a.id === 'spell-teach')?.ok;
  const rTeach = await act('cast', { region: 'hill_hold', spell: 'teach' });
  const teachOwner = (await page.evaluate(() => game.world().regions.find((r) => r.id === 'hill_hold')?.owner)) ?? '';
  check('a.teach', teachOk && rTeach?.cast && (await state()).modifiers.includes('dragon.teach>' + teachOwner), `teach on ${teachOwner}: ${rTeach?.cast} ${rTeach?.reason ?? ''}`);
  const partyCapital = (await state()).factions.find((f) => f.id === party)?.capital;
  await fly(partyCapital);
  const rBreach = await act('burn', { region: partyCapital, magnitude: 0.25 });
  const partyAfter = (await state()).factions.find((f) => f.id === party);
  await page.waitForTimeout(300);
  await shot('d-breach');
  check('d.breach', rBreach?.ok && partyAfter.faith <= 0.03 && rBreach.changed.some((c) => c.id === party && c.after.dread > c.before.dread), `burned ${party} under oath: faith ${partyAfter.faith.toFixed(3)} (betrayal sets 0.02), dread ${partyAfter.dread.toFixed(2)}`);

  // ---- e. spells tab: curse before sleeping (magic 2.5 covers 1.5), ward shown gated with its reason ----------------
  await page.evaluate(() => game.land());
  await page.evaluate(() => game.setLairTab('spells'));
  const magic = (await state()).dragon.magic;
  await page.selectOption('#spell-t-curse', rich.id);
  await page.click('[data-cast="curse"]');
  await page.waitForTimeout(100);
  const spellsText = await page.$eval('#lair-body', (e) => e.innerText.replace(/\s+/g, ' '));
  const s5 = await state();
  // ward: gated with its reason while magic < 1, castable (a Cast button with a target) once the hoard paid for it
  const wardCastable = await page.$('[data-cast="ward"]') !== null;
  const wardRow = s5.dragon.magic < 1 ? /Ward 1 magic magic \d\.\d+ < 1/.test(spellsText) && !wardCastable : wardCastable && /Ward 1 magic invasions/.test(spellsText);
  await shot('e-spells');
  await page.evaluate(() => game.takeOff());
  await fly(rich.capital);
  await frames(2);
  const curseFacts = await panelText();
  const curseLabel = (await page.evaluate(() => game.labels())).find((l) => l.id === rich.capital);
  await shot('e-cursed-region');
  check('e.cast', magic >= 1.5 && s5.modifiers.includes('dragon.curse>' + rich.id) && new RegExp('Curse on ' + rich.id, 'i').test(spellsText) && wardRow && /curse cohesion_decay mul 2 until/.test(curseFacts) && /cursed/.test(curseLabel.text), `magic ${magic.toFixed(2)} -> ${s5.dragon.magic.toFixed(2)}; modifiers ${s5.modifiers.join(', ')}; ward row ${s5.dragon.magic < 1 ? 'gated' : 'castable'} at magic ${s5.dragon.magic.toFixed(2)}: ${wardRow}; label "${curseLabel.text}"`);

  // ---- c. sleeps 300 + 300 with hard burns: wars, conquest, collapse, era, plague, settle, orcs, expiry ------------
  const wanted = { war_started: 'c-war', region_taken: 'c-conquest', collapse: 'c-collapse', era_advance: 'c-era', plague: 'c-plague', region_settled: 'c-settled', orcs_spawned: 'c-orcs', spell_expired: 'e-expiry', vassal_sworn: 'c-vassal' };
  const captured = {};
  async function playMontage() {
    await page.evaluate(() => game.pauseMontage());
    const info = await page.evaluate(() => game.montageInfo());
    for (let i = 0; i < info.kinds.length; i++) {
      const k = await page.evaluate(() => game.stepMontage());
      if (k === null) break;
      if (wanted[k] && !captured[k]) {
        await frames(1);
        const ticker = await page.$eval('#ticker', (e) => e.textContent);
        const st = await state();
        const worthy = (await page.evaluate(() => game.montageInfo())).lastWorthy;
        captured[k] = { ticker, worthy, arrows: st.arrows, armies: st.armyGroups, year: Math.floor(st.montageYear) };
        await shot(wanted[k]);
      }
    }
    await finishSleep();
    await frames(2);
    return info.kinds;
  }
  const ownerBefore = {};
  for (const r of regions) ownerBefore[r.id] = await page.evaluate((id) => game.tileOwnerRing(id), r.id);
  const eraBefore = await page.evaluate((id) => game.settlementCount(id), 'hill_hold');
  await act('burn', { region: rich.capital, magnitude: 0.5 });
  await sleepThrough(300);
  const kinds2 = await playMontage();
  await shot('c-wake');
  let s6 = await state();
  // item 06 moved the history on; whether the breached court's oath breaks inside these 300 years is chance, so sleep on until one does
  for (let extra = 0; extra < 10 && !s6.oaths.some((o) => o.status !== 'active'); extra++) {
    if ((await page.evaluate(() => game.wake())).open) await beginRaid();
    await sleepThrough(100);
    kinds2.push(...await playMontage());
    s6 = await state();
  }
  check('d.betrayal', s6.oaths.some((o) => o.status !== 'active') && s6.ages[1].events > 0, `oath statuses after the sleep: ${s6.oaths.map((o) => o.party + ' ' + oathStatus(o.status)).join(', ')}`);
  await beginRaid();
  // age 3: ward hill_hold from the raid panel (magic has regrown), burn two outlying regions hard, sleep 300
  await fly('hill_hold');
  const wardAction = (await actions()).find((a) => a.id === 'spell-ward');
  const rWard = await act('cast', { region: 'hill_hold', spell: 'ward' });
  await frames(2);
  const wardLabel = (await page.evaluate(() => game.labels())).find((l) => l.id === 'hill_hold');
  await shot('e-warded-region');
  check('e.ward', wardAction.ok && rWard?.cast && (await state()).modifiers.includes('dragon.ward>hill_hold') && /warded/.test(wardLabel.text), `ward on hill_hold; label "${wardLabel.text.replace(/\n/g, ' ')}"`);
  const s6regions = await page.evaluate(() => game.world().regions.map((r) => ({ id: r.id, owner: r.owner, capital: r.capital, wealth_share: r.wealth_share })));
  const multi = new Set(s6.factions.filter((f) => f.regions.length >= 2).map((f) => f.id));
  const targets = s6regions.filter((r) => r.owner && multi.has(r.owner) && !r.capital).sort((a, b) => b.wealth_share - a.wealth_share).slice(0, 2);
  for (const t of targets) { await fly(t.id); await act('burn', { region: t.id, magnitude: 0.8 }); }
  await page.waitForTimeout(300);
  await shot('c-raze');
  await sleepThrough(300);
  const kinds3 = await playMontage();
  await shot('c-wake-2');
  const kindsAll = [...kinds2, ...kinds3];
  // plague and settling are chance events and the curse and ward must have run out: when 600 years were not enough, sleep
  // on a century at a time (at most ten) until the valley has shown them; what the history holds depends on the sim's data
  const spellsLeft = () => page.evaluate(() => { const w = game.world(); return (w.modifiers ?? []).filter((m) => /^dragon\.(curse|ward)$/.test(m.source)).map((m) => `${m.source.slice(7)}>${m.target} until ${m.until}`); });
  const closeWake = async () => { if ((await page.evaluate(() => game.wake())).open) await beginRaid(); };
  // a full valley has no open land to settle: then a lab world frees an outlying region (burn 0) next to another court
  let settleLab = '';
  // the land may be taken again before anyone settles it, so the lab opens a region again before each extra century
  const openLand = async () => {
    await closeWake();
    settleLab = await page.evaluate(() => {
      const w = game.world();
      const court = (id) => w.factions.find((x) => x.id === w.regions.find((r) => r.id === id)?.owner);
      const outlying = w.regions.filter((r) => r.owner && !r.capital && !r.lair);
      const free = outlying.length ? outlying : w.regions.filter((r) => r.owner && !r.lair);   // every court holds one region: free a whole court's land
      const r = free.find((x) => x.neighbours.some((n) => court(n) && court(n).id !== x.owner && court(n).dominant_race !== 'orc')) ?? free[0];
      if (!r) return '';
      const f = court(r.id);
      f.regions = f.regions.filter((x) => x !== r.id);
      r.owner = ''; r.burn = 0; r.tags = r.tags.filter((t) => t !== 'wasteland' && t !== 'dug_in');
      w.factions = w.factions.filter((x) => x.regions.length);
      const alive = new Set(w.factions.map((x) => x.id));
      for (const x of w.factions) for (const k of ['at_war_with', 'allies']) if (Array.isArray(x[k])) x[k] = x[k].filter((id) => alive.has(id));
      for (const n of r.neighbours) { const g = court(n); if (g) g.cohesion = Math.max(g.cohesion, 0.9); }
      game.loadWorld(w);
      return r.id;
    });
  };
  if (!captured.region_settled) await openLand();
  // a valley of one-region courts has no conquest (a court's last region makes it a vassal instead): then a lab gives a
  // weaker neighbour a second region and starts a war it is bound to lose
  let conquestLab = '';
  if (!captured.region_taken) {
    await closeWake();
    conquestLab = await page.evaluate(() => {
      const w = game.world();
      const fac = (id) => w.factions.find((x) => x.id === id);
      const reg = (id) => w.regions.find((r) => r.id === id);
      for (const A of w.factions.filter((x) => x.regions.length && x.race !== 'orc'))
        for (const ra of A.regions) for (const n of reg(ra).neighbours) {
          const B = fac(reg(n)?.owner);
          if (!B || B.id === A.id || B.race === 'orc' || A.overlord === B.id || B.overlord === A.id) continue;
          // a warded or dug-in border, or an orc court, holds most decades of a five-decade war
          if (B.dominant_race === 'orc' || reg(n).tags.includes('dug_in') || (w.modifiers ?? []).some((m) => m.target === n && m.var === 'invasion_chance')) continue;
          const X = reg(n).neighbours.map(reg).find((r) => r && !r.lair && r.owner !== A.id && r.owner !== B.id);
          if (!X) continue;
          const C = X.owner ? fac(X.owner) : null;
          if (C) C.regions = C.regions.filter((id) => id !== X.id);
          X.owner = B.id; X.burn = 0; B.regions.push(X.id);
          w.factions = w.factions.filter((x) => x.regions.length);
          const alive = new Set(w.factions.map((x) => x.id));
          for (const x of w.factions) for (const k of ['at_war_with', 'allies']) if (Array.isArray(x[k])) x[k] = x[k].filter((id) => alive.has(id) && id !== A.id && id !== B.id);
          w.wars = (w.wars ?? []).filter((x) => alive.has(x.a) && alive.has(x.b) && ![x.a, x.b].includes(A.id) && ![x.a, x.b].includes(B.id));
          w.wars.push({ a: A.id, b: B.id, started: w.year, decades: 0, taken_from_a: 0, taken_from_b: 0, allies_a: [], allies_b: [], coalition: false, years_fought: 0, declared: 0, thread: `war:${A.id}:${B.id}:${w.year}` });
          A.at_war_with = [B.id]; B.at_war_with = [A.id];
          B.wealth = 1; A.wealth = Math.max(A.wealth, 300); A.cohesion = 1; B.cohesion = 0.5;   // B loses land (cohesion under war_target_cohesion 0.6) before it can fall apart
          game.loadWorld(w);
          return `${A.id} at war with ${B.id}, which also holds ${X.id}`;
        }
      return '';
    });
  }
  let extraSleeps = 0;
  while (extraSleeps < 10 && (!captured.plague || !captured.region_settled || !captured.region_taken || !captured.spell_expired || (await spellsLeft()).length) && !(await page.evaluate(() => game.world().tags.includes('dragon_slain')))) {
    extraSleeps += 1;
    if (!captured.region_settled) await openLand();
    await closeWake();
    await sleepThrough(100);
    kindsAll.push(...await playMontage());
  }
  const s7 = await state();
  const ownerAfter = {};
  for (const r of regions) ownerAfter[r.id] = await page.evaluate((id) => game.tileOwnerRing(id), r.id);
  const recoloured = regions.filter((r) => ownerBefore[r.id] !== ownerAfter[r.id]).map((r) => r.id);
  const eraAfter = await page.evaluate((id) => game.settlementCount(id), 'hill_hold');
  const have = (k) => kindsAll.includes(k);
  const more = extraSleeps ? ` (after ${extraSleeps} more centur${extraSleeps === 1 ? 'y' : 'ies'})` : '';
  // the ticker names events in the narrator's words and only shows the ones worth a line (importance, or the dragon's stake)
  const told = (k, re) => captured[k] && (captured[k].worthy ? re.test(captured[k].ticker) : true);
  check('c.war', have('war_started') && captured.war_started && captured.war_started.arrows > 0 && told('war_started', /declared war/), `arrows ${captured.war_started?.arrows}, army groups ${captured.war_started?.armies}, ticker "${captured.war_started?.ticker}"`);
  check('c.conquest', have('region_taken') && captured.region_taken && recoloured.length > 0, `"${captured.region_taken?.ticker}"; rings recoloured since age 2: ${recoloured.join(', ')}${conquestLab ? ` (lab: ${conquestLab})` : ''}`);
  check('c.collapse', have('collapse') && told('collapse', /fell/), captured.collapse?.ticker);   // narrator.json: "fell apart into" / "fell, and a new house"
  check('c.era', have('era_advance') && told('era_advance', /entered the \w+ age/) && eraAfter !== eraBefore, `"${captured.era_advance?.ticker}"; hill_hold settlement meshes ${eraBefore} -> ${eraAfter}`);
  check('c.plague', have('plague') && told('plague', /plague/i), `${captured.plague?.ticker}${more}`);
  check('c.settled', have('region_settled') && told('region_settled', /settled/), `${captured.region_settled?.ticker}${captured.region_settled?.worthy ? '' : ' (below the ticker threshold: map only)'}${settleLab ? ` (lab: ${settleLab} left open)` : ''}${more}`);
  const left = await spellsLeft();
  check('e.expiry', have('spell_expired') && told('spell_expired', /faded/) && !left.length, `"${captured.spell_expired?.ticker}"; curse and ward still running: ${left.join(', ') || 'none'}${more}`);
  await beginRaid();
  // orcs: since the 2026-10-15 data the orcs start as a remnant beyond the valley, and this seed's 600 years may not raise
  // a horde; then a lab world gives them the ground they rise on (the north scar burned to wasteland next to weak courts)
  let orcLab = '';
  if (!have('orcs_spawned')) {
    orcLab = ' (lab: a wasteland on the north scar, weak courts around it)';
    await page.evaluate(() => {
      const w = game.world();
      const r = w.regions.find((x) => x.id === 'north_scar');
      const f = w.factions.find((x) => x.id === r.owner);
      if (f) f.regions = f.regions.filter((x) => x !== r.id);
      r.owner = ''; r.burn = 0.9; r.tags = ['wasteland'];
      w.factions = w.factions.filter((x) => x.regions.length);
      const alive = new Set(w.factions.map((x) => x.id));
      for (const x of w.factions) for (const k of ['at_war_with', 'allies']) if (Array.isArray(x[k])) x[k] = x[k].filter((id) => alive.has(id));
      for (const x of w.factions) x.cohesion = Math.min(x.cohesion, 0.2);
      game.loadWorld(w);
    });
    for (let extra = 0; extra < 8 && !captured.orcs_spawned; extra++) {   // item 06 moved the history on: give the horde a few centuries
      await sleepThrough(100);
      await playMontage();
      await beginRaid();
    }
  }
  const orcs = (await state()).factions.filter((f) => f.race === 'orc');
  check('c.orcs', captured.orcs_spawned && orcs.length > 0 && told('orcs_spawned', /orc/i), `"${captured.orcs_spawned?.ticker}"${orcLab}; orc factions now ${orcs.map((o) => o.id).join(', ')}`);
  const orcRegions = await page.evaluate(() => game.world().regions.filter((r) => r.owner && game.world().factions.find((f) => f.id === r.owner).dominant_race === 'orc').map((r) => r.id));
  let inciteResult = null, inciteWhy = '';
  for (const rid of orcRegions) {
    await fly(rid);
    const a = (await actions()).find((x) => x.id === 'spell-incite');
    inciteWhy = a.why;
    if (a.ok) { inciteResult = await act('cast', { region: rid, spell: 'incite', against: await page.$eval('#incite-against', (e) => e.value) }); await page.waitForTimeout(300); await shot('a-incite'); break; }
  }
  check('a.incite', orcRegions.length > 0 && (inciteResult ? inciteResult.cast && inciteResult.war_with : /no neighbour left|no orc horde/.test(inciteWhy)), inciteResult ? `horde at ${orcRegions[0]} marches on ${inciteResult.war_with}` : `incite unavailable: ${inciteWhy}`);

  // ---- a. raise undead on a burned ownerless region, or the reason; sponsor remnant or the reason --------------------
  const openRegions = await page.evaluate(() => game.world().regions.filter((r) => !r.owner && !r.lair).map((r) => r.id));
  let undead = null, undeadWhy = '';
  for (const rid of openRegions) { await fly(rid); const a = (await actions()).find((x) => x.id === 'spell-raise_undead'); undeadWhy = a.why; if (a.ok) { undead = await act('cast', { region: rid, spell: 'raise_undead' }); await page.waitForTimeout(300); await shot('a-undead'); break; } }
  check('a.undead', undead ? undead.cast && undead.spawned_faction : /magic|site must|ownerless/.test(undeadWhy) || openRegions.length === 0, undead ? `${undead.spawned_faction} rises, war on ${undead.war_with}` : `unavailable: ${undeadWhy || 'no open region'}`);
  const campState = await state();
  let sponsorWhy = (await actions()).find((x) => x.id === 'sponsor')?.why ?? '';
  check('a.sponsor', campState.camps > 0 ? true : /no remnant/.test(sponsorWhy), `${campState.camps} camps; ${sponsorWhy || 'camp here'}`);

  // ---- h. growth: take gold everywhere, sleep 1000: the model is bigger and threat higher ----------------------------
  await page.evaluate((seed) => game.newGame(seed), SEED);
  const base = await state();
  for (const f of base.factions) { if (!f.capital) continue; await fly(f.capital); await act('take', { region: f.capital, category: 'gold', magnitude: 0.6 }); }
  await fly('hill_hold'); await act('take', { region: 'hill_hold', category: 'artifacts', magnitude: 0.6 });
  await fly('deep_wood'); await act('take', { region: 'deep_wood', category: 'books', magnitude: 0.6 });
  const forecastRich = await sleepThrough(1000);
  await finishSleep();
  await page.waitForTimeout(300);
  await shot('h-grown');
  const grown = await state();
  check('h.growth', grown.dragon.size > base.dragon.size && grown.dragon.threat > base.dragon.threat && grown.dragonScale > base.dragonScale, `size ${base.dragon.size.toFixed(3)} -> ${grown.dragon.size.toFixed(3)}, threat ${base.dragon.threat.toFixed(0)} -> ${grown.dragon.threat.toFixed(0)}, model scale ${base.dragonScale.toFixed(2)} -> ${grown.dragonScale.toFixed(2)}`);
  await beginRaid();
  await page.evaluate(() => game.land());
  await page.evaluate(() => game.setLairTab('sleep'));
  await page.$eval('#sleep-years', (el) => { el.value = '100'; el.dispatchEvent(new Event('change')); });
  const forecastSharp = await page.$eval('#forecast', (e) => e.textContent);
  const noise = (t) => Number((/noise ±(\d+)%/.exec(t) || [])[1]);
  check('g.forecast2', noise(forecast1) === 50 && noise(forecastSharp) < 20 && /knowledge/.test(forecastSharp), `noise ±${noise(forecast1)}% at knowledge 0 -> ±${noise(forecastSharp)}% at knowledge ${grown.dragon.knowledge.toFixed(2)}`);
  await shot('g-forecast');

  // ---- f. hunger: empty hoard, sleep 1000 -> wakes early by hunger, HUD shows it, size dropped ---------------------
  await page.evaluate((seed) => game.newGame(seed), SEED);
  await page.evaluate(() => game.flyTo('lair'));
  await sleepThrough(1000);
  await finishSleep();
  await page.waitForTimeout(300);
  await shot('f-hunger');
  const hungry = await state();
  const hudHunger = await page.$eval('#h-hunger', (e) => Number(e.textContent));
  const hudWarn = await page.$eval('#h-hunger-stat', (e) => e.classList.contains('warn'));
  const wakeHungry = await page.$eval('#wake', (e) => e.innerText);
  // since S1 the egg's gold bed grows the dragon, so the starving shows as atrophy, not as a small model
  check('f.hunger', hungry.ages[0].wake_reason === 'hunger' && hungry.year < 1000 && hudHunger >= 0.9 && hudWarn && /woke: hunger/.test(wakeHungry) && /smaller/.test(wakeHungry) && /Atrophy −[\d.]+ size/.test(wakeHungry), `woke ${hungry.ages[0].wake_reason} at year ${hungry.year}, HUD hunger ${hudHunger}, ${(wakeHungry.match(/Atrophy −[\d.]+ size/) ?? ['no atrophy'])[0]}, model scale ${hungry.dragonScale.toFixed(2)}`);
  await beginRaid();

  // ---- i. keyboard and mouse ----------------------------------------------------------------------------------------
  await page.evaluate(() => game.setPos(0, 6));
  const p0 = (await state()).pos;
  await holdKey('d');
  const p1 = (await state()).pos;
  await holdKey('ArrowUp');
  const p2 = (await state()).pos;
  await holdKey('a');
  const p3 = (await state()).pos;
  // camera yaw 45°: D moves +x −z on the map, W/ArrowUp moves −x −z, A moves −x +z
  check('i.keys', p1[0] > p0[0] + 0.5 && p1[1] < p0[1] - 0.5 && p2[0] < p1[0] - 0.5 && p2[1] < p1[1] - 0.5 && p3[0] < p2[0] - 0.5, `D ${p0.map((v) => v.toFixed(1))} -> ${p1.map((v) => v.toFixed(1))}, ArrowUp -> ${p2.map((v) => v.toFixed(1))}, A -> ${p3.map((v) => v.toFixed(1))}`);
  const target = await page.evaluate(() => game.screenPos('east_marches'));
  await page.mouse.move(target[0], target[1]);
  await page.mouse.click(target[0], target[1]);
  await page.waitForFunction(() => game.state().region === 'east_marches', null, { timeout: 30000 }).catch(() => {});
  const clicked = (await state()).region;
  await shot('i-click');
  check('i.click', clicked === 'east_marches', `click on east_marches at (${target.map((v) => v.toFixed(0))}) -> dragon over ${clicked}`);
  await page.keyboard.press('c');
  const chronOpen = !(await page.$eval('#chron', (e) => e.classList.contains('hidden')));
  await page.keyboard.press('Escape');
  const chronClosed = await page.$eval('#chron', (e) => e.classList.contains('hidden'));
  await page.evaluate(() => document.querySelector('#btn-system').click());
  const sysOpen = !(await page.$eval('#system', (e) => e.classList.contains('hidden')));
  await page.keyboard.press('Escape');
  const sysClosed = sysOpen && await page.$eval('#system', (e) => e.classList.contains('hidden'));
  await page.evaluate(() => game.flyTo('lair'));
  await page.keyboard.press('l');
  await frames(1);
  const landedByKey = (await state()).landed;
  await page.keyboard.press('Escape');
  const tookOff = !(await state()).landed;
  check('i.esc', chronOpen && chronClosed && sysClosed && landedByKey && tookOff, `C opens chronicle, Esc closes it, Esc closes game sheet, L lands, Esc takes off`);
  await page.evaluate(() => game.land());
  await sleepThrough(100);
  await page.evaluate(() => game.setSpeed(20));
  await page.keyboard.press(' ');
  const paused = (await page.evaluate(() => game.montageInfo())).paused;
  const y1 = (await page.evaluate(() => game.montageInfo())).year;
  await page.waitForTimeout(300);
  const y2 = (await page.evaluate(() => game.montageInfo())).year;
  await shot('i-paused');
  await page.keyboard.press(' ');
  await page.keyboard.press('Escape');
  await page.waitForFunction(() => !game.state().montage);
  check('i.space', paused && y1 === y2 && !(await state()).montage, `Space paused at year ${y1.toFixed(0)}, Esc skipped`);
  await beginRaid();
  await page.evaluate(() => document.querySelector('#btn-chron').focus());
  await page.keyboard.press('Escape');
  const chronStillClosed = await page.$eval('#chron', (e) => e.classList.contains('hidden'));
  const focusFree = await page.evaluate(() => document.activeElement === document.body);
  await page.evaluate(() => game.flyTo('east_vale'));
  await page.focus('#take-m');
  const beforeW = (await state()).pos;
  await page.keyboard.down('w'); await frames(3); await page.keyboard.up('w');
  const afterW = (await state()).pos;
  await page.keyboard.press('Escape');
  const sliderBlurred = await page.evaluate(() => document.activeElement === document.body);
  check('i.focus', chronStillClosed && focusFree && sliderBlurred && Math.abs(afterW[0] - beforeW[0]) < 0.01, `Esc blurs a focused button (${focusFree}); W inside a slider does not fly (moved ${Math.abs(afterW[0] - beforeW[0]).toFixed(2)}); Esc blurs the slider (${sliderBlurred})`);

  // ---- j. 1024 px wide --------------------------------------------------------------------------------------------
  await page.setViewportSize({ width: 1024, height: 700 });
  await page.waitForTimeout(200);
  await fly('east_vale');
  await page.waitForTimeout(100);
  const layout = await page.evaluate(() => {
    const hud = document.querySelector('#hud'), act = document.querySelector('#actions'), legend = document.querySelector('#legend');
    const hr = hud.getBoundingClientRect(), ar = act.getBoundingClientRect(), lr = legend.getBoundingClientRect();
    return { hudScroll: hud.scrollWidth <= innerWidth + 1, hudH: hr.height, actionsRight: ar.right <= innerWidth, actionsLeft: ar.left >= lr.right - 1 || ar.top > lr.top, overflowX: document.documentElement.scrollWidth <= innerWidth, actW: ar.width };
  });
  await shot('j-1024');
  const panelOverflow = await page.$eval('#actions', (e) => e.scrollWidth - e.clientWidth);
  check('j.1024', layout.hudScroll && layout.actionsRight && layout.overflowX && layout.hudH < 80 && panelOverflow <= 0, `hud fits: ${layout.hudScroll}, panel inside viewport: ${layout.actionsRight}, panel inner overflow ${panelOverflow}px, no horizontal scroll: ${layout.overflowX}, hud height ${layout.hudH.toFixed(0)}`);
  await page.evaluate(() => game.land());
  await shot('j-1024-lair');
  await page.evaluate(() => game.takeOff());
  await page.setViewportSize({ width: 1280, height: 800 });
  await frames(2);

  // ---- l. verb lab: a modified world (load_world) so the verbs the seed never reaches run once each ------------------
  // size 2.5 opens terraform, faith_pool 3 opens miracle and bless, a faithful court keeps an altar, north_scar is freed
  // for the orc remnant camp, east_marches is a burned ownerless site for the undead, magic 4 pays for it
  await page.evaluate(() => {
    const w = game.world();
    w.dragon.size = 2.5; w.dragon.faith_pool = 3; w.dragon.magic = 4; w.dragon.knowledge = 2; w.dragon.tags = w.dragon.tags.filter((t) => t !== 'stiff');
    const free = (id) => { const r = w.regions.find((x) => x.id === id); const f = w.factions.find((x) => x.id === r.owner); if (f) f.regions = f.regions.filter((x) => x !== id); r.owner = ''; return r; };
    free('north_scar');
    const em = free('east_marches'); em.burn = 0.7;
    for (const f of w.factions) if (f.regions.length === 0) w.factions = w.factions.filter((x) => x !== f);
    if (!w.remnants.some((r) => r.race === 'orc')) w.remnants.push({ race: 'orc', strength: 0.3, origin: 'north_scar', max_era: 0 });
    else w.remnants.find((r) => r.race === 'orc').strength = 0.3;
    const pious = w.factions.find((f) => f.id === 'dorun') || w.factions[0];
    pious.memory.push({ year: w.year, type: 'gift', magnitude: 1, weight: 1, axis: 'faith', tier: 'oral', strength: 0, faith: 0.8, act: -1, tags: ['gift'] });
    pious.faith = 0.8;
    game.loadWorld(w);
  });
  await frames(2);
  const lab = await state();
  const pious = lab.factions.find((f) => f.faith > 0.5);
  await fly(pious.capital);
  const altarRow = (await actions()).find((a) => a.id === 'altar');
  const fp0 = lab.dragon.faith_pool;
  const rOff = await act('accept_offering', { region: pious.capital });
  const fp1 = (await state()).dragon.faith_pool;
  await shot('l-altar');
  check('a.altar', altarRow.ok && rOff?.ok && fp1 > fp0 && rOff.events.some((e) => e.kind === 'act_accept_offering'), `${pious.id} faith ${pious.faith.toFixed(2)} keeps an altar at ${pious.capital}; offering accepted, faith pool ${fp0.toFixed(2)} -> ${fp1.toFixed(2)}`);
  const rDestroy = await act('destroy_altar', { region: pious.capital });
  const faithAfterDestroy = (await state()).factions.find((f) => f.id === pious.id).faith;
  const altarGone = (await actions()).find((a) => a.id === 'altar');
  check('a.destroy_altar', rDestroy?.ok && faithAfterDestroy < pious.faith && rDestroy.events.some((e) => e.kind === 'act_destroy_altar'), `${pious.id} faith ${pious.faith.toFixed(2)} -> ${faithAfterDestroy.toFixed(2)} (−0.3 sacrilege, altar ${altarGone.ok ? 'rebuilt while faith > 0.5' : 'gone: ' + altarGone.why})`);
  const faithBeforeMiracle = faithAfterDestroy;
  const rMir = await act('miracle', { region: pious.capital, terror: false });
  const faithAfterMiracle = (await state()).factions.find((f) => f.id === pious.id).faith;
  const fp2 = (await state()).dragon.faith_pool;
  const rTerror = await act('miracle', { region: pious.capital, terror: true });
  const dreadAfterTerror = (await state()).factions.find((f) => f.id === pious.id).dread;
  await shot('l-miracle');
  check('a.miracle', rMir?.ok && rTerror?.ok && fp2 < fp1 && dreadAfterTerror > 0.3, `protect: faith ${faithBeforeMiracle.toFixed(2)} -> ${faithAfterMiracle.toFixed(2)} (capped at 1), faith pool ${fp1.toFixed(2)} -> ${fp2.toFixed(2)}; terror: dread ${dreadAfterTerror.toFixed(2)}`);
  const blessRow = (await actions()).find((a) => a.id === 'spell-bless');
  const rBless = await act('cast', { region: pious.capital, spell: 'bless' });
  const blessLabel = (await (async () => { await frames(2); return page.evaluate(() => game.labels()); })()).find((l) => l.id === pious.capital);
  await shot('l-bless');
  check('a.bless', blessRow.ok && rBless?.cast && (await state()).modifiers.includes('dragon.bless>' + pious.capital) && /blessed/.test(blessLabel.text), `bless on ${pious.capital}: label "${blessLabel.text}"`);
  await fly('north_scar');
  const campRow = (await actions()).find((a) => a.id === 'sponsor');
  const campsNow = (await state()).camps;
  await shot('l-camp');
  const rSp = await act('sponsor_remnant', { region: 'north_scar' });
  await frames(2);
  const scarOwner = await page.evaluate(() => game.world().regions.find((r) => r.id === 'north_scar').owner);
  await shot('l-sponsored');
  check('a.sponsor_run', campsNow > 0 && campRow.ok && rSp?.ok && rSp.events.some((e) => e.kind === 'race_returned') && scarOwner, `camp rendered (${campsNow}); the orcs return to north_scar as ${scarOwner}`);
  await fly('east_marches');
  const undeadRow = (await actions()).find((a) => a.id === 'spell-raise_undead');
  const rUn = await act('cast', { region: 'east_marches', spell: 'raise_undead' });
  await frames(2);
  const undeadLabel = (await page.evaluate(() => game.labels())).find((l) => l.id === 'east_marches');
  await shot('l-undead');
  check('a.undead_run', undeadRow.ok && rUn?.cast && rUn.spawned_faction && rUn.war_with && (await state()).arrows > 0 && new RegExp(rUn.spawned_faction, 'i').test(undeadLabel.text), `${rUn?.spawned_faction} rises at east_marches, war on ${rUn?.war_with}; label "${undeadLabel?.text}"; arrows ${(await state()).arrows}`);
  await fly('south_fen');
  const tfRow = (await actions()).find((a) => a.id === 'terraform');
  const size0 = (await state()).dragon.size;
  const rLake = await act('terraform', { region: 'south_fen', kind: 'lake' });
  await frames(2);
  const lakeLabel = (await page.evaluate(() => game.labels())).find((l) => l.id === 'south_fen');
  const lakeDecor = await page.evaluate(() => game.settlementCount('south_fen'));
  await shot('l-lake');
  const tfAgain = (await actions()).find((a) => a.id === 'terraform');
  const rWaste = await act('terraform', { region: 'south_fen', kind: 'wasteland' });
  await frames(2);
  const wasteColour = await page.evaluate(() => game.tileColour('south_fen'));
  const wasteLabel = (await page.evaluate(() => game.labels())).find((l) => l.id === 'south_fen');
  await shot('l-wasteland');
  const size1 = (await state()).dragon.size;
  check('a.terraform', tfRow.ok && rLake?.done && rLake.tag === 'lake' && /lake/.test(lakeLabel.text) && tfAgain.ok && rWaste?.done && /wasteland/.test(wasteLabel.text) && wasteColour === '#6b6b66' && size1 < size0, `lake then wasteland on south_fen; size ${size0.toFixed(2)} -> ${size1.toFixed(2)}; tile colour ${wasteColour}; label "${wasteLabel.text}"`);
  const tfNone = (await actions()).find((a) => a.id === 'terraform');
  check('a.terraform_done', !tfNone.ok && /nothing left/.test(tfNone.why), tfNone.why);
  await page.evaluate(() => game.land());
  await page.evaluate(() => game.setLairTab('oaths'));
  await page.evaluate(() => game.setLairTab('hoard'));
  const hoardText = await page.$eval('#lair-body', (e) => e.innerText.replace(/\s+/g, ' '));
  await shot('l-hoard');
  check('l.hoard', /HOARD/.test(hoardText) && /GROWTH/.test(hoardText) && /HUNGER/.test(hoardText) && /threat/.test(hoardText) && /FACTIONS/.test(hoardText) && /REMNANTS/.test(hoardText), 'hoard tab shows hoard, growth, hunger, threat, factions, remnants');
  await page.evaluate(() => game.takeOff());
  await page.evaluate((seed) => game.newGame(seed), SEED);
  await frames(2);

  // ---- legibility: labels on every tile, legend present, HUD fields -------------------------------------------------
  await page.setViewportSize({ width: 1280, height: 800 });
  await page.waitForTimeout(200);
  await fly('east_vale');
  const labels = await page.evaluate(() => game.labels());
  const hudText = await page.$eval('#hud', (e) => e.innerText);
  const legendText = await page.$eval('#legend', (e) => e.innerText);
  await shot('k-legibility');
  // since S1 the growth chips take the pools' place in the HUD (the pools move into their tooltips)
  const hudFields = /Might/.test(hudText) ? ['stamina', 'health', 'hunger', 'Might', 'Flame', 'Arcana', 'Lore', 'Dominion'] : ['stamina', 'hunger', 'size', 'knowledge', 'magic', 'faith', 'threat'];
  check('k.labels', labels.length === 12 && labels.every((l) => l.shown && /vale|marches|hills|wood|fen|scar|lair/.test(l.text)) && hudFields.every((k) => hudText.includes(k)) && /human/.test(legendText) && /war arrow/.test(legendText) && /wasteland/.test(legendText), `${labels.length} labels, legend and HUD complete (${hudFields.join(', ')})`);

  // ---- v0.2: the rhythm of sleep, consequences in words, courts and people, envoys, the wake screen -----------------
  await page.evaluate((seed) => game.newGame(seed), SEED);
  await frames(2);
  // 5. phases: rest and moult buttons from the phase's windows; a 1-year rest advances the year by one
  await page.evaluate(() => game.land());
  await page.evaluate(() => game.setLairTab('sleep'));
  const ph = await page.evaluate(() => game.phase());
  const restText = await page.$eval('#sleep-rest', (e) => e.textContent);
  const moultText = await page.$eval('#sleep-moult', (e) => e.textContent);
  const rhythmText = await page.$eval('.rhythm-line', (e) => e.textContent);
  const hudPhase = await page.$eval('#h-phase-stat', (e) => e.textContent);
  await shot('v02-sleep-tab');
  const yRest0 = (await state()).year;
  await page.click('#sleep-rest');
  await page.evaluate(() => game.setSpeed(2000));
  await finishSleep();
  const yRest1 = (await state()).year;
  await shot('v02-rest');
  const restWake = await page.evaluate(() => game.wake());
  const yrs = (n) => `${n} year${n === 1 ? '' : 's'}`;
  const yearsWord = (n) => ({ 1: 'a year', 5: 'five years', 10: 'ten years', 20: 'twenty years', 100: 'a hundred years', 300: 'three hundred years', 1000: 'a thousand years', 3000: 'three thousand years' })[n] ?? `${n} years`;
  check('v02.rhythm', restText === `Rest (${yrs(ph.short)})` && moultText === `Moult (${yrs(ph.long)})` && yRest1 === yRest0 + ph.short && ph.short === 1 && restWake.open && new RegExp(ph.name).test(hudPhase) &&
    rhythmText.includes(`${yearsWord(ph.short)} of rest`) && rhythmText.includes(`moult of ${yearsWord(ph.long)}`),
    `${ph.name}: "${restText}" / "${moultText}"; year ${yRest0} -> ${yRest1}; HUD "${hudPhase.trim()}"; "${rhythmText.slice(0, 90)}..."`);
  await page.keyboard.press('Enter');
  const wakeClosedByEnter = !(await page.evaluate(() => game.wake())).open;
  await dismissGrowth();

  // 3. narration: a burn tells what happened and how each court took it
  await fly('east_vale');
  await act('burn', { region: 'east_vale', magnitude: 0.4 });
  await page.waitForTimeout(300);
  const narr = await page.evaluate(() => game.narration());
  const narrShown = await page.$eval('#narration', (e) => !e.classList.contains('hidden') && e.innerText);
  await shot('v02-narration');
  const narrMain = narr.find((l) => l.kind === 'main');
  const narrTarget = narr.find((l) => l.kind === 'react' && l.faction === 'aldmere');
  // a court's reaction is the sim's line when apply_acts sends `lines`, else the page's mood line
  check('v02.narration', narrMain && /burned/.test(narrMain.text) && narrTarget && (narrTarget.source === 'sim' || narrTarget.mood) && narrTarget.text.length > 20 && narrShown && narr.length <= 6,
    `"${narrMain?.text}" | ${narr.filter((l) => l.kind === 'react').map((l) => `${l.faction} (${l.source || l.mood}): ${l.text}`).join(' | ').slice(0, 300)}`);

  // 4. courts: at a capital the court card names the ruler and the heir; take the heir through the card. A court names an
  // heir only when the ruler's reign is nearing its end, so rest a year at a time (the hatchling's rhythm) until one has
  const courtWithHeir = () => page.evaluate(() => {
    const w = game.world();
    const has = (f, role) => w.people.some((p) => p.faction === f.id && p.role === role && !p.tags.includes('hostage'));
    return w.factions.find((f) => f.capital && has(f, 'ruler') && has(f, 'heir'))?.id ?? '';
  });
  let courtId = await courtWithHeir(), courtRests = 0;
  while (!courtId && courtRests < 100) {
    courtRests += 1;
    await page.evaluate(() => game.sleep(1));
    await finishSleep();
    await page.keyboard.press('Enter');
    await dismissGrowth();
    courtId = await courtWithHeir();
  }
  const people0 = await page.evaluate(() => game.people());
  const courtCap = (await state()).factions.find((f) => f.id === courtId)?.capital;
  const ruler = people0.find((p) => p.faction === courtId && p.role === 'ruler');
  const heir = people0.find((p) => p.faction === courtId && p.role === 'heir');
  if (courtCap) await fly(courtCap);
  await frames(1);
  const court = await page.evaluate(() => game.court());
  await shot('v02-court');
  check('v02.court', courtId && court.shown && court.faction === courtId && ruler && heir && court.text.includes(ruler.name) && court.text.includes(heir.name) && /reigned \d+ years?/.test(court.text) && /age \d+/.test(court.text) && /Take hostage/.test(court.text) && /Kill/.test(court.text) && /never forgotten/.test(court.text),
    `court of ${courtId || 'none'} after ${courtRests} one-year rests: ruler ${ruler?.name}, heir ${heir?.name}; ${court.people.length} people listed`);
  if (!heir) throw new Error('no court named an heir in 100 years of rests');
  await page.click(`[data-pverb="hostage"][data-pid="${heir.id}"]`);
  await frames(1);
  const held = (await page.evaluate(() => game.world().dragon.hostages));
  const narrHostage = (await page.evaluate(() => game.narration())).find((l) => l.kind === 'main' && /lair/.test(l.text));
  await page.evaluate(() => game.land());
  await page.evaluate(() => game.setLairTab('known'));
  const knownText = await page.$eval('#lair-body', (e) => e.innerText);
  await shot('v02-known');
  check('v02.hostage', held.includes(heir.id) && knownText.includes(heir.name) && /years in the lair/.test(knownText) && /Release/.test(knownText) && narrHostage, `${heir.name} (${heir.id}) in the lair: ${held.join(', ')}; "${narrHostage?.text}"`);

  // 1. the wake screen after a long sleep: headline, paragraphs, people, threads, opportunities; Last sleep reopens it
  await page.evaluate(() => game.setLairTab('sleep'));
  await page.$eval('#sleep-years', (el) => { el.value = '100'; el.dispatchEvent(new Event('change')); });
  await page.click('#sleep-go');
  await page.evaluate(() => game.setSpeed(40));
  await page.waitForTimeout(400);
  const lapse = await page.evaluate(() => game.montageInfo());
  await shot('v02-montage');
  await finishSleep();
  const wake = await page.evaluate(() => game.wake());
  const wakePage = await page.$eval('#wake', (e) => e.innerText);
  await shot('v02-wake');
  await page.keyboard.press('Escape');
  const closedByEsc = !(await page.evaluate(() => game.wake())).open;
  await dismissGrowth();
  await page.click('#btn-last');
  const reopened = (await page.evaluate(() => game.wake())).open;
  const rawIds = lapse.texts.filter((t) => /\b(human|dwarf|elf|orc|undead)\d+\b/.test(t));
  check('v02.wake', wake.open && wake.headline.length > 0 && wake.paragraphs.length >= 1 && wake.paragraphs[0].length > 20 && !/\{[^{}]*\}/.test(wakePage) && /people/i.test(wakePage) && /threads/i.test(wakePage) && /opportunities/i.test(wakePage) && closedByEsc && reopened && wakeClosedByEnter,
    `headline "${wake.headline.slice(0, 80)}"; ${wake.paragraphs.length} paragraphs; ${wake.opportunities.length} opportunities; Enter, Esc close it, Last sleep reopens it`);
  check('v02.ticker', rawIds.length === 0 && lapse.total > 0, `${lapse.total} events in the time-lapse told with names (${rawIds.length} with raw ids)${rawIds.length ? ': ' + rawIds[0] : ''}`);

  // 2. go: an opportunity closes the page, selects the court and flies the dragon there
  const goIndex = wake.opportunities.findIndex((o) => o.kind !== 'offer' && o.faction);
  const goFaction = wake.opportunities[goIndex]?.faction;
  const goRegion = await page.evaluate((f) => { const w = game.world(); const fac = w.factions.find((x) => x.id === f); const army = w.armies.find((a) => a.defender === f && fac.at_war_with.includes(a.attacker)); return army && fac.at_war_with.length ? army.region : fac.capital; }, goFaction);
  await page.click(`[data-go="${goIndex}"]`);
  await page.waitForFunction((r) => game.state().region === r, goRegion, { timeout: 30000 }).catch(() => {});
  const goState = await state();
  const goCourt = await page.evaluate(() => game.court());
  await shot('v02-go');
  check('v02.go', goIndex >= 0 && !(await page.evaluate(() => game.wake())).open && goState.region === goRegion && goCourt.faction === goFaction, `go on "${wake.opportunities[goIndex]?.text.slice(0, 70)}" -> dragon over ${goState.region}, court of ${goCourt.faction}`);

  // 4. envoys: each offer is a person with a stated reason; accept one (it becomes a task), eat another. Courts send
  // envoys when their situation calls for it, so sleep on a century at a time (at most ten) until two have come in person
  const envoysWaiting = () => page.evaluate(() => game.world().offers.filter((o) => o.envoy && game.people().some((p) => p.id === o.envoy)).length);
  for (let extra = 0; extra < 10 && (await envoysWaiting()) < 2; extra++) {
    await seeLair();
    await sleepThrough(100);
    await finishSleep();
    if ((await page.evaluate(() => game.wake())).open) await beginRaid();
  }
  await page.evaluate(() => game.land());
  await page.evaluate(() => game.setLairTab('envoys'));
  const offersV = await page.evaluate(() => game.world().offers.map((o) => ({ kind: o.kind, faction: o.from_faction, envoy: o.envoy, name: o.envoy_name, ask: o.ask })));
  const speech = await page.$$eval('.audience .speech', (e) => e.map((x) => x.textContent));
  const audienceText = await page.$eval('#lair-body', (e) => e.innerText);
  await shot('v02-envoys');
  let accepted = null;
  if (offersV.length >= 2) {
    const i = Math.max(0, offersV.findIndex((o) => o.ask));
    accepted = offersV[i];
    await page.click(`[data-accept="${i}"]`);
  }
  const tasks = await page.evaluate(() => game.tasks());
  const offersLeft = await page.evaluate(() => game.world().offers.map((o) => ({ envoy: o.envoy, name: o.envoy_name, faction: o.from_faction })));
  const eaten = offersLeft.find((o) => o.envoy);
  await page.evaluate(() => game.setLairTab('envoys'));
  const eatIndex = offersLeft.indexOf(eaten);
  const st0 = (await state()).stamina;
  await page.click(`[data-eat="${eatIndex}"]`);
  await frames(1);
  const afterEat = await page.evaluate((id) => ({ alive: game.people().some((p) => p.id === id), offers: game.world().offers.length, known: game.known().find((k) => k.id === id) }), eaten?.envoy);
  const narrEat = (await page.evaluate(() => game.narration())).find((l) => l.kind === 'main' && /You ate/.test(l.text));
  const st1 = (await state()).stamina;
  await shot('v02-eat');
  check('v02.envoys', offersV.length > 0 && speech.length === offersV.length && speech.every((t) => /^“.+”$/.test(t)) && !/\{[^{}]*\}/.test(audienceText) && /They ask/.test(audienceText) && /Eat the envoy/.test(audienceText),
    `${offersV.length} audiences: ${offersV.map((o, i) => `${o.name} of ${o.faction} (${o.kind}) ${speech[i]}`).join(' | ').slice(0, 220)}`);
  check('v02.eat', eaten && !afterEat.alive && afterEat.offers < offersLeft.length && afterEat.known?.tags.includes('dead') && narrEat && st0 - st1 === 10,
    `ate ${eaten?.name} of ${eaten?.faction}: dead ${!afterEat.alive}, offers ${offersLeft.length} -> ${afterEat.offers}, relationship ${afterEat.known?.relationship}, stamina ${st0} -> ${st1}; "${narrEat?.text}"`);
  await page.evaluate(() => game.takeOff());
  await frames(2);
  const tasksCard = await page.$eval('#tasks', (e) => !e.classList.contains('hidden') && e.innerText);
  if (accepted) {
    await shot('v02-tasks-late');
    check('v02.tasks_late', tasks.some((t) => t.faction === accepted.faction) && tasksCard && /(before|until) \d+/.test(tasksCard), `accepted ${accepted.kind} from ${accepted.faction}: ${tasks.map((t) => t.text).join(' | ')}`);
  }

  // the sim's coming fields, fed in by hand: `lines` on an apply_acts result and `importance` on events win when present;
  // a briefing with placeholders the wasm left open is filled or cleaned, a long thread is told as first and last line
  const synth = await page.evaluate(() => {
    game.narrateWith({ kind: 'burn', target: 'aldmere', region: 'east_vale' }, { ok: true, events: [], lines: [{ faction: '', text: 'The sky over the vale burned.' }, { faction: 'aldmere', text: 'Aldmere swore revenge{,why}.' }] });
    const ev = (kind, importance) => ({ kind, subject: 'nobody', object: 'nowhere', tags: [], magnitude: 2, year: 1, ...(importance === undefined ? {} : { importance }) });
    const imp = [game.tickerWorthy(ev('region_settled', 0.9)), game.tickerWorthy(ev('collapse', 0.05)), game.tickerWorthy(ev('collapse')), game.tickerWorthy(ev('region_settled'))];
    const b = game.briefingCleaned({ headline: { text: '{A_rose_role} rose in Aldmere{,why}.', event: 1 }, people: [], opportunities: [],
      while_you_slept: [{ from: 0, to: 99, lines: ['A.', 'B.', 'C.', 'D.'].map((text, i) => ({ text, thread: 'war:x:y:1', year: i, event: 10 + i })) }],
      threads: [{ kind: 'remnant', id: 'orc', text: 'A remnant of the {race_plural} waits near the north scar.' }] },
      [{ id: 1, kind: 'person_rose', subject: 'nobody', object: 'aldmere', tags: ['hero', 'slayer'], magnitude: 0, year: 1 }]);
    return { n: game.narration().slice(-2), imp, b };
  });
  const [synMain, synReact] = synth.n;
  const synLines = synth.b.while_you_slept[0].lines;
  check('v02.simfields', synMain.text === 'The sky over the vale burned.' && synReact.source === 'sim' && synReact.text === 'Aldmere swore revenge.' &&
    JSON.stringify(synth.imp) === '[true,false,true,false]' && !/[{}]/.test(synth.b.headline.text) && synLines.length === 1 && /^A\..*D\.$/.test(synLines[0].text) && synth.b.threads[0].text.includes('the orcs'),
    `lines -> "${synMain.text}" / "${synReact.text}"; importance 0.9 / 0.05 / en.json collapse / en.json settled -> ${synth.imp.join(' ')}; headline "${synth.b.headline.text}"; thread "${synLines[0].text}"; "${synth.b.threads[0].text}"`);

  await checksV04();
  await finish(browser, errors);
})().catch((e) => { console.error(e); process.exit(1); });

async function finish(browser, errors) {
  await browser.close();
  const failed = results.filter((r) => !r.ok);
  console.log('\nconsole errors:', errors.length ? errors : 'none');
  console.log(`${results.length - failed.length}/${results.length} checks passed${failed.length ? ': FAILED ' + failed.map((f) => f.id).join(', ') : ''}`);
  const ok = failed.length === 0 && errors.length === 0;
  console.log(ok ? 'GAME CHECK OK' : 'GAME CHECK FAILED');
  process.exit(ok ? 0 : 1);
}

// ---- v0.4: the new screens. S1 (growth, talents, defences, wounds, the slayer clock, death) and S2 (genesis, legends,
// region names) and S3 (hooks) run on the real wasm. Every screen is driven
// once and photographed (v04-*.png) ---------------------------------------------------------------------------------------
async function checksV04() {
  const v04 = () => page.evaluate(() => game.v04());
  const text = (sel) => page.$eval(sel, (e) => e.innerText.replace(/\s+/g, ' '));
  const shown = (sel) => page.$eval(sel, (e) => !e.classList.contains('hidden')).catch(() => false);
  // a lab world: `fn` edits the world view `w` (with `a` = arg) inside the page, then the page loads it
  const lab = (fn, arg = null) => page.evaluate(([src, a, helpers]) => { const w = game.world(); eval(helpers + '\n(' + src + ')();'); return game.loadWorld(w); }, [fn.toString(), arg, `const armBand = ${armBand.toString()};`]);
  const file = 'file://' + path.resolve(__dirname, 'hoard-game.html');
  // a court whose slayer band is forming or ready (a lab helper evaluated inside the page): its dread goes in as oral
  // dread plus the memory of a burn
  const armBand = (w, id, stage, dread = 0.8) => {
    const f = w.factions.find((x) => x.id === id);
    f.tags = [...new Set([...f.tags, 'slayers'])]; f.anti_dragon = Math.max(f.anti_dragon, 1.05); f.dread = { oral: dread, written: 0, monumental: 0 };
    f.memory.push({ year: w.year, type: 'burn', magnitude: 1, weight: 1, axis: 'dread', tier: 'oral', strength: dread, faith: 0, act: -1, tags: ['burn'] });
    w.timed_tags.push({ on: 'faction', target: id, tag: 'slayers_' + stage, from: w.year - 10, until: 1e9, event: 0, inherit: false });
  };

  // the real wasm (S1) on the classic map: growth, health, defences and the arms race show; hooks, legends and names do not
  await page.goto(file);
  await page.waitForSelector('body[data-ready="1"]', { timeout: 120000 });
  await page.evaluate((seed) => { try { localStorage.clear(); } catch (e) { /* ignore */ } game.newGame(seed); }, SEED);
  const plain = await v04();
  const catalogue = await page.evaluate(() => game.sim({ cmd: 'talents' }));
  check('v04.real', !plain.mock && plain.growth && plain.health?.max > 0 && plain.defences.length >= 8 && plain.arms.every((a) => a.next) && !plain.hooks.length && !plain.genesis && !plain.names.length && catalogue.talents?.length >= 40 && (await shown('#h-growth')) && (await shown('#h-health-stat')) && !(await shown('#hooks')),
    `health ${plain.health?.max}, ${plain.defences.length} defended regions, ${catalogue.talents?.length} talents in the catalogue, no hooks, no genesis, no names`);

  // 6. death by slaying: a band ready at a cowed court against a hoard that makes the odds certain; the panel says so first
  await lab(() => { armBand(w, 'vessary', 'ready', 0.9); w.dragon.hoard.gold = 1e8; });
  await page.evaluate(() => game.land());
  await page.evaluate(() => game.setLairTab('sleep'));
  const doomed = await text('#lair-body');
  await shot('v04-sleep-doomed');
  await page.click('#sleep-rest');
  await page.evaluate(() => game.setSpeed(2000));
  await finishSleep();
  await frames(2);
  const slainState = await page.evaluate(() => ({ v: game.v04(), ages: game.state().ages.map((a) => a.wake_reason) }));
  const tombSlain = await text('#death');
  await shot('v04-slain');
  check('v04.slain', /slayers ready: Vessary \(odds 100%\)/.test(doomed) && slainState.ages[0] === 'dragon_slain' && slainState.v.dead && slainState.v.deathOpen && /the slayers of Vessary came up the mountain while you slept/.test(tombSlain),
    `"${(doomed.match(/slayers ready[^·]*/) ?? [''])[0].trim()}"; woke ${slainState.ages[0]}; "${(tombSlain.match(/In the year[^.]*\./) ?? [''])[0]}"`);

  // 8. a fresh start and New world: reset with genesis, the legends page before the first flight, the sim's region names
  // everywhere (S2); the years stay absolute, the egg hatches at genesis.hatched_year
  await page.evaluate(() => { try { localStorage.clear(); } catch (e) { /* ignore */ } });
  await page.goto(file);
  await page.waitForSelector('body[data-ready="1"]', { timeout: 120000 });
  const fresh = await v04();
  await page.evaluate((seed) => game.newWorld(seed), SEED);
  await frames(2);
  const leg = await v04(), legText = await text('#wake'), legYear = (await state()).year;
  await shot('v04-legends');
  const legends = (legText.match(/years before/g) ?? []).length;
  check('v04.legends', fresh.wakeKind === 'legends' && fresh.genesis && leg.wakeKind === 'legends' && leg.genesis && legYear === leg.genesis.hatched_year && /legends of the valley/i.test(legText) && legends >= 3 && /Take wing/.test(legText) && !/\{[^{}]*\}/.test(legText),
    `fresh start ${fresh.genesis?.template}; seed ${SEED}: ${leg.genesis?.template}, hatched in ${leg.genesis?.hatched_year} after ${leg.genesis?.prehistory_years} years; ${legends} legends`);
  await page.keyboard.press('Enter');
  await fly('hill_hold');
  await frames(2);
  const names = (await v04()).names, labels = await page.evaluate(() => game.labels()), panel0 = await panelText();
  await shot('v04-names');
  check('v04.names', names.length >= 11 && labels.every((l) => names.some((n) => l.text.includes(n))) && names.some((n) => panel0.startsWith(n)), `${names.slice(0, 6).join(', ')}, ...`);

  // 7. hooks on the classic map of seed 39: a lab gives the court at Hill Hold a real "vein struck" hook (data/hooks.json
  // template `vein`), answered by the sim when the dragon takes there
  await page.goto(file);
  await page.waitForSelector('body[data-ready="1"]', { timeout: 120000 });
  await page.evaluate((seed) => { try { localStorage.clear(); } catch (e) { /* ignore */ } game.newGame(seed); }, SEED);
  await lab(() => {
    const owner = w.regions.find((r) => r.id === 'hill_hold').owner;
    w.hooks = [{ id: 'lab_vein', kind: 'vein', source: 'vein', title: 'A vein struck', text: 'The miners struck a vein under Hill Hold.', region: 'hill_hold', faction: owner,
      resolve: { acts: ['take'], choices: [] }, reward_text: 'artifacts, and Arcana', ignore_text: '', fight: false, offered: w.year, expires_year: w.year }];
  });
  await fly('hill_hold');
  await frames(2);
  const panelHooks = await panelText();

  // a row in the HUD list and a pin over the region; 4. the preview of a harmful act at a defended region
  const s0 = await v04(), hookCard = (await shown('#hooks')) && await text('#hooks');
  const flat = panelHooks.replace(/\s+/g, ' ');
  await shot('v04-map');
  check('v04.hooks_hud', s0.hooks.length >= 1 && s0.pins >= 1 && hookCard && hookCard.includes(s0.hooks[0].title) && s0.growth && s0.health, `${s0.hooks.map((h) => `${h.title} (${h.acts.join('/')} at ${h.region})`).join('; ')}; ${s0.pins} pins; the real S1 fields stay`);
  check('v04.preview', /If you act here levies [\d.]+: [\d.]+ wounds, leaving \d+ of \d+ health/i.test(flat) && /\d+ st · [\d.]+ wounds → \d+ left/.test(flat), (flat.match(/If you act here[^.]*?health/i) ?? [''])[0]);

  // a raid: answer the hook, take, burn; every harmful act costs what the panel showed (the sim's wounds)
  const hook = s0.hooks.find((h) => h.region === 'hill_hold' && h.acts.includes('take'));
  const rVein = await act('take', { region: 'hill_hold', category: 'artifacts', magnitude: 0.6 });
  await frames(1);
  const s1 = await v04(), hookCard1 = await text('#hooks');
  await shot('v04-hook-answered');
  check('v04.hook_answered', hook && rVein?.hooks_resolved?.some((x) => (x.id ?? x) === hook.id) && s1.answered.includes(hook.id) && /✓/.test(hookCard1), `"${hook?.title}" answered by a take at hill_hold: ${hookCard1.slice(0, 80)}`);
  check('v04.wounds', rVein?.wounds?.taken > 0 && s1.health.wounds === rVein.wounds.total && s1.hits.length === 1 && Math.abs(s1.hits[0].shown - rVein.wounds.taken) < 0.01 && await shown('#h-health-stat'),
    `levies at hill_hold: shown ${s1.hits[0]?.shown}, taken ${rVein?.wounds?.taken}; health ${s1.health.left.toFixed(0)}/${s1.health.max}`);
  await fly('east_vale');
  await act('take', { region: 'east_vale', category: 'gold', magnitude: 0.6 });
  await act('burn', { region: 'east_vale', magnitude: 0.5 });
  await frames(1);
  await shot('v04-hit');
  await fly('deep_wood');
  await act('take', { region: 'deep_wood', category: 'books', magnitude: 0.6 });

  // 5. the sleep panel: the bet for each length (healing, growth per axis from the forecast, the danger line)
  await page.evaluate(() => game.land());
  await page.evaluate(() => game.setLairTab('sleep'));
  const bet = await text('#lair-body');
  await shot('v04-sleep-bet');
  const betMoult = /Moult \(10 years\)(.*?)not ready/.exec(bet)?.[1] ?? '';
  const estimated = /grows (Might|Lore|Arcana|Flame) \+([\d.]+)/.exec(betMoult);
  check('v04.sleep_bet', /heals all [\d.]+ wounds/.test(bet) && /Might \+[\d.]+/.test(bet) && /Flame \+[\d.]+/.test(bet) && /danger: /.test(bet) && estimated, (bet.match(/heals[^·]*·[^·]*·[^·]*/) ?? [''])[0].slice(0, 160));

  // 1. a moult: the wake page leads with the hooks, then the chronicle, the arms race, what you grew (the sim's
  // growth_gained, which the estimate must have matched); closing it opens the growth screen: two big cards, keys 1 and 2
  await page.click('#sleep-moult');
  await page.evaluate(() => game.setSpeed(2000));
  await finishSleep();
  await frames(2);
  const w1 = await text('#wake'), firstSection = await page.$eval('#wake .page section', (e) => e.className);
  const grew = new RegExp(`${estimated?.[1]} \\+([\\d.]+)`).exec(w1);
  await shot('v04-wake');
  check('v04.wake', firstSection === 'hooks' && /this age/i.test(w1) && /You answered A vein struck/.test(w1) && /arms race/i.test(w1) && /You grew in your sleep/.test(w1) && /ready to choose/.test(w1) && grew && Math.abs(Number(grew[1]) - Number(estimated[2])) <= 0.15,
    `first section "${firstSection}"; ${(w1.match(/You grew in your sleep[^.]*\./) ?? [''])[0]}; estimated ${estimated?.[1]} +${estimated?.[2]}`);
  await page.keyboard.press('Enter');
  await frames(2);
  const g1 = await v04();
  const cards = await page.$$eval('#growth .gcard', (e) => e.map((c) => ({ name: c.querySelector('.tn').textContent, size: parseFloat(getComputedStyle(c.querySelector('.tn')).fontSize), see: c.querySelector('.see')?.textContent ?? '' })));
  await shot('v04-growth');
  check('v04.growth', g1.growthOpen && cards.length === 2 && cards.every((c) => c.size >= 24) && g1.picks.length >= 1, `${g1.picks.length} picks; cards ${cards.map((c) => `${c.name} (${c.see})`).join(' | ')}`);
  const look0 = JSON.stringify(g1.look), talents0 = g1.talents.length;
  await page.keyboard.press('1');
  await page.waitForTimeout(250);
  await shot('v04-flourish');
  await page.waitForTimeout(1300);
  const g2 = await v04();
  check('v04.pick', g2.talents.length === talents0 + 1 && g2.talents.includes(g1.picks[0].options[0]) && JSON.stringify(g2.look) !== look0 && Object.keys(g2.effects).length > 0, `key 1 took ${g2.talents.join(', ')}; look ${look0} -> ${JSON.stringify(g2.look)}; effects ${JSON.stringify(g2.effects)}`);
  await page.keyboard.press('Escape');
  await frames(2);
  const g3 = await v04(), badge = (await shown('#h-picks')) && await text('#h-picks');
  await shot('v04-hud');
  check('v04.badge', !g3.growthOpen && (g3.picks.filter((p) => p.open).length ? /talents? ready/.test(badge) : !badge), `picks waiting: ${g3.picks.filter((p) => p.open).length}; badge "${badge || 'hidden'}"`);
  await page.evaluate(() => game.land());
  await page.evaluate(() => game.setLairTab('growth'));
  const lairGrowth = await text('#lair-body');
  await shot('v04-lair-growth');
  check('v04.lair_growth', /Growth/.test(lairGrowth) && /Might/.test(lairGrowth) && /Lore/.test(lairGrowth) && /Thick Hide|Deep Lungs/.test(lairGrowth) && /dearer/.test(lairGrowth), lairGrowth.slice(0, 120));
  await page.evaluate(() => game.takeOff());

  // 2. every look key on the dragon, in two sets (plates and fire have one value each); a rank adds its step
  const lookA = ['thick_hide', 'great_wings', 'laden_flight', 'old_bones', 'earth_shaker', 'hotter_breath', 'spell_sense', 'glamour', 'ward_craft', 'keen_eyes', 'riddle_master', 'gilded_throne', 'dragon_king'];
  await lab(() => { w.dragon.growth = { might: 60, flame: 30, arcana: 30, lore: 30, dominion: 30 }; w.dragon.talents = a; w.dragon.wounds = 0; w.dragon.hoard.gold = 40; }, lookA);
  await fly('lair');
  await page.evaluate(() => game.openGrowth());
  await frames(3);
  const la = await v04();
  await shot('v04-look-a');
  const keysA = ['plates', 'fire', 'wings', 'spikes', 'horns', 'runes', 'crest', 'eyes', 'veins', 'shadow', 'aura', 'hoard_glitter', 'banners', 'scale'].filter((k) => la.look[k] !== undefined);
  check('v04.look_a', keysA.length === 14 && la.look.plates === 'dark' && la.look.fire === 'gold' && la.scale > 1, `look ${JSON.stringify(la.look)}; model scale ${la.scale.toFixed(2)}`);
  await page.evaluate(() => game.closeGrowth());
  await lab(() => { w.dragon.growth = { might: 30, flame: 60, arcana: 60, lore: 30, dominion: 0 }; w.dragon.talents = a; }, ['iron_scales', 'tower_breaker', 'arcane_hide', 'sleepers_ward', 'chronicler']);
  await page.evaluate(() => game.openGrowth());
  await frames(3);
  const lb = await v04();
  await shot('v04-look-b');
  await page.evaluate(() => game.closeGrowth());
  await fly('west_vale');
  await act('burn', { region: 'west_vale', magnitude: 0.5 });
  await frames(1);
  const fire = await v04();
  await shot('v04-fire');
  check('v04.look_b', lb.look.plates === 'metal' && lb.look.fire === 'blue' && lb.look.runes === 2 && lb.look.aura >= 2 && lb.look.beam > 0 && fire.beamColour === '#5ab6ff', `look ${JSON.stringify(lb.look)}; beam ${fire.beamColour}`);

  // 3. defences in a lab: bolt-throwers, cannons, wards, a slayer band ready; rings everywhere with the sight talent
  await lab(() => {
    const f = (id) => w.factions.find((x) => x.id === id);
    w.dragon.growth = { might: 10, flame: 0, arcana: 20, lore: 0, dominion: 0 }; w.dragon.talents = ['dragon_sight']; w.dragon.wounds = 0;
    Object.assign(f('aldmere'), { era: 2, anti_dragon: 0.25, dread: { oral: 0, written: 0, monumental: 0 } });
    Object.assign(f('dorun'), { era: 4, anti_dragon: 4.2, dread: { oral: 0, written: 0, monumental: 0 } });   // cannons: era 2 and anti-dragon 4 (data/defences.json)
    Object.assign(f('ilvareth'), { era: 2, dread: { oral: 0, written: 0, monumental: 0 } });
    f('vessary').era = 1;
    armBand(w, 'vessary', 'ready');
  });
  await page.evaluate(() => game.view(0.62, 0, 2));
  await frames(3);
  const d0 = await v04();
  await shot('v04-defences');
  const kinds = new Set(d0.defences.flatMap((r) => r.list));
  check('v04.defences', ['levies', 'bolt_throwers', 'cannons', 'wards', 'slayer_band'].every((k) => kinds.has(k)) && d0.defenceGroups === d0.defences.length && d0.effects.sight === 1,
    d0.defences.map((r) => `${r.id}: ${r.list.join('+')} (${r.wound} per act${r.passive ? `, ${r.passive}/s` : ''})`).join('; '));
  await lab(() => { w.dragon.talents = []; });
  await page.evaluate(() => game.view(1));
  const hp = (await v04()).health;
  const dwelt = await page.evaluate(() => game.dwell('east_vale', 3));
  await frames(1);
  const d1 = await v04(), warn = (await page.$('#range-warn.show')) && await text('#range-warn'), court = await page.evaluate(() => game.court());
  await shot('v04-damage');
  check('v04.passive', dwelt.wounds > hp.wounds && /bolt-throwers of Aldmere/.test(warn) && /wounds a second/.test(warn) && d1.hits.some((h) => h.act === 'passive'),
    `3 s inside the ring at east_vale: wounds ${hp.wounds.toFixed(1)} -> ${dwelt.wounds.toFixed(1)}; "${warn?.slice(0, 90)}"`);
  check('v04.arms', /arms race: \d+% of the way to a slayer order/.test(court.text), (court.text.match(/arms race[^\n]*/) ?? [''])[0]);
  // the slayer clock: ready on the sleep tab with its odds; with Premonition the attempt wakes you instead
  await lab(() => { w.dragon.talents = ['premonition']; });
  await page.evaluate(() => game.land());
  await page.evaluate(() => game.setLairTab('sleep'));
  const sleepReady = await text('#lair-body');
  await shot('v04-slayers-ready');
  check('v04.slayers_ready', /Vessary's slayer band is ready/.test(sleepReady) && /slayers ready: Vessary \(odds \d+%\)/.test(sleepReady) && /wake premonition/.test(sleepReady), (sleepReady.match(/Vessary's slayer band[^.]*\./) ?? [''])[0]);
  await page.click('#sleep-rest');
  await page.evaluate(() => game.setSpeed(2000));
  await finishSleep();
  const w2 = await text('#wake'), p2 = await v04();
  await shot('v04-premonition');
  check('v04.premonition', /woke: premonition/.test(w2) && /wait at the foot of your mountain/.test(w2) && /arms race/i.test(w2) && p2.door.includes('vessary') && p2.arms.find((a) => a.id === 'vessary')?.stage === 'ready', (w2.match(/woke: premonition.{0,140}/) ?? [''])[0]);
  await page.keyboard.press('Escape');
  if ((await v04()).growthOpen) await page.keyboard.press('Escape');

  // 4 and 6. a lethal act is marked and needs a second press; death is final: who, when, the worst hits, the save gone.
  // Deep Wood lies outside every ring (with 1 health left a ring would kill before the second press, as it should)
  await lab(() => { w.dragon.wounds = w.dragon.health_max - 1; });
  await fly('deep_wood');
  await frames(1);
  const lethalPanel = (await panelText()).replace(/\s+/g, ' ');
  await shot('v04-lethal');
  const first = await act('burn', { region: 'deep_wood', magnitude: 0.25 });
  const alive = await v04();
  const second = await act('burn', { region: 'deep_wood', magnitude: 0.25 });
  await frames(2);
  const dead = await v04(), tomb = await text('#death'), saved = await page.evaluate(() => { try { return localStorage.getItem('hoard-game-save-v1'); } catch (e) { return 'n/a'; } });
  await shot('v04-death');
  check('v04.lethal', /lethal/.test(lethalPanel) && /you would die/.test(lethalPanel) && first === null && !alive.dead && second?.slain === true, `first press refused (${first === null}), second slays (${second?.slain})`);
  check('v04.death', dead.dead && dead.deathOpen && /The line of the dragon ends/i.test(tomb) && /of Ilvareth brought you down/.test(tomb) && /The worst of it/i.test(tomb) && (tomb.match(/The panel showed|The rings showed/g) ?? []).length === 3 && /New world/.test(tomb) && saved === null,
    (tomb.match(/In the year[^.]*\./) ?? [''])[0]);
  await page.click('#death-new');
  await frames(2);
  const reborn = await v04();
  check('v04.new_world', !reborn.dead && !reborn.deathOpen && reborn.wakeKind === 'legends' && reborn.genesis && reborn.health.wounds === 0, `after death: a new world (${reborn.genesis?.template}, hatched in ${reborn.genesis?.hatched_year}), legends open`);
  await page.keyboard.press('Enter');
}
