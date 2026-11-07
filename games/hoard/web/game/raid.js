// The panel reports facts, the core's raid_diff makes the acts (docs/raid-design.md section 3). Every panel action is a
// sortie of one: begin_sortie, the facts below, end_sortie. The result has the shape apply_acts had.
function sortie(build) {
  let open = sim({ cmd: 'begin_sortie', sortie: 1 });
  if (!open.ok && open.reason === 'sortie.already_open') {   // left open by a page that died between the two calls
    sim({ cmd: 'end_sortie', report: { sortie: 0 } });
    open = sim({ cmd: 'begin_sortie', sortie: 1 });
  }
  if (!open.ok) return open;
  return sim({ cmd: 'end_sortie', report: { sortie: 1, shell: 'tactical', settlements: [], armies: [], ...build(open) } });
}

const settlementIn = (snap, region) => snap.settlements.find((s) => s.region === region);

// m is the share of the settlement the burn destroys; the crowd the tactical page does not have is estimated by the toll factor
function burnFacts(snap, region, m) {
  const s = settlementIn(snap, region);
  if (!s) return {};
  return { settlements: [{ region, destroyed: { house: m * s.value }, killed: { civilians: m * s.population * snap.civilian_toll_factor } }] };
}

function takeFacts(snap, region, court, category, m) {
  const held = snap.courts.find((c) => c.id === court)?.holds[category] ?? 0;
  return { settlements: [{ region, collected: { [category]: m * held } }] };
}

const killFacts = (region, attacker, defender, share) => ({ armies: [{ attacker, defender, region, killed_share: share }] });
const defendFacts = (region, attacker, defender) => ({ armies: [{ attacker, defender, region, repelled: true }] });
const roarFacts = (region) => ({ settlements: [{ region, presence: { roared: true } }] });
const hostageFacts = (region, person = '') => ({ settlements: [{ region, taken: [person] }] });
const speechFacts = (acts) => ({ speech: acts });
const woundFacts = (wounds) => ({ wounds });
