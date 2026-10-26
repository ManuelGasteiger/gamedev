// Short synthesized sounds (WebAudio): a hit, a warning, the talent chime, an answered hook, the death toll. The context
// starts on the first click or key (browsers block audio before that); until then, and without audio, sfx() is silent.
const SFX = { ctx: null, gain: 0.16 };

function sfxWake() {
  try {
    if (!SFX.ctx) SFX.ctx = new (window.AudioContext || window.webkitAudioContext)();
    if (SFX.ctx.state === 'suspended') SFX.ctx.resume().catch(() => {});
  } catch (e) { SFX.ctx = null; }
}

function sfxTone(c, { freq, to = freq, at = 0, dur = 0.2, type = 'sine', vol = 1 }) {
  const t = c.currentTime + at, o = c.createOscillator(), g = c.createGain();
  o.type = type;
  o.frequency.setValueAtTime(freq, t);
  o.frequency.exponentialRampToValueAtTime(Math.max(20, to), t + dur);
  g.gain.setValueAtTime(0.0001, t);
  g.gain.exponentialRampToValueAtTime(SFX.gain * vol, t + 0.012);
  g.gain.exponentialRampToValueAtTime(0.0001, t + dur);
  o.connect(g).connect(c.destination);
  o.start(t);
  o.stop(t + dur + 0.03);
}

function sfxNoise(c, { at = 0, dur = 0.15, vol = 1, lowpass = 700 }) {
  const n = Math.floor(c.sampleRate * dur), buf = c.createBuffer(1, n, c.sampleRate), data = buf.getChannelData(0);
  for (let i = 0; i < n; i++) data[i] = (Math.random() * 2 - 1) * (1 - i / n);
  const src = c.createBufferSource(), f = c.createBiquadFilter(), g = c.createGain();
  src.buffer = buf;
  f.type = 'lowpass';
  f.frequency.value = lowpass;
  g.gain.value = SFX.gain * vol;
  src.connect(f).connect(g).connect(c.destination);
  src.start(c.currentTime + at);
}

const SFX_PLAY = {
  hit: (c) => { sfxNoise(c, { dur: 0.2, vol: 1.6, lowpass: 520 }); sfxTone(c, { freq: 150, to: 55, dur: 0.25, type: 'triangle', vol: 1.3 }); },
  warn: (c) => { sfxTone(c, { freq: 620, dur: 0.12, type: 'square', vol: 0.3 }); sfxTone(c, { freq: 415, at: 0.15, dur: 0.18, type: 'square', vol: 0.3 }); },
  talent: (c) => [523, 659, 784, 1047, 1319].forEach((f, i) => sfxTone(c, { freq: f, at: i * 0.07, dur: 0.6, type: 'triangle', vol: 0.65 })),
  hook: (c) => { sfxTone(c, { freq: 880, dur: 0.3, vol: 0.55 }); sfxTone(c, { freq: 1320, at: 0.09, dur: 0.45, vol: 0.45 }); },
  death: (c) => [196, 147, 110, 73].forEach((f, i) => sfxTone(c, { freq: f, at: i * 0.5, dur: 1.6, vol: 1.1 })),
};

function sfx(name) {
  if (!SFX.ctx || SFX.ctx.state !== 'running' || !SFX_PLAY[name]) return;
  try { SFX_PLAY[name](SFX.ctx); } catch (e) { /* sound is optional */ }
}

for (const type of ['pointerdown', 'keydown']) window.addEventListener(type, (e) => { if (e.isTrusted) sfxWake(); });
