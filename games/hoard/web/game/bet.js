// The bet of a sleep (docs/core-loop.md 2, rule 2): what a sleep of a given length heals and grows, and the danger the
// forecast sees. The forecast's growth_gained and healed win when it sends them; the S1 forecast does not, so the page
// estimates both the way hoard.cpp digests on the phase clock and wounds.cpp heals, over the years the forecast expects
// you to sleep, and labels them as an estimate (tribute and other gold that reaches the hoard during the sleep are not in it).
const DRAGON_C = CONSTANTS.dragon ?? {};

function sleepEstimate(planned, slept) {
  const ph = world.phase ?? {}, h = model.dragon.hoard ?? {}, hl = DRAGON_C.halflife ?? {}, pts = DRAGON_C.points ?? {};
  const W = Math.max(1, ph.long_sleep ?? 10);
  const moult = planned >= W && ph.ready_to_moult ? CONSTANTS.phases?.moult_digest_mult ?? 2 : 1;
  const eat = (stock, kind) => (stock > 0 && hl[kind] > 0 ? stock * (1 - Math.pow(0.5, (slept * moult) / (hl[kind] * W))) : 0);
  const dig = effect('digest_mult');
  const bedYears = (Math.min(slept, W) + (DRAGON_C.bed_overtime ?? 0) * Math.max(0, slept - W)) * moult;
  const growth = {
    might: (DRAGON_C.bed_rate ?? 0) * Math.sqrt(Math.max(0, h.gold ?? 0)) * (bedYears / W) * effect('bed_mult') + eat(h.food, 'food') * (pts.food ?? 0) * dig,
    flame: eat(h.embers, 'embers') * (pts.embers ?? 0) * dig,
    arcana: eat(h.artifacts, 'artifacts') * (pts.artifacts ?? 0) * dig,
    lore: (eat(h.books, 'books') * (pts.books ?? 0) * effect('lore_mult') + eat(h.royals, 'royals') * (pts.royals ?? 0) * effect('hostage_lore_mult')) * dig,
    dominion: eat(h.art, 'art') * (pts.art ?? 0) * dig,
  };
  return { growth, healed: healFor(slept) };
}

// a sleep heals `years / rest window` of your health (all of it from one rest window on)
function healFor(years) {
  if (!hasHealth()) return 0;
  const h = health();
  return Math.min(h.wounds, h.max * Math.min(1, years / Math.max(1, phaseInfo().short)) * effect('heal_mult'));
}

// {growth, healed, estimate} for a sleep of `years`, given the forecast for it
function sleepBet(years, f) {
  const slept = Math.max(1, Math.min(years, f?.years ?? years));
  if (f?.growth_gained && typeof f.healed === 'number') return { growth: f.growth_gained, healed: f.healed, estimate: false };
  return { ...sleepEstimate(years, slept), estimate: true };
}

function sleepBetHtml(years, f) {
  if (!f?.ok || (!hasHealth() && !hasGrowth())) return '';
  const bet = sleepBet(years, f), bits = [];
  if (hasHealth()) {
    const h = health();
    bits.push(h.wounds < 0.5 ? 'no wounds to heal' : bet.healed >= h.wounds - 0.5 ? `heals all ${numText(h.wounds)} wounds` : `heals ${numText(bet.healed)} of ${numText(h.wounds)} wounds`);
  }
  if (hasGrowth()) {
    const grown = Object.entries(bet.growth).filter(([, v]) => v >= 0.05).sort((a, b) => b[1] - a[1]);
    const chips = grown.map(([k, v]) => `<span class="gain" style="color:${axisColour(k)}">${axisMeta(k).name} +${pointsText(v)}</span>`).join(' ');
    const est = bet.estimate ? ' <span class="est" title="Estimated from the hoard you sleep on now. Tribute and the gold that reaches the hoard during the sleep are not counted.">(estimate)</span>' : '';
    bits.push(grown.length ? `grows ${chips}${est}` : 'nothing to grow on');
  }
  bits.push(dangerHtml(f));
  return `<div class="bet">${bits.join(' · ')}</div>`;
}

// the slaying line from the slayer clock: only a band that is ready when you lie down can try in this sleep (a forming
// one can be ready when you wake and try in the sleep after), so the line is exact where the forecast's is blurred
function dangerHtml(f) {
  const wounded = hasHealth() && health().wounds > 0.5 ? ', and the wounds you sleep with are in the odds' : '';
  const ready = world.factions.filter((x) => x.regions?.length && x.slayers?.stage === 'ready');
  if (ready.length) return `<span class="down"><b>slayers ready: ${ready.map((x) => `${escapeHtml(factionTitle(x.id))} (odds ${oddsText(x)})`).join(', ')}</b>${wounded}</span>`;
  const forming = world.factions.filter((x) => x.regions?.length && x.slayers?.stage === 'forming').map((x) => escapeHtml(factionTitle(x.id)));
  if (f.wake_reason === 'dragon_slain') return `<span class="down"><b>the forecast sees you slain in this sleep</b></span>`;
  return forming.length ? `danger: no band is ready to strike in this sleep; <span class="warn">${forming.join(', ')} ${forming.length === 1 ? 'is' : 'are'} gathering one</span>` : 'danger: no slayer band anywhere, no slaying in this sleep';
}

addStyle(`
  .sleep-choice .bet, #forecast .bet { margin-top: 6px; font: 14px/1.45 Georgia, 'Palatino Linotype', 'DejaVu Serif', serif; color: #efe7d4; }
  .bet .gain { font-weight: 600; white-space: nowrap; font-family: system-ui, sans-serif; font-size: 12.5px; }
  .bet .est { color: var(--dim); font: italic 12px system-ui, sans-serif; cursor: help; }
  .bet .warn { color: #e8b56a; }
`);
