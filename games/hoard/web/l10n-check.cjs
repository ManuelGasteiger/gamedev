// node web/l10n-check.cjs [--lang pseudo,de] [--size 1280x800] — loads web/hoard-game.html?lang=L headless, opens
// each screen (flight HUD, the lair tabs, game sheet, chronicle, sleep montage, wake page) and fails when a
// translated element (data-t) or any box around it is wider inside than out
// (scrollWidth > clientWidth), or, under pseudo, when a translated text lost its closing bracket. Writes one
// screenshot per screen and language to web/shots/l10n-<lang>-<screen>.png and renders the font test string.
const path = require('path');
const { chromium } = require('playwright');

const arg = (name, dflt) => { const i = process.argv.indexOf(name); return i > 0 ? process.argv[i + 1] : dflt; };
const LANGS = arg('--lang', 'pseudo,de').split(',');
const [W, H] = arg('--size', '1280x800').split('x').map(Number);
const FONT_TEST = 'Weiß ẞ „Äpfel“ »Öl«';
const page_url = (lang) => 'file://' + path.resolve(__dirname, 'hoard-game.html') + '?lang=' + lang;
const shots = path.resolve(__dirname, 'shots');
let failures = 0;

async function audit(page, lang, screen) {
  const bad = await page.evaluate(({ lang }) => {
    const out = [];
    const shown = (el) => el.offsetParent !== null || getComputedStyle(el).position === 'fixed';
    for (const el of document.querySelectorAll('[data-t]')) {
      if (!shown(el)) continue;
      for (let box = el; box && box !== document.body; box = box.parentElement) {
        if (box.clientWidth > 0 && box.scrollWidth > box.clientWidth + 1)
          out.push(`${box === el ? '' : (box.id ? '#' + box.id : box.tagName.toLowerCase() + '.' + box.className) + ' around '}${el.dataset.t}: ${box.scrollWidth} > ${box.clientWidth}`);
      }
      const text = el.textContent.trim();
      if (lang === 'pseudo' && !(text.startsWith('[') && text.endsWith(']'))) out.push(`${el.dataset.t}: bracket missing in "${text}"`);
    }
    // the HUD row: any box in it that clips its content (chips shrink when the labels grow)
    for (const box of document.querySelectorAll('#hud, #hud *')) {
      if (!shown(box) || box.clientWidth === 0) continue;
      const clip = getComputedStyle(box).overflowX;
      if ((clip === 'hidden' || clip === 'clip') && box.scrollWidth > box.clientWidth + 1)
        out.push(`#hud clips ${box.id ? '#' + box.id : box.tagName.toLowerCase() + '.' + box.className}: ${box.scrollWidth} > ${box.clientWidth}`);
    }
    return [...new Set(out)];
  }, { lang });
  await page.screenshot({ path: path.join(shots, `l10n-${lang}-${screen}.png`) });
  for (const b of bad) console.log(`FAIL ${lang} ${screen}: ${b}`);
  if (!bad.length) console.log(`PASS ${lang} ${screen}`);
  failures += bad.length;
}

async function run(browser, lang) {
  const page = await browser.newPage({ viewport: { width: W, height: H } });
  const errors = [];
  page.on('pageerror', (e) => errors.push(String(e)));
  await page.goto(page_url(lang));
  await page.waitForFunction(() => document.body.dataset.ready === '1', null, { timeout: 180000 });
  await page.evaluate(() => { for (const id of ['legends', 'wake']) document.getElementById(id)?.classList.add('hidden'); });
  await page.evaluate(() => { localStorage.clear(); });
  await audit(page, lang, 'flight');
  await page.evaluate(() => game.land());
  for (const tab of ['hoard', 'envoys', 'oaths', 'known', 'spells', 'sleep', 'chronicle']) {
    await page.evaluate((t) => game.setLairTab(t), tab);
    await audit(page, lang, 'lair-' + tab);
  }
  await page.evaluate(() => { game.takeOff(); document.getElementById('btn-system').click(); });
  await audit(page, lang, 'game');
  await page.evaluate(() => { document.getElementById('system-close').click(); document.getElementById('btn-chron').click(); });
  await audit(page, lang, 'chronicle');
  await page.evaluate(() => { document.getElementById('chron-close').click(); game.land(); game.sleep(10); });
  await page.waitForFunction(() => game.montageInfo().active, null, { timeout: 60000 });
  await page.evaluate(() => game.pauseMontage());
  await audit(page, lang, 'montage');
  await page.evaluate(() => game.finishMontage());
  await page.waitForFunction(() => !document.getElementById('wake').classList.contains('hidden'), null, { timeout: 60000 });
  await audit(page, lang, 'wake');
  const font = await page.evaluate((s) => {
    const el = document.createElement('div');
    el.textContent = s;
    el.style.cssText = 'position:fixed;left:20px;top:20px;font:28px/1.4 system-ui,sans-serif;background:#000;color:#fff;padding:8px;z-index:99';
    el.id = 'font-test';
    document.body.appendChild(el);
    return getComputedStyle(el).fontFamily;
  }, FONT_TEST);
  await page.locator('#font-test').screenshot({ path: path.join(shots, `l10n-${lang}-font.png`) });
  console.log(`font test "${FONT_TEST}" in ${font}: see web/shots/l10n-${lang}-font.png`);
  for (const e of errors) { console.log(`FAIL ${lang} page error: ${e}`); failures++; }
  await page.close();
}

(async () => {
  const browser = await chromium.launch({ args: ['--use-gl=swiftshader', '--enable-webgl', '--ignore-gpu-blocklist'] });
  try { for (const lang of LANGS) await run(browser, lang); } finally { await browser.close(); }
  console.log(failures ? `${failures} problems` : 'l10n check passed');
  process.exit(failures ? 1 : 0);
})();
