// Die-cast stompboxes: rounded enclosure with a slight draft, base plate and rubber feet,
// side jacks with nuts, knurled knobs, threaded footswitch, LED bezel and silkscreen graphics
// printed into the paint (so the print takes the same light and clearcoat as the metal).
import * as THREE from "three";
import { toNorm } from "./pedals.js";

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
    const st = { def, grp, ctl, led: ledMat, on: def.id === "gate", values: {}, jack: { y: jy, z: jz } };
    for (const q of def.params) st.values[q.id] = toNorm(q, q.def);
    return st;
  }

  function apply(st) {
    for (const id in st.ctl) {
      const c = st.ctl[id], n = st.values[id];
      if (c.kind === "knob") c.obj.rotation.y = -THREE.MathUtils.degToRad(-135 + 270 * n);
      else if (c.kind === "choice") { const k = c.par.choices.length - 1; c.obj.rotation.y = -THREE.MathUtils.degToRad(-135 + 270 * Math.round(n * k) / k); }
      else if (c.kind === "slider") { const s = c.obj.userData.slider; c.obj.position.set(s.x, s.y, s.z1 + (s.z0 - s.z1) * n); }
      else if (c.kind === "bool") c.arm.rotation.x = n >= 0.5 ? -0.45 : 0.45;
    }
    st.led.emissiveIntensity = st.on ? 7 : 0;
  }
  return { build, apply };
}
