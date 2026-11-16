// Microphones on a boom stand, aimed at the speaker. Position/distance follow the plugin's
// mic_pos (0 = dust cap, 1 = cone edge) and mic_dist (0 = touching the grille, 1 = ~30 cm).
import * as THREE from "three";

export function micFactory(M, tex) {
  const darkMetal = new THREE.MeshStandardMaterial({ color: 0x2c2e31, metalness: 0.9, roughness: 0.42, roughnessMap: tex("grunge.jpg", false, 0.1) });
  const satin = new THREE.MeshStandardMaterial({ color: 0xc9c2b4, metalness: 1, roughness: 0.32, roughnessMap: tex("grunge.jpg", false, 0.1) });
  const blackPaint = new THREE.MeshPhysicalMaterial({ color: 0x0d0d0f, metalness: 0.4, roughness: 0.45, clearcoat: 0.6, clearcoatRoughness: 0.2 });
  // Mic grilles: woven steel mesh. Solid (not cut-out): at this size real mesh reads as a
  // fine metallic texture with dark gaps, which is what the hex map gives as colour/roughness.
  const hexT = tex("hex.png", true, 0.0025, 0.0025 * Math.sqrt(3) / 3);
  const mesh = new THREE.MeshStandardMaterial({ map: hexT, color: 0xc4c8ce, metalness: 1, roughness: 0.38, side: THREE.DoubleSide });
  const inner = new THREE.MeshStandardMaterial({ color: 0x050505, roughness: 1 });
  const gold = new THREE.MeshStandardMaterial({ color: 0xd9ad55, metalness: 1, roughness: 0.25 });

  // Each mic points along -Z (towards the speaker); its back end is at z = +len.
  function dynamic() {
    const g = new THREE.Group();
    const lathe = (pts, mat) => { const m = new THREE.Mesh(new THREE.LatheGeometry(pts.map(([r, y]) => new THREE.Vector2(r, y)), 48), mat); m.rotation.x = Math.PI / 2; g.add(m); return m; };
    lathe([[0.0, 0.0], [0.0105, 0.001], [0.0125, 0.008], [0.0125, 0.03]], inner);
    const gr = new THREE.Mesh(new THREE.CylinderGeometry(0.0128, 0.0128, 0.03, 48, 1, true), mesh); gr.rotation.x = Math.PI / 2; gr.position.z = 0.016; g.add(gr);
    const capm = new THREE.Mesh(new THREE.CircleGeometry(0.0128, 48), mesh); capm.position.z = 0.0005; capm.rotation.y = Math.PI; g.add(capm);
    lathe([[0.013, 0.03], [0.0145, 0.034], [0.0135, 0.05], [0.011, 0.12], [0.0105, 0.16], [0.0, 0.161]], darkMetal);
    const ring = new THREE.Mesh(new THREE.TorusGeometry(0.0136, 0.0012, 8, 48), satin); ring.position.z = 0.032; g.add(ring);
    g.userData.len = 0.16; return g;
  }
  function ribbon() {
    const g = new THREE.Group();
    const head = new THREE.Mesh(new THREE.BoxGeometry(0.044, 0.07, 0.024), mesh); head.rotation.x = Math.PI / 2; head.position.z = 0.035;
    // a ribbon mic is side-addressed: the wide face looks at the speaker
    const headIn = new THREE.Mesh(new THREE.BoxGeometry(0.04, 0.066, 0.02), inner); headIn.rotation.x = Math.PI / 2; headIn.position.z = 0.035;
    const frame = new THREE.Mesh(new THREE.BoxGeometry(0.048, 0.006, 0.028), satin); frame.position.z = 0.0;
    const frame2 = frame.clone(); frame2.position.z = 0.07;
    const body = new THREE.Mesh(new THREE.CylinderGeometry(0.012, 0.012, 0.09, 32), blackPaint); body.rotation.x = Math.PI / 2; body.position.z = 0.118;
    const neck = new THREE.Mesh(new THREE.CylinderGeometry(0.009, 0.012, 0.012, 32), satin); neck.rotation.x = Math.PI / 2; neck.position.z = 0.076;
    g.add(headIn, head, frame, frame2, neck, body); g.userData.len = 0.163; return g;
  }
  function condenser() {
    // Large-diaphragm, side-addressed: body vertical, the logo side faces the speaker (-Z).
    const g = new THREE.Group(), h = new THREE.Group();
    const headIn = new THREE.Mesh(new THREE.CylinderGeometry(0.023, 0.023, 0.07, 32), inner);
    const head = new THREE.Mesh(new THREE.CylinderGeometry(0.026, 0.026, 0.075, 48, 1, true), mesh);
    const top = new THREE.Mesh(new THREE.SphereGeometry(0.026, 32, 16, 0, Math.PI * 2, 0, Math.PI / 2), mesh); top.position.y = 0.0375;
    const bot = top.clone(); bot.rotation.x = Math.PI; bot.position.y = -0.0375;
    const band = new THREE.Mesh(new THREE.CylinderGeometry(0.0268, 0.0268, 0.006, 48), satin); band.position.y = -0.045;
    const body = new THREE.Mesh(new THREE.CylinderGeometry(0.026, 0.023, 0.11, 48), satin); body.position.y = -0.1;
    const logo = new THREE.Mesh(new THREE.CircleGeometry(0.004, 24), gold); logo.position.set(0, -0.06, -0.0262); logo.rotation.y = Math.PI;
    h.add(headIn, head, top, bot, band, body, logo); h.position.z = 0.028; g.add(h);
    g.userData.len = 0.06; g.userData.vertical = true; return g;
  }
  // Boom stand built from real parts: tripod base, pole, clutch, boom arm passing through the
  // clutch with a counterweight behind, swivel clip at the boom tip holding the mic.
  function stand() {
    const g = new THREE.Group();
    const part = (name, geo, m = blackPaint) => { const x = new THREE.Mesh(geo, m); x.name = name; x.castShadow = x.receiveShadow = true; g.add(x); return x; };
    part("pole", new THREE.CylinderGeometry(0.0095, 0.0095, 1, 20));
    part("poleUpper", new THREE.CylinderGeometry(0.0075, 0.0075, 1, 20), satin);
    part("hub", new THREE.CylinderGeometry(0.02, 0.022, 0.05, 20));
    part("clutch", new THREE.CylinderGeometry(0.017, 0.017, 0.045, 20));
    part("knob", new THREE.CylinderGeometry(0.012, 0.012, 0.012, 16));
    part("boom", new THREE.CylinderGeometry(0.0075, 0.0075, 1, 16));
    part("weight", new THREE.CylinderGeometry(0.02, 0.02, 0.06, 20));
    part("clip", new THREE.CylinderGeometry(0.009, 0.009, 0.03, 16));
    part("swivel", new THREE.SphereGeometry(0.009, 16, 8));
    part("clipRing", new THREE.TorusGeometry(0.0145, 0.0032, 12, 32, Math.PI * 1.6), blackPaint);
    for (let i = 0; i < 3; i++) { part("leg" + i, new THREE.CylinderGeometry(0.0065, 0.0065, 1, 12)); part("foot" + i, new THREE.SphereGeometry(0.009, 12, 8), M.Rubber); }
    return g;
  }
  const between = (m, a, b) => { const d = new THREE.Vector3().subVectors(b, a); m.position.copy(a).addScaledVector(d, 0.5); m.scale.set(1, d.length(), 1); m.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), d.normalize()); };

  function rig() {
    const root = new THREE.Group();
    const mics = { dyn: dynamic(), ribbon: ribbon(), cond: condenser() };
    for (const k in mics) { mics[k].traverse((m) => { if (m.isMesh) { m.castShadow = true; m.userData.ctl = { kind: "mic" }; } }); root.add(mics[k]); }
    const st = stand(); root.add(st);
    st.traverse((m) => { m.userData.ctl = { kind: "mic" }; });
    const P = (n) => st.getObjectByName(n);
    function place(type, speaker, frontZ, pos, dist) {
      for (const k in mics) mics[k].visible = k === type;
      const m = mics[type];
      const tip = new THREE.Vector3(speaker.x + pos * speaker.r * 0.92, speaker.y, frontZ + 0.012 + dist * 0.29);
      m.position.copy(tip); m.rotation.set(0, 0, 0);
      const side = speaker.x <= 0 ? -1 : 1;
      // Where the clip grabs the mic: the back of a pencil mic, the bottom of a big condenser.
      // One attachment point on the mic body's axis. Clip, swivel and boom all derive from it,
      // so the arm always ends exactly on the surface of the body it holds.
      const grip = m.userData.vertical ? tip.clone().add(new THREE.Vector3(0, -0.12, 0.028)) : tip.clone().add(new THREE.Vector3(0, 0, 0.12));
      const bodyR = m.userData.vertical ? 0.026 : 0.0115;
      const axisV = m.userData.vertical ? new THREE.Vector3(0, 1, 0) : new THREE.Vector3(0, 0, 1);
      const clutch = new THREE.Vector3(grip.x + side * 0.42, grip.y + (m.userData.vertical ? -0.02 : 0.1), grip.z + 0.2);
      const base = new THREE.Vector3(clutch.x, -0.012, clutch.z);
      const hubY = 0.12;
      between(P("pole"), base.clone().setY(hubY), clutch.clone().setY(Math.max(hubY + 0.05, clutch.y * 0.55)));
      between(P("poleUpper"), clutch.clone().setY(Math.max(hubY + 0.05, clutch.y * 0.55) - 0.02), clutch.clone().add(new THREE.Vector3(0, -0.02, 0)));
      P("hub").position.copy(base).setY(hubY);
      P("clutch").position.copy(clutch);
      // Boom end: the point on the body surface that faces the arm (projected off the body axis).
      const toArm = clutch.clone().sub(grip);
      const t = toArm.clone().addScaledVector(axisV, -toArm.dot(axisV)).normalize();
      const boomEnd = grip.clone().addScaledVector(t, bodyR + 0.002);
      const dir = boomEnd.clone().sub(clutch).normalize();
      P("clutch").quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), new THREE.Vector3(-dir.z, 0, dir.x).normalize());
      P("knob").position.copy(clutch).add(new THREE.Vector3(-dir.z, 0, dir.x).normalize().multiplyScalar(0.028));
      P("knob").quaternion.copy(P("clutch").quaternion);
      const tail = clutch.clone().addScaledVector(dir, -0.2);
      between(P("boom"), tail, boomEnd);
      P("weight").position.copy(tail); P("weight").quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), dir);
      // Swivel joint sits exactly on the body surface: the arm ends there.
      P("swivel").position.copy(boomEnd);
      // Clip: short bracket between the swivel and the body, aligned with the arm's last bit.
      P("clip").position.copy(boomEnd);
      P("clip").quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), t);
      P("clip").scale.set(1, 1, 1);
      // Horseshoe clip wrapped around the body at the attachment point, sized to its radius.
      P("clipRing").position.copy(grip);
      P("clipRing").scale.setScalar((bodyR + 0.003) / 0.0145);
      P("clipRing").quaternion.setFromUnitVectors(new THREE.Vector3(0, 0, 1), axisV).multiply(new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(0, 0, 1), Math.PI * 0.7));
      for (let i = 0; i < 3; i++) {
        const a = i * Math.PI * 2 / 3 + 0.5; const foot = base.clone().add(new THREE.Vector3(Math.cos(a) * 0.27, 0.0, Math.sin(a) * 0.27));
        between(P("leg" + i), base.clone().setY(hubY - 0.01), foot.clone().setY(-0.004));
        P("foot" + i).position.copy(foot).setY(-0.004);
      }
      // Cable route: from the mic, along the boom, down the pole, out on the floor.
      const route = [grip.clone().add(new THREE.Vector3(0, -0.012, 0.01)), boomEnd.clone().lerp(clutch, 0.5).add(new THREE.Vector3(0, -0.02, 0)),
        clutch.clone().add(new THREE.Vector3(0.012 * side, -0.05, 0)), base.clone().add(new THREE.Vector3(0.016 * side, 0.18, 0.01)), base.clone().add(new THREE.Vector3(0.05 * side, -0.004, 0.08))];
      return { tip, back: grip, route };
    }
    return { root, place, mics };
  }
  return { rig };
}
