// "Report a problem" (docs/research-playtesting-telemetry.md §4): the session's replay (.hrp), the current save and the
// build as one base64 blob, previewed and copied to the clipboard. Nothing is sent anywhere.
function reportBlob() {
  const r = sim({ cmd: 'report' });
  if (!r?.ok) return null;
  const json = JSON.stringify({ build: r.build, hrp: r.hrp, save: r.save, ui: { year: model?.year, seed: play?.seed } });
  const bytes = new TextEncoder().encode(json);
  let bin = '';
  for (let i = 0; i < bytes.length; i += 0x8000) bin += String.fromCharCode(...bytes.subarray(i, i + 0x8000));
  return { text: btoa(bin), inputs: r.hrp.split('\n').filter((l) => l.includes('"t":')).length, size: bytes.length, build: r.build };
}

function reportProblem() {
  const b = reportBlob();
  const box = $('report-text');
  if (!b) { box.value = 'The simulation could not build a report.'; box.classList.remove('hidden'); return; }
  $('report-note').textContent = `build ${b.build}, ${b.inputs} recorded inputs, ${Math.round(b.size / 1024)} KB: the replay of this session and the current save. Copied to the clipboard; paste it into a mail or message.`;
  box.value = b.text;
  box.classList.remove('hidden');
  box.select();
  try { navigator.clipboard?.writeText(b.text)?.catch(() => {}); } catch (e) { /* clipboard blocked: the text stays selected */ }
}
