// ToxicRig - real-time 3D stage (v2): three amps, club stage, pedals, cab + microphone.
import * as THREE from "three";
import { GLTFLoader } from "three/examples/jsm/loaders/GLTFLoader.js";
import { RoundedBoxGeometry } from "three/examples/jsm/geometries/RoundedBoxGeometry.js";
import { PEDALS, AMPS, CABS, MICS, toValue, toNorm, fmt } from "./pedals.js";
import { makeMaterials } from "./materials.js";
import { overlordPanel, overlordBadge } from "./art.js";
import { RectAreaLightUniformsLib } from "three/examples/jsm/lights/RectAreaLightUniformsLib.js";
RectAreaLightUniformsLib.init();
import { buildStage } from "./stage.js";
import { pedalFactory, PH, PD, BOARD_TOP } from "./pedal3d.js";
import { buildRack } from "./rack.js";
import { micFactory } from "./mic3d.js";

const $ = (s) => document.querySelector(s);
const MOBILE = matchMedia("(pointer: coarse)").matches;
const REDUCED = matchMedia("(prefers-reduced-motion: reduce)").matches;

// ------------------------------------------------------------------ renderer
const canvas = $("#gl");
const renderer = new THREE.WebGLRenderer({ canvas, antialias: true, powerPreference: "high-performance" });
renderer.shadowMap.enabled = true; renderer.shadowMap.type = THREE.PCFShadowMap;
// Only the key light casts shadows and it never moves: the shadow map is redrawn only while something in
// the scene moves (amp rising, pedals sliding, controls turning), not on every frame.
renderer.shadowMap.autoUpdate = false;
let shadowFrames = 30;
const shadowsDirty = (n = 3) => { shadowFrames = Math.max(shadowFrames, n); };
let lastInput = 0;
addEventListener("pointerdown", () => { lastInput = performance.now(); shadowsDirty(); });
addEventListener("pointermove", (e) => { lastInput = performance.now(); if (e.buttons) shadowsDirty(); });
addEventListener("wheel", () => { lastInput = performance.now(); }, { passive: true });
renderer.toneMapping = THREE.AgXToneMapping; renderer.toneMappingExposure = 1.15;
renderer.outputColorSpace = THREE.SRGBColorSpace;
const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(30, 1, 0.02, 40);

const manager = new THREE.LoadingManager();
manager.onProgress = (_u, done, total) => { const b = $("#loading b"); if (b) b.style.width = (100 * done / total) + "%"; };
const { M, tex } = makeMaterials(renderer, manager);
const stage = buildStage(renderer, scene, camera, tex, M);
const PF = pedalFactory(renderer, M, tex);
const MIC = micFactory(M, tex);

// Quality: "Alta" = ambient occlusion + lens depth of field; "Media" for phones and slower PCs.
let maxRatio = 1, curRatio = 1;   // adaptive resolution: curRatio drops when frames take too long
let quality = (() => { try { return localStorage.getItem("tr.quality") || (MOBILE ? "media" : "alta"); } catch { return MOBILE ? "media" : "alta"; } })();
function applyQuality() {
  const hi = quality === "alta";
  maxRatio = Math.min(devicePixelRatio, hi ? 2 : 1.25); curRatio = maxRatio;
  renderer.setPixelRatio(curRatio); shadowsDirty(5);
  // AO is baked per object; screen-space AO and lens blur are off: they cost sharpness.
  stage.gtao.enabled = false; stage.bokeh.enabled = false;
  stage.L.key.shadow.mapSize.set(hi ? 2048 : 1024, hi ? 2048 : 1024); if (stage.L.key.shadow.map) { stage.L.key.shadow.map.dispose(); stage.L.key.shadow.map = null; }
  $("#quality").textContent = "Qualità: " + (hi ? "Alta" : "Media");
  try { localStorage.setItem("tr.quality", quality); } catch {}
  resize();
}
$("#quality").addEventListener("click", () => { quality = quality === "alta" ? "media" : "alta"; applyQuality(); });

// ------------------------------------------------------------------ amps
// Plugin parameters are per amp slot, not per model: switching model keeps the knobs where they are.
const ampVals = { gain: 0.5, bass: 0.5, mid: 0.5, treble: 0.5, presence: 0.5, resonance: 0.5, master: 0.5, sag: 0.5, bright: 0, channel: 1,
  attack: 0.5, tight: 1, vapor: 0.4, bloom: 0.5, amb: 0.25, amb_width: 0.7, standby: 0, power: 1, eq1: 0, eq2: 0, eq3: 0, eq4: 0, eq5: 0, eq6: 0, eq7: 0, eq8: 0 };
const EQ_TRAVEL = 0.034;
const AMP = {};          // id -> loaded amp
let amp = null;          // amp on stage
const ampHolder = new THREE.Group(); scene.add(ampHolder);
const Z_AXIS = new THREE.Vector3(0, 0, -1);

function loadAmp(def, mgr) {
  return new Promise((res, rej) => new GLTFLoader(mgr).load(def.file, (g) => res(processAmp(def, g.scene)), undefined, rej));
}
function applyAO(root, url) {
  const ao = new THREE.TextureLoader().load(url); ao.flipY = false; ao.channel = 1; ao.colorSpace = THREE.NoColorSpace;
  const cache = new Map();
  root.traverse((o) => {
    if (!o.isMesh || !o.geometry.attributes.uv1) return;
    const m = o.material;
    if (!cache.has(m)) { const c = m.clone(); c.aoMap = ao; c.aoMapIntensity = 1.0; cache.set(m, c);
      const g = root.userData.amp?.glowMats.find((x) => x.m === m); if (g) g.m = c; }
    o.material = cache.get(m);
  });
}
const ART = {};
function artMaterial(def, o) {
  const key = def.id + o.name;
  if (!ART[key]) {
    const L = o.userData.layout ? JSON.parse(o.userData.layout) : null;
    if (o.name.startsWith("Art_panel") && def.id === "overlord") ART[key] = overlordPanel(L);
    else if (o.name.startsWith("Art_badge") && def.id === "overlord") ART[key] = overlordBadge(o.userData.w, o.userData.h);
    else ART[key] = M.PanelBronze;
  }
  return ART[key];
}
// Vapor-30 screen: neon logo whose rings breathe with the VAPOR amount (redrawn ~30 fps).
function screenMaterial(a) {
  const c = document.createElement("canvas"); c.width = 512; c.height = 288; const g = c.getContext("2d");
  const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.flipY = false;
  const m = new THREE.MeshBasicMaterial({ map: t, toneMapped: false });
  let last = -1;
  a.screen = { draw(time, power) {
    if (time - last < 0.033) return; last = time;
    g.fillStyle = "#050308"; g.fillRect(0, 0, 512, 288);
    if (power < 0.05) { t.needsUpdate = true; return; }
    g.save(); g.globalAlpha = Math.min(1, power * 1.2);
    const v = ampVals.vapor, pulse = 0.5 + 0.5 * Math.sin(time * (1.5 + 3 * ampVals.bloom));
    for (let k = 0; k < 5; k++) {
      g.strokeStyle = `rgba(155,77,255,${0.25 + 0.5 * v * (k % 2 ? pulse : 1 - pulse)})`; g.lineWidth = 5;
      g.beginPath(); g.arc(320, 150, 40 + k * 22 + 6 * v * pulse, -1.2, 1.2); g.stroke();
    }
    g.strokeStyle = "rgba(57,255,20,0.85)"; g.lineWidth = 4; g.beginPath(); g.moveTo(30, 150); g.lineTo(140, 150); g.moveTo(380, 150); g.lineTo(490, 150); g.stroke();
    g.shadowColor = "#39ff14"; g.shadowBlur = 24; g.fillStyle = "#9dff7a"; g.font = "italic 700 96px Georgia, serif"; g.textAlign = "center"; g.textBaseline = "middle";
    g.fillText(ampVals.standby ? "Standby" : "Vapor", 256, 146); g.shadowBlur = 0;
    g.fillStyle = "rgba(200,180,255,0.8)"; g.font = "700 22px Rajdhani"; g.fillText(`VAPOR ${(v * 10).toFixed(1)}   ·   D/R ${(ampVals.amb * 10).toFixed(1)}`, 256, 250);
    g.restore(); t.needsUpdate = true;
  } };
  return m;
}
function processAmp(def, root) {
  const a = { def, root, knobs: {}, toggles: {}, sliders: {}, speakers: [], glowMats: [] };
  root.userData.amp = a;
  // Tubes and lit parts get their own material copies, tinted with this amp's colour,
  // so power/standby and the heater flicker act on this amp only.
  const tint = new THREE.Color(def.tube);
  const own = {};
  const ownMat = (name) => {
    if (!own[name]) { own[name] = M[name].clone(); if (own[name].emissive && name !== "Jewel") own[name].emissive.copy(tint); a.glowMats.push({ m: own[name], base: own[name].emissiveIntensity }); }
    return own[name];
  };
  root.traverse((o) => {
    if (o.isMesh) {
      o.castShadow = true; o.receiveShadow = true;
      if (o.name.startsWith("WinGlass")) { o.visible = false; return; }
      const n = o.material.name;
      if (["Heater", "TubePlate", "WindowBack", "Jewel", "JewelPurple", "TubeLightGreen", "TubeLightPurple", "WindowBackPurple", "Acrylic", "AcrylicCore", "AcrylicRing", "LedPurple", "VentGlow", "VentGlowGreen"].includes(n) && M[n]) o.material = ownMat(n);
      else if (n === "Screen") o.material = screenMaterial(a);
      else if (n.startsWith("Art_")) o.material = artMaterial(def, o);
      else if (M[n]) o.material = M[n];
      if (o.material.alphaTest > 0 || o.material.transparent) o.castShadow = false;
    }
  });
  // Baked ambient occlusion (second UV set): soft darkening in every crevice.
  applyAO(root, def.file.replace(".json", "_ao.jpg"));
  for (const c of def.knobs) {
    const obj = root.getObjectByName(c.id + "_cap");
    a.knobs[c.id] = { obj, q0: obj.quaternion.clone(), label: c.label };
    obj.traverse((m) => { m.userData.ctl = { kind: "ampKnob", id: c.id }; });
  }
  for (const t of def.toggles) {
    const obj = root.getObjectByName(t.id + "_pivot");
    a.toggles[t.id] = { obj, q0: obj.quaternion.clone(), n: t.n, def: t };
    obj.traverse((m) => { m.userData.ctl = { kind: "ampToggle", id: t.id }; });
  }
  for (const sl of def.sliders || []) {
    const obj = root.getObjectByName(sl.id + "_cap");
    a.sliders[sl.id] = { obj, p0: obj.position.clone(), label: sl.label };
    obj.traverse((m) => { m.userData.ctl = { kind: "ampSlider", id: sl.id }; });
  }
  const P = root.getObjectByName("ShotPanel"), C = root.getObjectByName("ShotCab");
  a.panel = { p: P.position.clone(), w: P.userData.w, h: P.userData.h };
  a.cab = { p: C.position.clone(), w: C.userData.w, h: C.userData.h };
  for (let i = 0; i < 4; i++) { const s = root.getObjectByName(`spk${i}_center`); if (s) a.speakers.push({ x: s.position.x, y: s.position.y, z: s.position.z, r: s.userData.radius }); }
  a.input = root.getObjectByName("input_hole").position.clone();
  a.loop = root.getObjectByName("fxloop_hole").position.clone();
  a.box = new THREE.Box3().setFromObject(root);
  // Framing box: from the floor up, no wider than the real head/cab front. A stray mesh in a model
  // (a glow disc, a hidden helper) must not push the camera away.
  const halfW = Math.max(a.panel.w, a.cab.w) / 2 + 0.04;
  a.box.min.x = Math.max(a.box.min.x, -halfW); a.box.max.x = Math.min(a.box.max.x, halfW);
  a.box.min.y = Math.max(a.box.min.y, 0); a.box.min.z = Math.max(a.box.min.z, -0.25); a.box.max.z = Math.min(a.box.max.z, 0.25);
  a.cabTop = a.cab.p.y + a.cab.h / 2 + 0.006;
  // Light spilling from the tubes, flickering a little like real heaters.
  const tubeObj = root.getObjectByName("tube0_heater") || root.getObjectByName("Heater");
  a.glow = new THREE.PointLight(def.tube, 0, 1.4, 2);
  if (tubeObj) { const p = new THREE.Vector3(); tubeObj.getWorldPosition(p); a.glow.position.set(p.x * 0.3, p.y, p.z + 0.05); }
  root.add(a.glow);
  const wl = root.getObjectByName("WinLight");
  if (wl) {
    // LED bar inside the window: a real area light washing down over the tubes.
    a.winLight = new THREE.RectAreaLight(def.tube, 0, wl.userData.w, 0.012);
    a.winLight.position.copy(wl.position); a.winLight.lookAt(wl.position.clone().add(new THREE.Vector3(0, -1, -0.15)));
    root.add(a.winLight);
  }
  applyAmpVals(a);
  AMP[def.id] = a; return a;
}
function applyAmpVals(a) {
  for (const id in a.knobs) { const k = a.knobs[id]; k.obj.quaternion.copy(new THREE.Quaternion().setFromAxisAngle(Z_AXIS, THREE.MathUtils.degToRad(270 * ampVals[id])).multiply(k.q0)); }
  for (const id in a.sliders) { const sl = a.sliders[id]; sl.obj.position.copy(sl.p0); sl.obj.position.y += (ampVals[id] / 12) * EQ_TRAVEL / 2; }
  for (const id in a.toggles) {
    const t = a.toggles[id], s = ampVals[id];
    const q = t.n === 2 ? new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(1, 0, 0), s ? 0.5 : -0.5)
                        : new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(0, 1, 0), [-0.55, 0, 0.55][s]);
    t.obj.quaternion.copy(q.multiply(t.q0));
  }
}

// ------------------------------------------------------------------ pedalboard
const BOARD = { x: 0, z: 0.7, w: 1.06, d: 0.42 };
const ROWS = { pre: { z: BOARD.z + 0.095, label: "INGRESSO  ▸  AMPLI" }, loop: { z: BOARD.z - 0.1, label: "LOOP FX  (dopo il preamp)" } };
const boardGroup = new THREE.Group(); boardGroup.visible = false; scene.add(boardGroup);
{
  const alu = new THREE.MeshPhysicalMaterial({ color: 0x17171b, metalness: 0.8, roughness: 0.55, roughnessMap: tex("grunge.jpg", false, 0.3), clearcoat: 0.2 });
  const base = new THREE.Mesh(new RoundedBoxGeometry(BOARD.w, 0.03, BOARD.d, 4, 0.008), alu);
  base.position.set(BOARD.x, BOARD_TOP - 0.015, BOARD.z); base.castShadow = base.receiveShadow = true; boardGroup.add(base);
  // Hook-and-loop strips under the pedals.
  const velcro = new THREE.MeshStandardMaterial({ color: 0x0b0b0c, roughness: 1, normalMap: tex("rug_nrm.jpg", false, 0.05), normalScale: new THREE.Vector2(0.6, 0.6) });
  for (const z of [ROWS.pre.z, ROWS.loop.z]) { const r = new THREE.Mesh(new THREE.BoxGeometry(BOARD.w - 0.05, 0.0015, 0.12), velcro); r.position.set(BOARD.x, BOARD_TOP + 0.0008, z); r.receiveShadow = true; boardGroup.add(r); }
  const edge = new THREE.Mesh(new THREE.BoxGeometry(BOARD.w - 0.03, 0.003, 0.003), new THREE.MeshStandardMaterial({ color: 0, emissive: 0x39ff14, emissiveIntensity: 2.4 }));
  edge.position.set(BOARD.x, BOARD_TOP - 0.012, BOARD.z + BOARD.d / 2 + 0.001); boardGroup.add(edge);
  for (const key of ["pre", "loop"]) {
    const c = document.createElement("canvas"); c.width = 1024; c.height = 64;
    const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 8;
    const m = new THREE.Mesh(new THREE.PlaneGeometry(0.4, 0.025), new THREE.MeshBasicMaterial({ map: t, transparent: true, opacity: 0.85, toneMapped: false }));
    m.rotation.x = -Math.PI / 2; m.position.set(BOARD.x - BOARD.w / 2 + 0.22, BOARD_TOP + 0.0025, ROWS[key].z + (key === "pre" ? 0.077 : -0.077));
    boardGroup.add(m); ROWS[key].canvas = c; ROWS[key].caption = t;
  }
}
function paintCaption(key, extra) {
  const c = ROWS[key].canvas, g = c.getContext("2d"); g.clearRect(0, 0, c.width, c.height);
  g.font = "700 40px Rajdhani, sans-serif"; g.fillStyle = key === "pre" ? "#39ff14" : "#a875ff"; g.textBaseline = "middle";
  g.fillText(ROWS[key].label + (extra ? "   " + extra : ""), 8, 34); ROWS[key].caption.needsUpdate = true;
}
const PEDAL = {};
function pedalState(id) { if (!PEDAL[id]) PEDAL[id] = PF.build(PEDALS.find((d) => d.id === id)); return PEDAL[id]; }
const chain = { pre: [], loop: [] };
const ROW_W = BOARD.w - 0.06, GAP = 0.056;
function rowLayout(key, ids) {
  const widths = ids.map((id) => PEDALS.find((d) => d.id === id).w);
  const total = widths.reduce((a, b) => a + b, 0) + GAP * Math.max(0, ids.length - 1);
  let x = BOARD.x - ROW_W / 2 + 0.03;
  const out = ids.map((id, i) => { const cx = x + widths[i] / 2; x += widths[i] + GAP; return { id, x: cx, z: ROWS[key].z }; });
  return { out, nextX: x, free: ROW_W - 0.03 - total - (ids.length ? GAP : 0) };
}
function plusTexture(key) {
  const c = document.createElement("canvas"); c.width = 296; c.height = 472; const g = c.getContext("2d");
  const col = key === "pre" ? "57,255,20" : "168,117,255";
  g.strokeStyle = `rgba(${col},0.75)`; g.lineWidth = 6; g.setLineDash([18, 12]); g.beginPath(); g.roundRect(8, 8, 280, 456, 26); g.stroke(); g.setLineDash([]);
  g.fillStyle = `rgba(${col},0.08)`; g.fill(); g.fillStyle = `rgba(${col},0.95)`; g.font = "700 150px Rajdhani"; g.textAlign = "center"; g.textBaseline = "middle";
  g.fillText("+", 148, 200); g.font = "700 44px Rajdhani"; g.fillText("PEDALE", 148, 320);
  const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; return t;
}
function makeAddSlot(key) {
  const g = new THREE.Group();
  const p = new THREE.Mesh(new THREE.PlaneGeometry(0.074, PD), new THREE.MeshBasicMaterial({ transparent: true, map: plusTexture(key), depthWrite: false, toneMapped: false }));
  p.rotation.x = -Math.PI / 2; p.position.y = BOARD_TOP + 0.003; g.add(p); p.userData.ctl = { kind: "addSlot", row: key };
  boardGroup.add(g); return g;
}
const addSlots = { pre: makeAddSlot("pre"), loop: makeAddSlot("loop") };
const anims = [];
function moveTo(obj, x, z, instant) {
  const to = new THREE.Vector3(x, 0, z);
  if (instant || REDUCED) { obj.position.copy(to); return; }
  anims.push({ obj, from: obj.position.clone(), to, t0: performance.now(), dur: 560 });
}
let dragging = null;
function layoutChain(instant) {
  for (const key of ["pre", "loop"]) {
    const { out, nextX, free } = rowLayout(key, chain[key]);
    out.forEach((o, i) => {
      const st = PEDAL[o.id];
      if (st.grp.parent !== boardGroup) { boardGroup.add(st.grp); st.grp.position.set(o.x, 0.25, o.z); moveTo(st.grp, o.x, o.z, instant); }
      else if (st !== dragging?.st) moveTo(st.grp, o.x, o.z, instant);
      st.index = i; st.row = key;
    });
    addSlots[key].visible = free >= 0.074 && chain.pre.length + chain.loop.length < PEDALS.length;
    addSlots[key].position.set(nextX + 0.037, 0, ROWS[key].z);
    paintCaption(key, chain[key].length ? "" : "(vuoto)");
  }
  rebuildCables();
}

// ------------------------------------------------------------------ cables (braided with chrome plugs)
const cableMat = new THREE.MeshPhysicalMaterial({ color: 0x141416, roughness: 0.55, sheen: 0.5, sheenRoughness: 0.5, sheenColor: new THREE.Color(0x303036), normalMap: tex("cloth_nrm.png", false, 0.004), normalScale: new THREE.Vector2(0.6, 0.6) });
const cableMatG = cableMat.clone(); cableMatG.color.set(0x16200f); cableMatG.sheenColor = new THREE.Color(0x2c6a1a);
const cableMatV = cableMat.clone(); cableMatV.color.set(0x1a1222); cableMatV.sheenColor = new THREE.Color(0x5a2aa0);
const cables = new THREE.Group(); scene.add(cables);
function plug(at, dir) {
  const g = new THREE.Group();
  const b = new THREE.Mesh(new THREE.CylinderGeometry(0.0046, 0.0046, 0.024, 24), M.Chrome); b.position.y = 0.006;
  const knurl = new THREE.Mesh(new THREE.CylinderGeometry(0.0049, 0.0049, 0.008, 12), M.Nickel); knurl.position.y = 0.004;
  const boot = new THREE.Mesh(new THREE.CylinderGeometry(0.0034, 0.0045, 0.014, 16), M.Rubber); boot.position.y = 0.025;
  g.add(b, knurl, boot); g.position.copy(at); g.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), dir.clone().normalize());
  g.traverse((m) => { if (m.isMesh) m.castShadow = true; }); return g;
}
function cable(a, b, mat, o = {}) {
  const da = (o.da || new THREE.Vector3(0, 1, 0)).clone().normalize(), db = (o.db || new THREE.Vector3(0, 1, 0)).clone().normalize();
  const la = o.la ?? 0.04, lb = o.lb ?? 0.04;
  const a0 = a.clone().addScaledVector(da, 0.032), b0 = b.clone().addScaledVector(db, 0.032);
  const a1 = a0.clone().addScaledVector(da, la), b1 = b0.clone().addScaledVector(db, lb);
  const mid = a1.clone().lerp(b1, 0.5);
  mid.y = o.arc != null ? Math.max(a1.y, b1.y) + o.arc : Math.max(o.floor ?? BOARD_TOP + 0.004, Math.min(a1.y, b1.y) - (o.sag ?? 0.02));
  if (o.midZ != null) mid.z = o.midZ;
  const curve = new THREE.CatmullRomCurve3(o.via ? [a0, a1, ...o.via, b1, b0] : [a0, a1, mid, b1, b0], false, "centripetal");
  const m = new THREE.Mesh(new THREE.TubeGeometry(curve, 90, 0.0034, 10, false), mat); m.castShadow = true; cables.add(m);
  cables.add(plug(a, da), plug(b, db));
}
const LEFT = new THREE.Vector3(-1, 0, 0), RIGHT = new THREE.Vector3(1, 0, 0), FRONT = new THREE.Vector3(0, 0, 1);
function jackPos(id, side) {
  const st = PEDAL[id], p = st.grp.position;
  if (st.jack.top) return new THREE.Vector3(p.x + side * st.jack.x, st.jack.y, p.z + st.jack.z);
  return new THREE.Vector3(p.x + side * (st.def.w / 2 + 0.004), st.jack.y, p.z + st.jack.z);
}
const UP = new THREE.Vector3(0, 1, 0);
const jackDir = (id, side) => (PEDAL[id].jack.top ? UP : side < 0 ? LEFT : RIGHT);
let micCable = null;
function rebuildCables() {
  shadowsDirty(2);
  cables.children.slice().forEach((c) => { cables.remove(c); c.traverse((m) => m.geometry?.dispose()); });
  if (!boardGroup.visible || !amp) return;
  const pre = chain.pre, loop = chain.loop;
  const guitar = new THREE.Vector3(BOARD.x - BOARD.w / 2 - 0.3, -0.004, BOARD.z + 0.34);
  cables.add(plug(guitar, new THREE.Vector3(-1, 0.02, 0.35)));
  const ampIn = amp.input.clone(), ampLoop = amp.loop.clone();
  if (pre.length) {
    cable(guitar, jackPos(pre[0], -1), cableMatG, { da: new THREE.Vector3(1, 0, -0.3), db: jackDir(pre[0], -1), floor: -0.006, sag: 0.1, la: 0.12, lb: 0.03 });
    for (let i = 0; i + 1 < pre.length; i++) cable(jackPos(pre[i], 1), jackPos(pre[i + 1], -1), cableMat, { da: jackDir(pre[i], 1), db: jackDir(pre[i + 1], -1), la: 0.006, lb: 0.006, arc: 0.03 });
    // Out of the last pedal along the front row, round the right end of the board, then behind it to the amp:
    // never over the loop row.
    const pa = jackPos(pre[pre.length - 1], 1), ex = BOARD.x + BOARD.w / 2 + 0.05;
    const via = [new THREE.Vector3((pa.x + ex) / 2, BOARD_TOP + 0.03, pa.z + 0.02), new THREE.Vector3(ex, 0.01, pa.z + 0.03),
      new THREE.Vector3(ex + 0.02, -0.006, BOARD.z - BOARD.d / 2 - 0.08), new THREE.Vector3((ex + ampIn.x) / 2, -0.006, BOARD.z - BOARD.d / 2 - 0.12)];
    cable(pa, ampIn, cableMatG, { da: jackDir(pre[pre.length - 1], 1), db: FRONT, la: 0.03, lb: 0.12, via });
  } else cable(guitar, ampIn, cableMatG, { da: new THREE.Vector3(1, 0, -0.3), db: FRONT, floor: -0.006, sag: 0.3, la: 0.15, lb: 0.12 });
  if (loop.length) {
    cable(ampLoop, jackPos(loop[0], -1), cableMatV, { da: FRONT, db: jackDir(loop[0], -1), la: 0.1, lb: 0.03, floor: -0.006, sag: 0.3, midZ: BOARD.z - BOARD.d / 2 - 0.05 });
    for (let i = 0; i + 1 < loop.length; i++) cable(jackPos(loop[i], 1), jackPos(loop[i + 1], -1), cableMat, { da: jackDir(loop[i], 1), db: jackDir(loop[i + 1], -1), la: 0.006, lb: 0.006, arc: 0.03 });
    cable(jackPos(loop[loop.length - 1], 1), ampLoop.clone().add(new THREE.Vector3(0, -0.007, 0)), cableMatV, { da: jackDir(loop[loop.length - 1], 1), db: FRONT, la: 0.05, lb: 0.09, floor: -0.006, sag: 0.3, midZ: BOARD.z - BOARD.d / 2 - 0.1 });
  }
  if (micCable) {
    // XLR: follows the stand, then lies on the floor towards the side of the stage.
    const pts = [...micCable.route, micCable.route[micCable.route.length - 1].clone().add(new THREE.Vector3(0.5 * micCable.side, 0, 0.25)), micCable.route[micCable.route.length - 1].clone().add(new THREE.Vector3(1.4 * micCable.side, 0, 0.1))];
    pts.forEach((p, i) => { if (i >= pts.length - 3) p.y = -0.008; });
    const m = new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts, false, "centripetal"), 140, 0.0032, 10, false), cableMat); m.castShadow = true; cables.add(m);
  }
}

// ------------------------------------------------------------------ cab + microphone
const cabState = { on: true, type: 0, mic: "dyn", pos: 0.3, dist: 0.2, ir: null };
const micRig = MIC.rig(); micRig.root.visible = false; scene.add(micRig.root);
function placeMic() {
  shadowsDirty(2);
  if (!amp) return;
  const spk = amp.speakers[0];
  const front = amp.cab.p.z;
  const r = micRig.place(cabState.mic, spk, front, cabState.pos, cabState.dist);
  micCable = { route: r.route, side: spk.x <= 0 ? -1 : 1 };
}

// ------------------------------------------------------------------ camera shots
const cam = { tgt: new THREE.Vector3(0, 0.4, 0.3), sph: new THREE.Spherical(5, 1.2, -0.4) };
let shotAnim = null, shot = null;
const sway = { x: 0, y: 0, tx: 0, ty: 0 };
function shotSpec(name, arg) {
  if (name === "empty") return { tgt: new THREE.Vector3(0, 0.55, 0.0), theta: -0.3, phi: 1.36, sphere: 1.2, ap: 0.002 };
  if (name === "wide") {
    const b = amp.box.clone(); if (boardGroup.visible) b.expandByPoint(new THREE.Vector3(BOARD.x - BOARD.w / 2, 0, BOARD.z + BOARD.d / 2)).expandByPoint(new THREE.Vector3(BOARD.x + BOARD.w / 2, 0, BOARD.z));
    const s = b.getBoundingSphere(new THREE.Sphere());
    return { tgt: s.center.clone().add(new THREE.Vector3(0, -0.02, 0)), theta: -0.38, phi: 1.16, sphere: s.radius * 0.8, ap: 0.003 };
  }
  if (name === "hero") { const b = amp.box; const c = new THREE.Vector3(0, amp.panel.p.y, amp.panel.p.z); return { tgt: c, theta: -0.3, phi: Math.PI / 2 - 0.04, box: [amp.panel.w * 1.18, amp.panel.h * 1.25], ap: 0.006 }; }
  if (name === "amp") return { tgt: amp.panel.p.clone(), theta: 0.0, phi: Math.PI / 2 - 0.05, box: [amp.panel.w + 0.03, amp.panel.h + 0.04], ap: 0.012 };
  if (name === "cab") {
    const s = amp.speakers[0];
    const side = s.x <= 0 ? -1 : 1;
    return { tgt: new THREE.Vector3(s.x, s.y, amp.cab.p.z + 0.06), theta: -side * 0.38, phi: Math.PI / 2 - 0.1, box: [0.5, 0.42], ap: 0.01, panel: true };
  }
  if (name === "board") return { tgt: new THREE.Vector3(BOARD.x, BOARD_TOP + 0.03, BOARD.z + 0.02), theta: -0.12, phi: 0.82, box: [BOARD.w + 0.08, BOARD.d + 0.12], ap: 0.006 };
  if (name === "pedal") { const st = PEDAL[arg], p = st.grp.position; return { tgt: new THREE.Vector3(p.x, BOARD_TOP + PH, p.z + 0.004), theta: 0, phi: 0.66, box: [Math.max(st.def.w + 0.04, 0.11), (st.def.d || PD) + 0.035], ap: 0.016, panel: true }; }
}
function resolveShot(spec) {
  const vfov = THREE.MathUtils.degToRad(camera.fov), aspect = innerWidth / innerHeight;
  const hfov = 2 * Math.atan(Math.tan(vfov / 2) * aspect);
  // Frame the subject inside the free part of the screen: the rack bar sits on top, the dock (and the
  // control chips in close-ups) at the bottom. Bigger amps get a farther camera, smaller ones a closer one.
  const h = innerHeight, ui = !document.body.classList.contains("clean");
  const rt = ui ? Math.min(0.16, 74 / h) : 0, rb = spec.panel ? 0.2 : ui ? Math.min(0.2, (innerWidth < 1180 ? 170 : 112) / h) : 0;
  const uW = 1, uH = 1 - rt - rb;
  let r;
  if (spec.sphere) r = spec.sphere / Math.sin(Math.min(vfov * uH, hfov) / 2);
  else { const [bw, bh] = spec.box; r = Math.max((bh / 2) / Math.tan(vfov / 2) / uH, (bw / 2) / Math.tan(hfov / 2) / uW) * 1.04; }
  const tgt = spec.tgt.clone();
  {
    const off = new THREE.Vector3().setFromSpherical(new THREE.Spherical(r, spec.phi, spec.theta));
    const fwd = off.clone().negate().normalize(), right = new THREE.Vector3().crossVectors(fwd, new THREE.Vector3(0, 1, 0)).normalize(), upv = new THREE.Vector3().crossVectors(right, fwd).normalize();
    const visH = 2 * Math.tan(vfov / 2) * r;
    tgt.addScaledVector(upv, -visH * (rb - rt) / 2);
  }
  return { tgt, sph: new THREE.Spherical(r, spec.phi, spec.theta), ap: spec.ap, focus: spec.tgt.clone() };
}
let focusPoint = new THREE.Vector3(), aperture = 0.003, camFill = 0;
function goShot(name, arg, instant) {
  shot = { name, arg };
  const to = resolveShot(shotSpec(name, arg));
  if (instant || REDUCED) { cam.tgt.copy(to.tgt); cam.sph.copy(to.sph); focusPoint.copy(to.focus); aperture = to.ap; shotAnim = null; return; }
  const far = Math.abs(to.sph.theta - cam.sph.theta) + Math.abs(to.sph.phi - cam.sph.phi);
  shotAnim = { from: { tgt: cam.tgt.clone(), sph: cam.sph.clone(), focus: focusPoint.clone(), ap: aperture }, to, t0: performance.now(), dur: 1250 + far * 500 };
  document.body.dataset.shot = name; syncDock();
}
const ease = (t) => (t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2);
function updateCamera(now) {
  if (shotAnim) {
    const t = Math.min(1, (now - shotAnim.t0) / shotAnim.dur), e = ease(t), a = shotAnim.from, b = shotAnim.to;
    cam.tgt.lerpVectors(a.tgt, b.tgt, e);
    cam.sph.radius = Math.exp(THREE.MathUtils.lerp(Math.log(a.sph.radius), Math.log(b.sph.radius), e));
    cam.sph.theta = THREE.MathUtils.lerp(a.sph.theta, b.sph.theta, e);
    cam.sph.phi = THREE.MathUtils.lerp(a.sph.phi, b.sph.phi, e) - Math.sin(Math.PI * e) * 0.07;
    focusPoint.lerpVectors(a.focus, b.focus, e);
    // While travelling the lens opens up a little less: the move reads as a camera, not a cut.
    aperture = THREE.MathUtils.lerp(a.ap, b.ap, e) * (1 - 0.6 * Math.sin(Math.PI * e));
    if (t >= 1) shotAnim = null;
  }
  const wideish = shot && (shot.name === "wide" || shot.name === "empty");
  sway.x += ((wideish ? sway.tx : 0) - sway.x) * 0.04; sway.y += ((wideish ? sway.ty : 0) - sway.y) * 0.04;
  const hand = REDUCED ? 0 : 1;
  const s = new THREE.Spherical(cam.sph.radius, cam.sph.phi + sway.y * 0.04 + hand * (Math.sin(now / 2900) * 0.004 + Math.sin(now / 1300) * 0.0015),
    cam.sph.theta + sway.x * 0.07 + hand * (Math.sin(now / 3700) * 0.006 + Math.sin(now / 1700) * 0.002));
  camera.position.setFromSpherical(s).add(cam.tgt); camera.lookAt(cam.tgt);
  const close = shot && shot.name !== "wide" && shot.name !== "empty" ? 1 : 0;
  camFill += (close - camFill) * 0.05;
  const cl = stage.L.cam; cl.position.copy(camera.position).add(new THREE.Vector3(-0.15, 0.35, 0)); cl.target.position.copy(cam.tgt); cl.intensity = 10 * camFill;
  if (window.__noCam) cl.intensity = 0;
  const u = stage.bokeh.uniforms; u.focus.value = camera.position.distanceTo(focusPoint); u.aperture.value = aperture; u.maxblur.value = 0.009;
}
addEventListener("pointermove", (e) => { sway.tx = (e.clientX / innerWidth - 0.5) * 2; sway.ty = (e.clientY / innerHeight - 0.5) * 2; });

// ------------------------------------------------------------------ interaction
const ray = new THREE.Raycaster(), ndc = new THREE.Vector2();
const pickRoots = () => [ampHolder, boardGroup, micRig.root];
function pick(e) {
  ndc.set((e.clientX / innerWidth) * 2 - 1, -(e.clientY / innerHeight) * 2 + 1); ray.setFromCamera(ndc, camera);
  return ray.intersectObjects(pickRoots(), true).find((h) => isVisible(h.object) && !(h.object.material?.alphaTest > 0 && false));
}
function isVisible(o) { while (o) { if (!o.visible) return false; o = o.parent; } return true; }
const isAmpObj = (o) => { while (o) { if (o === ampHolder) return true; o = o.parent; } return false; };
const zoneOf = (hit) => (hit.object.userData.ctl?.kind === "mic" ? "cab" : (hit.point.y < amp.cabTop ? "cab" : "amp"));
const bubble = $("#bubble");
function showBubble(e, text) { bubble.textContent = text; bubble.style.left = e.clientX + "px"; bubble.style.top = e.clientY + "px"; bubble.classList.add("on"); clearTimeout(showBubble.t); }
function hideBubble(ms = 600) { clearTimeout(showBubble.t); showBubble.t = setTimeout(() => bubble.classList.remove("on"), ms); }

let drag = null, downAt = null;
const boardPlane = new THREE.Plane(new THREE.Vector3(0, 1, 0), -(BOARD_TOP + 0.03));
canvas.addEventListener("pointerdown", (e) => {
  if (!amp || shotAnim) return;
  const hit = pick(e); downAt = { x: e.clientX, y: e.clientY, hit };
  const c = hit?.object.userData.ctl;
  if (c && shot.name === "amp" && (c.kind === "ampKnob" || c.kind === "ampSlider")) {
    drag = { c, y: e.clientY, v0: c.kind === "ampSlider" ? (ampVals[c.id] + 12) / 24 : ampVals[c.id] }; canvas.setPointerCapture(e.pointerId); e.preventDefault(); showValue(c, e);
  } else if (c && shot.name === "pedal" && c.kind === "pedalCtl" && c.pedal === shot.arg && PEDAL[c.pedal].ctl[c.param].kind !== "bool") {
    drag = { c, y: e.clientY, v0: PEDAL[c.pedal].values[c.param] }; canvas.setPointerCapture(e.pointerId); e.preventDefault(); showValue(c, e);
  } else if (shot.name === "cab" && (c?.kind === "mic" || (hit && zoneOf(hit) === "cab"))) {
    drag = { c: { kind: "micDrag" }, x: e.clientX, y: e.clientY, p0: cabState.pos, d0: cabState.dist }; canvas.setPointerCapture(e.pointerId);
  } else if ((shot.name === "wide" || shot.name === "board") && hit?.object.userData.pedalId) {
    dragging = { st: PEDAL[hit.object.userData.pedalId], armed: true, pointerId: e.pointerId };
  }
});
canvas.addEventListener("pointermove", (e) => {
  if (drag) {
    if (drag.c.kind === "micDrag") {
      cabState.pos = THREE.MathUtils.clamp(drag.p0 + (e.clientX - drag.x) / 260, 0, 1);
      cabState.dist = THREE.MathUtils.clamp(drag.d0 - (e.clientY - drag.y) / -260, 0, 1);
      placeMic(); rebuildCables(); refreshCabPanel(); showBubble(e, `Posizione ${Math.round(cabState.pos * 100)}%  ·  Distanza ${micCm()} cm`); return;
    }
    setCtl(drag.c, THREE.MathUtils.clamp(drag.v0 + (drag.y - e.clientY) / (MOBILE ? 170 : 230), 0, 1)); showValue(drag.c, e); return;
  }
  if (dragging) {
    if (dragging.armed && Math.hypot(e.clientX - downAt.x, e.clientY - downAt.y) < 7) return;
    if (dragging.armed) { dragging.armed = false; canvas.setPointerCapture(dragging.pointerId); setHint("drag"); }
    ndc.set((e.clientX / innerWidth) * 2 - 1, -(e.clientY / innerHeight) * 2 + 1); ray.setFromCamera(ndc, camera);
    const p = new THREE.Vector3(); if (!ray.ray.intersectPlane(boardPlane, p)) return;
    p.x = THREE.MathUtils.clamp(p.x, BOARD.x - BOARD.w / 2, BOARD.x + BOARD.w / 2); p.z = THREE.MathUtils.clamp(p.z, BOARD.z - BOARD.d / 2, BOARD.z + BOARD.d / 2);
    dragging.st.grp.position.set(p.x, 0.03, p.z); rebuildCables(); return;
  }
  if (!amp || shotAnim) return;
  const hit = pick(e), c = hit?.object.userData.ctl; let cur = "default";
  if ((shot.name === "wide" || shot.name === "board") && hit) cur = hit.object.userData.pedalId ? "grab" : (c?.kind === "addSlot" ? "pointer" : "zoom-in");
  else if (shot.name === "cab" && hit && (c?.kind === "mic" || zoneOf(hit) === "cab")) cur = "move";
  else if (c && (c.kind === "ampKnob" || c.kind === "ampSlider" || c.kind === "pedalCtl")) cur = c.kind === "pedalCtl" && PEDAL[c.pedal].ctl[c.param].kind === "bool" ? "pointer" : "ns-resize";
  else if (c && (c.kind === "ampToggle" || c.kind === "footswitch" || c.kind === "addSlot")) cur = "pointer";
  else if (hit) cur = "zoom-in";
  canvas.style.cursor = cur;
});
canvas.addEventListener("pointerup", (e) => {
  if (drag) { const was = drag; drag = null; hideBubble(); if (was.c.kind !== "micDrag" || Math.hypot(e.clientX - was.x, e.clientY - was.y) > 6) return; }
  if (dragging && !dragging.armed) { const st = dragging.st; dragging = null; dropPedal(st); return; }
  dragging = null;
  if (!downAt || Math.hypot(e.clientX - downAt.x, e.clientY - downAt.y) > 7) return;
  click(e, downAt.hit);
});
canvas.addEventListener("dblclick", (e) => {
  const hit = pick(e), c = hit?.object.userData.ctl; if (!c) return;
  if ((c.kind === "ampKnob" || c.kind === "ampSlider") && shot.name === "amp") { setCtl(c, 0.5); showValue(c, e); hideBubble(800); }
  if (c.kind === "pedalCtl" && shot.name === "pedal") { const par = PEDAL[c.pedal].ctl[c.param].par; if (!par.isBool) { setCtl(c, toNorm(par, par.def)); showValue(c, e); hideBubble(800); } }
});
canvas.addEventListener("wheel", (e) => {
  if (shot?.name === "cab") { e.preventDefault(); cabState.dist = THREE.MathUtils.clamp(cabState.dist + Math.sign(e.deltaY) * 0.03, 0, 1); placeMic(); rebuildCables(); refreshCabPanel(); showBubble(e, `Distanza ${micCm()} cm`); hideBubble(700); return; }
  const hit = pick(e), c = hit?.object.userData.ctl; if (!c) return;
  const ok = ((c.kind === "ampKnob" || c.kind === "ampSlider") && shot.name === "amp") || (c.kind === "pedalCtl" && shot.name === "pedal" && shot.arg === c.pedal && PEDAL[c.pedal].ctl[c.param].kind !== "bool");
  if (!ok) return; e.preventDefault();
  const v = c.kind === "ampKnob" ? ampVals[c.id] : c.kind === "ampSlider" ? (ampVals[c.id] + 12) / 24 : PEDAL[c.pedal].values[c.param];
  const par = c.kind === "pedalCtl" ? PEDAL[c.pedal].ctl[c.param].par : null, step = par?.choices ? 1 / (par.choices.length - 1) : 0.02;
  setCtl(c, THREE.MathUtils.clamp(v - Math.sign(e.deltaY) * step, 0, 1)); showValue(c, e); hideBubble(700);
}, { passive: false });
const micCm = () => Math.round(1 + cabState.dist * 29);

function setCtl(c, v) {
  shadowsDirty(2);
  changed();
  if (c.kind === "ampKnob") { ampVals[c.id] = v; applyAmpVals(amp); }
  else if (c.kind === "ampSlider") { ampVals[c.id] = Math.round((v * 24 - 12) * 2) / 2; applyAmpVals(amp); }
  else { const st = PEDAL[c.pedal], par = st.ctl[c.param].par; st.values[c.param] = par.choices ? Math.round(v * (par.choices.length - 1)) / (par.choices.length - 1) : v; PF.apply(st); refreshPanel(); }
}
function showValue(c, e) {
  if (c.kind === "ampKnob") showBubble(e, `${amp.knobs[c.id].label}  ${(ampVals[c.id] * 10).toFixed(1)}`);
  else if (c.kind === "ampSlider") showBubble(e, `EQ ${amp.sliders[c.id].label}  ${ampVals[c.id] > 0 ? "+" : ""}${ampVals[c.id].toFixed(1)} dB`);
  else { const st = PEDAL[c.pedal], par = st.ctl[c.param].par; showBubble(e, `${par.name}  ${fmt(par, toValue(par, st.values[c.param]))}`); }
}
function click(e, hit) {
  const c = hit?.object.userData.ctl;
  if (shot.name === "wide" || shot.name === "board") {
    if (c?.kind === "addSlot") return openPedalCatalog(c.row);
    if (hit?.object.userData.pedalId) return openPedal(hit.object.userData.pedalId);
    if (hit && (isAmpObj(hit.object) || c?.kind === "mic")) return zoneOf(hit) === "cab" ? openCab() : openAmp();
    return;
  }
  if (shot.name === "amp") {
    if (c?.kind === "ampToggle") {
      const t = amp.toggles[c.id]; ampVals[c.id] = (ampVals[c.id] + 1) % t.n; applyAmpVals(amp);
      showBubble(e, `${t.def.label}  ${t.def.states[ampVals[c.id]]}`); hideBubble(900); return;
    }
    if (!hit) return back();
    if (hit.object.userData.pedalId) return openPedal(hit.object.userData.pedalId);
    if (isAmpObj(hit.object) && zoneOf(hit) === "cab") return openCab();
    return;
  }
  if (shot.name === "pedal") {
    if (c?.kind === "footswitch" && c.pedal === shot.arg) { const st = PEDAL[c.pedal]; st.on = !st.on; PF.apply(st); refreshPanel(); changed(); return; }
    if (c?.kind === "pedalCtl" && c.pedal === shot.arg) {
      const st = PEDAL[c.pedal]; if (st.ctl[c.param].kind === "bool") { st.values[c.param] = st.values[c.param] >= 0.5 ? 0 : 1; PF.apply(st); refreshPanel(); showValue(c, e); hideBubble(800); } return;
    }
    if (hit?.object.userData.pedalId) return openPedal(hit.object.userData.pedalId);
    if (hit && isAmpObj(hit.object)) return zoneOf(hit) === "cab" ? openCab() : openAmp();
    if (!hit) return back();
  }
  if (shot.name === "cab") {
    if (hit && isAmpObj(hit.object) && zoneOf(hit) === "amp") return openAmp();
    if (hit?.object.userData.pedalId) return openPedal(hit.object.userData.pedalId);
    if (!hit) return back();
  }
}
const LOOP_ONLY = new Set(["delay", "reverb"]);
function dropPedal(st) {
  const p = st.grp.position;
  let row = p.z > BOARD.z ? "pre" : "loop";
  if (row === "pre" && LOOP_ONLY.has(st.def.id)) { row = "loop"; say(st.def.name + " lavora solo nel loop FX, dopo l'ampli: lì ha senso (le ripetizioni non vengono distorte)."); }
  const from = chain[st.row]; from.splice(from.indexOf(st.def.id), 1);
  const idx = chain[row].map((id) => PEDAL[id].grp.position.x).filter((x) => x < p.x).length;
  if (rowLayout(row, [...chain[row], st.def.id]).free < -0.001) { from.splice(st.index, 0, st.def.id); say("Quella fila è piena: sposta prima un altro pedale."); }
  else chain[row].splice(idx, 0, st.def.id);
  layoutChain(); setHint(shot.name === "board" ? "board" : "wide"); changed();
}

// ------------------------------------------------------------------ UI flow
const HINTS = {
  empty: "Il palco è vuoto: scegli l'amplificatore.",
  wide: "Tocca la testata, la cassa o un pedale per avvicinarti. Trascina i pedali per cambiare l'ordine della catena.",
  drag: "Rilascia il pedale dove vuoi: fila davanti = prima dell'ampli, fila dietro = loop FX.",
  board: "Trascina un pedale per cambiarne il posto nella catena · toccalo per avvicinarti · + per aggiungerne uno",
  amp: "Trascina su/giù le manopole (o rotella) · doppio clic = valore iniziale · tocca BRIGHT e CHANNEL · Esc per allontanarti",
};
function setHint(k) { $("#hint").textContent = HINTS[k] || ""; }
function say(t) { const el = $("#toast"); el.textContent = t; el.classList.add("on"); clearTimeout(say.t); say.t = setTimeout(() => el.classList.remove("on"), 2800); }
function openAmp() { if (!amp) return; clearHud(); goShot("amp"); setHint("amp"); showTitle(amp.def.name, amp.def.tags.join("  ·  ")); }
function back() { closePanel(); goShot("wide"); setHint("wide"); }
addEventListener("keydown", (e) => {
  if (e.key !== "Escape") return;
  if ($("#cat").classList.contains("open")) return $("#cat").classList.remove("open");
  if (shot && shot.name !== "wide" && shot.name !== "empty") back();
});
$("#cat").addEventListener("click", (e) => { if (e.target.id === "cat") $("#cat").classList.remove("open"); });
$("#cta").addEventListener("click", openAmpCatalog);

const thumbs = {};
function openAmpCatalog() {
  const box = $("#catCards"); box.innerHTML = "";
  $("#catTitle").textContent = "Scegli l'amplificatore";
  $("#catSub").textContent = "Ogni ampli è un modello 3D con gli stessi comandi del plugin. Le manopole restano dove le hai lasciate quando cambi ampli.";
  for (const a of AMPS) {
    const el = document.createElement("article"); const on = amp?.def.id === a.id; el.className = "card" + (on ? " off" : "");
    el.innerHTML = `<div class="pic">${thumbs[a.id] ? `<img alt="" src="${thumbs[a.id]}">` : `<span class="soon">Carico il modello…</span>`}</div>
      <div class="txt"><h3>${a.name}</h3><div class="tags">${a.tags.map((t) => `<span>${t}</span>`).join("")}</div>
      ${a.text.map((t) => `<p>${t}</p>`).join("")}<div class="state">${on ? "SUL PALCO" : "TOCCA PER METTERLO SUL PALCO ▸"}</div></div>`;
    if (!on) el.addEventListener("click", () => { $("#cat").classList.remove("open"); placeAmp(a.id); });
    box.appendChild(el);
  }
  $("#cat").classList.add("open");
}
let rise = null;
async function placeAmp(id) {
  const def = AMPS.find((a) => a.id === id);
  if (!AMP[id]) { say("Carico " + def.name + "…"); await loadAmp(def); thumbs[id] = ampThumb(AMP[id]); }
  const next = AMP[id], prev = amp;
  closePanel();
  $("#cta").classList.add("gone");
  cabState.type = CABS.indexOf(def.cab);
  if (prev) { rise = { t0: performance.now(), dur: REDUCED ? 1 : 700, out: prev, then: () => enter(next) }; }
  else enter(next);
  function enter(a) {
    if (prev) ampHolder.remove(prev.root);
    amp = a; ampHolder.add(a.root); applyAmpVals(a);
    a.root.position.y = -0.9;
    if (!boardGroup.visible) {
      boardGroup.visible = true;
      for (const pid of ["gate", "od", "delay", "reverb"]) { pedalState(pid); PF.apply(PEDAL[pid]); }
      chain.pre = ["gate", "od"]; chain.loop = ["delay", "reverb"];
      layoutChain(true); for (const pid of [...chain.pre, ...chain.loop]) PEDAL[pid].grp.position.y = 0;
    }
    micRig.root.visible = true; placeMic(); rebuildCables();
    rise = { t0: performance.now(), dur: REDUCED ? 1 : 1500, in: a, first: !prev };
    goShot("wide"); setHint("wide");
  }
}

// Pedal catalogue
let catRow = "pre";
function openPedalCatalog(row) {
  catRow = row; const box = $("#catCards"); box.innerHTML = "";
  $("#catTitle").textContent = row === "pre" ? "Aggiungi un pedale: ingresso" : "Aggiungi un pedale: loop FX";
  $("#catSub").textContent = row === "pre" ? "Prima dell'ampli: cambia quanto e come satura." : "Dopo il preamp: modulazioni, delay e riverbero restano puliti anche col gain alto.";
  const used = new Set([...chain.pre, ...chain.loop]);
  for (const d of PEDALS) {
    const inUse = used.has(d.id), el = document.createElement("article"); el.className = "card" + (inUse ? " off" : "");
    el.innerHTML = `<div class="pic"><img alt="" src="${pedalThumb(d.id)}"></div>
      <div class="txt"><h3>${d.name}</h3><div class="tags">${d.params.map((q) => `<span>${q.name}</span>`).join("")}</div>
      <p>${d.desc}</p><div class="state">${inUse ? "GIÀ SULLA PEDALIERA" : "TOCCA PER AGGIUNGERLO ▸"}</div></div>`;
    if (!inUse) el.addEventListener("click", () => { $("#cat").classList.remove("open"); addPedal(d.id, catRow); });
    box.appendChild(el);
  }
  $("#cat").classList.add("open");
}
function addPedal(id, row) {
  const st = pedalState(id); PF.apply(st);
  if (row === "pre" && LOOP_ONLY.has(id)) row = "loop";
  if (rowLayout(row, [...chain[row], id]).free < -0.001) { say("Questa fila è piena."); return; }
  chain[row].push(id); layoutChain(); changed(); setTimeout(() => openPedal(id), 480);
}

// ------------------------------------------------------------------ in-scene HUD
// No side menus: controls live on the objects. Small glass chips are pinned to 3D points
// (projected every frame), arrows sit beside the pedal, the dock at the bottom navigates.
const hud = $("#hud"), titleEl = $("#title"), dock = $("#dock"), tray = $("#tray");
let anchors = [];
const ICON = {
  left: '<svg viewBox="0 0 24 24"><path d="M15 5l-7 7 7 7"/></svg>',
  right: '<svg viewBox="0 0 24 24"><path d="M9 5l7 7-7 7"/></svg>',
  swap: '<svg viewBox="0 0 24 24"><path d="M7 7h11l-3-3M17 17H6l3 3"/></svg>',
  trash: '<svg viewBox="0 0 24 24"><path d="M5 7h14M10 7V4h4v3M7 7l1 13h8l1-13"/></svg>',
  power: '<svg viewBox="0 0 24 24"><path d="M12 3v9M6.3 6.3a8 8 0 1 0 11.4 0"/></svg>',
  upload: '<svg viewBox="0 0 24 24"><path d="M12 16V4M7 9l5-5 5 5M5 20h14"/></svg>',
};
function clearHud() { hud.innerHTML = ""; anchors = []; titleEl.classList.remove("on"); tray.classList.remove("on"); tray.innerHTML = ""; delete hud.dataset.pedal; }
function pin(el, getPoint, dx = 0, dy = 0) { hud.appendChild(el); anchors.push({ el, getPoint, dx, dy }); return el; }
function updateAnchors() {
  const v = new THREE.Vector3();
  for (const a of anchors) {
    v.copy(a.getPoint()).project(camera);
    const x = (v.x + 1) / 2 * innerWidth + a.dx, y = (1 - v.y) / 2 * innerHeight + a.dy;
    a.el.style.transform = `translate(${x}px, ${y}px) translate(-50%, -50%)`;
    a.el.style.opacity = shotAnim ? 0 : 1;
  }
}
function showTitle(name, sub) { titleEl.innerHTML = `<b>${name}</b><span>${sub}</span>`; titleEl.classList.add("on"); }
const chip = (cls, html) => { const d = document.createElement("div"); d.className = "chip " + cls; d.innerHTML = html; return d; };
const ibtn = (icon, label, cls = "") => { const b = document.createElement("button"); b.className = "ibtn " + cls; b.innerHTML = ICON[icon] + (label ? `<span>${label}</span>` : ""); b.setAttribute("aria-label", label || icon); return b; };

function openPedal(id) { goShot("pedal", id); buildPedalHud(id); }
function buildPedalHud(id) {
  clearHud(); hud.dataset.pedal = id;
  const st = PEDAL[id], d = st.def, wp = () => st.grp.getWorldPosition(new THREE.Vector3());
  showTitle(d.name, d.desc);
  const halfW = d.w / 2 + 0.024, D = d.d || PD;
  const L = pin(ibtn("left", "", "round"), () => wp().add(new THREE.Vector3(-halfW, BOARD_TOP + 0.03, 0)));
  const R = pin(ibtn("right", "", "round"), () => wp().add(new THREE.Vector3(halfW, BOARD_TOP + 0.03, 0)));
  L.title = "Sposta prima nella catena"; R.title = "Sposta dopo nella catena";
  L.onclick = () => reorder(id, "left"); R.onclick = () => reorder(id, "right");
  // Value chips under every control: tap a switch chip to flip it.
  for (const q of d.params) {
    const c = st.ctl[q.id]; if (!c) continue;
    const el = pin(chip("val", ""), () => c.obj.getWorldPosition(new THREE.Vector3()).add(new THREE.Vector3(0, 0, -0.016)), 0, -26);
    el.dataset.p = q.id;
    if (q.isBool || q.choices) { el.classList.add("tap"); el.onclick = () => { const k = q.choices ? q.choices.length - 1 : 1; st.values[q.id] = (Math.round(st.values[q.id] * k) + 1) % (k + 1) / k; PF.apply(st); refreshHud(); }; }
  }
  // Under the pedal: position in the chain, move between rows, remove.
  const bar = pin(document.createElement("div"), () => wp().add(new THREE.Vector3(0, BOARD_TOP, D / 2 + 0.03)));
  bar.className = "hudbar";
  const where = document.createElement("span"); where.className = "where"; bar.appendChild(where);
  const sw = ibtn("swap", "", ""); sw.onclick = () => reorder(id, "row"); bar.appendChild(sw);
  const pw = ibtn("power", "", "pw"); pw.onclick = () => { st.on = !st.on; PF.apply(st); refreshHud(); }; bar.appendChild(pw);
  const rm = ibtn("trash", "", "danger"); rm.title = "Togli dalla pedaliera"; rm.onclick = () => reorder(id, "remove"); bar.appendChild(rm);
  refreshHud();
}
function refreshHud() {
  const id = hud.dataset.pedal; if (!id) return;
  const st = PEDAL[id], d = st.def;
  for (const q of d.params) { const el = hud.querySelector(`[data-p="${q.id}"]`); if (el) el.innerHTML = `<i>${q.name}</i>${fmt(q, toValue(q, st.values[q.id]))}`; }
  const arr = chain[st.row];
  hud.querySelector(".where").innerHTML = `<b>${st.row === "pre" ? "INGRESSO" : "LOOP FX"}</b> ${st.index + 1}/${arr.length}`;
  const sw = hud.querySelector(".hudbar .ibtn:not(.pw):not(.danger)"); sw.innerHTML = ICON.swap + `<span>${st.row === "pre" ? "Loop FX" : "Ingresso"}</span>`;
  hud.querySelector(".pw").classList.toggle("on", st.on); hud.querySelector(".pw").innerHTML = ICON.power + `<span>${st.on ? "Acceso" : "Bypass"}</span>`;
  const btns = hud.querySelectorAll(".ibtn.round"); btns[0].disabled = st.index === 0; btns[1].disabled = st.index === arr.length - 1;
}
const refreshPanel = refreshHud;
function closePanel() { clearHud(); }
function reorder(id, a) {
  const st = PEDAL[id], arr = chain[st.row], i = arr.indexOf(id);
  if (a === "left" && i > 0) { arr.splice(i, 1); arr.splice(i - 1, 0, id); }
  else if (a === "right" && i < arr.length - 1) { arr.splice(i, 1); arr.splice(i + 1, 0, id); }
  else if (a === "row") { const to = st.row === "pre" ? "loop" : "pre"; if (to === "pre" && LOOP_ONLY.has(id)) return say(st.def.name + " lavora solo nel loop FX, dopo l'ampli."); if (rowLayout(to, [...chain[to], id]).free < -0.001) return say("L'altra fila è piena."); arr.splice(i, 1); chain[to].push(id); }
  else if (a === "remove") { arr.splice(i, 1); boardGroup.remove(st.grp); layoutChain(); changed(); back(); return; }
  layoutChain(); refreshHud(); changed();
  // The camera follows the pedal to its new place.
  setTimeout(() => { if (hud.dataset.pedal === id) goShot("pedal", id); }, 580);
}
function openCab() {
  if (!amp) return;
  goShot("cab"); clearHud();
  showTitle("Cassa e microfono", "Trascina il microfono: destra/sinistra = dal centro al bordo del cono · su/giù o rotella = distanza");
  const read = pin(chip("val mic", ""), () => micRig.mics[cabState.mic].getWorldPosition(new THREE.Vector3()), 0, -46); read.id = "micRead";
  tray.innerHTML = `<div class="mics">${MICS.map((m) => `<button class="micbtn" data-v="${m.id}"><img alt="" src="${micThumb(m.id)}"><span>${m.name}</span></button>`).join("")}</div>
    <div class="sep"></div>
    <label class="sel"><i>Risposta</i><select data-k="type">${CABS.map((c, i) => `<option value="${i}">${c}</option>`).join("")}</select></label>
    <label class="ibtn up" data-k="irw">${ICON.upload}<span>IR .wav</span><input type="file" accept=".wav,audio/wav" data-k="ir" hidden></label>
    <button class="ibtn pw" data-k="on">${ICON.power}<span></span></button>`;
  tray.classList.add("on");
  tray.querySelectorAll(".micbtn").forEach((b) => b.addEventListener("click", () => { cabState.mic = b.dataset.v; placeMic(); rebuildCables(); refreshCabPanel(); }));
  tray.querySelector('[data-k="type"]').addEventListener("change", (e) => { cabState.type = +e.target.value; refreshCabPanel(); });
  // In the plugin the engine loads the IR from disk, so the system file dialog is opened natively.
  tray.querySelector('[data-k="ir"]').addEventListener("click", (e) => { if (PLUGIN) { e.preventDefault(); JB.emitEvent("rig", { type: "pickIR" }); } });
  tray.querySelector('[data-k="ir"]').addEventListener("change", (e) => { const f = e.target.files[0]; if (f) { cabState.ir = f.name; cabState.type = 3; refreshCabPanel(); say("IR caricato: " + f.name); } });
  tray.querySelector('[data-k="on"]').addEventListener("click", () => { cabState.on = !cabState.on; refreshCabPanel(); });
  refreshCabPanel();
}
function refreshCabPanel() {
  const r = $("#micRead");
  if (r) r.innerHTML = `<i>${MICS.find((m) => m.id === cabState.mic).name}</i>${cabState.pos < 0.08 ? "centro" : cabState.pos > 0.92 ? "bordo" : Math.round(cabState.pos * 100) + "%"} · ${micCm()} cm`;
  tray.querySelectorAll(".micbtn").forEach((b) => b.classList.toggle("sel", b.dataset.v === cabState.mic));
  const t = tray.querySelector('[data-k="type"]'); if (t) t.value = cabState.type;
  const on = tray.querySelector('[data-k="on"]'); if (on) { on.classList.toggle("on", cabState.on); on.querySelector("span").textContent = cabState.on ? "Cassa ON" : "Cassa OFF"; }
}
function micThumb(id) {
  if (thumbs["m_" + id]) return thumbs["m_" + id];
  const m = micRig.mics[id], vis = m.visible; m.visible = true;
  const c = m.userData.vertical ? new THREE.Vector3(0, -0.04, 0.028) : new THREE.Vector3(0, 0, m.userData.len / 2);
  const url = snapshot(m, c, m.userData.vertical ? 0.42 : 0.36, 1.2, 0.9);
  m.visible = vis; return (thumbs["m_" + id] = url);
}

// ------------------------------------------------------------------ rig state (presets, A/B)
// A rig is everything that defines the sound: amp and its controls, the pedal chain in order with each
// pedal's on/off and settings, cab and mic. Presets and A/B slots are snapshots of this object.
function snapshotRig() {
  const pedals = {};
  for (const id of [...chain.pre, ...chain.loop]) pedals[id] = { on: PEDAL[id].on, values: { ...PEDAL[id].values } };
  const { ir, ...cab } = cabState;
  return { amp: amp?.def.id || null, ampVals: { ...ampVals }, chain: { pre: [...chain.pre], loop: [...chain.loop] }, pedals, cab };
}
const AMP_DEFAULTS = { ...ampVals };
async function applyRig(r) {
  if (r.amp && (!amp || amp.def.id !== r.amp)) { await placeAmp(r.amp); await new Promise((ok) => { const w = () => (rise ? setTimeout(w, 60) : ok()); w(); }); }
  Object.assign(ampVals, AMP_DEFAULTS, r.ampVals || {}); if (amp) applyAmpVals(amp);
  const keep = new Set([...r.chain.pre, ...r.chain.loop]);
  for (const id of [...chain.pre, ...chain.loop]) if (!keep.has(id)) { boardGroup.remove(PEDAL[id].grp); PEDAL[id].on = false; }
  chain.pre = [...r.chain.pre]; chain.loop = [...r.chain.loop];
  for (const id of keep) {
    const st = pedalState(id), p = r.pedals?.[id];
    for (const q of st.def.params) st.values[q.id] = p?.values?.[q.id] ?? toNorm(q, q.def);
    st.on = p ? !!p.on : !!r.on?.[id]; PF.apply(st);
  }
  if (r.cab) { Object.assign(cabState, r.cab); placeMic(); }
  layoutChain(); refreshHud?.();
}
const rack = buildRack($("#rack"), { snapshot: snapshotRig, applyRig, say });
const changed = () => rack.markDirty();

// ------------------------------------------------------------------ plugin bridge
// Inside the plugin (JUCE web view) the audio engine is the source of truth. The scene sends the
// parameter values it wants (normalised 0..1, same ids as the plugin) and redraws what the engine
// reports: host automation, presets, A/B and the 2D editor all move the 3D controls too.
const JB = window.__JUCE__?.backend || null;
const PLUGIN = !!JB;
const AMP_KEYS = Object.keys(ampVals);
const ampNorm = (k, v) => k.startsWith("eq") ? (v + 12) / 24 : k === "channel" ? v / 2 : v;
const ampReal = (k, n) => k.startsWith("eq") ? n * 24 - 12 : k === "channel" ? Math.round(n * 2) : (k === "bright" || k === "tight" || k === "standby" || k === "power") ? (n >= 0.5 ? 1 : 0) : n;
const LEVELS = { in_gain: { min: -24, max: 24, centre: 0 }, out_vol: { min: -48, max: 12, centre: 0 } };   // as in Params.cpp
function sceneParams() {
  const v = {};
  if (amp) { v.a_model = amp.def.model / 4; for (const k of AMP_KEYS) v["a_" + k] = ampNorm(k, ampVals[k]); }
  v.a_cab_on = cabState.on ? 1 : 0; v.a_cab = cabState.type / 3; v.a_mic = MICS.findIndex((m) => m.id === cabState.mic) / 2;
  v.a_mic_pos = cabState.pos; v.a_mic_dist = cabState.dist;
  const onBoard = new Set([...chain.pre, ...chain.loop]);
  for (const d of PEDALS) {
    const st = PEDAL[d.id];
    v[d.id + "_on"] = onBoard.has(d.id) && st?.on ? 1 : 0;
    if (st && onBoard.has(d.id)) for (const q of d.params) v[d.id + "_" + q.id] = st.values[q.id];
  }
  const L = rack.levels();
  for (const k of ["in_gain", "out_vol"]) v[k] = toNorm(LEVELS[k], L[k === "in_gain" ? "in" : "out"]);
  return v;
}
const eng = {};            // last values known to be in the engine
let engChain = "", bridgeLive = false, applying = false;
function rigFromEngine(chainJson, routing) {
  let ch = null;
  try { ch = chainJson ? JSON.parse(chainJson) : null; } catch (e) { ch = null; }
  if (!ch) {   // no 3D board saved (preset, old session, 2D editor): show the pedals that are on, in signal order
    const on = (id) => (eng[id + "_on"] ?? 0) >= 0.5;
    ch = { pre: (routing?.board || []).filter(on), loop: (routing?.loop || []).filter(on) };
  }
  const pedals = {};
  for (const id of [...ch.pre, ...ch.loop]) {
    const d = PEDALS.find((x) => x.id === id); if (!d) continue;
    const values = {}; for (const q of d.params) values[q.id] = eng[id + "_" + q.id] ?? toNorm(q, q.def);
    pedals[id] = { on: (eng[id + "_on"] ?? 0) >= 0.5, values };
  }
  const av = {}; for (const k of AMP_KEYS) if (eng["a_" + k] != null) av[k] = ampReal(k, eng["a_" + k]);
  const model = Math.round((eng.a_model ?? 0) * 4);
  return { amp: (AMPS.find((a) => a.model === model) || AMPS[0]).id, ampVals: av, chain: ch, pedals, cab: cabFromEngine() };
}
const cabFromEngine = () => ({ on: (eng.a_cab_on ?? 1) >= 0.5, type: Math.round((eng.a_cab ?? 0) * 3), mic: MICS[Math.round((eng.a_mic ?? 0) * 2)].id, pos: eng.a_mic_pos ?? 0.3, dist: eng.a_mic_dist ?? 0.2 });
function levelsFromEngine() {
  const r = (k) => eng[k] == null ? 0 : Math.round(toValue(LEVELS[k], eng[k]) * 2) / 2;
  rack.setLevels({ in: r("in_gain"), out: r("out_vol") });
}
let lastSent = {}, lastRoute = "";
async function applyEngine(full, msg) {
  applying = true;
  try {
    const prevModel = Math.round((eng.a_model ?? -1) * 4), prevOn = PEDALS.map((d) => eng[d.id + "_on"]).join();
    Object.assign(eng, msg.values || {});
    if (full) { engChain = msg.chain || ""; window.__rigRouting = msg.routing; }
    const modelChanged = Math.round(eng.a_model * 4) !== prevModel;
    // A pedal switched on from outside (automation, 2D editor, MIDI) that is not on the 3D board yet is
    // added to it; switching one off never removes it from the board.
    const onBoard = new Set([...chain.pre, ...chain.loop]), r = window.__rigRouting || { board: [], loop: [] };
    const arriving = PEDALS.filter((d) => msg.values[d.id + "_on"] >= 0.5 && !onBoard.has(d.id)).map((d) => d.id);
    if (!full && arriving.length) {
      const ch = { pre: [...chain.pre], loop: [...chain.loop] };
      for (const id of arriving) (r.loop.includes(id) ? ch.loop : ch.pre).push(id);
      engChain = JSON.stringify(ch);
    }
    if (full || modelChanged || arriving.length) await applyRig(rigFromEngine(engChain, window.__rigRouting));
    else {   // automation of single controls: move them in place, no relayout
      for (const k of AMP_KEYS) if (msg.values["a_" + k] != null) ampVals[k] = ampReal(k, eng["a_" + k]);
      if (amp) applyAmpVals(amp);
      for (const id of [...chain.pre, ...chain.loop]) {
        const st = PEDAL[id], d = st.def; let touched = false;
        if (msg.values[id + "_on"] != null) { st.on = eng[id + "_on"] >= 0.5; touched = true; }
        for (const q of d.params) if (msg.values[id + "_" + q.id] != null) { st.values[q.id] = eng[id + "_" + q.id]; touched = true; }
        if (touched) PF.apply(st);
      }
      if (["a_cab_on", "a_cab", "a_mic", "a_mic_pos", "a_mic_dist"].some((k) => msg.values[k] != null)) { Object.assign(cabState, cabFromEngine()); placeMic(); }
      refreshHud?.();
    }
    levelsFromEngine();
    if (full) rack.sync({ presets: msg.presets, preset: msg.preset, ab: msg.ab });
    lastSent = sceneParams(); lastRoute = routeKey();
    // No board saved with the project yet (new instance, preset, old session): save the one now shown,
    // so later changes keep exactly these pedals in this order.
    if (!engChain || arriving.length) { engChain = lastRoute; lastRoute = ""; }
    shadowsDirty(3);
  } finally { applying = false; }
}
const routeKey = () => JSON.stringify(chain);
function bridgeTick() {
  if (!bridgeLive || applying) return;
  const v = sceneParams(), out = {}; let n = 0;
  for (const k in v) if (Math.abs((lastSent[k] ?? -1) - v[k]) > 1e-4) { out[k] = v[k]; lastSent[k] = v[k]; n++; }
  if (n) JB.emitEvent("rig", { type: "params", values: out });
  const rk = routeKey();
  if (rk !== lastRoute) { lastRoute = rk; JB.emitEvent("rig", { type: "routing", board: chain.pre, loop: chain.loop, chain: rk }); }
}
if (PLUGIN) {
  rack.remote({ load: (i) => JB.emitEvent("rig", { type: "preset", index: i }), save: (name) => JB.emitEvent("rig", { type: "savePreset", name }),
    ab: (s) => JB.emitEvent("rig", { type: "ab", slot: s === "B" ? 1 : 0 }), copy: () => JB.emitEvent("rig", { type: "abCopy" }) });
  JB.addEventListener("engine", (m) => {
    if (m.type === "meters") { rack.meters({ in: m.in, out: m.out }); if (m.hz != null) rack.pitch(m.hz); return; }
    if (m.type === "state") { const first = !bridgeLive; rack.run(() => applyEngine(true, m)).then(() => { bridgeLive = true; if (first) setHint("wide"); }); return; }
    if (m.type === "params") rack.run(() => applyEngine(false, m));
  });
  setInterval(bridgeTick, 40);
  const cb = $("#classic"); cb.hidden = false;
  cb.addEventListener("click", () => { JB.emitEvent("rig", { type: "classicUi" }); say("Chiudi e riapri la finestra di ToxicRig: si aprirà l'interfaccia 2D."); });
}
window.__bridge = { PLUGIN, eng, sceneParams };

// Dock: the four places of the stage, always one tap away.
function buildDock() {
  const items = [["wide", "Palco"], ["amp", "Ampli"], ["cab", "Cassa"], ["board", "Pedaliera"]];
  dock.innerHTML = items.map(([k, l]) => `<button data-k="${k}"><span class="dot"></span>${l}</button>`).join("") + `<button data-k="cat" class="alt">Cambia ampli</button>`;
  dock.querySelectorAll("button").forEach((b) => b.addEventListener("click", () => {
    const k = b.dataset.k;
    if (k === "wide") back(); else if (k === "amp") openAmp(); else if (k === "cab") openCab(); else if (k === "board") { clearHud(); goShot("board"); setHint("board"); } else openAmpCatalog();
  }));
}
function syncDock() { dock.querySelectorAll("button").forEach((b) => b.classList.toggle("on", shot && (b.dataset.k === shot.name || (b.dataset.k === "board" && shot.name === "pedal")))); }

// ------------------------------------------------------------------ thumbnails
let thumbR;
function thumbRenderer() {
  if (thumbR) return thumbR;
  const r = new THREE.WebGLRenderer({ antialias: true, alpha: true, preserveDrawingBuffer: true });
  r.setSize(420, 280); r.toneMapping = THREE.AgXToneMapping; r.outputColorSpace = THREE.SRGBColorSpace;
  const s = new THREE.Scene(); s.environment = scene.environment; s.environmentIntensity = 0.9;
  const l = new THREE.DirectionalLight(0xffe8d0, 3.4); l.position.set(-1, 2, 2); s.add(l);
  const l2 = new THREE.DirectionalLight(0x9b4dff, 2.4); l2.position.set(2, 1, -1); s.add(l2);
  const l3 = new THREE.DirectionalLight(0x39ff14, 1.2); l3.position.set(-2, 0.6, -1); s.add(l3);
  thumbR = { r, s, c: new THREE.PerspectiveCamera(28, 1.5, 0.01, 20) }; return thumbR;
}
function snapshot(obj, center, dist, phi, theta) {
  const T = thumbRenderer(), parent = obj.parent, pos = obj.position.clone();
  T.s.add(obj); obj.position.set(0, 0, 0);
  T.c.position.setFromSpherical(new THREE.Spherical(dist, phi, theta)).add(center); T.c.lookAt(center);
  T.r.render(T.s, T.c); const url = T.r.domElement.toDataURL("image/png");
  T.s.remove(obj); if (parent) parent.add(obj); obj.position.copy(pos); return url;
}
function ampThumb(a) { const s = a.box.getBoundingSphere(new THREE.Sphere()); return snapshot(a.root, s.center, s.radius / Math.sin(THREE.MathUtils.degToRad(13)), 1.38, -0.45); }
function pedalThumb(id) {
  if (thumbs["p_" + id]) return thumbs["p_" + id];
  const st = pedalState(id); PF.apply(st);
  return (thumbs["p_" + id] = snapshot(st.grp, new THREE.Vector3(0, BOARD_TOP + PH / 2, 0), st.def.w > 0.1 ? 0.38 : 0.32, 0.9, -0.45));
}

// ------------------------------------------------------------------ loop
function resize() {
  const w = innerWidth, h = innerHeight;
  renderer.setSize(w, h, false); stage.composer.setPixelRatio(renderer.getPixelRatio()); stage.composer.setSize(w, h);
  camera.aspect = w / h; camera.fov = w < h ? 42 : 30; camera.updateProjectionMatrix();
  stage.dust.material.uniforms.px.value = renderer.getPixelRatio();
  if (shot && (amp || shot.name === "empty")) goShot(shot.name, shot.arg, true);
}
addEventListener("resize", resize);

let frameNo = 0, lastFrame = 0, frameMs = 16.7, lastAdapt = 0;
function frame(now) {
  requestAnimationFrame(frame);
  if (document.hidden) return;                       // plugin window hidden or minimised: draw nothing
  const busy = rise || anims.length || shotAnim || now - lastInput < 1500;
  // In the plugin, a still scene is drawn at 30 fps: the DAW and the audio get the time back.
  if (window.__bridge?.PLUGIN && !busy && (++frameNo & 1)) return;
  const dt = lastFrame ? now - lastFrame : 16.7; lastFrame = now;
  if (dt < 250) frameMs += (dt / (busy || !window.__bridge?.PLUGIN ? 1 : 2) - frameMs) * 0.05;
  // Adaptive resolution: frames slower than ~42 fps lower the pixel ratio a step; fast frames raise it back.
  if (now - lastAdapt > 1500) {
    lastAdapt = now;
    const next = frameMs > 24 ? Math.max(0.6, curRatio - 0.15) : frameMs < 14 ? Math.min(maxRatio, curRatio + 0.1) : curRatio;
    if (Math.abs(next - curRatio) > 0.01) { curRatio = next; renderer.setPixelRatio(curRatio); resize(); }
  }
  if (rise || anims.length) shadowsDirty(1);
  if (shadowFrames > 0) { renderer.shadowMap.needsUpdate = true; shadowFrames--; }
  const t = now / 1000;
  if (rise) {
    const k = Math.min(1, (now - rise.t0) / rise.dur);
    if (rise.out) { const e = k * k; rise.out.root.position.y = -0.9 * e; if (k >= 1) { const f = rise.then; rise = null; f(); } }
    else {
      const e = 1 - Math.pow(1 - k, 3);
      rise.in.root.position.y = -0.9 * (1 - e);
      if (rise.first) { boardGroup.position.y = -0.25 * (1 - e); micRig.root.position.y = -0.6 * (1 - e); stage.setLights(0.2 + 0.8 * e); }
      if (k >= 1) { rise = null; rebuildCables(); }
    }
  }
  for (let i = anims.length - 1; i >= 0; i--) {
    const a = anims[i], k = Math.min(1, (now - a.t0) / a.dur), e = ease(k);
    a.obj.position.lerpVectors(a.from, a.to, e); a.obj.position.y += Math.sin(Math.PI * e) * 0.045;
    if (k >= 1) anims.splice(i, 1);
  }
  if (anims.length) { rebuildCables(); rebuildCables.pending = true; } else if (rebuildCables.pending) { rebuildCables.pending = false; rebuildCables(); }
  const mm = PEDAL.mutant;
  if (mm?.extras?.tubeGlow) {
    // The tube pulses with the LFO (in the plugin the phase comes from the audio engine).
    const hz = toValue(mm.def.params[1], mm.values.speed);
    mm.phase = ((mm.phase || 0) + hz * (now - (mm.last || now)) / 1000) % 1; mm.last = now;
    const k = mm.on ? 1.2 + 3.8 * (0.5 + 0.5 * Math.sin(mm.phase * Math.PI * 2)) : 0.25;
    mm.extras.tubeGlow[0].emissiveIntensity = k; mm.extras.tubeGlow[1].emissiveIntensity = k * 0.15;
  }
  if (amp) {
    const fl = 1 + Math.sin(now / 700) * 0.05 + Math.sin(now / 53) * 0.012;
    // POWER off = everything dark; STANDBY = heaters stay warm (dim) but no signal.
    const on = amp.def.id !== "vapor" || ampVals.power ? 1 : 0;
    const warm = on * (amp.def.id === "vapor" && ampVals.standby ? 0.35 : 1);
    amp.powerLevel = (amp.powerLevel ?? warm) + (warm - (amp.powerLevel ?? warm)) * 0.05;
    amp.glow.intensity = 0.05 * fl * amp.powerLevel;
    if (amp.winLight) amp.winLight.intensity = 5 * amp.powerLevel * fl;
    for (const g of amp.glowMats) g.m.emissiveIntensity = g.base * amp.powerLevel * (g.m.name === "Heater" ? fl : 1);
    if (amp.screen) amp.screen.draw(t, amp.powerLevel);
  }
  stage.update(t);
  updateCamera(now);
  updateAnchors();
  stage.composer.render();
}

// ------------------------------------------------------------------ start
manager.onLoad = async () => {
  if (manager.started) return; manager.started = true;
  await Promise.all([...document.fonts].map((f) => f.load().catch(() => {})));
  for (const k of ["pre", "loop"]) { const m = addSlots[k].children[0].material; m.map.dispose(); m.map = plusTexture(k); m.needsUpdate = true; }
  applyQuality(); buildDock();
  goShot("empty", null, true); document.body.dataset.shot = "empty"; setHint("empty");
  $("#loading").style.opacity = 0; setTimeout(() => $("#loading")?.remove(), 600);
  requestAnimationFrame(frame);
  thumbs.british30 = ampThumb(AMP.british30);
  // Load the other two amps in the background so the catalogue opens instantly.
  for (const a of AMPS) if (!AMP[a.id]) loadAmp(a, new THREE.LoadingManager()).then((x) => { thumbs[a.id] = ampThumb(x); }).catch(() => {});
  const auto = location.hash.match(/#auto(?:=(\w+))?/); if (auto) placeAmp(auto[1] || "british30");
  if (PLUGIN) JB.emitEvent("rig", { type: "ready" });   // the engine answers with the full state, the amp walks on stage
  if (/[?&]clean/.test(location.search)) document.body.classList.add("clean");
};
loadAmp(AMPS[0], manager);
// Props: guitar on its stand, road case, monitor wedge, drum kit.
new GLTFLoader(manager).load("models/props.json", (g) => {
  g.scene.traverse((o) => { if (o.isMesh) { o.castShadow = o.receiveShadow = true; const m = M[o.material.name]; if (m) o.material = m; } });
  applyAO(g.scene, "models/props_ao.jpg");
  scene.add(g.scene);
});

window.__rig = { meters: (m) => rack.meters(m), pitch: (hz) => rack.pitch(hz), snapshotRig, applyRig, rack, setPass: (k, v) => { stage[k].enabled = v; }, placeAmp, goShot, openPedal, openAmp, openCab, back, chain, PEDAL, addPedal, reorder, ampVals, cabState, placeMic,
  screenOf: (o) => { const v = new THREE.Vector3(); o.getWorldPosition(v); v.project(camera); return [(v.x + 1) / 2 * innerWidth, (1 - v.y) / 2 * innerHeight]; },
  get amp() { return amp; }, stage, scene, setQuality: (q) => { quality = q; applyQuality(); } };
