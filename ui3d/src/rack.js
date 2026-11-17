// Rack bar: input level, presets, A/B rigs, tuner, output level.
// Everything here talks to the audio engine through window.__rigBridge (set by the plugin's WebView).
// Without the plugin (browser preview) the controls still work on the scene; meters stay silent and the
// tuner can listen to the computer's microphone.
const NOTES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];

export const FACTORY = [
  { name: "Red Special", amp: "british30", ampVals: { gain: 0.62, bass: 0.45, mid: 0.6, treble: 0.62, master: 0.55, bright: 1 },
    chain: { pre: ["boost"], loop: ["delay"] }, on: { boost: true, delay: true } },
  { name: "Texas Clean", amp: "american", ampVals: { gain: 0.32, bass: 0.5, mid: 0.45, treble: 0.6, master: 0.55 },
    chain: { pre: ["comp", "od"], loop: ["reverb"] }, on: { comp: true, reverb: true } },
  { name: "Modern Lead", amp: "modern", ampVals: { gain: 0.72, bass: 0.55, mid: 0.6, treble: 0.55, master: 0.5, channel: 1 },
    chain: { pre: ["gate", "od"], loop: ["delay", "reverb"] }, on: { gate: true, od: true, delay: true, reverb: true } },
  { name: "Djent Overlord", amp: "overlord", ampVals: { gain: 0.68, bass: 0.5, mid: 0.55, treble: 0.6, master: 0.5, tight: 1, attack: 0.65 },
    chain: { pre: ["nfuzz", "bioboost"], loop: ["reverb"] }, on: { nfuzz: true, bioboost: true } },
  { name: "Vapor Shimmer", amp: "vapor", ampVals: { gain: 0.3, treble: 0.6, master: 0.5, vapor: 0.65, bloom: 0.7, amb: 0.45, amb_width: 0.9 },
    chain: { pre: ["comp"], loop: ["mutant", "delay", "reverb"] }, on: { comp: true, mutant: true, delay: true, reverb: true } },
];

export function buildRack(el, api) {
  const send = (type, payload) => { try { window.__rigBridge?.send?.(type, payload); } catch (e) { /* engine not attached */ } };
  const store = {
    get() { try { return JSON.parse(localStorage.getItem("toxicrig.presets") || "[]"); } catch (e) { return mem; } },
    set(v) { mem = v; try { localStorage.setItem("toxicrig.presets", JSON.stringify(v)); } catch (e) { /* preview without storage */ } },
  };
  let mem = [];
  const all = () => remote ? remoteList : [...FACTORY.map((p) => ({ ...p, factory: true })), ...store.get()];
  let idx = -1, dirty = false;
  // Rig changes run one at a time: an amp swap is animated, and a second change arriving meanwhile
  // must start from the finished rig, not from a half-loaded one.
  let queue = Promise.resolve();
  const run = (f) => (queue = queue.then(f).catch((e) => console.error(e)));
  const slots = { A: null, B: null }; let active = "A";
  const lvl = { in: 0, out: 0 };   // dB
  const painters = {};
  // In the plugin the engine owns presets and A/B: the rack shows its list and asks it to switch.
  let remote = null, remoteList = [];

  el.innerHTML = `
    <div class="rk-lvl" data-k="in"><span class="rk-l">IN</span><div class="rk-knob" title="Livello di ingresso (trascina, doppio clic = 0 dB)"><i></i></div><div class="rk-meter"><b></b><b></b></div><span class="rk-v">0.0</span></div>
    <div class="rk-sep"></div>
    <div class="rk-preset"><button class="rk-ar" data-d="-1" aria-label="Preset precedente">‹</button>
      <button class="rk-name" title="Lista preset"><span>—</span><em></em></button>
      <button class="rk-ar" data-d="1" aria-label="Preset successivo">›</button>
      <button class="rk-save" title="Salva il rig come preset">Salva</button></div>
    <div class="rk-sep"></div>
    <div class="rk-ab"><button data-s="A" class="on">A</button><button data-s="B">B</button><button class="rk-copy" title="Copia il rig attivo nell'altro slot">⇄</button></div>
    <div class="rk-sep"></div>
    <button class="rk-tuner" title="Accordatore (il segnale viene silenziato)"><span class="rk-fork"></span>TUNER</button>
    <div class="rk-sep"></div>
    <div class="rk-lvl" data-k="out"><span class="rk-l">OUT</span><div class="rk-knob" title="Livello di uscita (trascina, doppio clic = 0 dB)"><i></i></div><div class="rk-meter"><b></b><b></b></div><span class="rk-v">0.0</span></div>
    <div class="rk-list glass"></div>
    <div class="rk-tune glass"><div class="tn-note">—</div><div class="tn-oct"></div>
      <div class="tn-scale"><div class="tn-needle"></div><div class="tn-mid"></div></div>
      <div class="tn-row"><span class="tn-hz">— Hz</span><span class="tn-c">0 cent</span></div>
      <div class="tn-row"><button class="tn-ref" data-d="-1">−</button><span class="tn-a4">A4 = 440 Hz</span><button class="tn-ref" data-d="1">+</button></div>
      <button class="tn-mic">Ascolta dal microfono del computer</button></div>`;
  const $ = (s) => el.querySelector(s);

  // ---------- levels: vertical drag on the knob, -24..+12 dB
  for (const k of ["in", "out"]) {
    const box = el.querySelector(`.rk-lvl[data-k="${k}"]`), knob = box.querySelector(".rk-knob"), v = box.querySelector(".rk-v");
    const paint = () => { knob.querySelector("i").style.transform = `rotate(${-135 + 270 * (lvl[k] + 24) / 36}deg)`; v.textContent = (lvl[k] > 0 ? "+" : "") + lvl[k].toFixed(1); };
    let drag = null;
    knob.addEventListener("pointerdown", (e) => { drag = { y: e.clientY, v: lvl[k] }; knob.setPointerCapture(e.pointerId); });
    knob.addEventListener("pointermove", (e) => { if (!drag) return; lvl[k] = Math.max(-24, Math.min(12, Math.round((drag.v + (drag.y - e.clientY) * 0.15) * 2) / 2)); paint(); send("level", { which: k, db: lvl[k] }); });
    knob.addEventListener("pointerup", () => { drag = null; });
    knob.addEventListener("wheel", (e) => { e.preventDefault(); lvl[k] = Math.max(-24, Math.min(12, lvl[k] + (e.deltaY < 0 ? 0.5 : -0.5))); paint(); send("level", { which: k, db: lvl[k] }); }, { passive: false });
    knob.addEventListener("dblclick", () => { lvl[k] = 0; paint(); send("level", { which: k, db: 0 }); });
    painters[k] = paint; paint();
  }
  // Meters: peak values in dBFS for left/right, pushed by the engine ~30 times a second.
  function meters(m) {
    for (const k of ["in", "out"]) {
      const bars = el.querySelectorAll(`.rk-lvl[data-k="${k}"] .rk-meter b`), v = m[k] || [-120, -120];
      bars.forEach((b, i) => { const db = v[i] ?? v[0], h = Math.max(0, Math.min(1, (db + 60) / 60));
        b.style.transform = `scaleY(${h})`; b.classList.toggle("clip", db > -0.5); });
    }
  }

  // ---------- presets
  function paintName() {
    const p = all()[idx];
    $(".rk-name span").textContent = p ? p.name : "Rig personale";
    $(".rk-name em").textContent = dirty ? "modificato" : p ? (p.factory ? "fabbrica" : "utente") : "";
  }
  async function load(i) {
    const list = all(); idx = (i + list.length) % list.length; const p = list[idx];
    if (remote) { dirty = false; paintName(); remote.load(idx); api.say("Preset: " + p.name); return; }
    await api.applyRig(p); dirty = false; paintName(); send("preset", { name: p.name, rig: api.snapshot() }); api.say("Preset: " + p.name);
  }
  el.querySelectorAll(".rk-ar").forEach((b) => b.addEventListener("click", () => run(() => load(idx + +b.dataset.d))));
  $(".rk-name").addEventListener("click", () => {
    const L = $(".rk-list"), list = all();
    L.innerHTML = list.map((p, i) => `<div class="rk-it${i === idx ? " on" : ""}" data-i="${i}"><span>${p.name}</span><small>${p.factory ? "fabbrica" : "utente"}</small>${p.factory || remote ? "" : `<button data-del="${i}" title="Elimina">×</button>`}</div>`).join("");
    L.classList.toggle("on");
    L.querySelectorAll(".rk-it").forEach((it) => it.addEventListener("click", (e) => {
      if (e.target.dataset.del && !remote) { const u = store.get(); u.splice(+e.target.dataset.del - FACTORY.length, 1); store.set(u); if (idx >= FACTORY.length) idx = -1; paintName(); L.classList.remove("on"); return; }
      L.classList.remove("on"); run(() => load(+it.dataset.i));
    }));
  });
  $(".rk-save").addEventListener("click", () => {
    const L = $(".rk-list"), cur = all()[idx];
    L.innerHTML = `<form class="rk-form"><label>Nome del preset</label><input maxlength="40" value="${cur && !cur.factory ? cur.name.replace(/ \*$/, "") : "Il mio rig"}"><button>Salva</button></form>`;
    L.classList.add("on"); const inp = L.querySelector("input"); inp.focus(); inp.select();
    L.querySelector("form").addEventListener("submit", (e) => { e.preventDefault(); L.classList.remove("on"); savePreset(inp.value.trim()); });
  });
  function savePreset(name) {
    if (!name) return;
    if (remote) { remote.save(name); dirty = false; api.say("Salvato: " + name); return; }
    const u = store.get(), snap = { name, ...api.snapshot() }, at = u.findIndex((p) => p.name === name);
    if (at >= 0) u[at] = snap; else u.push(snap);
    store.set(u); idx = FACTORY.length + (at >= 0 ? at : u.length - 1); dirty = false; paintName(); send("presetSaved", snap); api.say("Salvato: " + name);
  }

  // ---------- A/B: two complete rigs, switch instantly
  async function setSlot(s) {
    if (s === active) return;
    if (remote) { active = s; paintAB(); remote.ab(s); api.say("Rig " + s); return; }
    slots[active] = { ...api.snapshot(), idx, dirty };
    active = s;
    if (!slots[s]) slots[s] = { ...api.snapshot(), idx, dirty };
    else { await api.applyRig(slots[s]); idx = slots[s].idx; dirty = slots[s].dirty; paintName(); }
    paintAB(); send("ab", { slot: s }); api.say("Rig " + s);
  }
  el.querySelectorAll(".rk-ab [data-s]").forEach((b) => b.addEventListener("click", () => run(() => setSlot(b.dataset.s))));
  function paintAB() { el.querySelectorAll(".rk-ab [data-s]").forEach((b) => b.classList.toggle("on", b.dataset.s === active)); }
  $(".rk-copy").addEventListener("click", () => run(() => {
    const other = active === "A" ? "B" : "A";
    if (remote) { remote.copy(); api.say(`Rig ${active} copiato in ${other}`); return; } slots[other] = { ...api.snapshot(), idx, dirty };
    send("abCopy", { from: active, to: other }); api.say(`Rig ${active} copiato in ${other}`);
  }));

  // ---------- tuner
  let a4 = 440, tunerOn = false, micStop = null;
  function tunerOpen(on) {
    tunerOn = on; $(".rk-tuner").classList.toggle("on", on); $(".rk-tune").classList.toggle("on", on);
    send("tuner", { on }); if (!on && micStop) { micStop(); micStop = null; }
  }
  $(".rk-tuner").addEventListener("click", () => tunerOpen(!tunerOn));
  el.querySelectorAll(".tn-ref").forEach((b) => b.addEventListener("click", () => { a4 = Math.max(430, Math.min(450, a4 + +b.dataset.d)); $(".tn-a4").textContent = `A4 = ${a4} Hz`; send("tunerRef", { a4 }); }));
  function pitch(hz) {
    if (!tunerOn) return;
    if (!(hz > 20)) { $(".tn-note").textContent = "—"; $(".tn-oct").textContent = ""; $(".tn-hz").textContent = "— Hz"; $(".tn-c").textContent = ""; $(".tn-needle").style.transform = "rotate(0deg)"; $(".rk-tune").classList.remove("ok"); return; }
    const n = 12 * Math.log2(hz / a4) + 69, r = Math.round(n), c = Math.round((n - r) * 100);
    $(".tn-note").textContent = NOTES[(r % 12 + 12) % 12]; $(".tn-oct").textContent = Math.floor(r / 12) - 1;
    $(".tn-hz").textContent = hz.toFixed(1) + " Hz"; $(".tn-c").textContent = (c > 0 ? "+" : "") + c + " cent";
    $(".tn-needle").style.transform = `rotate(${Math.max(-50, Math.min(50, c)) * 0.9}deg)`;
    $(".rk-tune").classList.toggle("ok", Math.abs(c) <= 3);
  }
  $(".tn-mic").addEventListener("click", async () => {
    if (micStop) return;
    try {
      const s = await navigator.mediaDevices.getUserMedia({ audio: { echoCancellation: false, noiseSuppression: false, autoGainControl: false } });
      const ac = new AudioContext(), an = ac.createAnalyser(); an.fftSize = 4096; ac.createMediaStreamSource(s).connect(an);
      const buf = new Float32Array(an.fftSize); let raf = 0;
      const tick = () => { an.getFloatTimeDomainData(buf); pitch(detect(buf, ac.sampleRate));
        let pk = 0; for (const x of buf) pk = Math.max(pk, Math.abs(x)); const db = 20 * Math.log10(pk + 1e-9); meters({ in: [db, db], out: [-120, -120] });
        raf = requestAnimationFrame(tick); };
      tick(); $(".tn-mic").textContent = "In ascolto dal microfono";
      micStop = () => { cancelAnimationFrame(raf); s.getTracks().forEach((t) => t.stop()); ac.close(); $(".tn-mic").textContent = "Ascolta dal microfono del computer"; meters({}); };
    } catch (e) { api.say("Microfono non disponibile: nel plugin l'accordatore legge direttamente la chitarra."); }
  });

  paintName();
  return {
    meters, pitch, idle: () => queue, run,
    // Plugin mode: r = { load(i), save(name), ab("A"|"B"), copy() }. sync() gets the engine's list and state.
    remote(r) { remote = r; $(".tn-mic").style.display = "none"; },
    sync({ presets, preset, ab }) { if (presets) remoteList = presets.map((n) => ({ name: n.replace(/ \*$/, ""), factory: !/ \*$/.test(n) }));
      if (preset != null) idx = preset; if (ab != null) { active = ab ? "B" : "A"; paintAB(); } dirty = false; paintName(); },
    markDirty() { if (!dirty) { dirty = true; paintName(); } },
    setLevels(v) { Object.assign(lvl, v); painters.in(); painters.out(); },
    levels: () => ({ ...lvl }),
  };
}

// Pitch by normalised autocorrelation (YIN-like difference function), good for guitar range 60..1400 Hz.
function detect(x, sr) {
  let rms = 0; for (const v of x) rms += v * v; rms = Math.sqrt(rms / x.length); if (rms < 0.01) return 0;
  const N = x.length >> 1, d = new Float32Array(N); let run = 0; d[0] = 1;
  for (let t = 1; t < N; t++) { let s = 0; for (let i = 0; i < N; i++) { const q = x[i] - x[i + t]; s += q * q; } run += s; d[t] = s * t / (run || 1); }
  const lo = Math.floor(sr / 1400), hi = Math.min(N - 1, Math.ceil(sr / 60));
  for (let t = lo; t < hi; t++) if (d[t] < 0.15 && d[t] <= d[t + 1]) {
    const a = d[t - 1], b = d[t], c = d[t + 1], p = (a - c) / (2 * (a - 2 * b + c) || 1);
    return sr / (t + p);
  }
  return 0;
}
