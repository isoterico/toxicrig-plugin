// ToxicRig - real-time 3D stage (v2): three amps, club stage, pedals, cab + microphone.
import * as THREE from "three";
import { GLTFLoader } from "three/examples/jsm/loaders/GLTFLoader.js";
import { RoundedBoxGeometry } from "three/examples/jsm/geometries/RoundedBoxGeometry.js";
import { PEDALS, AMPS, AMP_CONTROLS, CABS, MICS, toValue, toNorm, fmt } from "./pedals.js";
import { makeMaterials } from "./materials.js";
import { buildStage } from "./stage.js";
import { pedalFactory, PH, PD, BOARD_TOP } from "./pedal3d.js";
import { micFactory } from "./mic3d.js";

const $ = (s) => document.querySelector(s);
const MOBILE = matchMedia("(pointer: coarse)").matches;
const REDUCED = matchMedia("(prefers-reduced-motion: reduce)").matches;

// ------------------------------------------------------------------ renderer
const canvas = $("#gl");
const renderer = new THREE.WebGLRenderer({ canvas, antialias: true, powerPreference: "high-performance" });
renderer.shadowMap.enabled = true; renderer.shadowMap.type = THREE.PCFShadowMap;
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
let quality = (() => { try { return localStorage.getItem("tr.quality") || (MOBILE ? "media" : "alta"); } catch { return MOBILE ? "media" : "alta"; } })();
function applyQuality() {
  const hi = quality === "alta";
  renderer.setPixelRatio(Math.min(devicePixelRatio, hi ? 2 : 1.25));
  stage.gtao.enabled = hi; stage.bokeh.enabled = hi;
  stage.L.key.shadow.mapSize.set(hi ? 2048 : 1024, hi ? 2048 : 1024); if (stage.L.key.shadow.map) { stage.L.key.shadow.map.dispose(); stage.L.key.shadow.map = null; }
  $("#quality").textContent = "Qualità: " + (hi ? "Alta" : "Media");
  try { localStorage.setItem("tr.quality", quality); } catch {}
  resize();
}
$("#quality").addEventListener("click", () => { quality = quality === "alta" ? "media" : "alta"; applyQuality(); });

// ------------------------------------------------------------------ amps
// Plugin parameters are per amp slot, not per model: switching model keeps the knobs where they are.
const ampVals = { gain: 0.5, bass: 0.5, mid: 0.5, treble: 0.5, presence: 0.5, resonance: 0.5, master: 0.5, sag: 0.5, bright: 0, channel: 1 };
const AMP = {};          // id -> loaded amp
let amp = null;          // amp on stage
const ampHolder = new THREE.Group(); scene.add(ampHolder);
const Z_AXIS = new THREE.Vector3(0, 0, -1);

function loadAmp(def, mgr) {
  return new Promise((res, rej) => new GLTFLoader(mgr).load(def.file, (g) => res(processAmp(def, g.scene)), undefined, rej));
}
function processAmp(def, root) {
  const a = { def, root, knobs: {}, toggles: {}, speakers: [] };
  root.traverse((o) => {
    if (o.isMesh) {
      o.castShadow = true; o.receiveShadow = true;
      if (o.name.startsWith("WinGlass")) { o.visible = false; return; }
      const m = M[o.material.name]; if (m) o.material = m;
      if (o.material.alphaTest > 0) o.castShadow = false;
    }
  });
  for (const c of AMP_CONTROLS) {
    const obj = root.getObjectByName(c.id + "_cap");
    a.knobs[c.id] = { obj, q0: obj.quaternion.clone(), label: def.id === "american" && c.id === "gain" ? "VOLUME" : c.label };
    obj.traverse((m) => { m.userData.ctl = { kind: "ampKnob", id: c.id }; });
  }
  for (const [id, n] of [["bright", 2], ["channel", 3]]) {
    const obj = root.getObjectByName(id + "_pivot");
    a.toggles[id] = { obj, q0: obj.quaternion.clone(), n };
    obj.traverse((m) => { m.userData.ctl = { kind: "ampToggle", id }; });
  }
  const P = root.getObjectByName("ShotPanel"), C = root.getObjectByName("ShotCab");
  a.panel = { p: P.position.clone(), w: P.userData.w, h: P.userData.h };
  a.cab = { p: C.position.clone(), w: C.userData.w, h: C.userData.h };
  for (let i = 0; i < 4; i++) { const s = root.getObjectByName(`spk${i}_center`); if (s) a.speakers.push({ x: s.position.x, y: s.position.y, z: s.position.z, r: s.userData.radius }); }
  a.input = root.getObjectByName("input_hole").position.clone();
  a.loop = root.getObjectByName("fxloop_hole").position.clone();
  a.box = new THREE.Box3().setFromObject(root);
  a.cabTop = a.cab.p.y + a.cab.h / 2 + 0.006;
  // Light spilling from the tubes, flickering a little like real heaters.
  const tubeObj = root.getObjectByName("tube0_heater") || root.getObjectByName("Heater");
  a.glow = new THREE.PointLight(0x5cff3a, 0, 1.4, 2);
  if (tubeObj) { const p = new THREE.Vector3(); tubeObj.getWorldPosition(p); a.glow.position.set(p.x * 0.3, p.y, p.z + 0.05); }
  root.add(a.glow);
  applyAmpVals(a);
  AMP[def.id] = a; return a;
}
function applyAmpVals(a) {
  for (const id in a.knobs) { const k = a.knobs[id]; k.obj.quaternion.copy(new THREE.Quaternion().setFromAxisAngle(Z_AXIS, THREE.MathUtils.degToRad(270 * ampVals[id])).multiply(k.q0)); }
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
  const curve = new THREE.CatmullRomCurve3([a0, a1, mid, b1, b0], false, "centripetal");
  const m = new THREE.Mesh(new THREE.TubeGeometry(curve, 90, 0.0034, 10, false), mat); m.castShadow = true; cables.add(m);
  cables.add(plug(a, da), plug(b, db));
}
const LEFT = new THREE.Vector3(-1, 0, 0), RIGHT = new THREE.Vector3(1, 0, 0), FRONT = new THREE.Vector3(0, 0, 1);
function jackPos(id, side) {
  const st = PEDAL[id], p = st.grp.position;
  return new THREE.Vector3(p.x + side * (st.def.w / 2 + 0.004), st.jack.y, p.z + st.jack.z);
}
let micCable = null;
function rebuildCables() {
  cables.children.slice().forEach((c) => { cables.remove(c); c.traverse((m) => m.geometry?.dispose()); });
  if (!boardGroup.visible || !amp) return;
  const pre = chain.pre, loop = chain.loop;
  const guitar = new THREE.Vector3(BOARD.x - BOARD.w / 2 - 0.3, -0.004, BOARD.z + 0.34);
  cables.add(plug(guitar, new THREE.Vector3(-1, 0.02, 0.35)));
  const ampIn = amp.input.clone(), ampLoop = amp.loop.clone();
  if (pre.length) {
    cable(guitar, jackPos(pre[0], -1), cableMatG, { da: new THREE.Vector3(1, 0, -0.3), db: LEFT, floor: -0.006, sag: 0.1, la: 0.12, lb: 0.03 });
    for (let i = 0; i + 1 < pre.length; i++) cable(jackPos(pre[i], 1), jackPos(pre[i + 1], -1), cableMat, { da: RIGHT, db: LEFT, la: 0.004, lb: 0.004, arc: 0.03 });
    cable(jackPos(pre[pre.length - 1], 1), ampIn, cableMatG, { da: RIGHT, db: FRONT, la: 0.05, lb: 0.12, floor: -0.006, sag: 0.3, midZ: BOARD.z - BOARD.d / 2 - 0.06 });
  } else cable(guitar, ampIn, cableMatG, { da: new THREE.Vector3(1, 0, -0.3), db: FRONT, floor: -0.006, sag: 0.3, la: 0.15, lb: 0.12 });
  if (loop.length) {
    cable(ampLoop, jackPos(loop[0], -1), cableMatV, { da: FRONT, db: LEFT, la: 0.1, lb: 0.03, floor: -0.006, sag: 0.3, midZ: BOARD.z - BOARD.d / 2 - 0.05 });
    for (let i = 0; i + 1 < loop.length; i++) cable(jackPos(loop[i], 1), jackPos(loop[i + 1], -1), cableMat, { da: RIGHT, db: LEFT, la: 0.004, lb: 0.004, arc: 0.03 });
    cable(jackPos(loop[loop.length - 1], 1), ampLoop.clone().add(new THREE.Vector3(0, -0.007, 0)), cableMatV, { da: RIGHT, db: FRONT, la: 0.05, lb: 0.09, floor: -0.006, sag: 0.3, midZ: BOARD.z - BOARD.d / 2 - 0.1 });
  }
  if (micCable) cable(micCable.a, micCable.b, cableMat, { da: new THREE.Vector3(0, -0.3, 1), db: new THREE.Vector3(1, 0, 0), la: 0.06, lb: 0.1, floor: -0.006, sag: 0.4 });
}

// ------------------------------------------------------------------ cab + microphone
const cabState = { on: true, type: 0, mic: "dyn", pos: 0.3, dist: 0.2, ir: null };
const micRig = MIC.rig(); micRig.root.visible = false; scene.add(micRig.root);
function placeMic() {
  if (!amp) return;
  const spk = amp.speakers[0];
  const front = amp.cab.p.z;
  const r = micRig.place(cabState.mic, spk, front, cabState.pos, cabState.dist);
  micCable = { a: r.back, b: new THREE.Vector3(spk.x + (spk.x <= 0 ? -0.9 : 0.9), -0.004, r.tip.z + 0.3) };
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
    return { tgt: s.center.clone().add(new THREE.Vector3(0, -0.02, 0)), theta: -0.38, phi: 1.16, sphere: s.radius * 0.86, ap: 0.003 };
  }
  if (name === "amp") return { tgt: amp.panel.p.clone(), theta: 0.0, phi: Math.PI / 2 - 0.05, box: [amp.panel.w + 0.03, amp.panel.h + 0.04], ap: 0.012 };
  if (name === "cab") {
    const s = amp.speakers[0];
    const side = s.x <= 0 ? -1 : 1;
    return { tgt: new THREE.Vector3(s.x, s.y, amp.cab.p.z + 0.06), theta: -side * 0.38, phi: Math.PI / 2 - 0.1, box: [0.5, 0.42], ap: 0.01, panel: true };
  }
  if (name === "pedal") { const st = PEDAL[arg], p = st.grp.position; return { tgt: new THREE.Vector3(p.x, BOARD_TOP + PH, p.z + 0.004), theta: 0, phi: 0.66, box: [st.def.w + 0.04, PD + 0.035], ap: 0.016, panel: true }; }
}
function resolveShot(spec) {
  const vfov = THREE.MathUtils.degToRad(camera.fov), aspect = innerWidth / innerHeight;
  const hfov = 2 * Math.atan(Math.tan(vfov / 2) * aspect);
  let uW = 1, uH = 1; const wideScreen = innerWidth >= 820;
  if (spec.panel) { if (wideScreen) uW = 1 - 390 / innerWidth; else uH = 0.5; }
  let r;
  if (spec.sphere) r = spec.sphere / Math.sin(Math.min(vfov, hfov) / 2);
  else { const [bw, bh] = spec.box; r = Math.max((bh / 2) / Math.tan(vfov / 2) / uH, (bw / 2) / Math.tan(hfov / 2) / uW) * 1.04; }
  const tgt = spec.tgt.clone();
  if (spec.panel) {
    const off = new THREE.Vector3().setFromSpherical(new THREE.Spherical(r, spec.phi, spec.theta));
    const fwd = off.clone().negate().normalize(), right = new THREE.Vector3().crossVectors(fwd, new THREE.Vector3(0, 1, 0)).normalize(), upv = new THREE.Vector3().crossVectors(right, fwd).normalize();
    const visW = 2 * Math.tan(hfov / 2) * r, visH = 2 * Math.tan(vfov / 2) * r;
    if (wideScreen) tgt.addScaledVector(right, visW * (1 - uW) / 2); else tgt.addScaledVector(upv, -visH * (1 - uH) / 2);
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
  document.body.dataset.shot = name;
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
  if (c && shot.name === "amp" && c.kind === "ampKnob") {
    drag = { c, y: e.clientY, v0: ampVals[c.id] }; canvas.setPointerCapture(e.pointerId); e.preventDefault(); showValue(c, e);
  } else if (c && shot.name === "pedal" && c.kind === "pedalCtl" && c.pedal === shot.arg && PEDAL[c.pedal].ctl[c.param].kind !== "bool") {
    drag = { c, y: e.clientY, v0: PEDAL[c.pedal].values[c.param] }; canvas.setPointerCapture(e.pointerId); e.preventDefault(); showValue(c, e);
  } else if (shot.name === "cab" && (c?.kind === "mic" || (hit && zoneOf(hit) === "cab"))) {
    drag = { c: { kind: "micDrag" }, x: e.clientX, y: e.clientY, p0: cabState.pos, d0: cabState.dist }; canvas.setPointerCapture(e.pointerId);
  } else if (shot.name === "wide" && hit?.object.userData.pedalId) {
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
  if (shot.name === "wide" && hit) cur = hit.object.userData.pedalId ? "grab" : (c?.kind === "addSlot" ? "pointer" : "zoom-in");
  else if (shot.name === "cab" && hit && (c?.kind === "mic" || zoneOf(hit) === "cab")) cur = "move";
  else if (c && (c.kind === "ampKnob" || c.kind === "pedalCtl")) cur = c.kind === "pedalCtl" && PEDAL[c.pedal].ctl[c.param].kind === "bool" ? "pointer" : "ns-resize";
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
  if (c.kind === "ampKnob" && shot.name === "amp") { setCtl(c, 0.5); showValue(c, e); hideBubble(800); }
  if (c.kind === "pedalCtl" && shot.name === "pedal") { const par = PEDAL[c.pedal].ctl[c.param].par; if (!par.isBool) { setCtl(c, toNorm(par, par.def)); showValue(c, e); hideBubble(800); } }
});
canvas.addEventListener("wheel", (e) => {
  if (shot?.name === "cab") { e.preventDefault(); cabState.dist = THREE.MathUtils.clamp(cabState.dist + Math.sign(e.deltaY) * 0.03, 0, 1); placeMic(); rebuildCables(); refreshCabPanel(); showBubble(e, `Distanza ${micCm()} cm`); hideBubble(700); return; }
  const hit = pick(e), c = hit?.object.userData.ctl; if (!c) return;
  const ok = (c.kind === "ampKnob" && shot.name === "amp") || (c.kind === "pedalCtl" && shot.name === "pedal" && shot.arg === c.pedal && PEDAL[c.pedal].ctl[c.param].kind !== "bool");
  if (!ok) return; e.preventDefault();
  const v = c.kind === "ampKnob" ? ampVals[c.id] : PEDAL[c.pedal].values[c.param];
  const par = c.kind === "pedalCtl" ? PEDAL[c.pedal].ctl[c.param].par : null, step = par?.choices ? 1 / (par.choices.length - 1) : 0.02;
  setCtl(c, THREE.MathUtils.clamp(v - Math.sign(e.deltaY) * step, 0, 1)); showValue(c, e); hideBubble(700);
}, { passive: false });
const micCm = () => Math.round(1 + cabState.dist * 29);

function setCtl(c, v) {
  if (c.kind === "ampKnob") { ampVals[c.id] = v; applyAmpVals(amp); }
  else { const st = PEDAL[c.pedal], par = st.ctl[c.param].par; st.values[c.param] = par.choices ? Math.round(v * (par.choices.length - 1)) / (par.choices.length - 1) : v; PF.apply(st); refreshPanel(); }
}
function showValue(c, e) {
  if (c.kind === "ampKnob") showBubble(e, `${amp.knobs[c.id].label}  ${(ampVals[c.id] * 10).toFixed(1)}`);
  else { const st = PEDAL[c.pedal], par = st.ctl[c.param].par; showBubble(e, `${par.name}  ${fmt(par, toValue(par, st.values[c.param]))}`); }
}
function click(e, hit) {
  const c = hit?.object.userData.ctl;
  if (shot.name === "wide") {
    if (c?.kind === "addSlot") return openPedalCatalog(c.row);
    if (hit?.object.userData.pedalId) return openPedal(hit.object.userData.pedalId);
    if (hit && (isAmpObj(hit.object) || c?.kind === "mic")) return zoneOf(hit) === "cab" ? openCab() : openAmp();
    return;
  }
  if (shot.name === "amp") {
    if (c?.kind === "ampToggle") {
      ampVals[c.id] = (ampVals[c.id] + 1) % amp.toggles[c.id].n; applyAmpVals(amp);
      showBubble(e, c.id === "bright" ? `BRIGHT  ${ampVals.bright ? "ON" : "OFF"}` : `CHANNEL  ${["CLEAN", "CRUNCH", "LEAD"][ampVals.channel]}`); hideBubble(900); return;
    }
    if (!hit) return back();
    if (hit.object.userData.pedalId) return openPedal(hit.object.userData.pedalId);
    if (isAmpObj(hit.object) && zoneOf(hit) === "cab") return openCab();
    return;
  }
  if (shot.name === "pedal") {
    if (c?.kind === "footswitch" && c.pedal === shot.arg) { const st = PEDAL[c.pedal]; st.on = !st.on; PF.apply(st); refreshPanel(); return; }
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
function dropPedal(st) {
  const p = st.grp.position, row = p.z > BOARD.z ? "pre" : "loop";
  const from = chain[st.row]; from.splice(from.indexOf(st.def.id), 1);
  const idx = chain[row].map((id) => PEDAL[id].grp.position.x).filter((x) => x < p.x).length;
  if (rowLayout(row, [...chain[row], st.def.id]).free < -0.001) { from.splice(st.index, 0, st.def.id); say("Quella fila è piena: sposta prima un altro pedale."); }
  else chain[row].splice(idx, 0, st.def.id);
  layoutChain(); setHint("wide");
}

// ------------------------------------------------------------------ UI flow
const HINTS = {
  empty: "Il palco è vuoto: scegli l'amplificatore.",
  wide: "Tocca la testata, la cassa o un pedale per avvicinarti. Trascina i pedali per cambiare l'ordine della catena.",
  drag: "Rilascia il pedale dove vuoi: fila davanti = prima dell'ampli, fila dietro = loop FX.",
  amp: "Trascina su/giù le manopole (o rotella) · doppio clic = valore iniziale · tocca BRIGHT e CHANNEL · Esc per allontanarti",
};
function setHint(k) { $("#hint").textContent = HINTS[k] || ""; }
function say(t) { const el = $("#toast"); el.textContent = t; el.classList.add("on"); clearTimeout(say.t); say.t = setTimeout(() => el.classList.remove("on"), 2800); }
function openAmp() { if (!amp) return; closePanel(); goShot("amp"); setHint("amp"); }
function back() { closePanel(); goShot("wide"); setHint("wide"); }
addEventListener("keydown", (e) => {
  if (e.key !== "Escape") return;
  if ($("#cat").classList.contains("open")) return $("#cat").classList.remove("open");
  if (shot && shot.name !== "wide" && shot.name !== "empty") back();
});
$("#back").addEventListener("click", back);
$("#cat").addEventListener("click", (e) => { if (e.target.id === "cat") $("#cat").classList.remove("open"); });
$("#cta").addEventListener("click", openAmpCatalog);
$("#changeAmp").addEventListener("click", openAmpCatalog);

const thumbs = {};
function openAmpCatalog() {
  const box = $("#catCards"); box.innerHTML = "";
  $("#catTitle").textContent = "Scegli l'amplificatore";
  $("#catSub").textContent = "Ogni ampli è un modello 3D con gli stessi comandi del plugin. Le manopole restano dove le hai lasciate quando cambi ampli.";
  for (const a of AMPS) {
    const el = document.createElement("article"); el.className = "card";
    const on = amp?.def.id === a.id;
    el.innerHTML = `<div class="pic">${thumbs[a.id] ? `<img alt="" src="${thumbs[a.id]}">` : `<span class="soon">Carico il modello…</span>`}</div>
      <div class="txt"><h3>${a.name}</h3><div class="tags">${a.tags.map((t) => `<span>${t}</span>`).join("")}</div>
      <ul>${a.text.map((t) => `<li>${t}</li>`).join("")}</ul><button ${on ? "disabled" : ""}>${on ? "Sul palco" : "Mettilo sul palco"}</button></div>`;
    el.querySelector("button").addEventListener("click", () => { $("#cat").classList.remove("open"); placeAmp(a.id); });
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
    const el = document.createElement("article"); el.className = "card"; const inUse = used.has(d.id);
    el.innerHTML = `<div class="pic"><img alt="" src="${pedalThumb(d.id)}"></div>
      <div class="txt"><h3>${d.name}</h3><div class="tags">${d.params.map((q) => `<span>${q.name}</span>`).join("")}</div>
      <ul><li>${d.desc}</li></ul><button ${inUse ? "disabled" : ""}>${inUse ? "Già sulla pedaliera" : "Aggiungi"}</button></div>`;
    if (!inUse) el.querySelector("button").addEventListener("click", () => { $("#cat").classList.remove("open"); addPedal(d.id, catRow); });
    box.appendChild(el);
  }
  $("#cat").classList.add("open");
}
function addPedal(id, row) {
  const st = pedalState(id); PF.apply(st);
  if (rowLayout(row, [...chain[row], id]).free < -0.001) { say("Questa fila è piena."); return; }
  chain[row].push(id); layoutChain(); setTimeout(() => openPedal(id), 480);
}

// Side/bottom panel
const panel = $("#panel");
function closePanel() { panel.classList.remove("open"); delete panel.dataset.pedal; delete panel.dataset.cab; }
function openPedal(id) { goShot("pedal", id); panel.dataset.pedal = id; delete panel.dataset.cab; panel.classList.add("open"); refreshPanel(true); }
function refreshPanel(rebuild) {
  const id = panel.dataset.pedal; if (!id) return;
  const st = PEDAL[id], d = st.def;
  if (rebuild) {
    panel.innerHTML = `<div class="ph"><div><h2>${d.name}</h2><small></small></div><button class="x" aria-label="Chiudi">✕</button></div>
      <p>${d.desc}</p><button class="onoff"></button>
      <div class="params">${d.params.map((q) => `<div class="pr" data-p="${q.id}"><span>${q.name}</span><b></b></div>`).join("")}</div>
      <div class="order"><button data-a="left">◀ Prima</button><button data-a="right">Dopo ▶</button><button data-a="row"></button><button data-a="remove" class="danger">Rimuovi</button></div>
      <p class="note">Gira le manopole sul pedale (trascina su/giù o rotella). Il footswitch lo accende e lo spegne.</p>`;
    panel.querySelector(".x").addEventListener("click", back);
    panel.querySelector(".onoff").addEventListener("click", () => { st.on = !st.on; PF.apply(st); refreshPanel(); });
    panel.querySelectorAll(".order button").forEach((b) => b.addEventListener("click", () => reorder(id, b.dataset.a)));
  }
  panel.querySelector(".onoff").textContent = st.on ? "● ACCESO" : "○ SPENTO (bypass)";
  panel.querySelector(".onoff").classList.toggle("on", st.on);
  for (const q of d.params) panel.querySelector(`[data-p="${q.id}"] b`).textContent = fmt(q, toValue(q, st.values[q.id]));
  panel.querySelector('[data-a="row"]').textContent = st.row === "pre" ? "Sposta nel loop FX" : "Sposta in ingresso";
  panel.querySelector('[data-a="left"]').disabled = st.index === 0;
  panel.querySelector('[data-a="right"]').disabled = st.index === chain[st.row].length - 1;
  panel.querySelector("small").textContent = `${st.row === "pre" ? "Ingresso (prima dell'ampli)" : "Loop FX (dopo il preamp)"} · posizione ${st.index + 1} di ${chain[st.row].length}`;
}
function reorder(id, a) {
  const st = PEDAL[id], arr = chain[st.row], i = arr.indexOf(id);
  if (a === "left" && i > 0) { arr.splice(i, 1); arr.splice(i - 1, 0, id); }
  else if (a === "right" && i < arr.length - 1) { arr.splice(i, 1); arr.splice(i + 1, 0, id); }
  else if (a === "row") { const to = st.row === "pre" ? "loop" : "pre"; if (rowLayout(to, [...chain[to], id]).free < -0.001) return say("L'altra fila è piena."); arr.splice(i, 1); chain[to].push(id); }
  else if (a === "remove") { arr.splice(i, 1); boardGroup.remove(st.grp); layoutChain(); back(); return; }
  layoutChain();
  setTimeout(() => { if (panel.dataset.pedal === id) { goShot("pedal", id); refreshPanel(); } }, 580);
  refreshPanel();
}
function openCab() {
  if (!amp) return;
  goShot("cab"); panel.dataset.cab = "1"; delete panel.dataset.pedal; panel.classList.add("open");
  panel.innerHTML = `<div class="ph"><div><h2>Cassa e microfono</h2><small>${amp.def.name}</small></div><button class="x" aria-label="Chiudi">✕</button></div>
    <p>Trascina il microfono: <b>destra/sinistra</b> = dal centro del cono al bordo (più brillante al centro, più scuro al bordo), <b>su/giù</b> o rotella = distanza dalla tela.</p>
    <button class="onoff" data-k="on"></button>
    <div class="seg" data-k="mic">${MICS.map((m) => `<button data-v="${m.id}">${m.name}</button>`).join("")}</div>
    <p class="note" data-k="micText"></p>
    <label class="field">Posizione <input type="range" min="0" max="1" step="0.01" data-k="pos"><b data-k="posv"></b></label>
    <label class="field">Distanza <input type="range" min="0" max="1" step="0.01" data-k="dist"><b data-k="distv"></b></label>
    <label class="field">Risposta cassa <select data-k="type">${CABS.map((c, i) => `<option value="${i}">${c}</option>`).join("")}</select></label>
    <label class="field ir">File IR (.wav) <input type="file" accept=".wav,audio/wav" data-k="ir"></label>
    <p class="note" data-k="irName"></p>`;
  panel.querySelector(".x").addEventListener("click", back);
  panel.querySelector('[data-k="on"]').addEventListener("click", () => { cabState.on = !cabState.on; refreshCabPanel(); });
  panel.querySelectorAll('[data-k="mic"] button').forEach((b) => b.addEventListener("click", () => { cabState.mic = b.dataset.v; placeMic(); rebuildCables(); refreshCabPanel(); }));
  panel.querySelector('[data-k="pos"]').addEventListener("input", (e) => { cabState.pos = +e.target.value; placeMic(); rebuildCables(); refreshCabPanel(); });
  panel.querySelector('[data-k="dist"]').addEventListener("input", (e) => { cabState.dist = +e.target.value; placeMic(); rebuildCables(); refreshCabPanel(); });
  panel.querySelector('[data-k="type"]').addEventListener("change", (e) => { cabState.type = +e.target.value; refreshCabPanel(); });
  panel.querySelector('[data-k="ir"]').addEventListener("change", (e) => { const f = e.target.files[0]; if (f) { cabState.ir = f.name; cabState.type = 3; refreshCabPanel(); } });
  refreshCabPanel();
}
function refreshCabPanel() {
  if (!panel.dataset.cab) return;
  const q = (k) => panel.querySelector(`[data-k="${k}"]`);
  q("on").textContent = cabState.on ? "● CASSA ATTIVA" : "○ CASSA SPENTA (solo testata, per IR esterni)"; q("on").classList.toggle("on", cabState.on);
  panel.querySelectorAll('[data-k="mic"] button').forEach((b) => b.classList.toggle("sel", b.dataset.v === cabState.mic));
  q("micText").textContent = MICS.find((m) => m.id === cabState.mic).text;
  q("pos").value = cabState.pos; q("dist").value = cabState.dist;
  q("posv").textContent = cabState.pos < 0.08 ? "centro" : cabState.pos > 0.92 ? "bordo" : Math.round(cabState.pos * 100) + "%";
  q("distv").textContent = micCm() + " cm";
  q("type").value = cabState.type;
  panel.querySelector(".ir").style.display = cabState.type === 3 ? "" : "none";
  q("irName").textContent = cabState.type === 3 ? (cabState.ir ? "IR caricato: " + cabState.ir : "Nessun IR caricato: scegli un file .wav.") : "";
}

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

function frame(now) {
  requestAnimationFrame(frame);
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
  if (amp) {
    const fl = 1 + Math.sin(now / 700) * 0.05 + Math.sin(now / 53) * 0.012;
    amp.glow.intensity = 0.05 * fl; M.Heater.emissiveIntensity = 6 * fl;
  }
  stage.update(t);
  updateCamera(now);
  stage.composer.render();
}

// ------------------------------------------------------------------ start
manager.onLoad = async () => {
  if (manager.started) return; manager.started = true;
  await Promise.all([...document.fonts].map((f) => f.load().catch(() => {})));
  for (const k of ["pre", "loop"]) { const m = addSlots[k].children[0].material; m.map.dispose(); m.map = plusTexture(k); m.needsUpdate = true; }
  applyQuality();
  goShot("empty", null, true); document.body.dataset.shot = "empty"; setHint("empty");
  $("#loading").style.opacity = 0; setTimeout(() => $("#loading")?.remove(), 600);
  requestAnimationFrame(frame);
  thumbs.british30 = ampThumb(AMP.british30);
  // Load the other two amps in the background so the catalogue opens instantly.
  for (const a of AMPS) if (!AMP[a.id]) loadAmp(a, new THREE.LoadingManager()).then((x) => { thumbs[a.id] = ampThumb(x); }).catch(() => {});
  const auto = location.hash.match(/#auto(?:=(\w+))?/); if (auto) placeAmp(auto[1] || "british30");
};
loadAmp(AMPS[0], manager);

window.__rig = { placeAmp, goShot, openPedal, openAmp, openCab, back, chain, PEDAL, addPedal, reorder, ampVals, cabState, placeMic,
  screenOf: (o) => { const v = new THREE.Vector3(); o.getWorldPosition(v); v.project(camera); return [(v.x + 1) / 2 * innerWidth, (1 - v.y) / 2 * innerHeight]; },
  get amp() { return amp; }, stage, scene, setQuality: (q) => { quality = q; applyQuality(); } };
