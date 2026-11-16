// Die-cast stompboxes: rounded enclosure with a slight draft, base plate and rubber feet,
// side jacks with nuts, knurled knobs, threaded footswitch, LED bezel and silkscreen graphics
// printed into the paint (so the print takes the same light and clearcoat as the metal).
import * as THREE from "three";
import { RoundedBoxGeometry } from "three/examples/jsm/geometries/RoundedBoxGeometry.js";
import { toNorm } from "./pedals.js";
import { drawHazard } from "./materials.js";
import { GLTFLoader } from "three/examples/jsm/loaders/GLTFLoader.js";

export const PH = 0.05, PD = 0.118, BOARD_TOP = 0.034;

export function pedalFactory(renderer, M, tex) {
  const aniso = renderer.capabilities.getMaxAnisotropy();
  const peel = tex("peel_nrm.jpg", false, 0.04), grunge = tex("grunge.jpg", false, 0.2);
  const ptrWhite = new THREE.MeshStandardMaterial({ color: 0xf2f2ee, roughness: 0.5 });
  const ptrBlack = new THREE.MeshStandardMaterial({ color: 0x0a0a0a, roughness: 0.5 });
  const knobBlack = M.KnobBlack, knobCream = new THREE.MeshPhysicalMaterial({ color: 0xe6dfcd, roughness: 0.6, roughnessMap: grunge, clearcoat: 0.5, clearcoatRoughness: 0.2 });
  const basePlate = new THREE.MeshStandardMaterial({ color: 0x1a1a1c, metalness: 0.8, roughness: 0.55 });

  function roundedRect(w, d, r) {
    const s = new THREE.Shape(); const x = -w / 2, y = -d / 2;
    s.moveTo(x + r, y); s.lineTo(x + w - r, y); s.quadraticCurveTo(x + w, y, x + w, y + r); s.lineTo(x + w, y + d - r);
    s.quadraticCurveTo(x + w, y + d, x + w - r, y + d); s.lineTo(x + r, y + d); s.quadraticCurveTo(x, y + d, x, y + d - r);
    s.lineTo(x, y + r); s.quadraticCurveTo(x, y, x + r, y); return s;
  }
  function gearShape(r, teeth, depth) {
    const s = new THREE.Shape(); const n = teeth * 2;
    for (let i = 0; i <= n * 2; i++) {
      const a = (i / (n * 2)) * Math.PI * 2; const rr = r - ((Math.floor(i / 2) % 2) ? depth : 0);
      const px = Math.cos(a) * rr, py = Math.sin(a) * rr; i ? s.lineTo(px, py) : s.moveTo(px, py);
    }
    return s;
  }
  const geoCache = {};
  function knurledKnob(r, light, h = 0.014) {
    const key = r + "_" + h;
    if (!geoCache[key]) {
      const g = new THREE.ExtrudeGeometry(gearShape(r, 18, r * 0.06), { depth: h, bevelEnabled: true, bevelThickness: 0.0012, bevelSize: 0.0008, bevelSegments: 3, curveSegments: 2 });
      g.rotateX(-Math.PI / 2); geoCache[key] = g;
    }
    const grp = new THREE.Group();
    const mat = light ? knobCream : knobBlack;
    const body = new THREE.Mesh(geoCache[key], mat); body.position.y = 0.002;
    const skirt = new THREE.Mesh(new THREE.CylinderGeometry(r * 1.12, r * 1.2, 0.003, 48), mat); skirt.position.y = 0.0015;
    const insert = new THREE.Mesh(new THREE.CylinderGeometry(r * 0.68, r * 0.7, 0.0012, 48), M.Nickel); insert.position.y = h + 0.0036;
    const dome = new THREE.Mesh(new THREE.CylinderGeometry(r * 0.86, r * 0.92, 0.0018, 48), mat); dome.position.y = h + 0.0026;
    const ptr = new THREE.Mesh(new THREE.BoxGeometry(r * 0.13, 0.0008, r * 0.62), light ? ptrBlack : ptrWhite); ptr.position.set(0, h + 0.0044, -r * 0.5);
    const notch = new THREE.Mesh(new THREE.BoxGeometry(r * 0.12, 0.0035, r * 0.2), light ? ptrBlack : ptrWhite); notch.position.set(0, 0.002, -r * 1.12);
    grp.add(skirt, body, dome, insert, ptr, notch);
    grp.traverse((m) => { if (m.isMesh) { m.castShadow = true; m.receiveShadow = true; } });
    return grp;
  }
  function screw(x, y, z) {
    const g = new THREE.Group();
    const head = new THREE.Mesh(new THREE.SphereGeometry(0.0024, 16, 8, 0, Math.PI * 2, 0, Math.PI / 2), M.Nickel); head.scale.y = 0.45;
    const s1 = new THREE.Mesh(new THREE.BoxGeometry(0.0034, 0.0008, 0.0006), M.Baffle); s1.position.y = 0.001;
    const s2 = s1.clone(); s2.rotation.y = Math.PI / 2;
    g.add(head, s1, s2); g.position.set(x, y, z); return g;
  }
  function sideJack(x, y, z, dir) {
    const g = new THREE.Group();
    const nut = new THREE.Mesh(new THREE.CylinderGeometry(0.0072, 0.0072, 0.0028, 6), M.Nickel);
    const washer = new THREE.Mesh(new THREE.CylinderGeometry(0.0085, 0.0085, 0.0008, 24), M.Nickel); washer.position.y = -0.0016;
    const barrel = new THREE.Mesh(new THREE.CylinderGeometry(0.0052, 0.0052, 0.004, 24), M.Nickel); barrel.position.y = 0.0028;
    const hole = new THREE.Mesh(new THREE.CircleGeometry(0.0034, 20), M.Baffle); hole.rotation.x = -Math.PI / 2; hole.position.y = 0.0049;
    g.add(nut, washer, barrel, hole); g.traverse((m) => { if (m.isMesh) m.castShadow = true; });
    g.position.set(x, y, z); g.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), dir);
    return g;
  }

  function build(def) {
    if (def.custom) return buildCustom(def);
    const W = def.w, grp = new THREE.Group(); grp.userData.pedal = def.id;
    const knobs = def.params.filter((q) => !q.isBool), bools = def.params.filter((q) => q.isBool);
    const oneRow = !def.sliders && knobs.length <= 3;
    const ctl = {}, labels = [];

    // Silkscreen canvas: paint colour + graphics, mapped on the top face (4096 px/m).
    const PX = 4096, cw = Math.round(W * PX), ch = Math.round(PD * PX);
    const cv = document.createElement("canvas"); cv.width = cw; cv.height = ch; const g = cv.getContext("2d");
    const toPx = (x, z) => [(x + W / 2) * PX, (z + PD / 2) * PX];
    const base = new THREE.Color(def.color);
    g.fillStyle = "#" + base.getHexString(); g.fillRect(0, 0, cw, ch);
    const grd = g.createLinearGradient(0, 0, 0, ch); grd.addColorStop(0, "rgba(255,255,255,0.06)"); grd.addColorStop(1, "rgba(0,0,0,0.12)");
    g.fillStyle = grd; g.fillRect(0, 0, cw, ch);

    const topY = BOARD_TOP + PH;
    if (def.sliders) {
      const n = knobs.length, span = W - 0.024;
      knobs.forEach((q, i) => {
        const x = -span / 2 + span * (i + 0.5) / n, z0 = -0.047, z1 = -0.008;
        const cap = new THREE.Group();
        const c1 = new THREE.Mesh(new THREE.BoxGeometry(0.0078, 0.0085, 0.0058), knobBlack); c1.position.y = 0.0043;
        const line = new THREE.Mesh(new THREE.BoxGeometry(0.0079, 0.0004, 0.0011), ptrWhite); line.position.y = 0.0087;
        cap.add(c1, line); cap.traverse((m) => { if (m.isMesh) m.castShadow = true; });
        cap.userData.slider = { x, z0, z1, y: topY };
        grp.add(cap); ctl[q.id] = { obj: cap, kind: "slider", par: q };
        labels.push({ text: q.name, x, z: z1 + 0.0072, size: 0.004 });
        const [sx, sy0] = toPx(x, z0), [, sy1] = toPx(x, z1);
        g.fillStyle = "rgba(0,0,0,0.9)"; g.fillRect(sx - 4, sy0, 8, sy1 - sy0);
        g.fillStyle = def.ink; for (let k = 0; k <= 8; k++) { const yy = sy0 + (sy1 - sy0) * k / 8; g.fillRect(sx + 9, yy - 1.5, k === 4 ? 14 : 8, 3); }
      });
    } else {
      const cols = knobs.length <= 3 ? knobs.length : Math.ceil(knobs.length / 2);
      const r = oneRow && W < 0.1 ? 0.0088 : 0.0078;
      knobs.forEach((q, i) => {
        const row = Math.floor(i / cols), col = i % cols, inRow = Math.min(cols, knobs.length - row * cols);
        const x = cols === 1 ? 0 : (col - (inRow - 1) / 2) * (W - 0.02) / Math.max(cols, 2);
        const z = oneRow ? -0.040 : (row === 0 ? -0.044 : -0.016);
        const k = knurledKnob(r, def.metal || def.id === "eq"); k.position.set(x, topY, z);
        grp.add(k); ctl[q.id] = { obj: k, kind: q.choices ? "choice" : "knob", par: q };
        labels.push({ text: q.name.toUpperCase(), x, z: z + r * 1.3 + 0.0052, size: 0.0042 });
        const [cx, cy] = toPx(x, z), steps = q.choices ? q.choices.length : 11;
        for (let s = 0; s < steps; s++) {
          const a = THREE.MathUtils.degToRad(-135 + 270 * s / (steps - 1)) - Math.PI / 2, rr = r * 1.48 * PX;
          g.beginPath(); g.arc(cx + Math.cos(a) * rr, cy + Math.sin(a) * rr, s === 0 || s === steps - 1 ? 4.5 : 2.8, 0, Math.PI * 2); g.fillStyle = def.ink; g.fill();
        }
      });
    }
    bools.forEach((q, i) => {
      const x = (i === 0 ? -1 : 1) * (W / 2 - 0.018), z = PD / 2 - 0.03;
      const piv = new THREE.Group(); piv.position.set(x, topY, z);
      const nut = new THREE.Mesh(new THREE.CylinderGeometry(0.0045, 0.0045, 0.002, 6), M.Chrome); nut.position.y = 0.001;
      const lever = new THREE.Mesh(new THREE.CylinderGeometry(0.0012, 0.0016, 0.012, 16), M.Chrome); lever.position.y = 0.007;
      const tip = new THREE.Mesh(new THREE.SphereGeometry(0.0019, 12, 8), M.Chrome); tip.position.y = 0.013;
      const arm = new THREE.Group(); arm.add(lever, tip); piv.add(nut, arm); piv.traverse((m) => { if (m.isMesh) m.castShadow = true; });
      grp.add(piv); ctl[q.id] = { obj: piv, arm, kind: "bool", par: q };
      labels.push({ text: q.name.toUpperCase(), x, z: z + 0.0115, size: 0.0038 });
    });

    // Graphics
    g.fillStyle = def.ink; g.strokeStyle = def.ink; g.textAlign = "center"; g.textBaseline = "middle";
    for (const l of labels) { const [x, y] = toPx(l.x, l.z); g.font = `700 ${Math.round(l.size * PX)}px Rajdhani`; g.fillText(l.text, x, y); }
    const nameZ = oneRow ? -0.003 : 0.012;
    const [nx, ny] = toPx(0, nameZ);
    let fsz = Math.round(0.0102 * PX); g.font = `800 ${fsz}px Orbitron`;
    while (g.measureText(def.short).width > cw * 0.84 && fsz > 10) { fsz -= 2; g.font = `800 ${fsz}px Orbitron`; }
    g.fillText(def.short, nx, ny);
    g.globalAlpha = 0.55; g.lineWidth = 3;
    const m = 0.0045 * PX; g.beginPath(); g.roundRect(m, m, cw - 2 * m, ch - 2 * m, 0.004 * PX); g.stroke();
    g.font = `700 ${Math.round(0.0034 * PX)}px Rajdhani`; g.globalAlpha = 0.75;
    g.fillText("TOXICZONE  ·  TOXICRIG", nx, ny + 0.0088 * PX);
    for (const [sx, t] of [[-1, "◂ IN"], [1, "OUT ▸"]]) { const [x, y] = toPx(sx * (W / 2 - 0.012), -0.0); g.save(); g.translate(x, y); g.rotate(-Math.PI / 2 * sx); g.font = `700 ${Math.round(0.0032 * PX)}px Rajdhani`; g.fillText(t, 0, 0); g.restore(); }
    g.font = `700 ${Math.round(0.0028 * PX)}px Rajdhani`; const [dx, dy] = toPx(0, -PD / 2 + 0.006); g.fillText("9V DC", dx, dy);
    g.globalAlpha = 1;

    const ct = new THREE.CanvasTexture(cv); ct.colorSpace = THREE.SRGBColorSpace; ct.anisotropy = aniso;
    ct.repeat.set(1 / W, 1 / PD); ct.offset.set(0.5, 0.5);
    const paintOpts = { metalness: def.metal ? 1 : 0.22, roughness: def.metal ? 0.42 : 0.42, roughnessMap: grunge, normalMap: peel, normalScale: new THREE.Vector2(0.18, 0.18),
      clearcoat: def.metal ? 0.2 : 0.8, clearcoatRoughness: 0.16, envMapIntensity: 0.7, clearcoatNormalMap: peel, clearcoatNormalScale: new THREE.Vector2(0.12, 0.12) };
    const topMat = new THREE.MeshPhysicalMaterial({ ...paintOpts, map: ct });
    const sideMat = new THREE.MeshPhysicalMaterial({ ...paintOpts, color: base });
    // Enclosure: rounded rectangle extruded upward with a bevel (die-cast look).
    const shape = roundedRect(W - 0.004, PD - 0.004, 0.007);
    const eg = new THREE.ExtrudeGeometry(shape, { depth: PH - 0.008, bevelEnabled: true, bevelThickness: 0.0025, bevelSize: 0.002, bevelSegments: 4, curveSegments: 10 });
    eg.rotateX(-Math.PI / 2); eg.translate(0, BOARD_TOP + 0.0045 + 0.0025, 0);
    const body = new THREE.Mesh(eg, [topMat, sideMat]); body.castShadow = body.receiveShadow = true; grp.add(body);
    const plate = new THREE.Mesh(new THREE.BoxGeometry(W - 0.006, 0.0045, PD - 0.006), basePlate); plate.position.y = BOARD_TOP + 0.0022; plate.castShadow = true; grp.add(plate);
    for (const sx of [-1, 1]) for (const sz of [-1, 1]) {
      grp.add(screw(sx * (W / 2 - 0.006), BOARD_TOP + 0.0046, sz * (PD / 2 - 0.006)));
    }
    // Footswitch: hex nut, washer, threaded bushing, domed cap.
    const fs = new THREE.Group(); fs.position.set(0, topY, oneRow ? 0.037 : 0.045);
    const nut = new THREE.Mesh(new THREE.CylinderGeometry(0.0078, 0.0078, 0.0032, 6), M.Nickel); nut.position.y = 0.0016;
    const washer = new THREE.Mesh(new THREE.CylinderGeometry(0.0095, 0.0095, 0.0008, 32), M.Nickel); washer.position.y = 0.0004;
    const bush = new THREE.Mesh(new THREE.CylinderGeometry(0.0058, 0.0058, 0.006, 32), M.Nickel); bush.position.y = 0.006;
    const cap = new THREE.Mesh(new THREE.SphereGeometry(0.0062, 32, 16, 0, Math.PI * 2, 0, Math.PI / 2), M.Chrome); cap.position.y = 0.009; cap.scale.y = 0.6;
    const stem = new THREE.Mesh(new THREE.CylinderGeometry(0.0062, 0.0062, 0.003, 32), M.Chrome); stem.position.y = 0.0075;
    fs.add(washer, nut, bush, stem, cap); fs.traverse((m) => { if (m.isMesh) m.castShadow = true; }); grp.add(fs);
    // LED in a chrome bezel.
    const ledMat = new THREE.MeshPhysicalMaterial({ color: 0x300606, emissive: 0xff2a1a, emissiveIntensity: 0, roughness: 0.1, clearcoat: 1 });
    const ledZ = oneRow ? 0.02 : 0.03;
    const led = new THREE.Mesh(new THREE.SphereGeometry(0.0026, 20, 10), ledMat); led.position.set(0, topY + 0.0008, ledZ); grp.add(led);
    const bez = new THREE.Mesh(new THREE.TorusGeometry(0.0034, 0.0009, 10, 28), M.Chrome); bez.rotation.x = Math.PI / 2; bez.position.copy(led.position); grp.add(bez);
    // Jacks on the sides (in left, out right) and the DC socket at the back.
    const jy = BOARD_TOP + 0.026, jz = -0.02;
    grp.add(sideJack(-W / 2 + 0.0005, jy, jz, new THREE.Vector3(-1, 0, 0)), sideJack(W / 2 - 0.0005, jy, jz, new THREE.Vector3(1, 0, 0)));
    const dc = new THREE.Mesh(new THREE.BoxGeometry(0.009, 0.009, 0.004), M.Bakelite); dc.position.set(0, BOARD_TOP + 0.03, -PD / 2 - 0.001); grp.add(dc);
    // Rubber feet are hidden on the board but give the pedal its stance in thumbnails.
    grp.traverse((m) => { if (m.isMesh) m.userData.pedalId = def.id; });
    for (const id in ctl) ctl[id].obj.traverse((o) => { o.userData.ctl = { kind: "pedalCtl", pedal: def.id, param: id }; });
    fs.traverse((o) => { o.userData.ctl = { kind: "footswitch", pedal: def.id }; });
    const st = { def, grp, ctl, led: ledMat, on: def.id === "gate", values: {}, jack: { y: jy, z: jz, side: true } };
    for (const q of def.params) st.values[q.id] = toNorm(q, q.def);
    return st;
  }


  // ------------------------------------------------------------------ custom pedals (Nuclear Fuzz, Bio-Boost, Mutant Mod)
  function enclosure(def, W, D, H, topMat, sideMat) {
    const shape = roundedRect(W - 0.004, D - 0.004, def.custom === "nuke" ? 0.004 : 0.007);
    const eg = new THREE.ExtrudeGeometry(shape, { depth: H - 0.008, bevelEnabled: true, bevelThickness: 0.0025, bevelSize: 0.002, bevelSegments: 4, curveSegments: 10 });
    eg.rotateX(-Math.PI / 2); eg.translate(0, BOARD_TOP + 0.0045 + 0.0025, 0);
    const body = new THREE.Mesh(eg, [topMat, sideMat]); body.castShadow = body.receiveShadow = true; return body;
  }
  function footswitch(x, y, z) {
    const fs = new THREE.Group(); fs.position.set(x, y, z);
    const nut = new THREE.Mesh(new THREE.CylinderGeometry(0.0078, 0.0078, 0.0032, 6), M.Nickel); nut.position.y = 0.0016;
    const washer = new THREE.Mesh(new THREE.CylinderGeometry(0.0095, 0.0095, 0.0008, 32), M.Nickel); washer.position.y = 0.0004;
    const bush = new THREE.Mesh(new THREE.CylinderGeometry(0.0058, 0.0058, 0.006, 32), M.Nickel); bush.position.y = 0.006;
    const stem = new THREE.Mesh(new THREE.CylinderGeometry(0.0062, 0.0062, 0.003, 32), M.Chrome); stem.position.y = 0.0075;
    const cap = new THREE.Mesh(new THREE.SphereGeometry(0.0062, 32, 16, 0, Math.PI * 2, 0, Math.PI / 2), M.Chrome); cap.position.y = 0.009; cap.scale.y = 0.6;
    fs.add(washer, nut, bush, stem, cap); fs.traverse((m) => { if (m.isMesh) m.castShadow = true; }); return fs;
  }
  function topJack(x, y, z) {
    const g = new THREE.Group();
    const nut = new THREE.Mesh(new THREE.CylinderGeometry(0.0072, 0.0072, 0.0028, 6), M.Nickel);
    const barrel = new THREE.Mesh(new THREE.CylinderGeometry(0.0052, 0.0052, 0.004, 24), M.Nickel); barrel.position.y = 0.0028;
    const hole = new THREE.Mesh(new THREE.CircleGeometry(0.0034, 20), M.Baffle); hole.rotation.x = -Math.PI / 2; hole.position.y = 0.0049;
    g.add(nut, barrel, hole); g.position.set(x, y, z); g.traverse((m) => { if (m.isMesh) m.castShadow = true; }); return g;
  }
  function selectorKnob(r) {
    // Big rotary selector: black knurled skirt, brushed chrome cap.
    const g = knurledKnob(r, false, 0.012);
    const ring = new THREE.Mesh(new THREE.TorusGeometry(r * 1.35, r * 0.07, 10, 48), M.Chrome); ring.rotation.x = Math.PI / 2; ring.position.y = 0.0012;
    g.add(ring); return g;
  }
  function buildCustom(def) {
    const W = def.w, D = def.d, H = def.custom === "bio" ? 0.048 : 0.052;
    const grp = new THREE.Group(); grp.userData.pedal = def.id;
    const topY = BOARD_TOP + H;
    const PX = 4096, cw = Math.round(W * PX), ch = Math.round(D * PX);
    const cv = document.createElement("canvas"); cv.width = cw; cv.height = ch; const g = cv.getContext("2d");
    const P = (x, z) => [(x + W / 2) * PX, (z + D / 2) * PX];
    const base = new THREE.Color(def.color);
    const ctl = {}, extras = {};
    const txt = (t, x, z, size, weight = 700, family = "Rajdhani", color = def.ink) => { const [px, py] = P(x, z); g.fillStyle = color; g.font = `${weight} ${Math.round(size * PX)}px ${family}`; g.textAlign = "center"; g.textBaseline = "middle"; g.fillText(t, px, py); };
    const dots = (x, z, r, n, angles) => { const [cx, cy] = P(x, z); for (let s = 0; s < n; s++) { const deg = angles ? angles[s] : -135 + 270 * s / (n - 1); const a = THREE.MathUtils.degToRad(deg) - Math.PI / 2; g.beginPath(); g.arc(cx + Math.cos(a) * r * PX, cy + Math.sin(a) * r * PX, 3.2, 0, Math.PI * 2); g.fillStyle = def.ink; g.fill(); } };
    const arc = (x, z, r, from = -135, to = 135, w = 4) => { const [cx, cy] = P(x, z); g.strokeStyle = def.ink; g.lineWidth = w; g.beginPath(); g.arc(cx, cy, r * PX, THREE.MathUtils.degToRad(from - 90), THREE.MathUtils.degToRad(to - 90)); g.stroke(); };
    const addKnob = (param, x, z, r, kind = "knob", obj = null) => {
      const k = obj || knurledKnob(r, false); k.position.set(x, topY, z); grp.add(k);
      const q = def.params.find((pp) => pp.id === param); ctl[param] = { obj: k, kind: q.choices ? "choice" : "knob", par: q }; return k;
    };
    const addToggle = (param, x, z) => {
      const piv = new THREE.Group(); piv.position.set(x, topY, z);
      const nut = new THREE.Mesh(new THREE.CylinderGeometry(0.0042, 0.0042, 0.002, 6), M.Chrome); nut.position.y = 0.001;
      const lever = new THREE.Mesh(new THREE.CylinderGeometry(0.0011, 0.0015, 0.011, 16), M.Chrome); lever.position.y = 0.0065;
      const tip = new THREE.Mesh(new THREE.SphereGeometry(0.0017, 12, 8), M.Chrome); tip.position.y = 0.012;
      const arm = new THREE.Group(); arm.add(lever, tip); piv.add(nut, arm); piv.traverse((m) => { if (m.isMesh) m.castShadow = true; });
      grp.add(piv); const q = def.params.find((pp) => pp.id === param); ctl[param] = { obj: piv, arm, kind: "bool", par: q, side: true };
    };
    // Paint
    g.fillStyle = "#" + base.getHexString(); g.fillRect(0, 0, cw, ch);
    let fsPos, ledPos, ledKind = "dot";
    if (def.custom === "nuke") {
      // Scratched industrial finish: frame, hazard stripes, scratches.
      const rnd = mulberry(5);
      g.strokeStyle = "rgba(220,210,235,0.55)"; g.lineWidth = 5; g.strokeRect(0.005 * PX, 0.005 * PX, cw - 0.01 * PX, ch - 0.01 * PX);
      for (let i = 0; i < 140; i++) { g.strokeStyle = `rgba(230,220,245,${0.03 + 0.07 * rnd()})`; g.lineWidth = 1 + rnd() * 2; const x = rnd() * cw, y = rnd() * ch, a = rnd() * Math.PI, l = 20 + rnd() * 160; g.beginPath(); g.moveTo(x, y); g.lineTo(x + Math.cos(a) * l, y + Math.sin(a) * l); g.stroke(); }
      for (const zz of [0.006, 0.03]) {
        const [x0, y0] = P(-W / 2 + 0.012, zz - 0.003), [x1] = P(W / 2 - 0.012, zz);
        g.save(); g.beginPath(); g.rect(x0, y0, x1 - x0, 0.006 * PX); g.clip();
        g.fillStyle = "rgba(225,215,240,0.75)"; for (let x = x0 - 60; x < x1; x += 46) { g.beginPath(); g.moveTo(x, y0 + 0.006 * PX); g.lineTo(x + 24, y0); g.lineTo(x + 46, y0); g.lineTo(x + 22, y0 + 0.006 * PX); g.fill(); }
        g.restore();
      }
      txt("NUCLEAR FUZZ", 0, 0.018, 0.0135, 800, "Rajdhani");
      txt("GATED INDUSTRIAL", 0, 0.053, 0.0068, 700);
      const kx = [-0.036, 0, 0.036], kz = -0.034;
      [["drive", "DRIVE"], ["gate", "GATE"], ["volume", "VOLUME"]].forEach(([id, lab], i) => {
        addKnob(id, kx[i], kz, 0.0098); arc(kx[i], kz, 0.0165); txt(lab, kx[i], kz + 0.0215, 0.0062);
      });
      txt("▲", kx[0], kz - 0.023, 0.004); txt("▼", kx[2], kz - 0.023, 0.004);
      // NOISE REDUCTION bracket under GATE with the gate LED
      const [bx0, by] = P(kx[1] - 0.016, kz + 0.028), [bx1] = P(kx[1] + 0.016, kz + 0.028);
      g.strokeStyle = def.ink; g.lineWidth = 3; g.beginPath(); g.moveTo(bx0, by - 30); g.lineTo(bx0, by); g.lineTo(bx1, by); g.lineTo(bx1, by - 30); g.stroke();
      txt("NOISE REDUCTION", kx[1], kz + 0.034, 0.0042);
      fsPos = [0, 0.041]; ledPos = [kx[1], kz + 0.028];
    } else if (def.custom === "bio") {
      txt("BOOST", 0, 0.006, 0.0058);
      txt("BIO-BOOST", 0, 0.0145, 0.0078, 800, "Rajdhani");
      const kz = -0.012; addKnob("boost", 0, kz, 0.0105, "knob", selectorKnob(0.0105)); arc(0, kz, 0.0175, -140, 140, 6);
      // Mode toggle at the bottom: CLEAN / TREBLE
      addToggle("mode", 0, 0.0445); txt("CLEAN", -0.0125, 0.0445, 0.0034); txt("TREBLE", 0.013, 0.0445, 0.0034);
      fsPos = [0, 0.03]; ledPos = [0, -0.036]; ledKind = "bio";
    } else if (def.custom === "mutant") {
      // Two-tone: carbon black back half, deep purple front half.
      g.fillStyle = "#141417"; g.fillRect(0, 0, cw, ch * 0.42);
      g.fillStyle = "rgba(255,255,255,0.05)"; g.fillRect(0, ch * 0.42 - 2, cw, 4);
      const sx = -0.042, sz = -0.03;
      const modeQ = def.params[0];
      addKnob("mode", sx, sz, 0.0118, "choice", selectorKnob(0.0118)); dots(sx, sz, 0.021, 3, modeQ.angles);
      [["CHORUS", -50], ["FLANGER", 50], ["VIBRATO", -140]].forEach(([t, deg]) => { const a = THREE.MathUtils.degToRad(deg) - Math.PI / 2; txt(t, sx + Math.cos(a) * 0.031, sz + Math.sin(a) * 0.029, 0.0046); });
      arc(sx, sz, 0.0185, -150, 150, 3);
      addKnob("speed", sx, 0.012, 0.0088); arc(sx, 0.012, 0.0145); txt("SPEED", sx, 0.0285, 0.0056);
      txt("MUTANT MOD", 0.03, 0.044, 0.0092, 600, "Rajdhani"); txt("LFO TUBE", 0.03, 0.0535, 0.0048);
      fsPos = [sx, 0.046]; ledPos = [sx - 0.02, 0.046];
      // Window with a lying tube that pulses with the LFO.
      const wx = 0.03, wz = -0.008, ww = 0.07, wd = 0.075;
      const frame = new THREE.Mesh(new RoundedBoxGeometry(ww + 0.008, 0.004, wd + 0.008, 3, 0.002), M.CornerMetal || M.Nickel); frame.position.set(wx, topY + 0.001, wz); grp.add(frame);
      const well = new THREE.Mesh(new THREE.BoxGeometry(ww, 0.002, wd), new THREE.MeshStandardMaterial({ color: 0x08060c, roughness: 0.9 })); well.position.set(wx, topY + 0.0012, wz); grp.add(well);
      const glass = new THREE.Mesh(new THREE.BoxGeometry(ww, 0.002, wd), M.Glass); glass.position.set(wx, topY + 0.0105, wz); grp.add(glass);
      for (const a of [-1, 1]) for (const b of [-1, 1]) { const sc = screw(wx + a * (ww / 2 + 0.0015), topY + 0.003, wz + b * (wd / 2 + 0.0015)); grp.add(sc); }
      const tubeG = new THREE.Group(); tubeG.position.set(wx, topY + 0.008, wz); tubeG.rotation.x = Math.PI / 2;
      const env = new THREE.Mesh(new THREE.CylinderGeometry(0.0075, 0.0075, 0.05, 32, 1, true), M.Glass); tubeG.add(env);
      const dome = new THREE.Mesh(new THREE.SphereGeometry(0.0075, 24, 12, 0, Math.PI * 2, 0, Math.PI / 2), M.Glass); dome.position.y = -0.025; dome.rotation.x = Math.PI; tubeG.add(dome);
      const glowM = new THREE.MeshStandardMaterial({ color: 0x1a0a2a, emissive: 0xb050ff, emissiveIntensity: 4 });
      const plate = new THREE.Mesh(new THREE.CylinderGeometry(0.0035, 0.0035, 0.03, 6), new THREE.MeshStandardMaterial({ color: 0x333338, metalness: 1, roughness: 0.4, emissive: 0x6a20c0, emissiveIntensity: 0.5 })); tubeG.add(plate);
      const fil = new THREE.Mesh(new THREE.CylinderGeometry(0.0012, 0.0012, 0.026, 8), glowM); tubeG.add(fil);
      const sock = new THREE.Mesh(new THREE.CylinderGeometry(0.009, 0.009, 0.008, 24), glowM); sock.position.y = 0.028; tubeG.add(sock);
      grp.add(tubeG); extras.tubeGlow = [glowM, plate.material];
    }
    // Shared hardware: footswitch, LED, top jacks, base plate, screws.
    const fs = footswitch(fsPos[0], topY, fsPos[1]); grp.add(fs);
    const ledMat = new THREE.MeshPhysicalMaterial({ color: 0x300606, emissive: ledKind === "bio" ? 0x39ff14 : 0xff2a1a, emissiveIntensity: 0, roughness: 0.1, clearcoat: 1 });
    if (ledKind === "bio") {
      // Hazard-shaped LED: a lit emblem under a dark lens.
      const c2 = document.createElement("canvas"); c2.width = c2.height = 256; const k = c2.getContext("2d"); drawHazard(k, 128, 128, 110, "#ffffff");
      const lt = new THREE.CanvasTexture(c2);
      const em = new THREE.MeshStandardMaterial({ color: 0x050805, emissive: 0x39ff14, emissiveMap: lt, emissiveIntensity: 0, roughness: 0.2, transparent: true, alphaMap: lt });
      const disc = new THREE.Mesh(new THREE.PlaneGeometry(0.024, 0.024), em); disc.rotation.x = -Math.PI / 2; disc.position.set(ledPos[0], topY + 0.0006, ledPos[1]); grp.add(disc);
      const lens = new THREE.Mesh(new THREE.CircleGeometry(0.0135, 40), new THREE.MeshPhysicalMaterial({ color: 0x0a120a, roughness: 0.05, clearcoat: 1, transparent: true, opacity: 0.35 }));
      lens.rotation.x = -Math.PI / 2; lens.position.set(ledPos[0], topY + 0.0009, ledPos[1]); grp.add(lens);
      const ring = new THREE.Mesh(new THREE.TorusGeometry(0.0138, 0.0009, 10, 48), M.Chrome); ring.rotation.x = Math.PI / 2; ring.position.copy(lens.position); grp.add(ring);
      extras.bioLed = em;
    } else {
      const led = new THREE.Mesh(new THREE.SphereGeometry(0.0024, 20, 10), ledMat); led.position.set(ledPos[0], topY + 0.0008, ledPos[1]); grp.add(led);
      const bez = new THREE.Mesh(new THREE.TorusGeometry(0.0032, 0.0009, 10, 28), M.Chrome); bez.rotation.x = Math.PI / 2; bez.position.copy(led.position); grp.add(bez);
    }
    const jz = -D / 2 + 0.009, jx = W / 2 - (W < 0.06 ? 0.011 : 0.016);
    grp.add(topJack(-jx, topY, jz), topJack(jx, topY, jz));
    txt("IN", -jx, jz + 0.0105, 0.0034); txt("OUT", jx, jz + 0.0105, 0.0034);
    const ct = new THREE.CanvasTexture(cv); ct.colorSpace = THREE.SRGBColorSpace; ct.anisotropy = aniso; ct.repeat.set(1 / W, 1 / D); ct.offset.set(0.5, 0.5);
    const paint = { metalness: def.metal ? 0.85 : 0.25, roughness: def.metal ? 0.38 : 0.5, roughnessMap: grunge, normalMap: peel, normalScale: new THREE.Vector2(0.15, 0.15),
      clearcoat: def.custom === "nuke" ? 0.15 : 0.85, clearcoatRoughness: 0.15, envMapIntensity: 0.8 };
    grp.add(enclosure(def, W, D, H, new THREE.MeshPhysicalMaterial({ ...paint, map: ct }), new THREE.MeshPhysicalMaterial({ ...paint, color: def.custom === "mutant" ? 0x1c1c20 : base })));
    const plate = new THREE.Mesh(new THREE.BoxGeometry(W - 0.006, 0.0045, D - 0.006), basePlate); plate.position.y = BOARD_TOP + 0.0022; grp.add(plate);
    if (def.custom === "nuke") for (const a of [-1, 1]) for (const b of [-1, 1]) grp.add(screw(a * (W / 2 - 0.007), topY + 0.0003, b * (D / 2 - 0.007)));
    grp.traverse((m) => { if (m.isMesh) m.userData.pedalId = def.id; });
    for (const id in ctl) ctl[id].obj.traverse((o) => { o.userData.ctl = { kind: "pedalCtl", pedal: def.id, param: id }; });
    fs.traverse((o) => { o.userData.ctl = { kind: "footswitch", pedal: def.id }; });
    // Pedals built in Blender: enclosure, knobs and footswitch come from the model file. Procedural
    // parts are hidden; jacks and cable anchors keep using the same coordinates.
    const MODELS = {
      nuke: { file: "models/nfuzz.json", ids: ["drive", "gate", "volume"] },
      bio: { file: "models/bioboost.json", ids: ["boost", "mode"] },
      mutant: { file: "models/mutant.json", ids: ["mode", "speed"] },
    };
    const mdlDef = MODELS[def.custom];
    if (mdlDef) {
      const mdl = new THREE.Group(); mdl.name = "pedalModel"; mdl.position.y = BOARD_TOP; grp.add(mdl);
      grp.children.forEach((c) => { if (c !== mdl) c.visible = false; });
      new GLTFLoader().load(mdlDef.file, (g) => {
        mdl.add(g.scene);
        g.scene.traverse((o) => {
          if (o.isMesh) {
            o.castShadow = o.receiveShadow = true;
            const named = M[o.material && o.material.name]; if (named) o.material = named;
            o.userData.pedalId = def.id;
            // This pedal's own LED and tube glow, so they light up independently of other pedals
            if (o.name === "led") o.material = ledMat;
            if (def.custom === "mutant" && o.name.startsWith("tube0_heater")) { o.material = o.material.clone(); o.material.emissive = new THREE.Color(0xb050ff); extras.tubeGlow = [o.material, extras.tubeGlow ? extras.tubeGlow[1] : o.material]; }
            if (def.custom === "mutant" && o.name.startsWith("tube0_plate")) { o.material = o.material.clone(); o.material.emissive = new THREE.Color(0x6a20c0); if (extras.tubeGlow) extras.tubeGlow[1] = o.material; else extras.tubeGlow = [o.material, o.material]; }
          }
          if (mdlDef.ids.includes(o.name)) {
            const q = def.params.find((pp) => pp.id === o.name);
            ctl[o.name] = { obj: o, kind: q && q.choices ? "choice" : "knob", par: q };
            o.traverse((x) => { x.userData.ctl = { kind: "pedalCtl", pedal: def.id, param: o.name }; });
          }
          if (o.name === "footswitch" || o.name === "footswitch_bezel") o.traverse((x) => { x.userData.ctl = { kind: "footswitch", pedal: def.id }; });
        });
      });
    }
    const st = { def, grp, ctl, led: ledMat, extras, on: false, values: {}, jack: { y: topY + 0.006, z: jz, x: jx, top: true } };
    for (const q of def.params) st.values[q.id] = toNorm(q, q.def);
    return st;
  }
  function mulberry(a) { return () => { a |= 0; a = a + 0x6D2B79F5 | 0; let t = Math.imul(a ^ a >>> 15, 1 | a); t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t; return ((t ^ t >>> 14) >>> 0) / 4294967296; }; }

  function apply(st) {
    for (const id in st.ctl) {
      const c = st.ctl[id], n = st.values[id];
      if (c.kind === "knob") c.obj.rotation.y = -THREE.MathUtils.degToRad(-135 + 270 * n);
      else if (c.kind === "choice") { const k = c.par.choices.length - 1, idx = Math.round(n * k); c.obj.rotation.y = -THREE.MathUtils.degToRad(c.par.angles ? c.par.angles[idx] : -135 + 270 * idx / k); }
      else if (c.kind === "slider") { const s = c.obj.userData.slider; c.obj.position.set(s.x, s.y, s.z1 + (s.z0 - s.z1) * n); }
      else if (c.kind === "bool") { if (c.side) c.arm.rotation.z = n >= 0.5 ? -0.5 : 0.5; else c.arm.rotation.x = n >= 0.5 ? -0.45 : 0.45; }
    }
    st.led.emissiveIntensity = st.on ? 7 : 0;
    if (st.extras?.bioLed) st.extras.bioLed.emissiveIntensity = st.on ? 5 : 0.15;
  }
  return { build, apply };
}
