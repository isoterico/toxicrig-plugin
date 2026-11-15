// Microphones on a boom stand, aimed at the speaker. Position/distance follow the plugin's
// mic_pos (0 = dust cap, 1 = cone edge) and mic_dist (0 = touching the grille, 1 = ~30 cm).
import * as THREE from "three";

export function micFactory(M, tex) {
  const darkMetal = new THREE.MeshStandardMaterial({ color: 0x2c2e31, metalness: 0.9, roughness: 0.42, roughnessMap: tex("grunge.jpg", false, 0.1) });
  const satin = new THREE.MeshStandardMaterial({ color: 0xc9c2b4, metalness: 1, roughness: 0.32, roughnessMap: tex("grunge.jpg", false, 0.1) });
  const blackPaint = new THREE.MeshPhysicalMaterial({ color: 0x0d0d0f, metalness: 0.4, roughness: 0.45, clearcoat: 0.6, clearcoatRoughness: 0.2 });
  const mesh = new THREE.MeshStandardMaterial({ map: tex("hex.png", true, 0.004, 0.004 * Math.sqrt(3) / 3), color: 0xb8bcc2, metalness: 1, roughness: 0.35, alphaTest: 0.5, side: THREE.DoubleSide });
  const inner = new THREE.MeshStandardMaterial({ color: 0x050505, roughness: 1 });
  const gold = new THREE.MeshStandardMaterial({ color: 0xd9ad55, metalness: 1, roughness: 0.25 });

  // Each mic points along -Z (towards the speaker); its back end is at z = +len.
  function dynamic() {
    const g = new THREE.Group();
    const lathe = (pts, mat) => { const m = new THREE.Mesh(new THREE.LatheGeometry(pts.map(([r, y]) => new THREE.Vector2(r, y)), 48), mat); m.rotation.x = -Math.PI / 2; g.add(m); return m; };
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
  function stand() {
    const g = new THREE.Group();
    const pole = new THREE.Mesh(new THREE.CylinderGeometry(0.009, 0.009, 1, 16), blackPaint); pole.name = "pole";
    const boom = new THREE.Mesh(new THREE.CylinderGeometry(0.007, 0.007, 1, 16), blackPaint); boom.name = "boom";
    const clutch = new THREE.Mesh(new THREE.CylinderGeometry(0.016, 0.016, 0.04, 16), blackPaint); clutch.name = "clutch";
    g.add(pole, boom, clutch);
    for (let i = 0; i < 3; i++) {
      const leg = new THREE.Mesh(new THREE.CylinderGeometry(0.006, 0.006, 0.32, 10), blackPaint); leg.name = "leg" + i; g.add(leg);
    }
    g.traverse((m) => { if (m.isMesh) { m.castShadow = true; m.receiveShadow = true; } });
    return g;
  }
  const between = (m, a, b) => { const d = new THREE.Vector3().subVectors(b, a); m.position.copy(a).addScaledVector(d, 0.5); m.scale.set(1, d.length() / m.geometry.parameters.height, 1); m.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), d.normalize()); };

  function rig() {
    const root = new THREE.Group();
    const mics = { dyn: dynamic(), ribbon: ribbon(), cond: condenser() };
    for (const k in mics) { mics[k].traverse((m) => { if (m.isMesh) { m.castShadow = true; m.userData.ctl = { kind: "mic" }; } }); root.add(mics[k]); }
    const st = stand(); root.add(st);
    st.traverse((m) => { m.userData.ctl = { kind: "mic" }; });
    function place(type, speaker, frontZ, pos, dist) {
      for (const k in mics) mics[k].visible = k === type;
      const m = mics[type];
      const tip = new THREE.Vector3(speaker.x + pos * speaker.r * 0.92, speaker.y, frontZ + 0.012 + dist * 0.29);
      m.position.copy(tip); m.rotation.set(0, 0, 0);
      const back = tip.clone().add(new THREE.Vector3(0, m.userData.vertical ? 0.06 : 0, m.userData.vertical ? 0.03 : m.userData.len));
      // Boom stand: base out to the side and in front, boom reaches the mic's back end.
      const side = speaker.x <= 0 ? -1 : 1;
      const base = new THREE.Vector3(speaker.x + side * 0.55, -0.012, tip.z + 0.22);
      const top = new THREE.Vector3(base.x, Math.max(tip.y + 0.12, 0.4), base.z);
      between(st.getObjectByName("pole"), base, top);
      between(st.getObjectByName("boom"), top.clone().add(new THREE.Vector3(-side * 0.1, 0.05, 0.08)), back);
      st.getObjectByName("clutch").position.copy(top);
      for (let i = 0; i < 3; i++) {
        const a = i * Math.PI * 2 / 3 + 0.4; const foot = base.clone().add(new THREE.Vector3(Math.cos(a) * 0.26, 0, Math.sin(a) * 0.26));
        between(st.getObjectByName("leg" + i), base.clone().add(new THREE.Vector3(0, 0.1, 0)), foot);
      }
      return { tip, back };
    }
    return { root, place };
  }
  return { rig };
}
