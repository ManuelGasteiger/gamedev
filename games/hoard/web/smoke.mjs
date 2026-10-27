// node web/smoke.mjs  — loads web/ages.wasm, runs a short scenario, and checks both golden hashes against native.
import { readFileSync, readdirSync, writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

const root = join(dirname(fileURLToPath(import.meta.url)), '..');
const data = Object.fromEntries(['constants', 'races', 'reactions', 'regions', 'factions', 'shocks', 'names', 'offers']
  .map(k => [k, readFileSync(join(root, 'data', k + '.json'), 'utf8')]));
for (const k of ['talents', 'defences']) data[k] = readFileSync(join(root, 'data', k + '.json'), 'utf8');   // v0.4 (S1)
data.hooks = readFileSync(join(root, 'data/hooks.json'), 'utf8');   // v0.4 hooks (S3)
data.text = readFileSync(join(root, 'data/text/en/narrator.json'), 'utf8');
data.importance = readFileSync(join(root, 'data/importance.json'), 'utf8');
data.text_bundles = Object.fromEntries(readdirSync(join(root, 'data/text'), { withFileTypes: true }).filter((d) => d.isDirectory()).map((d) => {
  const b = { lang: d.name };
  for (const f of readdirSync(join(root, 'data/text', d.name)).filter((f) => f.endsWith('.json')).sort()) b[f.slice(0, -5)] = JSON.parse(readFileSync(join(root, 'data/text', d.name, f), 'utf8'));
  return [d.name, JSON.stringify(b)];
}));
// the event cards, joined in file-name order into one JSON array as data_io.cpp does
data.events = '[' + readdirSync(join(root, 'data/events')).filter((f) => f.endsWith('.json')).sort()
  .map((f) => readFileSync(join(root, 'data/events', f), 'utf8')).join(',\n') + ']';
data.genesis = readFileSync(join(root, 'data/genesis.json'), 'utf8');   // v0.4 world (S2)
// item 14: the pattern bank joined as data_io.cpp does, and the dry-run base rates
data.patterns = '[' + readdirSync(join(root, 'data/patterns')).filter((f) => f.endsWith('.json') && f !== 'base_rates.json').sort()
  .map((f) => readFileSync(join(root, 'data/patterns', f), 'utf8')).join(',\n') + ']';
data.base_rates = readFileSync(join(root, 'data/patterns/base_rates.json'), 'utf8');
const golden = readFileSync(join(root, 'tests/golden/ages_v0.hash'), 'utf8').trim();
const goldenRaze = readFileSync(join(root, 'tests/golden/ages_v0_1_raze.hash'), 'utf8').trim();

function wasiShim(getMemory) {
  const dec = new TextDecoder();
  const u32 = (ptr) => new DataView(getMemory().buffer).getUint32(ptr, true);
  const setU32 = (ptr, v) => new DataView(getMemory().buffer).setUint32(ptr, v, true);
  return {
    fd_write(fd, iovs, iovsLen, nwritten) {
      let out = '', total = 0;
      for (let i = 0; i < iovsLen; i++) {
        const ptr = u32(iovs + i * 8), len = u32(iovs + i * 8 + 4);
        out += dec.decode(new Uint8Array(getMemory().buffer, ptr, len));
        total += len;
      }
      setU32(nwritten, total);
      if (out) console.log('[wasm fd' + fd + ']', out.trimEnd());
      return 0;
    },
    fd_close: () => 0,
    fd_seek: () => 70,
    fd_fdstat_get: () => 8,
    proc_exit(code) { throw new Error('wasm proc_exit ' + code); },
    environ_sizes_get(c, s) { setU32(c, 0); setU32(s, 0); return 0; },
    environ_get: () => 0,
    args_sizes_get(c, s) { setU32(c, 0); setU32(s, 0); return 0; },
    args_get: () => 0,
    clock_time_get(id, prec, out) { new DataView(getMemory().buffer).setBigUint64(out, BigInt(Date.now()) * 1000000n, true); return 0; },
    random_get(ptr, len) { crypto.getRandomValues(new Uint8Array(getMemory().buffer, ptr, len)); return 0; },
  };
}

async function loadSim(bytes) {
  let instance;
  const shim = wasiShim(() => instance.exports.memory);
  const wasi = new Proxy(shim, { get: (t, k) => t[k] ?? ((...a) => { console.warn('wasi ' + k + ' unimplemented', a); return 52; }) });
  ({ instance } = await WebAssembly.instantiate(bytes, { wasi_snapshot_preview1: wasi }));
  instance.exports._initialize();
  const enc = new TextEncoder(), dec = new TextDecoder();
  return function cmd(obj) {
    const text = enc.encode(JSON.stringify(obj) + '\0');
    const ptr = instance.exports.ages_alloc(text.length);
    new Uint8Array(instance.exports.memory.buffer).set(text, ptr);
    const out = instance.exports.ages_cmd(ptr);
    instance.exports.ages_free(ptr);
    const mem = new Uint8Array(instance.exports.memory.buffer);
    let end = out;
    while (mem[end] !== 0) end++;
    return JSON.parse(dec.decode(mem.subarray(out, end)));
  };
}

const cmd = await loadSim(readFileSync(join(root, 'web/ages.wasm')));

console.log('load', cmd({ cmd: 'load', data, seed: 7 }));
const burn = { type: 'burn', target: 'aldmere', region: 'east_vale', magnitude: 0.4, witnesses: ['vessary', 'dorun', 'ilvareth'] };
const applied = cmd({ cmd: 'apply_acts', acts: [burn] });
console.log('apply_acts', JSON.stringify(applied.changed));
console.log('apply_acts lines', JSON.stringify(applied.lines));
if (!Array.isArray(applied.lines) || !applied.lines.length || !applied.lines.every((l) => l.faction && l.text)) { console.error('apply_acts lines missing'); process.exit(1); }
for (const years of [300, 1000]) {
  const f = cmd({ cmd: 'forecast', years });
  const r = cmd({ cmd: 'sleep', years });
  console.log(`forecast ${years}: ${f.wake_reason} hunger ~${f.hunger.toFixed(2)}; sleep ${years}: woke ${r.wake_reason} after ${r.years_slept} y, ${r.chronicle.length} events, ${r.offers.length} offers`);
}
console.log('hunger', cmd({ cmd: 'world' }).dragon.hunger.toFixed(2), 'remnants', JSON.stringify(cmd({ cmd: 'world' }).remnants));
console.log('hash', cmd({ cmd: 'hash' }).hash);
const world = cmd({ cmd: 'world' });
console.log('factions', world.factions.map(f => `${f.id}(${f.race} ${f.era_name} pow ${f.power.toFixed(1)})`).join(', '));
console.log('phase', JSON.stringify(world.phase), 'people', cmd({ cmd: 'people' }).people.length);
const brief = cmd({ cmd: 'briefing' });
console.log('briefing:', brief.headline.text, '|', (brief.while_you_slept[0]?.lines ?? []).length, 'lines in the first group,', brief.opportunities.length, 'opportunities');
const short = cmd({ cmd: 'sleep', years: 3 });
console.log(`sleep 3: woke ${short.wake_reason} after ${short.years_slept} y, ${short.chronicle.length} events`);
if (short.years_slept !== 3) { console.error('sub-decade sleep failed'); process.exit(1); }

// golden: seed 42, fixtures.h sample log, 2000 years in sleeps of 100 (run_scenario)
cmd({ cmd: 'reset', seed: 42 });
const sampleLog = [
  burn,
  { type: 'take', target: 'vessary', magnitude: 0.3, payload: { gold: 1.0, royals: 1.0 } },
  { type: 'show', target: 'dorun', magnitude: 0.2 },
  { type: 'give', target: 'ilvareth', magnitude: 0.2 },
];
cmd({ cmd: 'apply_acts', acts: sampleLog });
let remaining = 2000;
while (remaining > 0) {
  const r = cmd({ cmd: 'sleep', years: Math.min(100, remaining) });
  remaining -= Math.max(r.years_slept, 10);
  if (r.wake_reason === 'dragon_slain') break;
}
const hash = cmd({ cmd: 'hash' }).hash;
console.log(`golden idle: wasm ${hash} native ${golden} ${hash === golden ? 'MATCH' : 'MISMATCH'}`);

// golden v0.1: seed 42, raze_all (burn 0.5 largest region + take 0.3 gold from each faction) every sleep of 100, 2000 years
cmd({ cmd: 'reset', seed: 42 });
function largestRegion(world, f) {   // same tie-break as policy.cpp: first in id order with the strictly greatest land
  let best = '', bestLand = -1;
  for (const id of f.regions) { const r = world.regions.find((x) => x.id === id); if (r && r.land > bestLand) { bestLand = r.land; best = id; } }
  return best;
}
remaining = 2000;
while (remaining > 0) {
  const world = cmd({ cmd: 'world' });
  const all = world.factions.map((f) => f.id);
  const acts = [];
  for (const f of world.factions) acts.push({ type: 'burn', target: f.id, region: largestRegion(world, f), magnitude: 0.5, witnesses: all });
  for (const f of world.factions) acts.push({ type: 'take', target: f.id, magnitude: 0.3, payload: { gold: 1.0 }, witnesses: all });
  cmd({ cmd: 'apply_acts', acts });
  const r = cmd({ cmd: 'sleep', years: Math.min(100, remaining) });
  remaining -= Math.max(r.years_slept, 10);
  if (r.wake_reason === 'dragon_slain') break;
}
const hashRaze = cmd({ cmd: 'hash' }).hash;
console.log(`golden raze: wasm ${hashRaze} native ${goldenRaze} ${hashRaze === goldenRaze ? 'MATCH' : 'MISMATCH'}`);
let ok = hash === golden && hashRaze === goldenRaze;

// node web/smoke.mjs --render-batch tests/text/five.json: the text renderer in wasm; prints the hash `hoard-sim --render-batch` prints
const ri = process.argv.indexOf('--render-batch');
if (ri > 0) {
  const jobs = JSON.parse(readFileSync(process.argv[ri + 1], 'utf8'));
  const r = cmd({ cmd: 'render_batch', jobs });
  const bad = r.renders.filter((x, i) => (jobs[i].expect !== undefined && x.text !== jobs[i].expect) || x.problems.length);
  for (const x of bad) console.log('text mismatch:', x.text, x.problems.join('; '));
  console.log(`render batch: ${r.renders.length} strings, hash ${r.hash}, ${bad.length} wrong`);
  ok = ok && bad.length === 0;
  for (const lang of ['de', 'en']) {   // a refusal worded in the session's language
    cmd({ cmd: 'lang', lang });
    const d = cmd({ cmd: 'demand', target: 'nobody', pred: 'tribute' });
    console.log(`demand refused (${lang}): ${d.reason} -> ${d.reason_text}`);
    ok = ok && !!d.reason_text && d.reason_text !== d.reason;
  }
}

// node web/smoke.mjs --golden 1-60 --out hashes.txt: the state-hash golden, the same lines as `hoard-sim --golden`
const gi = process.argv.indexOf('--golden');
if (gi > 0) {
  const [from, to] = process.argv[gi + 1].split('-').map(Number);
  const oi = process.argv.indexOf('--out');
  const policy = readFileSync(join(root, 'data/policies/played.json'), 'utf8');
  const lines = [];
  for (let seed = from; seed <= (to || from); seed++) {
    const r = cmd({ cmd: 'golden', seed, policy });
    if (!r.ok) { console.error('golden failed', seed, r.error); ok = false; break; }
    lines.push(`${seed} ${r.hash}`);
  }
  if (oi > 0) writeFileSync(process.argv[oi + 1], lines.join('\n') + '\n'); else console.log(lines.join('\n'));
  console.log(`state-hash golden: ${lines.length} seeds`);
}
process.exit(ok ? 0 : 1);
