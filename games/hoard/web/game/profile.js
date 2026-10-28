// Item 04: what carries between runs (legends, ending atoms, seen, bestiary, glossary). Knowledge only; the sim reads it at
// genesis and updates it as the run teaches. Kept in localStorage, outside any save, so a death does not delete it.
const PROFILE_KEY = 'hoard-profile-v1';

function profileLoad() {
  try { return JSON.parse(localStorage.getItem(PROFILE_KEY) || '{}'); } catch (e) { return {}; }
}

function profileStore() {
  const res = sim({ cmd: 'profile' });
  if (!res?.ok) return;
  try { localStorage.setItem(PROFILE_KEY, JSON.stringify(res.profile)); } catch (e) { /* storage unavailable */ }
}
