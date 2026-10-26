// The page's chrome in the player's language: ui.json of data/text/<lang>, rendered by the core. ?lang=de, ?lang=pseudo.
// Static labels carry data-t (text), data-t-title and data-t-placeholder; scripts call t(key, english).
const UI = { lang: 'en', strings: {} };

function t(key, english) { return UI.strings[key] ?? english ?? key; }

function applyUi(root = document) {
  for (const el of root.querySelectorAll('[data-t]')) el.textContent = t(el.dataset.t, el.textContent);
  for (const el of root.querySelectorAll('[data-t-title]')) el.title = t(el.dataset.tTitle, el.title);
  for (const el of root.querySelectorAll('[data-t-placeholder]')) el.placeholder = t(el.dataset.tPlaceholder, el.placeholder);
}

function loadUi() {
  const want = new URLSearchParams(location.search).get('lang') || 'en';
  UI.lang = sim({ cmd: 'lang', lang: want }).ok ? want : 'en';
  if (UI.lang !== want) sim({ cmd: 'lang', lang: 'en' });
  UI.strings = sim({ cmd: 'ui', lang: UI.lang }).strings ?? {};
  document.documentElement.lang = UI.lang === 'pseudo' ? 'en' : UI.lang;
  applyUi();
  // a longer language can wrap the HUD to two rows; the panels below it follow its height
  const hud = document.getElementById('hud');
  const follow = () => document.documentElement.style.setProperty('--hud-h', hud.offsetHeight + 'px');
  new ResizeObserver(follow).observe(hud);
  follow();
}

// a ui string with tokens, rendered by the core; ctx as text/batch.h takes it ({roles: {w: {numbers: {from: 1200}}}})
function tf(key, english, ctx) {
  const tmpl = UI.strings[key] ?? english;
  if (!tmpl.includes('{')) return tmpl;
  return sim({ cmd: 'render', lang: UI.lang, template: tmpl, ctx }).text ?? tmpl;
}
