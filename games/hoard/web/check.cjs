// node web/check.cjs — opens ages-sandbox.html headless: burn + apply, read the forecast line, sleep 100 (the briefing
// panel shows a headline and paragraphs with no unfilled {placeholder}, the people table lists the named rulers and heirs), then reset and sleep 1000
// on an empty hoard and assert hunger rose. Screenshots to web/sandbox-check.png.
const path = require('path');
const { chromium } = require('playwright');

(async () => {
  const browser = await chromium.launch();
  const page = await browser.newPage({ viewport: { width: 1700, height: 1000 } });
  const errors = [];
  page.on('console', (m) => { if (m.type() === 'error' || m.type() === 'warning') errors.push(m.type() + ': ' + m.text()); });
  page.on('pageerror', (e) => errors.push('pageerror: ' + e.message));

  await page.goto('file://' + path.resolve(__dirname, 'ages-sandbox.html'));
  await page.waitForSelector('body[data-ready="1"]', { timeout: 20000 });

  const regions = await page.$$eval('#map g.region', (g) => g.length);
  await page.click('#factions tbody tr[data-id="aldmere"]');
  const wealthBefore = Number(await page.$eval('#factions tbody tr[data-id="aldmere"] td:nth-child(6)', (td) => td.textContent));
  await page.selectOption('#act-type', 'burn');
  await page.$eval('#act-mag', (el) => { el.value = '0.4'; el.dispatchEvent(new Event('input')); });
  await page.click('#act-add');
  const queued = await page.$$eval('#queue li', (li) => li.length);
  await page.click('#act-apply');
  const wealthAfterAct = Number(await page.$eval('#factions tbody tr[data-id="aldmere"] td:nth-child(6)', (td) => td.textContent));
  await page.$eval('#sleep-years', (el) => { el.value = '300'; el.dispatchEvent(new Event('change')); });
  const forecast = await page.$eval('#forecast', (e) => e.textContent);
  await page.click('.sleep-btn[data-years="100"]');
  await page.waitForSelector('body[data-sleeps="1"]');
  const events = await page.$$eval('#chronicle .ev', (e) => e.length);
  const wake = await page.$eval('#wake', (e) => e.textContent);
  const hash = await page.$eval('#h-hash', (e) => e.textContent);
  const vassalHeader = await page.$eval('#factions thead', (t) => t.textContent.includes('vassal'));
  const briefingHeadline = await page.$eval('#briefing h3', (e) => e.textContent).catch(() => '');
  const briefingParas = await page.$$eval('#briefing .bgroup p', (p) => p.length);
  const briefingCols = await page.$eval('#briefing', (e) => /People/i.test(e.innerText) && /Threads/i.test(e.innerText) && /Opportunities/i.test(e.innerText));
  const briefingClean = await page.$eval('#briefing', (e) => !/\{[^{}]*\}/.test(e.innerText));
  const peopleRows = await page.$$eval('#people tbody tr', (r) => r.map((x) => x.children[3]?.textContent));
  await page.screenshot({ path: path.resolve(__dirname, 'sandbox-check.png'), fullPage: true });

  // empty hoard, 1000 years: hunger must rise
  await page.click('#reset');
  const hungerBefore = Number(await page.$eval('#hunger', (e) => e.textContent));
  await page.click('.sleep-btn[data-years="1000"]');
  await page.waitForSelector('body[data-sleeps="1"]');
  const hungerAfter = Number(await page.$eval('#hunger', (e) => e.textContent));
  const wakeHungry = await page.$eval('#wake', (e) => e.textContent);
  await browser.close();

  console.log({ regions, queued, wealthBefore, wealthAfterAct, forecast, events, wake, hash, vassalHeader, briefingHeadline, briefingParas, briefingCols, briefingClean, people: peopleRows.length, hungerBefore, hungerAfter, wakeHungry, consoleErrors: errors });
  const ok = regions === 12 && queued === 1 && wealthAfterAct < wealthBefore && /forecast 300 y/.test(forecast) && /hunger ~/.test(forecast) &&
    events > 0 && vassalHeader && briefingHeadline.length > 0 && briefingParas > 0 && briefingCols && briefingClean && peopleRows.includes('ruler') && peopleRows.includes('heir') &&
    hungerAfter > hungerBefore && errors.length === 0;
  console.log(ok ? 'PLAYWRIGHT OK' : 'PLAYWRIGHT FAILED');
  process.exit(ok ? 0 : 1);
})();
