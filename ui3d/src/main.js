// ToxicRig - real-time 3D stage.
// One scene, one set of lights: amp, pedals, cables and floor share the same light,
// shadows and perspective, so nothing looks "pasted on".
import * as THREE from "three";
import { GLTFLoader } from "three/examples/jsm/loaders/GLTFLoader.js";
import { RoomEnvironment } from "three/examples/jsm/environments/RoomEnvironment.js";
import { RoundedBoxGeometry } from "three/examples/jsm/geometries/RoundedBoxGeometry.js";
import { EffectComposer } from "three/examples/jsm/postprocessing/EffectComposer.js";
import { RenderPass } from "three/examples/jsm/postprocessing/RenderPass.js";
import { UnrealBloomPass } from "three/examples/jsm/postprocessing/UnrealBloomPass.js";
import { OutputPass } from "three/examples/jsm/postprocessing/OutputPass.js";
import { PEDALS, AMPS, AMP_CONTROLS, toValue, toNorm, fmt } from "./pedals.js";

const $ = (s) => document.querySelector(s);
const MOBILE = matchMedia("(pointer: coarse)").matches;
const REDUCED = matchMedia("(prefers-reduced-motion: reduce)").matches;

// ------------------------------------------------------------------ renderer / scene
const canvas = $("#gl");
const renderer = new THREE.WebGLRenderer({ canvas, antialias: true, powerPreference: "high-performance" });
renderer.setPixelRatio(Math.min(devicePixelRatio, MOBILE ? 2 : 1.75));
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFShadowMap;
renderer.toneMapping = THREE.AgXToneMapping;
renderer.toneMappingExposure = 1.0;
renderer.outputColorSpace = THREE.SRGBColorSpace;

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x060609);
scene.fog = new THREE.Fog(0x060609, 3.2, 7.5);
const pmrem = new THREE.PMREMGenerator(renderer);
scene.environment = pmrem.fromScene(new RoomEnvironment(), 0.04).texture;
scene.environmentIntensity = 0.1;

const camera = new THREE.PerspectiveCamera(30, 1, 0.02, 30);

const composer = new EffectComposer(renderer);
composer.addPass(new RenderPass(scene, camera));
const bloom = new UnrealBloomPass(new THREE.Vector2(1, 1), 0.32, 0.22, 3.0);
composer.addPass(bloom);
composer.addPass(new OutputPass());

// ------------------------------------------------------------------ loading
const manager = new THREE.LoadingManager();
manager.onProgress = (_u, done, total) => { $("#loading b").style.width = (100 * done / total) + "%"; };
const texLoader = new THREE.TextureLoader(manager);
function tex(name, srgb, rx = 1, ry = 1) {
  const t = texLoader.load("tex/" + name);
  t.wrapS = t.wrapT = THREE.RepeatWrapping; t.repeat.set(rx, ry);
  t.anisotropy = renderer.capabilities.getMaxAnisotropy();
  if (srgb) t.colorSpace = THREE.SRGBColorSpace;
  return t;
}

// ------------------------------------------------------------------ lights
// Key from front-left (warm), violet rim from back-right, green rim from back-left.
const key = new THREE.SpotLight(0xffe2c4, 0, 9, 0.62, 0.65, 1.4);
key.position.set(-1.7, 2.5, 2.2); key.target.position.set(0, 0.25, 0.3);
key.castShadow = true; key.shadow.mapSize.set(MOBILE ? 1024 : 2048, MOBILE ? 1024 : 2048);
key.shadow.bias = -0.00012; key.shadow.normalBias = 0.012; key.shadow.radius = 6;
key.shadow.camera.near = 0.8; key.shadow.camera.far = 7;
scene.add(key, key.target);
const rimV = new THREE.SpotLight(0x9b4dff, 0, 8, 0.55, 0.7, 1.4);
rimV.position.set(1.9, 1.6, -1.3); rimV.target.position.set(0, 0.3, 0.2); scene.add(rimV, rimV.target);
const rimG = new THREE.SpotLight(0x39ff14, 0, 8, 0.55, 0.7, 1.4);
rimG.position.set(-1.9, 1.3, -1.2); rimG.target.position.set(0, 0.3, 0.2); scene.add(rimG, rimG.target);
const fill = new THREE.HemisphereLight(0x8c86ff, 0x0c0a08, 0); scene.add(fill);
// A soft pool of light on the floor where the rig stands.
const pool = new THREE.SpotLight(0xfff4ea, 0, 6, 0.75, 0.9, 1.2);
pool.position.set(0.3, 3.2, 1.2); pool.target.position.set(0, 0, 0.45); scene.add(pool, pool.target);
const LIGHTS = [[key, 15], [rimV, 9], [rimG, 6], [fill, 0.1], [pool, 1.6]];
function setLights(t) { for (const [l, v] of LIGHTS) l.intensity = v * t; }
setLights(0.25);

// ------------------------------------------------------------------ room
const floorMat = new THREE.MeshStandardMaterial({ color: 0x6a6a72, map: tex("floor_col.jpg", true, 4, 4), roughnessMap: tex("floor_rough.jpg", false, 4, 4), roughness: 1, metalness: 0 });
const floor = new THREE.Mesh(new THREE.PlaneGeometry(8, 8), floorMat);
floor.rotation.x = -Math.PI / 2; floor.position.y = -0.012; floor.receiveShadow = true; scene.add(floor);
{ // backdrop haze: a curved wall with a soft violet/green gradient
  const c = document.createElement("canvas"); c.width = 1024; c.height = 512; const g = c.getContext("2d");
  g.fillStyle = "#060609"; g.fillRect(0, 0, 1024, 512);
  const blob = (x, y, r, col) => { const gr = g.createRadialGradient(x, y, 0, x, y, r); gr.addColorStop(0, col); gr.addColorStop(1, "rgba(0,0,0,0)"); g.fillStyle = gr; g.fillRect(0, 0, 1024, 512); };
  blob(512, 260, 360, "rgba(110,50,200,0.28)"); blob(250, 200, 260, "rgba(57,255,20,0.10)"); blob(790, 190, 260, "rgba(155,77,255,0.18)");
  for (let x = 0; x < 1024; x += 64) { g.fillStyle = "rgba(255,255,255,0.012)"; g.fillRect(x, 0, 2, 512); }
  const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace;
  const wall = new THREE.Mesh(new THREE.CylinderGeometry(4.2, 4.2, 3.2, 64, 1, true, Math.PI * 0.62, Math.PI * 0.76),
    new THREE.MeshBasicMaterial({ map: t, side: THREE.BackSide, fog: false }));
  wall.position.set(0, 1.5, 1.3); scene.add(wall);
}

// ------------------------------------------------------------------ materials for the amp
const walnutCol = tex("walnut_col.jpg", true, 1.25, 2.5), walnutNrm = tex("walnut_nrm.jpg", false, 1.25, 2.5), walnutRgh = tex("walnut_rough.jpg", false, 1.25, 2.5);
const brushedRgh = tex("brushed_rough.jpg", false, 3, 6), brushedNrm = tex("brushed_nrm.jpg", false, 3, 6);
const MATS = {
  Walnut: new THREE.MeshPhysicalMaterial({ map: walnutCol, color: 0x8a7468, normalMap: walnutNrm, normalScale: new THREE.Vector2(0.35, 0.35), roughnessMap: walnutRgh, roughness: 1, clearcoat: 0.75, clearcoatRoughness: 0.14 }),
  WalnutDark: new THREE.MeshPhysicalMaterial({ map: walnutCol, color: 0x3c302b, normalMap: walnutNrm, normalScale: new THREE.Vector2(0.3, 0.3), roughnessMap: walnutRgh, roughness: 1, clearcoat: 0.7, clearcoatRoughness: 0.12 }),
  Brass: new THREE.MeshPhysicalMaterial({ color: 0xc49a4c, metalness: 1, roughness: 0.7, roughnessMap: brushedRgh, normalMap: brushedNrm, normalScale: new THREE.Vector2(0.25, 0.25), anisotropy: 0.6 }),
  BrassKnob: new THREE.MeshPhysicalMaterial({ color: 0xe0b468, metalness: 1, roughness: 0.22, clearcoat: 0.3, envMapIntensity: 3 }),
  BrandBrass: new THREE.MeshPhysicalMaterial({ color: 0xc89a50, metalness: 1, roughness: 0.25 }),
  Chrome: new THREE.MeshStandardMaterial({ color: 0xe8e8ec, metalness: 1, roughness: 0.08, envMapIntensity: 7 }),
  Nickel: new THREE.MeshStandardMaterial({ color: 0xb6b3ad, metalness: 1, roughness: 0.2, envMapIntensity: 5 }),
  Grille: new THREE.MeshStandardMaterial({ map: tex("grille_col.jpg", true, 100, 100), normalMap: tex("grille_nrm.jpg", false, 100, 100), roughness: 0.9, color: 0xbbb4a8 }),
  Glass: new THREE.MeshPhysicalMaterial({ color: 0xd8ffe0, metalness: 0, roughness: 0.03, transparent: true, opacity: 0.16, envMapIntensity: 6, depthWrite: false, clearcoat: 1 }),
  Heater: new THREE.MeshStandardMaterial({ color: 0x113311, emissive: 0x5cff3a, emissiveIntensity: 6 }),
  TubePlate: new THREE.MeshStandardMaterial({ color: 0x3a3c3e, metalness: 1, roughness: 0.35, emissive: 0x2a8a18, emissiveIntensity: 0.25 }),
  WindowBack: new THREE.MeshStandardMaterial({ color: 0x050805, emissive: 0x1d5a10, emissiveIntensity: 0.12, roughness: 0.7 }),
  Engrave: new THREE.MeshStandardMaterial({ color: 0x060504, roughness: 0.75 }),
  Jewel: new THREE.MeshStandardMaterial({ color: 0x103a10, emissive: 0x39ff14, emissiveIntensity: 4, roughness: 0.1 }),
};
for (const k of ["Chrome", "Nickel", "BrassKnob", "Brass", "BrandBrass", "Glass"]) MATS[k].envMap = scene.environment;
MATS.Brass.envMapIntensity = 0.3; MATS.BrandBrass.envMapIntensity = 0.35; MATS.Chrome.envMapIntensity = 0.38; MATS.Nickel.envMapIntensity = 0.28; MATS.Chrome.roughness = 0.14; MATS.BrassKnob.envMapIntensity = 0.4; MATS.Glass.envMapIntensity = 0.8;
const glassWindow = new THREE.MeshPhysicalMaterial({ color: 0xffffff, roughness: 0.02, transmission: 0.0, transparent: true, opacity: 0.12, metalness: 0, clearcoat: 1 });

// ------------------------------------------------------------------ amp
const ampRoot = new THREE.Group(); ampRoot.visible = false; scene.add(ampRoot);
const ampKnobs = {};   // id -> { obj, q0, v }
const ampToggles = {}; // id -> { obj, q0, n, v }
const ampState = { bright: 0, channel: 1 };
let ampMeshes = [];
let tubeLight;
new GLTFLoader(manager).load("models/british30.json", (g) => {
  const root = g.scene;
  root.traverse((o) => {
    if (o.isMesh) {
      o.castShadow = true; o.receiveShadow = true;
      const n = o.material.name;
      if (o.name.startsWith("WinGlass")) o.visible = false;
      else if (MATS[n]) o.material = MATS[n];
      ampMeshes.push(o);
    }
  });
  for (const c of AMP_CONTROLS) {
    const obj = root.getObjectByName(c.id + "_cap");
    ampKnobs[c.id] = { obj, q0: obj.quaternion.clone(), v: 0.5, label: c.label };
    obj.traverse((m) => { m.userData.ctl = { kind: "ampKnob", id: c.id }; });
  }
  for (const [id, n] of [["bright", 2], ["channel", 3]]) {
    const obj = root.getObjectByName(id + "_pivot");
    ampToggles[id] = { obj, q0: obj.quaternion.clone(), n };
    obj.traverse((m) => { m.userData.ctl = { kind: "ampToggle", id }; });
  }
  for (const id in ampKnobs) setAmpKnob(id, 0.5);
  setAmpToggle("bright", 0); setAmpToggle("channel", 1);
  // The tube window lights the room a little (and flickers like a real heater).
  tubeLight = new THREE.PointLight(0x5cff3a, 0, 1.6, 2); tubeLight.position.set(0, 0.53, 0.2);
  root.add(tubeLight);
  ampRoot.add(root);
});
// Knob turn: rotation about the panel normal (+Z faces the player), 270 degrees of travel.
const Z_AXIS = new THREE.Vector3(0, 0, -1);
function setAmpKnob(id, v) {
  const k = ampKnobs[id]; k.v = v;
  k.obj.quaternion.copy(new THREE.Quaternion().setFromAxisAngle(Z_AXIS, THREE.MathUtils.degToRad(270 * v)).multiply(k.q0));
}
function setAmpToggle(id, s) {
  const t = ampToggles[id]; ampState[id] = s;
  const q = t.n === 2 ? new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(1, 0, 0), s ? 0.5 : -0.5)
                      : new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(0, 1, 0), [-0.55, 0, 0.55][s]);
  t.obj.quaternion.copy(q.multiply(t.q0));
}

// ------------------------------------------------------------------ pedalboard
const BOARD = { x: 0, z: 0.66, w: 1.02, d: 0.42, top: 0.034 };
const ROWS = { pre: { z: BOARD.z + 0.095, label: "INGRESSO  >  AMPLI" }, loop: { z: BOARD.z - 0.1, label: "LOOP FX" } };
const boardGroup = new THREE.Group(); boardGroup.visible = false; scene.add(boardGroup);
{
  const alu = new THREE.MeshStandardMaterial({ color: 0x15151a, metalness: 0.7, roughness: 0.45 });
  const base = new THREE.Mesh(new RoundedBoxGeometry(BOARD.w, 0.03, BOARD.d, 4, 0.008), alu);
  base.position.set(BOARD.x, BOARD.top - 0.015, BOARD.z); base.castShadow = base.receiveShadow = true; boardGroup.add(base);
  const rail = new THREE.MeshStandardMaterial({ color: 0x0b0b0e, metalness: 0.5, roughness: 0.6 });
  for (const z of [ROWS.pre.z, ROWS.loop.z]) {
    const r = new THREE.Mesh(new THREE.BoxGeometry(BOARD.w - 0.04, 0.002, 0.13), rail);
    r.position.set(BOARD.x, BOARD.top + 0.001, z); r.receiveShadow = true; boardGroup.add(r);
  }
  // Neon edge strip so the board reads in the dark.
  const edge = new THREE.Mesh(new THREE.BoxGeometry(BOARD.w - 0.03, 0.003, 0.003), new THREE.MeshStandardMaterial({ color: 0x000000, emissive: 0x39ff14, emissiveIntensity: 5 }));
  edge.position.set(BOARD.x, BOARD.top - 0.012, BOARD.z + BOARD.d / 2 + 0.001); boardGroup.add(edge);
  // Row captions printed on the board.
  for (const key of ["pre", "loop"]) {
    const c = document.createElement("canvas"); c.width = 1024; c.height = 64; const g = c.getContext("2d");
    g.font = "700 40px Rajdhani, sans-serif"; g.fillStyle = key === "pre" ? "#39ff14" : "#a875ff"; g.textBaseline = "middle";
    g.fillText(ROWS[key].label, 8, 34);
    const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 8;
    const m = new THREE.Mesh(new THREE.PlaneGeometry(0.4, 0.025), new THREE.MeshBasicMaterial({ map: t, transparent: true, opacity: 0.9 }));
    m.rotation.x = -Math.PI / 2; m.position.set(BOARD.x - BOARD.w / 2 + 0.22, BOARD.top + 0.0025, ROWS[key].z + (key === "pre" ? 0.077 : -0.077));
    boardGroup.add(m); ROWS[key].caption = t;
    ROWS[key].canvas = c;
  }
}
function paintCaption(key, extra) {
  const c = ROWS[key].canvas, g = c.getContext("2d");
  g.clearRect(0, 0, c.width, c.height);
  g.font = "700 40px Rajdhani, sans-serif"; g.fillStyle = key === "pre" ? "#39ff14" : "#a875ff"; g.textBaseline = "middle";
  g.fillText(ROWS[key].label + (extra ? "   " + extra : ""), 8, 34);
  ROWS[key].caption.needsUpdate = true;
}

// ------------------------------------------------------------------ pedal construction
const PH = 0.052, PD = 0.118;  // height, depth (metres)
const chromeM = MATS.Chrome;
const knobBlack = new THREE.MeshPhysicalMaterial({ color: 0x0d0d0f, roughness: 0.35, clearcoat: 0.6 });
const knobCream = new THREE.MeshPhysicalMaterial({ color: 0xe8e2d2, roughness: 0.35, clearcoat: 0.5 });
const ptrWhite = new THREE.MeshBasicMaterial({ color: 0xffffff });
const ptrBlack = new THREE.MeshBasicMaterial({ color: 0x111111 });
const rubber = new THREE.MeshStandardMaterial({ color: 0x0a0a0b, roughness: 0.9 });

function knobMesh(r, light) {
  const g = new THREE.Group();
  const skirt = new THREE.Mesh(new THREE.CylinderGeometry(r * 1.12, r * 1.18, 0.004, 48), light ? knobCream : knobBlack);
  skirt.position.y = 0.002;
  const body = new THREE.Mesh(new THREE.CylinderGeometry(r * 0.9, r, 0.014, 48), light ? knobCream : knobBlack);
  body.position.y = 0.009;
  const cap = new THREE.Mesh(new THREE.CylinderGeometry(r * 0.72, r * 0.9, 0.002, 48), MATS.Nickel);
  cap.position.y = 0.0168;
  const ptr = new THREE.Mesh(new THREE.BoxGeometry(r * 0.16, 0.0012, r * 0.95), light ? ptrBlack : ptrWhite);
  ptr.position.set(0, 0.0179, -r * 0.42);
  const lineS = new THREE.Mesh(new THREE.BoxGeometry(r * 0.14, 0.0042, r * 0.3), light ? ptrBlack : ptrWhite);
  lineS.position.set(0, 0.002, -r * 1.05);
  g.add(skirt, body, cap, ptr, lineS);
  g.traverse((m) => { if (m.isMesh) { m.castShadow = true; m.receiveShadow = true; } });
  return g;
}

// Lays out the controls of a pedal on its top plate and paints the graphics with a canvas.
function buildPedal(def) {
  const W = def.w, grp = new THREE.Group(); grp.userData.pedal = def.id;
  const paint = new THREE.MeshPhysicalMaterial({ color: def.color, metalness: def.metal ? 1 : 0.35, roughness: def.metal ? 0.3 : 0.4, clearcoat: 1, clearcoatRoughness: 0.08 });
  const body = new THREE.Mesh(new RoundedBoxGeometry(W, PH, PD, 5, 0.006), paint);
  body.position.y = BOARD.top + PH / 2; body.castShadow = body.receiveShadow = true; grp.add(body);
  const topY = BOARD.top + PH;

  const knobs = def.params.filter((q) => !q.isBool);
  const bools = def.params.filter((q) => q.isBool);
  const ctl = {};  // param id -> { obj, kind }
  const labels = [];
  // Canvas for the printed graphics, 4096 px per metre.
  const PX = 4096, cw = Math.round(W * PX), ch = Math.round(PD * PX);
  const cv = document.createElement("canvas"); cv.width = cw; cv.height = ch; const g = cv.getContext("2d");
  const toPx = (x, z) => [(x + W / 2) * PX, (z + PD / 2) * PX];

  if (def.sliders) {
    const n = knobs.length, span = W - 0.024;
    knobs.forEach((q, i) => {
      const x = -span / 2 + span * (i + 0.5) / n, z0 = -0.047, z1 = -0.008;
      const cap = new THREE.Mesh(new THREE.BoxGeometry(0.0075, 0.009, 0.0055), knobBlack);
      cap.castShadow = true;
      const line = new THREE.Mesh(new THREE.BoxGeometry(0.0076, 0.0003, 0.0012), ptrWhite); line.position.y = 0.0046; cap.add(line);
      cap.userData.slider = { x, z0, z1, y: topY + 0.0045 };
      grp.add(cap); ctl[q.id] = { obj: cap, kind: "slider", par: q };
      labels.push({ text: q.name, x, z: z1 + 0.0072, size: 0.0040 });
      const [sx, sy0] = toPx(x, z0), [, sy1] = toPx(x, z1);
      g.fillStyle = "rgba(0,0,0,0.85)"; g.fillRect(sx - 3, sy0, 6, sy1 - sy0);
      for (let k = 0; k <= 8; k++) { const yy = sy0 + (sy1 - sy0) * k / 8; g.fillRect(sx - (k === 4 ? 14 : 9), yy - 1.5, 6, 3); }
    });
  } else {
    const cols = knobs.length <= 3 ? knobs.length : Math.ceil(knobs.length / 2);
    const rows = Math.ceil(knobs.length / cols);
    const big = rows === 1 && W < 0.1;
    const r = big ? 0.0088 : 0.0078;
    knobs.forEach((q, i) => {
      const row = Math.floor(i / cols), col = i % cols;
      const inRow = Math.min(cols, knobs.length - row * cols);
      const x = (col - (inRow - 1) / 2) * (W - 0.02) / Math.max(cols, 2) * (cols === 1 ? 0 : 1);
      const z = rows === 1 ? -0.040 : (row === 0 ? -0.044 : -0.016);
      const k = knobMesh(r, def.metal || def.id === "eq"); k.position.set(x, topY, z);
      grp.add(k); ctl[q.id] = { obj: k, kind: q.choices ? "choice" : "knob", par: q, r };
      labels.push({ text: q.name.toUpperCase(), x, z: z + r + 0.0062, size: 0.0042 });
      // Scale dots around the knob, printed on the enclosure.
      const [cx, cy] = toPx(x, z);
      const steps = q.choices ? q.choices.length : 11;
      for (let s = 0; s < steps; s++) {
        const a = THREE.MathUtils.degToRad(-135 + 270 * s / (steps - 1)) - Math.PI / 2;
        const rr = (r * 1.45) * PX;
        g.beginPath(); g.arc(cx + Math.cos(a) * rr, cy + Math.sin(a) * rr, s === 0 || s === steps - 1 ? 4 : 2.6, 0, Math.PI * 2);
        g.fillStyle = def.ink; g.fill();
      }
    });
  }
  // Mini toggles (Sync, Ping-Pong) along the front edge, next to the footswitch.
  bools.forEach((q, i) => {
    const x = (i === 0 ? -1 : 1) * (W / 2 - 0.018), z = PD / 2 - 0.03;
    const piv = new THREE.Group(); piv.position.set(x, topY, z);
    const nut = new THREE.Mesh(new THREE.CylinderGeometry(0.0045, 0.0045, 0.002, 6), chromeM); nut.position.y = 0.001;
    const lever = new THREE.Mesh(new THREE.CylinderGeometry(0.0013, 0.0016, 0.012, 16), chromeM); lever.position.y = 0.007;
    const arm = new THREE.Group(); arm.add(lever); piv.add(nut, arm);
    piv.traverse((m) => { if (m.isMesh) m.castShadow = true; });
    grp.add(piv); ctl[q.id] = { obj: piv, arm, kind: "bool", par: q };
    labels.push({ text: q.name.toUpperCase(), x, z: z + 0.0115, size: 0.0040 });
  });
  // Footswitch, LED, top jacks.
  const oneRow = !def.sliders && knobs.length <= 3;
  const fs = new THREE.Group(); fs.position.set(0, topY, oneRow ? 0.037 : 0.045);
  const fsBase = new THREE.Mesh(new THREE.CylinderGeometry(0.0075, 0.0075, 0.004, 6), chromeM); fsBase.position.y = 0.002;
  const fsCap = new THREE.Mesh(new THREE.CylinderGeometry(0.0062, 0.0066, 0.007, 40), chromeM); fsCap.position.y = 0.0075;
  fs.add(fsBase, fsCap); fs.traverse((m) => { if (m.isMesh) m.castShadow = true; }); grp.add(fs);
  const ledMat = new THREE.MeshStandardMaterial({ color: 0x220000, emissive: 0xff2a1a, emissiveIntensity: 0, roughness: 0.15 });
  const led = new THREE.Mesh(new THREE.SphereGeometry(0.0026, 20, 10), ledMat); led.position.set(0, topY + 0.0008, oneRow ? 0.02 : 0.03); grp.add(led);
  const ledRing = new THREE.Mesh(new THREE.TorusGeometry(0.0032, 0.0008, 8, 24), chromeM); ledRing.rotation.x = Math.PI / 2; ledRing.position.copy(led.position); grp.add(ledRing);
  for (const sx of [-1, 1]) {
    const j = new THREE.Mesh(new THREE.CylinderGeometry(0.0055, 0.0055, 0.006, 6), MATS.Nickel);
    j.position.set(sx * (W / 2 - 0.013), topY + 0.003, -PD / 2 + 0.008); j.castShadow = true; grp.add(j);
  }
  // Printed graphics: name, labels, IN/OUT, brand.
  g.fillStyle = def.ink; g.textAlign = "center"; g.textBaseline = "middle";
  for (const l of labels) { const [x, y] = toPx(l.x, l.z); g.font = `700 ${Math.round(l.size * PX)}px Rajdhani`; g.fillText(l.text, x, y); }
  const ROWS1 = !def.sliders && knobs.length <= 3;
  const nameZ = ROWS1 ? -0.003 : 0.012;
  const [nx, ny] = toPx(0, nameZ);
  let fsz = Math.round(0.0105 * PX); g.font = `800 ${fsz}px Orbitron`;
  while (g.measureText(def.short).width > cw * 0.86 && fsz > 10) { fsz -= 2; g.font = `800 ${fsz}px Orbitron`; }
  g.fillText(def.short, nx, ny);
  g.font = `700 ${Math.round(0.0036 * PX)}px Rajdhani`; g.globalAlpha = 0.75;
  g.fillText("TOXICZONE  ·  TOXICRIG", nx, ny + 0.009 * PX); g.globalAlpha = 1;
  for (const [sx, t] of [[-1, "IN"], [1, "OUT"]]) { const [x, y] = toPx(sx * (W / 2 - 0.013), -PD / 2 + 0.0165); g.font = `700 ${Math.round(0.0034 * PX)}px Rajdhani`; g.fillText(t, x, y); }
  const ct = new THREE.CanvasTexture(cv); ct.colorSpace = THREE.SRGBColorSpace; ct.anisotropy = renderer.capabilities.getMaxAnisotropy();
  const plate = new THREE.Mesh(new THREE.PlaneGeometry(W, PD), new THREE.MeshStandardMaterial({ map: ct, transparent: true, roughness: 0.5, metalness: 0, polygonOffset: true, polygonOffsetFactor: -2 }));
  plate.rotation.x = -Math.PI / 2; plate.position.y = topY + 0.0002; plate.receiveShadow = true; grp.add(plate);

  grp.traverse((m) => { if (m.isMesh) m.userData.pedalId = def.id; });
  for (const id in ctl) ctl[id].obj.traverse((m) => { m.userData.ctl = { kind: "pedalCtl", pedal: def.id, param: id }; });
  fs.traverse((m) => { m.userData.ctl = { kind: "footswitch", pedal: def.id }; });
  const st = { def, grp, ctl, led: ledMat, on: def.id === "gate", values: {} };
  for (const q of def.params) st.values[q.id] = toNorm(q, q.def);
  return st;
}
const PEDAL = {};    // id -> state (built lazily)
function pedalState(id) { if (!PEDAL[id]) PEDAL[id] = buildPedal(PEDALS.find((d) => d.id === id)); return PEDAL[id]; }
function applyPedal(st) {
  for (const id in st.ctl) {
    const c = st.ctl[id], n = st.values[id];
    if (c.kind === "knob") c.obj.rotation.y = -THREE.MathUtils.degToRad(-135 + 270 * n);
    else if (c.kind === "choice") { const k = c.par.choices.length - 1; c.obj.rotation.y = -THREE.MathUtils.degToRad(-135 + 270 * Math.round(n * k) / k); }
    else if (c.kind === "slider") { const s = c.obj.userData.slider; c.obj.position.set(s.x, s.y, s.z1 + (s.z0 - s.z1) * n); }
    else if (c.kind === "bool") c.arm.rotation.x = n >= 0.5 ? -0.45 : 0.45;
  }
  st.led.emissiveIntensity = st.on ? 6 : 0;
  st.led.color.set(st.on ? 0x551111 : 0x1a0505);
}

// ------------------------------------------------------------------ chain (order = signal order)
const chain = { pre: [], loop: [] };
const ROW_W = BOARD.w - 0.06, GAP = 0.022;
function rowLayout(key, ids) {
  // Pack pedals left to right; the signal flows left -> right.
  const widths = ids.map((id) => PEDALS.find((d) => d.id === id).w);
  const total = widths.reduce((a, b) => a + b, 0) + GAP * Math.max(0, ids.length - 1);
  let x = BOARD.x - ROW_W / 2 + 0.01;
  const out = ids.map((id, i) => { const cx = x + widths[i] / 2; x += widths[i] + GAP; return { id, x: cx, z: ROWS[key].z }; });
  return { out, nextX: x, free: ROW_W - total - (ids.length ? GAP : 0) };
}
const addSlots = { pre: makeAddSlot("pre"), loop: makeAddSlot("loop") };
function makeAddSlot(key) {
  const g = new THREE.Group();
  const ring = new THREE.Mesh(new THREE.PlaneGeometry(0.074, PD), new THREE.MeshBasicMaterial({ transparent: true, map: plusTexture(key), depthWrite: false }));
  ring.rotation.x = -Math.PI / 2; ring.position.y = BOARD.top + 0.003; g.add(ring);
  ring.userData.ctl = { kind: "addSlot", row: key };
  boardGroup.add(g); return g;
}
function plusTexture(key) {
  const c = document.createElement("canvas"); c.width = 296; c.height = 472; const g = c.getContext("2d");
  const col = key === "pre" ? "57,255,20" : "168,117,255";
  g.strokeStyle = `rgba(${col},0.75)`; g.lineWidth = 6; g.setLineDash([18, 12]);
  g.beginPath(); g.roundRect(8, 8, 280, 456, 26); g.stroke(); g.setLineDash([]);
  g.fillStyle = `rgba(${col},0.08)`; g.fill();
  g.fillStyle = `rgba(${col},0.95)`; g.font = "700 150px Rajdhani"; g.textAlign = "center"; g.textBaseline = "middle"; g.fillText("+", 148, 200);
  g.font = "700 44px Rajdhani"; g.fillText("PEDALE", 148, 320);
  const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; return t;
}
const anims = [];   // simple tweens: { obj, from, to, t0, dur }
function moveTo(obj, x, z, instant) {
  const to = new THREE.Vector3(x, 0, z);
  if (instant || REDUCED) { obj.position.copy(to); return; }
  anims.push({ obj, from: obj.position.clone(), to, t0: performance.now(), dur: 520 });
}
function layoutChain(instant) {
  for (const key of ["pre", "loop"]) {
    const { out, nextX, free } = rowLayout(key, chain[key]);
    out.forEach((o, i) => {
      const st = PEDAL[o.id];
      if (st.grp.parent !== boardGroup) { boardGroup.add(st.grp); st.grp.position.set(o.x, 0.25, o.z); moveTo(st.grp, o.x, o.z, instant); }
      else if (st !== dragging?.st) moveTo(st.grp, o.x, o.z, instant);
      st.index = i; st.row = key;
    });
    const slot = addSlots[key];
    slot.visible = free >= 0.074 + GAP && chain.pre.length + chain.loop.length < PEDALS.length;
    slot.position.set(nextX + 0.037, 0, ROWS[key].z);
    paintCaption(key, chain[key].length ? "" : "(vuoto)");
  }
  rebuildCables();
}

// ------------------------------------------------------------------ cables
const cableMat = new THREE.MeshStandardMaterial({ color: 0x101012, roughness: 0.45, metalness: 0.1 });
const cableMatG = new THREE.MeshStandardMaterial({ color: 0x0f1a0f, roughness: 0.45, metalness: 0.1, emissive: 0x0a3a06, emissiveIntensity: 0.4 });
const cableMatV = new THREE.MeshStandardMaterial({ color: 0x150f1d, roughness: 0.45, metalness: 0.1, emissive: 0x2a0a50, emissiveIntensity: 0.5 });
const cables = new THREE.Group(); scene.add(cables);
const AMP_IN = new THREE.Vector3(-0.268, 0.404, 0.137), AMP_LOOP = new THREE.Vector3(0.268, 0.404, 0.137);
function jackPos(id, side) {
  const st = PEDAL[id], W = st.def.w; const p = st.grp.position;
  return new THREE.Vector3(p.x + side * (W / 2 - 0.013), BOARD.top + PH + 0.006, p.z - PD / 2 + 0.008);
}
function plug(at, dir) {
  const g = new THREE.Group();
  const b = new THREE.Mesh(new THREE.CylinderGeometry(0.0042, 0.0042, 0.026, 20), chromeM); g.add(b);
  const boot = new THREE.Mesh(new THREE.CylinderGeometry(0.0034, 0.0042, 0.014, 16), rubber); boot.position.y = 0.019; g.add(boot);
  g.position.copy(at); g.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), dir.clone().normalize());
  g.position.addScaledVector(dir.clone().normalize(), 0.012);
  g.traverse((m) => { if (m.isMesh) m.castShadow = true; });
  return g;
}
function cable(a, b, mat, opts = {}) {
  // A drooping cable: leaves each jack along its direction, then hangs between.
  const da = (opts.da || new THREE.Vector3(0, 1, 0)).clone().normalize(), db = (opts.db || new THREE.Vector3(0, 1, 0)).clone().normalize();
  const la = opts.la ?? 0.04, lb = opts.lb ?? 0.04;
  const a1 = a.clone().addScaledVector(da, 0.026 + la), b1 = b.clone().addScaledVector(db, 0.026 + lb);
  const mid = a1.clone().lerp(b1, 0.5); mid.y = Math.max(opts.floor ?? BOARD.top + 0.004, Math.min(a1.y, b1.y) - (opts.sag ?? 0.02));
  if (opts.midZ != null) mid.z = opts.midZ;
  const pts = [a.clone().addScaledVector(da, 0.026), a1, mid, b1, b.clone().addScaledVector(db, 0.026)];
  const curve = new THREE.CatmullRomCurve3(pts, false, "centripetal");
  const m = new THREE.Mesh(new THREE.TubeGeometry(curve, 80, 0.0032, 10, false), mat);
  m.castShadow = true; cables.add(m);
  cables.add(plug(a, da), plug(b, db));
}
function rebuildCables() {
  cables.children.slice().forEach((c) => { cables.remove(c); c.traverse((m) => m.geometry?.dispose()); });
  if (!boardGroup.visible) return;
  const up = new THREE.Vector3(0, 1, 0), front = new THREE.Vector3(0, 0, 1);
  // Guitar cable: arrives from the left edge of the board.
  const guitar = new THREE.Vector3(BOARD.x - BOARD.w / 2 - 0.25, 0.0, BOARD.z + 0.32);
  const pre = chain.pre, loop = chain.loop;
  const firstPre = pre.length ? jackPos(pre[0], -1) : AMP_IN;
  cable(guitar, firstPre, cableMatG, { da: new THREE.Vector3(1, 0.05, -0.3), db: pre.length ? up : front, floor: -0.008, sag: 0.1, la: 0.1, lb: pre.length ? 0.03 : 0.12 });
  for (let i = 0; i + 1 < pre.length; i++) cable(jackPos(pre[i], 1), jackPos(pre[i + 1], -1), cableMat, { sag: 0.035 });
  if (pre.length) cable(jackPos(pre[pre.length - 1], 1), AMP_IN, cableMatG, { db: front, lb: 0.1, sag: 0.25, floor: -0.008, la: 0.05, midZ: BOARD.z - BOARD.d / 2 - 0.06 });
  if (loop.length) {
    cable(AMP_LOOP, jackPos(loop[0], -1), cableMatV, { da: front, la: 0.1, sag: 0.25, floor: -0.008, midZ: BOARD.z - BOARD.d / 2 - 0.05 });
    for (let i = 0; i + 1 < loop.length; i++) cable(jackPos(loop[i], 1), jackPos(loop[i + 1], -1), cableMat, { sag: 0.035 });
    const ret = AMP_LOOP.clone().add(new THREE.Vector3(0.0, -0.006, 0));
    cable(jackPos(loop[loop.length - 1], 1), ret, cableMatV, { db: front, lb: 0.08, sag: 0.25, floor: -0.008, midZ: BOARD.z - BOARD.d / 2 - 0.09 });
  }
  { // The guitar end lies on the floor with a jack plug.
    const p = plug(guitar, new THREE.Vector3(-1, 0.02, 0.3)); cables.add(p);
  }
}

// ------------------------------------------------------------------ camera shots
// Each shot = a target point, a direction to look from and a box that must fit on screen.
// Moving between shots interpolates angles around the target, so the camera swings
// (e.g. from three-quarter to dead front) instead of sliding in a straight line.
const cam = { tgt: new THREE.Vector3(0, 0.35, 0.3), sph: new THREE.Spherical(5, 1.2, -0.4) };
let shotAnim = null, shot = null;
const sway = { x: 0, y: 0, tx: 0, ty: 0 };
function shotSpec(name, arg) {
  const vw = innerWidth, vh = innerHeight, portrait = vh > vw;
  if (name === "empty") return { tgt: new THREE.Vector3(0, 0.35, 0.3), theta: -0.42, phi: 1.32, sphere: 1.0 };
  if (name === "wide") {
    const hasBoard = boardGroup.visible;
    return { tgt: new THREE.Vector3(0, hasBoard ? 0.27 : 0.33, hasBoard ? 0.36 : 0.0), theta: -0.36, phi: hasBoard ? 1.12 : 1.3, sphere: hasBoard ? 0.68 : 0.5 };
  }
  if (name === "amp") return { tgt: new THREE.Vector3(0, 0.468, 0.137), theta: 0, phi: Math.PI / 2 - 0.02, box: [0.69, 0.3], panel: false };
  if (name === "pedal") {
    const st = PEDAL[arg]; const p = st.grp.position;
    return { tgt: new THREE.Vector3(p.x, BOARD.top + PH, p.z + 0.004), theta: 0, phi: 0.62, box: [st.def.w + 0.035, PD + 0.03], panel: true };
  }
}
function resolveShot(spec) {
  const vfov = THREE.MathUtils.degToRad(camera.fov), aspect = innerWidth / innerHeight;
  const hfov = 2 * Math.atan(Math.tan(vfov / 2) * aspect);
  // Leave room for the side/bottom panel when a pedal is open.
  let usableW = 1, usableH = 1;
  const wideScreen = innerWidth >= 820;
  if (spec.panel) { if (wideScreen) usableW = 1 - 380 / innerWidth; else usableH = 0.52; }
  let r;
  if (spec.sphere) r = spec.sphere / Math.sin(Math.min(vfov, hfov) / 2);
  else { const [bw, bh] = spec.box; r = Math.max((bh / 2) / Math.tan(vfov / 2) / usableH, (bw / 2) / Math.tan(hfov / 2) / usableW) * 1.04; }
  const tgt = spec.tgt.clone();
  if (spec.panel) {
    // Shift the target so the object sits in the free part of the screen.
    const sph = new THREE.Spherical(r, spec.phi, spec.theta);
    const off = new THREE.Vector3().setFromSpherical(sph);
    const fwd = off.clone().negate().normalize();
    const right = new THREE.Vector3().crossVectors(fwd, new THREE.Vector3(0, 1, 0)).normalize();
    const upv = new THREE.Vector3().crossVectors(right, fwd).normalize();
    const visW = 2 * Math.tan(hfov / 2) * r, visH = 2 * Math.tan(vfov / 2) * r;
    if (wideScreen) tgt.addScaledVector(right, visW * (1 - usableW) / 2);
    else tgt.addScaledVector(upv, -visH * (1 - usableH) / 2);
  }
  return { tgt, sph: new THREE.Spherical(r, spec.phi, spec.theta) };
}
function goShot(name, arg, instant) {
  shot = { name, arg };
  const to = resolveShot(shotSpec(name, arg));
  if (instant || REDUCED) { cam.tgt.copy(to.tgt); cam.sph.copy(to.sph); shotAnim = null; return; }
  const far = Math.abs(to.sph.theta - cam.sph.theta) + Math.abs(to.sph.phi - cam.sph.phi);
  shotAnim = { from: { tgt: cam.tgt.clone(), sph: cam.sph.clone() }, to, t0: performance.now(), dur: 1150 + far * 500 };
}
const ease = (t) => (t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2);
function updateCamera(now) {
  if (shotAnim) {
    let t = Math.min(1, (now - shotAnim.t0) / shotAnim.dur); const e = ease(t);
    const a = shotAnim.from, b = shotAnim.to;
    cam.tgt.lerpVectors(a.tgt, b.tgt, e);
    // Radius eases on a log scale so zooming feels even; a slight lift mid-way
    // gives the move a crane-like arc.
    cam.sph.radius = Math.exp(THREE.MathUtils.lerp(Math.log(a.sph.radius), Math.log(b.sph.radius), e));
    cam.sph.theta = THREE.MathUtils.lerp(a.sph.theta, b.sph.theta, e);
    cam.sph.phi = THREE.MathUtils.lerp(a.sph.phi, b.sph.phi, e) - Math.sin(Math.PI * e) * 0.06;
    if (t >= 1) shotAnim = null;
  }
  // Idle "hand-held" drift plus mouse parallax, only in wide shots.
  const wideish = shot && (shot.name === "wide" || shot.name === "empty");
  sway.x += ((wideish ? sway.tx : 0) - sway.x) * 0.04; sway.y += ((wideish ? sway.ty : 0) - sway.y) * 0.04;
  const s = new THREE.Spherical(cam.sph.radius, cam.sph.phi + sway.y * 0.05 + (wideish ? Math.sin(now / 3100) * 0.006 : 0), cam.sph.theta + sway.x * 0.09 + (wideish ? Math.sin(now / 4300) * 0.01 : 0));
  camera.position.setFromSpherical(s).add(cam.tgt);
  camera.lookAt(cam.tgt);
}
addEventListener("pointermove", (e) => { sway.tx = (e.clientX / innerWidth - 0.5) * 2; sway.ty = (e.clientY / innerHeight - 0.5) * 2; });

// ------------------------------------------------------------------ interaction
const ray = new THREE.Raycaster();
const ndc = new THREE.Vector2();
function pick(e, roots) {
  ndc.set((e.clientX / innerWidth) * 2 - 1, -(e.clientY / innerHeight) * 2 + 1);
  ray.setFromCamera(ndc, camera);
  const hits = ray.intersectObjects(roots, true).filter((h) => h.object.visible && isVisible(h.object));
  return hits[0];
}
function isVisible(o) { while (o) { if (!o.visible) return false; o = o.parent; } return true; }
const bubble = $("#bubble");
function showBubble(e, text) { bubble.textContent = text; bubble.style.left = e.clientX + "px"; bubble.style.top = e.clientY + "px"; bubble.classList.add("on"); clearTimeout(showBubble.t); }
function hideBubble(ms = 600) { clearTimeout(showBubble.t); showBubble.t = setTimeout(() => bubble.classList.remove("on"), ms); }

let drag = null, dragging = null, downAt = null;
const boardPlane = new THREE.Plane(new THREE.Vector3(0, 1, 0), -(BOARD.top + 0.03));
canvas.addEventListener("pointerdown", (e) => {
  if (!ampRoot.visible || shotAnim) return;
  const hit = pick(e, [ampRoot, boardGroup]);
  downAt = { x: e.clientX, y: e.clientY, hit };
  const c = hit?.object.userData.ctl;
  const close = shot.name === "amp" || (shot.name === "pedal");
  if (c && close && (c.kind === "ampKnob" || (c.kind === "pedalCtl" && shot.arg === c.pedal))) {
    let par, st;
    if (c.kind === "pedalCtl") { st = PEDAL[c.pedal]; par = st.ctl[c.param]; if (par.kind === "bool") return; }
    drag = { c, y: e.clientY, v0: c.kind === "ampKnob" ? ampKnobs[c.id].v : st.values[c.param] };
    canvas.setPointerCapture(e.pointerId); e.preventDefault();
    showValue(c, e);
  } else if (shot.name === "wide" && hit?.object.userData.pedalId) {
    const st = PEDAL[hit.object.userData.pedalId];
    dragging = { st, armed: true, pointerId: e.pointerId };
  }
});
canvas.addEventListener("pointermove", (e) => {
  if (drag) {
    const v = THREE.MathUtils.clamp(drag.v0 + (drag.y - e.clientY) / (MOBILE ? 170 : 230), 0, 1);
    setCtl(drag.c, v); showValue(drag.c, e); return;
  }
  if (dragging) {
    if (dragging.armed && Math.hypot(e.clientX - downAt.x, e.clientY - downAt.y) < 7) return;
    if (dragging.armed) { dragging.armed = false; canvas.setPointerCapture(dragging.pointerId); $("#hint").textContent = "Trascina il pedale nella posizione che vuoi nella catena e rilascia."; }
    ndc.set((e.clientX / innerWidth) * 2 - 1, -(e.clientY / innerHeight) * 2 + 1); ray.setFromCamera(ndc, camera);
    const p = new THREE.Vector3(); if (!ray.ray.intersectPlane(boardPlane, p)) return;
    p.x = THREE.MathUtils.clamp(p.x, BOARD.x - BOARD.w / 2, BOARD.x + BOARD.w / 2);
    p.z = THREE.MathUtils.clamp(p.z, BOARD.z - BOARD.d / 2, BOARD.z + BOARD.d / 2);
    dragging.st.grp.position.set(p.x, 0.03, p.z);
    rebuildCables();
    return;
  }
  // Hover cursor
  if (!ampRoot.visible || shotAnim) return;
  const hit = pick(e, [ampRoot, boardGroup]);
  const c = hit?.object.userData.ctl;
  let cur = "default";
  if (shot.name === "wide" && hit) cur = hit.object.userData.pedalId ? "grab" : (c?.kind === "addSlot" ? "pointer" : "zoom-in");
  else if (c && (c.kind === "ampKnob" || c.kind === "pedalCtl")) cur = (c.kind === "pedalCtl" && PEDAL[c.pedal].ctl[c.param].kind === "bool") ? "pointer" : "ns-resize";
  else if (c && (c.kind === "ampToggle" || c.kind === "footswitch" || c.kind === "addSlot")) cur = "pointer";
  else if (hit && shot.name !== "wide") cur = "zoom-in";
  canvas.style.cursor = cur;
});
canvas.addEventListener("pointerup", (e) => {
  if (drag) { drag = null; hideBubble(); return; }
  if (dragging && !dragging.armed) { const st = dragging.st; dragging = null; dropPedal(st); return; }
  dragging = null;
  if (!downAt || Math.hypot(e.clientX - downAt.x, e.clientY - downAt.y) > 7) return;
  click(e, downAt.hit);
});
canvas.addEventListener("dblclick", (e) => {
  const hit = pick(e, [ampRoot, boardGroup]); const c = hit?.object.userData.ctl; if (!c) return;
  if (c.kind === "ampKnob" && shot.name === "amp") { setCtl(c, 0.5); showValue(c, e); hideBubble(800); }
  if (c.kind === "pedalCtl" && shot.name === "pedal") { const st = PEDAL[c.pedal]; const par = st.ctl[c.param].par; if (!par.isBool) { setCtl(c, toNorm(par, par.def)); showValue(c, e); hideBubble(800); } }
});
canvas.addEventListener("wheel", (e) => {
  const hit = pick(e, [ampRoot, boardGroup]); const c = hit?.object.userData.ctl; if (!c) return;
  if ((c.kind === "ampKnob" && shot.name === "amp") || (c.kind === "pedalCtl" && shot.name === "pedal" && shot.arg === c.pedal && PEDAL[c.pedal].ctl[c.param].kind !== "bool")) {
    e.preventDefault();
    const v = c.kind === "ampKnob" ? ampKnobs[c.id].v : PEDAL[c.pedal].values[c.param];
    const par = c.kind === "pedalCtl" ? PEDAL[c.pedal].ctl[c.param].par : null;
    const step = par?.choices ? 1 / (par.choices.length - 1) : 0.02;
    setCtl(c, THREE.MathUtils.clamp(v - Math.sign(e.deltaY) * step, 0, 1)); showValue(c, e); hideBubble(700);
  }
}, { passive: false });

function setCtl(c, v) {
  if (c.kind === "ampKnob") setAmpKnob(c.id, v);
  else { const st = PEDAL[c.pedal]; const par = st.ctl[c.param].par; st.values[c.param] = par.choices ? Math.round(v * (par.choices.length - 1)) / (par.choices.length - 1) : v; applyPedal(st); refreshPanel(); }
}
function showValue(c, e) {
  if (c.kind === "ampKnob") showBubble(e, `${ampKnobs[c.id].label}  ${(ampKnobs[c.id].v * 10).toFixed(1)}`);
  else { const st = PEDAL[c.pedal]; const par = st.ctl[c.param].par; showBubble(e, `${par.name}  ${fmt(par, toValue(par, st.values[c.param]))}`); }
}
function click(e, hit) {
  const c = hit?.object.userData.ctl;
  if (shot.name === "wide") {
    if (c?.kind === "addSlot") return openPedalCatalog(c.row);
    if (hit?.object.userData.pedalId) return openPedal(hit.object.userData.pedalId);
    if (hit && isAmp(hit.object)) return openAmp();
    return;
  }
  if (shot.name === "amp") {
    if (c?.kind === "ampToggle") { const t = ampToggles[c.id]; setAmpToggle(c.id, (ampState[c.id] + 1) % t.n);
      showBubble(e, c.id === "bright" ? `BRIGHT  ${ampState.bright ? "ON" : "OFF"}` : `CHANNEL  ${["CLEAN", "CRUNCH", "LEAD"][ampState.channel]}`); hideBubble(900); return; }
    if (!hit) return back();
    if (hit.object.userData.pedalId) return openPedal(hit.object.userData.pedalId);
    return;
  }
  if (shot.name === "pedal") {
    if (c?.kind === "footswitch" && c.pedal === shot.arg) { const st = PEDAL[c.pedal]; st.on = !st.on; applyPedal(st); refreshPanel(); return; }
    if (c?.kind === "pedalCtl" && c.pedal === shot.arg) {
      const st = PEDAL[c.pedal], k = st.ctl[c.param];
      if (k.kind === "bool") { st.values[c.param] = st.values[c.param] >= 0.5 ? 0 : 1; applyPedal(st); refreshPanel(); showValue(c, e); hideBubble(800); }
      return;
    }
    if (hit?.object.userData.pedalId && hit.object.userData.pedalId !== shot.arg) return openPedal(hit.object.userData.pedalId);
    if (hit && isAmp(hit.object)) return openAmp();
    if (!hit) return back();
  }
}
function isAmp(o) { while (o) { if (o === ampRoot) return true; o = o.parent; } return false; }

function dropPedal(st) {
  // Row from the drop depth, position in the row from x compared with the others.
  const p = st.grp.position;
  const row = p.z > BOARD.z ? "pre" : "loop";
  const from = chain[st.row]; from.splice(from.indexOf(st.def.id), 1);
  const others = chain[row].map((id) => PEDAL[id].grp.position.x);
  let idx = others.filter((x) => x < p.x).length;
  // Row full? put it back where it was.
  const widthOk = rowLayout(row, [...chain[row], st.def.id]).free >= -0.001;
  if (!widthOk) { from.splice(st.index, 0, st.def.id); say("Quella fila è piena: sposta prima un altro pedale."); }
  else chain[row].splice(idx, 0, st.def.id);
  layoutChain();
  $("#hint").textContent = HINTS.wide;
}

// ------------------------------------------------------------------ UI flow
const HINTS = {
  empty: "Il palco è vuoto: scegli l'amplificatore.",
  wide: "Tocca l'ampli o un pedale per avvicinarti. Trascina i pedali per cambiare l'ordine della catena.",
  amp: "Trascina su/giù le manopole (o rotella) · doppio clic = valore iniziale · tocca BRIGHT e CHANNEL · Esc per allontanarti",
  pedal: "Gira le manopole, premi il footswitch per accenderlo · Esc per allontanarti",
};
function setHint(k) { $("#hint").textContent = HINTS[k]; }
function say(t) { const el = $("#toast"); el.textContent = t; el.classList.add("on"); clearTimeout(say.t); say.t = setTimeout(() => el.classList.remove("on"), 2600); }
function openAmp() { closePanel(); goShot("amp"); setHint("amp"); document.body.dataset.shot = "amp"; }
function back() { closePanel(); goShot("wide"); setHint("wide"); document.body.dataset.shot = "wide"; }
addEventListener("keydown", (e) => {
  if (e.key !== "Escape") return;
  if ($("#cat").classList.contains("open")) return closeCat();
  if (shot && (shot.name === "amp" || shot.name === "pedal")) back();
});
$("#back").addEventListener("click", back);

// Amp catalogue
function closeCat() { $("#cat").classList.remove("open"); }
$("#cat").addEventListener("click", (e) => { if (e.target.id === "cat") closeCat(); });
$("#cta").addEventListener("click", openAmpCatalog);
$("#changeAmp").addEventListener("click", openAmpCatalog);
function openAmpCatalog() {
  const box = $("#catCards"); box.innerHTML = "";
  $("#catTitle").textContent = "Scegli l'amplificatore";
  $("#catSub").textContent = "Ogni ampli è un modello 3D con i suoi comandi reali, uguali a quelli del plugin.";
  for (const a of AMPS) {
    const el = document.createElement("article"); el.className = "card";
    el.innerHTML = `<div class="pic">${a.ready ? `<img alt="" src="${thumbs.amp || ""}">` : `<span class="soon">MODELLO 3D IN COSTRUZIONE</span>`}</div>
      <div class="txt"><h3>${a.name}</h3><div class="tags">${a.tags.map((t) => `<span>${t}</span>`).join("")}</div>
      <ul>${a.text.map((t) => `<li>${t}</li>`).join("")}</ul>
      <button ${a.ready ? "" : "disabled"}>${a.ready ? (ampRoot.visible ? "Sul palco" : "Mettilo sul palco") : "In costruzione"}</button></div>`;
    if (a.ready) el.querySelector("button").addEventListener("click", () => { closeCat(); placeAmp(); });
    box.appendChild(el);
  }
  $("#cat").classList.add("open");
}
function placeAmp() {
  if (ampRoot.visible) return;
  $("#cta").classList.add("gone");
  ampRoot.visible = true; boardGroup.visible = true;
  // Default board: the classic order Gate -> Green Drive in front, Delay -> Reverb in the loop.
  for (const id of ["gate", "od", "delay", "reverb"]) { pedalState(id); applyPedal(PEDAL[id]); }
  chain.pre = ["gate", "od"]; chain.loop = ["delay", "reverb"];
  ampRoot.position.y = -0.7; ampRoot.scale.setScalar(0.94);
  const t0 = performance.now();
  rise = { t0, dur: REDUCED ? 1 : 1500 };
  layoutChain(true);
  for (const id of [...chain.pre, ...chain.loop]) PEDAL[id].grp.position.y = 0;
  goShot("wide"); setHint("wide"); document.body.dataset.shot = "wide";
}
let rise = null;

// Pedal catalogue
let catRow = "pre";
function openPedalCatalog(row) {
  catRow = row;
  const box = $("#catCards"); box.innerHTML = "";
  $("#catTitle").textContent = row === "pre" ? "Aggiungi un pedale: ingresso" : "Aggiungi un pedale: loop FX";
  $("#catSub").textContent = row === "pre" ? "I pedali in ingresso lavorano prima dell'ampli: cambiano quanto e come satura." : "I pedali nel loop FX lavorano dopo il preamp: modulazioni, delay e riverbero restano puliti anche col gain alto.";
  const used = new Set([...chain.pre, ...chain.loop]);
  for (const d of PEDALS) {
    const el = document.createElement("article"); el.className = "card";
    const inUse = used.has(d.id);
    el.innerHTML = `<div class="pic"><img alt="" src="${pedalThumb(d.id)}"></div>
      <div class="txt"><h3>${d.name}</h3><div class="tags">${d.params.map((q) => `<span>${q.name}</span>`).join("")}</div>
      <ul><li>${d.desc}</li></ul>
      <button ${inUse ? "disabled" : ""}>${inUse ? "Già sulla pedaliera" : "Aggiungi"}</button></div>`;
    if (!inUse) el.querySelector("button").addEventListener("click", () => { closeCat(); addPedal(d.id, catRow); });
    box.appendChild(el);
  }
  $("#cat").classList.add("open");
}
function addPedal(id, row) {
  const st = pedalState(id); applyPedal(st);
  if (rowLayout(row, [...chain[row], id]).free < -0.001) { say("Questa fila è piena."); return; }
  chain[row].push(id); layoutChain();
  setTimeout(() => openPedal(id), 450);
}

// Pedal panel
const panel = $("#panel");
function openPedal(id) {
  goShot("pedal", id); setHint("pedal"); document.body.dataset.shot = "pedal";
  panel.dataset.pedal = id; panel.classList.add("open"); refreshPanel(true);
}
function closePanel() { panel.classList.remove("open"); delete panel.dataset.pedal; }
function refreshPanel(rebuild) {
  const id = panel.dataset.pedal; if (!id) return;
  const st = PEDAL[id], d = st.def;
  if (rebuild) {
    panel.innerHTML = `<div class="ph"><div><h2>${d.name}</h2><small>${st.row === "pre" ? "Ingresso" : "Loop FX"} · posizione ${st.index + 1} di ${chain[st.row].length}</small></div><button class="x" aria-label="Chiudi">✕</button></div>
      <p>${d.desc}</p>
      <button class="onoff"></button>
      <div class="params">${d.params.map((q) => `<div class="pr" data-p="${q.id}"><span>${q.name}</span><b></b></div>`).join("")}</div>
      <div class="order"><button data-a="left">◀ Prima</button><button data-a="right">Dopo ▶</button><button data-a="row"></button><button data-a="remove" class="danger">Rimuovi</button></div>`;
    panel.querySelector(".x").addEventListener("click", back);
    panel.querySelector(".onoff").addEventListener("click", () => { st.on = !st.on; applyPedal(st); refreshPanel(); });
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
  else if (a === "row") {
    const to = st.row === "pre" ? "loop" : "pre";
    if (rowLayout(to, [...chain[to], id]).free < -0.001) return say("L'altra fila è piena.");
    arr.splice(i, 1); chain[to].push(id);
  } else if (a === "remove") {
    arr.splice(i, 1); boardGroup.remove(st.grp); layoutChain(); back(); return;
  }
  layoutChain();
  // The camera follows the pedal to its new place.
  setTimeout(() => { if (panel.dataset.pedal === id) { goShot("pedal", id); refreshPanel(); } }, 540);
  refreshPanel();
}

// ------------------------------------------------------------------ thumbnails (rendered once with the same materials)
const thumbs = {};
let thumbR;
function thumbRenderer() {
  if (thumbR) return thumbR;
  const r = new THREE.WebGLRenderer({ antialias: true, alpha: true, preserveDrawingBuffer: true });
  r.setSize(360, 240); r.toneMapping = THREE.AgXToneMapping; r.outputColorSpace = THREE.SRGBColorSpace;
  const s = new THREE.Scene(); s.environment = pmremFor(r); s.environmentIntensity = 0.6;
  const l = new THREE.DirectionalLight(0xfff0e0, 3.2); l.position.set(-1, 2, 2); s.add(l);
  const l2 = new THREE.DirectionalLight(0x9b4dff, 2.2); l2.position.set(2, 1, -1); s.add(l2);
  thumbR = { r, s, c: new THREE.PerspectiveCamera(30, 1.5, 0.01, 10) };
  return thumbR;
}
function pmremFor(r) { const p = new THREE.PMREMGenerator(r); return p.fromScene(new RoomEnvironment(), 0.04).texture; }
function snapshot(obj, center, dist, phi, theta) {
  const T = thumbRenderer();
  const parent = obj.parent, pos = obj.position.clone();
  T.s.add(obj); obj.position.set(0, 0, 0);
  T.c.position.setFromSpherical(new THREE.Spherical(dist, phi, theta)).add(center); T.c.lookAt(center);
  T.r.render(T.s, T.c);
  const url = T.r.domElement.toDataURL("image/png");
  T.s.remove(obj); if (parent) parent.add(obj); obj.position.copy(pos);
  return url;
}
function pedalThumb(id) {
  if (thumbs[id]) return thumbs[id];
  const st = pedalState(id); applyPedal(st);
  thumbs[id] = snapshot(st.grp, new THREE.Vector3(0, BOARD.top + PH / 2, 0), st.def.w > 0.1 ? 0.36 : 0.3, 0.85, -0.45);
  return thumbs[id];
}

// ------------------------------------------------------------------ loop
function resize() {
  const w = innerWidth, h = innerHeight;
  renderer.setSize(w, h, false); composer.setSize(w, h); bloom.setSize(w, h);
  camera.aspect = w / h; camera.fov = w < h ? 40 : 30; camera.updateProjectionMatrix();
  if (shot) goShot(shot.name, shot.arg, true);
}
addEventListener("resize", resize);


function frame(now) {
  requestAnimationFrame(frame);
  if (rise) {
    const t = Math.min(1, (now - rise.t0) / rise.dur), e = 1 - Math.pow(1 - t, 3);
    ampRoot.position.y = -0.7 * (1 - e); ampRoot.scale.setScalar(0.94 + 0.06 * e);
    boardGroup.position.y = -0.25 * (1 - e);
    setLights(0.25 + 0.75 * e);
    if (t >= 1) { rise = null; rebuildCables(); }
  }
  for (let i = anims.length - 1; i >= 0; i--) {
    const a = anims[i], t = Math.min(1, (now - a.t0) / a.dur), e = ease(t);
    a.obj.position.lerpVectors(a.from, a.to, e); a.obj.position.y += Math.sin(Math.PI * e) * 0.04;
    if (t >= 1) anims.splice(i, 1);
  }
  if (anims.length) { rebuildCables(); rebuildCables.pending = true; }
  else if (rebuildCables.pending) { rebuildCables.pending = false; rebuildCables(); }
  if (tubeLight) {
    const fl = 1 + Math.sin(now / 700) * 0.06 + Math.sin(now / 53) * 0.015;
    tubeLight.intensity = 0.06 * fl * (ampRoot.visible ? 1 : 0);
    MATS.Heater.emissiveIntensity = 6 * fl;
  }
  updateCamera(now);
  composer.render();
}

// ------------------------------------------------------------------ start
manager.onLoad = async () => {
  await Promise.all([...document.fonts].map((f) => f.load().catch(() => {})));
  for (const k of ["pre", "loop"]) { const m = addSlots[k].children[0].material; m.map.dispose(); m.map = plusTexture(k); m.needsUpdate = true; }
  // Amp thumbnail for the catalogue.
  const amp = ampRoot.children[0];
  thumbs.amp = snapshot(amp, new THREE.Vector3(0, 0.33, 0), 2.2, 1.28, -0.5);
  resize();
  goShot("empty", null, true);
  document.body.dataset.shot = "empty";
  setHint("empty");
  $("#loading").style.opacity = 0; setTimeout(() => $("#loading").remove(), 600);
  requestAnimationFrame(frame);
  if (location.hash === "#auto") placeAmp();
};
window.__rig = { screenOf: (o) => { const v = new THREE.Vector3(); o.getWorldPosition(v); v.project(camera); return [(v.x + 1) / 2 * innerWidth, (1 - v.y) / 2 * innerHeight]; }, ampKnobs, ampState, placeAmp, goShot, openPedal, openAmp, back, LIGHTS, setLights, scene, renderer, bloom, MATS, chain, PEDAL, addPedal, reorder, openPedalCatalog };
