// Names and sentences. The narrator's templates live in data/text/en.json (embedded as DATA.text); this file fills them
// the way sim/ages/briefing.cpp (Narrator::fill) does, so the ticker, the chronicle, the narration strip and the
// known-people ledger read like the briefing instead of ids.
const TEXT = JSON.parse(DATA.text);
const NAMES = { people: new Map(), dead: new Map() };
const ROLE_TAGS = new Set(['ruler', 'heir', 'hero', 'sage', 'priest', 'hostage_ruler']);
// when neither the event nor en.json weighs an event: the kinds worth a line in the time-lapse ticker
const TICKER_KINDS = new Set([
  'war_started', 'war_ended', 'coalition_formed', 'coalition_ended', 'region_taken', 'region_settled', 'region_abandoned',
  'collapse', 'faction_destroyed', 'era_advance', 'plague', 'volcanic_winter', 'vassal_sworn', 'vassal_freed', 'vassal_revolt',
  'vassal_absorbed', 'hegemony', 'hegemony_ended', 'slayers_founded', 'slay_attempt_failed', 'dragon_slain', 'orcs_spawned',
  'race_returned', 'remnant_formed', 'undead_raised', 'elves_withdrew', 'elves_departed', 'fading_begins', 'fading_ends',
  'wasteland_formed', 'wasteland_healed', 'succession_crisis', 'oath_breached_by_them', 'oath_breached_by_dragon', 'oath_expired',
  'spell_expired', 'raised_in_lair', 'dragon_raised_lineage', 'deceived_dragon', 'offer_fulfilled', 'dragon_moulted',
  'dragon_starving', 'dragon_atrophied', 'hostage_released',
]);
// the dragon's own threads: always worth a line, whatever their importance
const DRAGON_KINDS = new Set(['spell_cast', 'spell_expired', 'oath_sworn', 'oath_expired', 'oath_breached_by_them', 'oath_breached_by_dragon', 'hostage_taken',
  'hostage_released', 'raised_in_lair', 'dragon_raised_lineage', 'deceived_dragon', 'offer_fulfilled', 'slayers_founded', 'slay_attempt_failed', 'dragon_slain',
  'dragon_starving', 'dragon_moulted', 'dragon_atrophied']);
const DEATH_REASONS = ['old_age', 'deposed', 'realm_fell', 'slain_by_dragon', 'killed_by_dragon', 'warlord'];
// the sim scores events 0..1 (importance.cpp); the briefing keeps the ones at or above briefing.importance_min
const IMPORTANCE_MIN = CONSTANTS.briefing?.importance_min ?? CONSTANTS.importance_min ?? 0.25;

function indexNames(view) {
  NAMES.people = new Map((view?.people ?? []).map((p) => [p.id, p]));
  NAMES.dead = new Map((view?.dead_people ?? []).map((p) => [p.id, p]));
}
function personOf(id) { return NAMES.people.get(id) ?? NAMES.dead.get(id) ?? null; }
function capFirst(s) { return s ? s[0].toUpperCase() + s.slice(1) : ''; }
function isRegionId(id) { return !!REGION_LAYOUT[id] || !!model?.regions?.has(id); }
// a generated world names its regions (v0.4 world view `regions[i].name`); the classic map falls back to its ids
function regionName(id) { return model?.regions?.get(id)?.name || world?.regions?.find((r) => r.id === id)?.name || ''; }
function nameOf(id) {
  if (!id) return '';
  if (id === 'dragon') return 'you';
  const p = personOf(id);
  if (p) return p.name;
  if (isRegionId(id)) return regionName(id) || 'the ' + id.replace(/_/g, ' ');
  return factionTitle(id);
}
// a court the sim could not name (names.cpp falls back to race_serial once its syllables run out) reads as "Orc band 100"
function factionTitle(id) { const m = /^([a-z]+)_(\d+)$/.exec(id || ''); return m ? `${capFirst(m[1])} band ${m[2]}` : capFirst(id || ''); }
function regionTitle(id) { return regionName(id) || (id || '').replace(/_/g, ' ').replace(/\b\w/g, (c) => c.toUpperCase()); }
function regionLabel(id) { return regionName(id) || (id || '').replace(/_/g, ' '); }
function roleWord(role) { return TEXT.roles?.[role] ?? role ?? ''; }
function personLabel(p) { return p ? `${p.name}, ${roleWord(p.role)} of ${factionTitle(p.faction)}` : ''; }
function relationWord(r) {
  const k = r > 0.5 ? 'relation_friend' : r > 0.1 ? 'relation_warm' : r < -0.5 ? 'relation_enemy' : r < -0.1 ? 'relation_cold' : 'relation_neutral';
  return TEXT.misc?.[k] ?? k;
}
function relationClass(r) { return r > 0.1 ? 'warm' : r < -0.1 ? 'cold' : ''; }
function traitWords(t) {
  if (!t) return [];
  const pick = (v, hi, lo) => (v >= 0.7 ? hi : v <= 0.3 ? lo : null);
  return [pick(t.bold, 'bold', 'timid'), pick(t.pious, 'pious', 'worldly'), pick(t.greedy, 'greedy', 'open-handed'), pick(t.cunning, 'cunning', 'guileless')].filter(Boolean);
}
function yearsWord(n) {
  const words = { 1: 'a year', 5: 'five years', 10: 'ten years', 20: 'twenty years', 100: 'a hundred years', 300: 'three hundred years', 1000: 'a thousand years', 3000: 'three thousand years' };
  return words[n] ?? `${n} years`;
}
function eraWord(e) { return TEXT.misc?.era_names?.[e] ?? ERA_NAMES[e] ?? String(e); }
function yearsNum(n) { return `${n} year${n === 1 ? '' : 's'}`; }
function ordinal(n) { return ['zeroth', 'first', 'second', 'third', 'fourth', 'fifth', 'sixth', 'seventh', 'eighth', 'ninth', 'tenth'][n] ?? `${n}th`; }
// an id in words, as briefing.cpp words it: en.json's words, a talent's name, else the id in plain letters
function wordFor(k) { return TEXT.words?.[k] ?? (typeof talentById === 'function' ? talentById(k)?.name : null) ?? String(k ?? '').replace(/_/g, ' '); }
function signed(v, digits = 2) { return (v >= 0 ? '+' : '−') + Math.abs(v).toFixed(digits); }

// ---- the narrator's templates ---------------------------------------------------------------------------------------
// mirrors template_key in sim/ages/importance.cpp; a variant is used only when en.json has it
function templateKey(e) {
  const tags = e.tags ?? [];
  const has = (k) => !!TEXT.events?.[k];
  if (e.kind === 'collapse' && e.magnitude <= 1) return 'collapse_one';
  if (e.kind === 'remnant_formed' && tags.includes('off_map')) return 'remnant_formed_off_map';
  if (e.kind === 'person_rose' && tags.includes('heir')) return 'person_rose_heir';
  if (e.kind === 'person_died') for (const r of DEATH_REASONS) if (tags.includes(r) && has('person_died_' + r)) return 'person_died_' + r;
  const tk = e.text_key ?? '';
  if (tk.startsWith('ev.') && tk.length > 3 && tk.slice(3) !== e.kind && has(tk.slice(3))) return tk.slice(3);
  return e.kind;
}

// the person's place at court and, for a hero, what made them one (briefing.cpp person_role / rose_role)
function personRoleWord(e) {
  const tags = e.tags ?? [];
  for (const r of ['ruler', 'heir', 'sage', 'priest', 'hostage_ruler', 'hero']) if (tags.includes(r)) return roleWord(r);
  return roleWord('hero');
}
function roseRoleWord(e) {
  const tags = e.tags ?? [];
  if (!tags.includes('hero')) return personRoleWord(e);
  const why = tags.find((t) => t !== 'hero');
  return roleWord(why ?? 'hero');
}
function aAn(w) { return (/^[aeiou]/i.test(w) ? 'an ' : 'a ') + w; }

// tags arrive sorted; person events carry {reason, role} and the templates expect the role second
function templateTags(e) {
  const tags = (e.tags ?? []).filter((t) => !t.startsWith('successor:'));
  if ((e.kind === 'person_died' || e.kind === 'hostage_taken') && tags.length === 2) {
    const role = tags.find((t) => ROLE_TAGS.has(t));
    if (role) return [tags.find((t) => t !== role), role];
  }
  return tags;
}

function fillTemplate(s, e) {
  const tags = templateTags(e);
  const word = (k) => (k ? wordFor(k) : '');
  const raceWord = (r) => (r === 'dwarf' ? 'dwarves' : r === 'elf' ? 'elves' : r === 'undead' ? 'undead' : r + 's');
  const pct = (v) => Math.round(v * 100) + '%';
  const rep = (from, to) => { s = s.split(from).join(to); };
  const bare = nameOf(e.object).replace(/^the /, '');
  if (s.includes('{A_rose_role}')) rep('{A_rose_role}', capFirst(aAn(roseRoleWord(e))));
  if (s.includes('{role}')) rep('{role}', personRoleWord(e));
  rep('{S}', capFirst(nameOf(e.subject))); rep('{s}', nameOf(e.subject));
  rep('{O}', capFirst(nameOf(e.object))); rep('{o_bare}', bare); rep('{o}', nameOf(e.object));
  rep('{T0}', capFirst(nameOf(tags[0])));
  rep('{t0_word_the}', word(tags[0])); rep('{t0_word}', word(tags[0])); rep('{t1_word}', word(tags[1]));
  rep('{o_word}', word(e.object));
  rep('{t0_role}', roleWord(tags[0])); rep('{t1_role}', roleWord(tags[1]));
  rep('{t0_race}', raceWord(tags[0] ?? '')); rep('{s_race}', raceWord(e.subject)); rep('{race_plural}', raceWord(e.subject));
  rep('{m%}', pct(e.magnitude)); rep('{m0}', Number(e.magnitude).toFixed(0)); rep('{m}', Number(e.magnitude).toFixed(2));
  rep('{year}', String(e.year));
  const eras = TEXT.misc?.era_names ?? ERA_NAMES, phases = TEXT.misc?.phase_names ?? [];
  rep('{era}', eras[Math.max(0, Math.floor(e.magnitude))] ?? String(e.magnitude));
  rep('{phase_word}', phases[Math.max(0, e.magnitude - 1)] ?? String(e.magnitude));
  rep('{wake_word}', word(e.object));
  if (s.includes('{successors}')) {
    const ids = (e.tags ?? []).filter((t) => t.startsWith('successor:')).map((t) => nameOf(t.slice(10)));
    rep('{successors}', ids.length > 1 ? ids.slice(0, -1).join(', ') + (TEXT.misc?.and ?? ' and ') + ids[ids.length - 1] : ids.join(''));
  }
  rep('{led_by}', e.object ? `, led by ${nameOf(e.object)},` : '');
  for (const [k, v] of Object.entries(e.cause?.params ?? {})) { rep(`{p:${k}0}`, v.toFixed(0)); rep(`{p:${k}%}`, pct(v)); rep(`{p:${k}}`, v.toFixed(2)); }
  rep('{why}', ''); rep('{,why}', '');   // the page tells events without their cause clause
  return s;
}

// the briefing's sentence for an event (without the cause clause), or null when en.json has no template for it or the
// template uses a placeholder this page does not know yet (the caller falls back to its own wording)
function eventSentence(e) {
  const tt = TEXT.events?.[templateKey(e)] ?? TEXT.events?.[e.kind];
  if (!tt?.text) return null;
  const s = fillTemplate(tt.text, e);
  if (/\{[\w:%,]+\}/.test(s)) return null;
  return s + (e.count > 1 ? ` (x${e.count})` : '');
}

// ---- sim text that reaches the page with placeholders -----------------------------------------------------------------
// en.json and the wasm can be from different builds; a template the wasm does not know arrives half filled. The page fills
// what it can from the event (by id) and from `extra`, then drops what is left, so no "{...}" reaches the player.
function cleanSimText(text, e = null, extra = {}) {
  let s = String(text ?? '');
  if (!s.includes('{')) return fixArticles(s);
  for (const [k, v] of Object.entries(extra)) s = s.split(`{${k}}`).join(v ?? '');
  if (e && s.includes('{')) s = fillTemplate(s, e);
  s = s.replace(/\{[^{}]*\}/g, '').replace(/\s+([.,;:])/g, '$1').replace(/ {2,}/g, ' ').trim();
  return fixArticles(s);
}
function fixArticles(s) { return s.replace(/\b([Aa]) (?=[AEIOaeio](?!ne\b))/g, '$1n '); }
function racePlural(r) { return r === 'dwarf' ? 'dwarves' : r === 'elf' ? 'elves' : r === 'undead' ? 'undead' : r ? r + 's' : ''; }

// a briefing as the page shows it: every text cleaned, a thread of three or more lines in one century told as its first
// and last line (when the sim did not collapse it already), people deduplicated against the hostage threads
function cleanBriefing(b, events = []) {
  if (!b) return b;
  const byId = new Map(events.map((e) => [e.id, e]));
  const out = JSON.parse(JSON.stringify(b));
  if (out.headline) out.headline.text = cleanSimText(out.headline.text, byId.get(out.headline.event));
  for (const g of out.while_you_slept ?? []) {
    const lines = (g.lines ?? []).map((l) => ({ ...l, text: cleanSimText(l.text, byId.get(l.event)) }));
    const count = {};
    for (const l of lines) if (l.thread && l.events === undefined) count[l.thread] = (count[l.thread] ?? 0) + 1;
    const done = new Set();
    g.lines = [];
    lines.forEach((l, i) => {
      if (!l.thread || !(count[l.thread] >= 3)) { g.lines.push(l); return; }
      if (done.has(l.thread)) return;
      done.add(l.thread);
      const arc = lines.slice(i).filter((x) => x.thread === l.thread);
      const last = arc[arc.length - 1];
      const middle = arc.length === 3 ? ' ' + arc[1].text + ' ' : (TEXT.misc?.thread_more ?? ' Later: ').replace('{n}', String(arc.length - 2));
      g.lines.push({ ...l, text: l.text + middle + last.text, events: arc.length });
    });
  }
  for (const t of out.threads ?? []) t.text = cleanSimText(t.text, null, { race_plural: t.kind === 'remnant' ? racePlural(t.id) : '' });
  for (const p of out.people ?? []) p.text = cleanSimText(p.text, byId.get(p.event));
  for (const o of out.opportunities ?? []) o.text = cleanSimText(o.text);
  if (out.phase?.text) out.phase.text = cleanSimText(out.phase.text);
  return out;
}

function hasImportance(e) { return typeof e.importance === 'number' && e.importance >= 0; }

// who the dragon has a stake in: people it has met, courts it is sworn to, its hostages
function youContext(views) {
  const known = new Set(), oath = new Set();
  for (const v of views) {
    if (!v) continue;
    for (const k of v.dragon?.known_people ?? []) known.add(k.id);
    for (const h of v.dragon?.hostages ?? []) known.add(h);
    for (const o of v.oaths ?? []) if (o.status === 'active') oath.add(o.parties[1]);
  }
  return { known, oath };
}

function involvesYou(e, ctx) {
  if (!ctx) return false;
  if (ctx.known.has(e.subject) || ctx.known.has(e.object)) return true;
  if (e.kind === 'tribute_paid' || e.kind.startsWith('act_')) return false;
  return ctx.oath.has(e.subject) || ctx.oath.has(e.object);
}

// the sim's score when the event carries one, else the per-kind weight in en.json, else null (the kind list decides)
function importanceOf(e) {
  if (hasImportance(e)) return e.importance;
  const w = TEXT.events?.[templateKey(e)]?.importance ?? TEXT.events?.[e.kind]?.importance;
  return typeof w === 'number' ? w : null;
}

// the time-lapse shows an event as a line when it matters: important enough, or about someone the dragon knows
function tickerWorthy(e, ctx) {
  if (e.kind === 'sleep_begin' || e.kind === 'wake' || e.kind === 'tribute_paid' || e.kind.startsWith('act_')) return false;
  if (DRAGON_KINDS.has(e.kind) || involvesYou(e, ctx)) return true;
  const imp = importanceOf(e);
  return imp !== null ? imp >= IMPORTANCE_MIN : TICKER_KINDS.has(e.kind);
}
