// One library of physically based materials, matched by name to the Blender models.
// Every surface gets a small amount of roughness variation (grunge): real objects are
// never uniformly glossy, and that variation is most of what separates "real" from "plastic".
import * as THREE from "three";

export function makeMaterials(renderer, manager) {
  const loader = new THREE.TextureLoader(manager);
  const aniso = renderer.capabilities.getMaxAnisotropy();
  const cache = {};
  function tex(name, srgb, tile = 1, tileY) {
    // tile = metres covered by one repeat (model UVs are world metres)
    if (!cache[name]) {
      const t = loader.load("tex/" + name);
      t.wrapS = t.wrapT = THREE.RepeatWrapping; t.anisotropy = aniso;
      if (srgb) t.colorSpace = THREE.SRGBColorSpace;
      cache[name] = t;
    }
    const t = cache[name].clone(); t.needsUpdate = true;
    t.repeat.set(1 / tile, 1 / (tileY ?? tile));
    return t;
  }
  const grunge = (tile = 0.5) => tex("grunge.jpg", false, tile);
  const P = (o) => new THREE.MeshPhysicalMaterial(o);
  const S = (o) => new THREE.MeshStandardMaterial(o);
  const brushedR = (t) => tex("brushed_rough.jpg", false, t, t / 2);
  const brushedN = (t) => tex("brushed_nrm.jpg", false, t, t / 2);

  const M = {
    Tolex: P({ map: tex("tolex_col.jpg", true, 0.07), normalMap: tex("tolex_nrm.jpg", false, 0.07), normalScale: new THREE.Vector2(0.7, 0.7),
      roughnessMap: tex("tolex_rough.jpg", false, 0.07), roughness: 1, color: 0x707070, clearcoat: 0.08, clearcoatRoughness: 0.5 }),
    Leather: P({ color: 0x24170f, normalMap: tex("tolex_nrm.jpg", false, 0.06), normalScale: new THREE.Vector2(0.6, 0.6), roughnessMap: tex("tolex_rough.jpg", false, 0.06), roughness: 0.9, clearcoat: 0.3, clearcoatRoughness: 0.3 }),
    Walnut: P({ map: tex("walnut_col.jpg", true, 0.8, 0.4), color: 0x8a7468, normalMap: tex("walnut_nrm.jpg", false, 0.8, 0.4), normalScale: new THREE.Vector2(0.35, 0.35),
      roughnessMap: tex("walnut_rough.jpg", false, 0.8, 0.4), roughness: 1, clearcoat: 0.85, clearcoatRoughness: 0.09, clearcoatRoughnessMap: grunge(0.4) }),
    WalnutDark: P({ map: tex("walnut_col.jpg", true, 0.8, 0.4), color: 0x3c302b, normalMap: tex("walnut_nrm.jpg", false, 0.8, 0.4), normalScale: new THREE.Vector2(0.3, 0.3),
      roughnessMap: tex("walnut_rough.jpg", false, 0.8, 0.4), roughness: 1, clearcoat: 0.8, clearcoatRoughness: 0.1 }),
    Brass: P({ color: 0xd8ad5c, metalness: 1, roughness: 1.3, envMapIntensity: 2.2, roughnessMap: brushedR(0.3), normalMap: brushedN(0.3), normalScale: new THREE.Vector2(0.25, 0.25), anisotropy: 0.6 }),
    BrassKnob: P({ color: 0xe8bd70, metalness: 1, roughness: 0.4, envMapIntensity: 1.5, roughnessMap: grunge(0.08), clearcoat: 0.3 }),
    BrandBrass: P({ color: 0xc89a50, metalness: 1, roughness: 0.5, roughnessMap: grunge(0.1) }),
    KnobGold: P({ color: 0xc9a25a, metalness: 1, roughness: 0.55, roughnessMap: grunge(0.06), anisotropy: 0.4 }),
    KnobGoldTop: P({ color: 0xd9b874, metalness: 1, roughness: 0.3, roughnessMap: brushedR(0.03), normalMap: brushedN(0.03), normalScale: new THREE.Vector2(0.4, 0.4) }),
    Chrome: S({ color: 0xdcdce0, metalness: 1, roughness: 0.24, roughnessMap: grunge(0.15), envMapIntensity: 0.6 }),
    Nickel: S({ color: 0xb8b4ac, metalness: 1, roughness: 0.4, roughnessMap: grunge(0.1) }),
    CornerMetal: S({ color: 0x2b2c30, metalness: 1, roughness: 0.75, roughnessMap: grunge(0.2) }),
    BlackMetal: S({ color: 0x1d1d20, metalness: 1, roughness: 0.75, roughnessMap: grunge(0.2) }),
    SteelPlate: P({ color: 0xa4a6aa, metalness: 1, roughness: 0.6, roughnessMap: brushedR(0.15), normalMap: brushedN(0.15), normalScale: new THREE.Vector2(0.3, 0.3), anisotropy: 0.7 }),
    PanelSteel: P({ color: 0x45474c, metalness: 1, roughness: 0.75, roughnessMap: brushedR(0.25), normalMap: brushedN(0.25), normalScale: new THREE.Vector2(0.25, 0.25), anisotropy: 0.8 }),
    PanelBlack: P({ color: 0x0c0c0d, metalness: 0.35, roughness: 0.9, roughnessMap: grunge(0.3), clearcoat: 0.4, clearcoatRoughness: 0.35 }),
    KnobBlack: P({ color: 0x0b0b0c, roughness: 0.75, roughnessMap: grunge(0.05), clearcoat: 0.6, clearcoatRoughness: 0.2 }),
    Bakelite: P({ color: 0x0e0d0c, roughness: 0.6, roughnessMap: grunge(0.05), clearcoat: 0.6, clearcoatRoughness: 0.15 }),
    PointerWhite: S({ color: 0xeeeeea, roughness: 0.5 }),
    PaintWhite: S({ color: 0xe9e3d3, roughness: 0.45 }),
    EngraveWhite: S({ color: 0xdcd9d0, roughness: 0.6 }),
    EngraveDark: S({ color: 0x0a0a0a, roughness: 0.7 }),
    Engrave: S({ color: 0x080706, roughness: 0.75 }),
    Rubber: S({ color: 0x0b0b0b, roughness: 0.92 }),
    BasketMetal: S({ color: 0x1c1c1e, metalness: 0.8, roughness: 0.55 }),
    Cone: S({ color: 0x2c251e, roughness: 0.95, normalMap: tex("tolex_nrm.jpg", false, 0.2), normalScale: new THREE.Vector2(0.15, 0.15) }),
    DustCap: S({ color: 0x0d0c0b, roughness: 0.55 }),
    Baffle: S({ color: 0x040404, roughness: 1 }),
    HexMesh: S({ map: tex("hex.png", true, 0.018, 0.018 * Math.sqrt(3) / 3), color: 0xb0b2b8, metalness: 0.9, roughness: 0.45, alphaTest: 0.5, side: THREE.DoubleSide }),
    SilverCloth: S({ map: tex("silvercloth.png", true, 0.008), color: 0x8a8e96, normalMap: tex("cloth_nrm.png", false, 0.008), metalness: 0.5, roughness: 0.5, alphaTest: 0.5, side: THREE.DoubleSide }),
    Grille: S({ map: tex("brit_cloth.png", true, 0.008), normalMap: tex("cloth_nrm.png", false, 0.008), roughness: 0.9, alphaTest: 0.5, side: THREE.DoubleSide, color: 0xd8cbb8 }),
    Glass: P({ color: 0xeefff2, roughness: 0.04, transmission: 1, thickness: 0.0005, ior: 1.5, envMapIntensity: 0.8, specularIntensity: 1 }),
    Heater: S({ color: 0x113311, emissive: 0x5cff3a, emissiveIntensity: 6 }),
    TubePlate: S({ color: 0x2e3032, metalness: 1, roughness: 0.45, emissive: 0x1e6a10, emissiveIntensity: 0.2 }),
    Getter: S({ color: 0x9a9aa0, metalness: 1, roughness: 0.05 }),
    WindowBack: S({ color: 0x050805, emissive: 0x1d5a10, emissiveIntensity: 0.12, roughness: 0.7 }),
    VentGlow: S({ color: 0x050308, emissive: 0x7a3cff, emissiveIntensity: 0.9, roughness: 0.8 }),
    Jewel: S({ color: 0x103a10, emissive: 0x39ff14, emissiveIntensity: 4, roughness: 0.1 }),
  };
  // --- Overlord / Vapor-30 finishes
  Object.assign(M, {
    AntiqueBrass: P({ color: 0xb08a4a, metalness: 1, roughness: 0.6, roughnessMap: grunge(0.1), envMapIntensity: 1.6 }),
    BurnishedBrass: P({ color: 0x5a4424, metalness: 1, roughness: 0.75, roughnessMap: grunge(0.08), envMapIntensity: 1.2 }),
    PanelBronze: P({ color: 0x3d311f, metalness: 0.95, roughness: 0.8, roughnessMap: brushedR(0.2), normalMap: brushedN(0.2), normalScale: new THREE.Vector2(0.2, 0.2), envMapIntensity: 1.4 }),
    KnobCopper: P({ color: 0xb8704a, metalness: 1, roughness: 0.5, roughnessMap: grunge(0.06), envMapIntensity: 1.5 }),
    KnobDarkCopper: P({ color: 0x6a3e2a, metalness: 1, roughness: 0.55, roughnessMap: grunge(0.06), envMapIntensity: 1.4 }),
    PointerGreen: S({ color: 0x0a2a06, emissive: 0x39ff14, emissiveIntensity: 3 }),
    VentGlowGreen: S({ color: 0x030603, emissive: 0x2a9a14, emissiveIntensity: 0.5, roughness: 0.8 }),
    TubeLightGreen: S({ color: 0x0a2a06, emissive: 0x46ff2a, emissiveIntensity: 5 }),
    TubeLightPurple: S({ color: 0x1a0a2a, emissive: 0xb050ff, emissiveIntensity: 5 }),
    WindowBackPurple: S({ color: 0x080510, emissive: 0x6a2ab0, emissiveIntensity: 0.3, roughness: 0.7 }),
    AluBrushed: P({ color: 0xc9cbd0, metalness: 1, roughness: 0.75, roughnessMap: brushedR(0.25), normalMap: brushedN(0.25), normalScale: new THREE.Vector2(0.3, 0.3), anisotropy: 0.8, envMapIntensity: 1.3 }),
    AluPanel: P({ color: 0xd5d7dc, metalness: 1, roughness: 0.7, roughnessMap: brushedR(0.15), normalMap: brushedN(0.15), normalScale: new THREE.Vector2(0.25, 0.25), anisotropy: 0.9, envMapIntensity: 1.4 }),
    CornerPurple: P({ color: 0x5a2a9a, metalness: 0.9, roughness: 0.4, roughnessMap: grunge(0.1), clearcoat: 0.4, envMapIntensity: 1.3 }),
    PurplePlate: P({ color: 0x6a3aa8, metalness: 0.9, roughness: 0.35, roughnessMap: brushedR(0.1), normalMap: brushedN(0.1), envMapIntensity: 1.4 }),
    EngraveLight: S({ color: 0xe6dcff, roughness: 0.6 }),
    LedPurple: S({ color: 0x1a0a2a, emissive: 0xb050ff, emissiveIntensity: 4 }),
    JewelPurple: S({ color: 0x1a0a2a, emissive: 0xb050ff, emissiveIntensity: 4, roughness: 0.1 }),
    Acrylic: P({ color: 0xe8dcff, roughness: 0.08, transmission: 0.85, thickness: 0.01, ior: 1.49, emissive: 0x8a40ff, emissiveIntensity: 0.6, envMapIntensity: 1.2 }),
    AcrylicRing: P({ color: 0xd8c8ff, roughness: 0.15, transmission: 0.6, thickness: 0.004, emissive: 0x9b4dff, emissiveIntensity: 1.6 }),
    AcrylicCore: S({ color: 0x2a1040, emissive: 0xb050ff, emissiveIntensity: 2.5 }),
  });
  // --- stage props
  Object.assign(M, {
    GuitarPaint: P({ color: 0x3a1466, metalness: 0.55, roughness: 0.35, clearcoat: 1, clearcoatRoughness: 0.04, envMapIntensity: 1.6 }),
    GuardPearl: P({ color: 0xe8e4dc, roughness: 0.35, clearcoat: 0.8, iridescence: 0.4, iridescenceIOR: 1.4 }),
    Maple: P({ color: 0xc9945a, roughness: 0.45, clearcoat: 0.7, clearcoatRoughness: 0.2, map: tex("walnut_col.jpg", true, 0.3, 0.15) }),
    Rosewood: S({ color: 0x3a2418, roughness: 0.7, map: tex("walnut_col.jpg", true, 0.2, 0.1) }),
    PickupBlack: P({ color: 0x0c0c0d, roughness: 0.35, clearcoat: 0.5 }),
    StringSteel: S({ color: 0xd0d0d4, metalness: 1, roughness: 0.25 }),
    StandBlack: P({ color: 0x111113, metalness: 0.6, roughness: 0.45, clearcoat: 0.5, roughnessMap: grunge(0.2) }),
    CaseBlack: S({ color: 0x0f0f11, roughness: 0.85, normalMap: tex("tolex_nrm.jpg", false, 0.08), normalScale: new THREE.Vector2(0.5, 0.5), roughnessMap: grunge(0.4) }),
    Aluminium: S({ color: 0xb8bcc2, metalness: 1, roughness: 0.35, roughnessMap: brushedR(0.2) }),
    StencilWhite: S({ color: 0xd8d4c8, roughness: 0.8 }),
    DrumWrap: P({ color: 0x1a0d2e, metalness: 0.7, roughness: 0.3, clearcoat: 1, clearcoatRoughness: 0.05, envMapIntensity: 1.4 }),
    DrumHead: S({ color: 0xe8e2d4, roughness: 0.7 }),
    Bronze: P({ color: 0xc08a3e, metalness: 1, roughness: 0.32, roughnessMap: brushedR(0.08), envMapIntensity: 1.6 }),
  });
  Object.assign(M, {
    KnobBrass: P({ color: 0xc9a663, metalness: 1, roughness: 0.42, roughnessMap: grunge(0.04), envMapIntensity: 1.8 }),
    KnobCopper: P({ color: 0xa8624a, metalness: 1, roughness: 0.42, roughnessMap: grunge(0.04), envMapIntensity: 1.8 }),
    KnobDarkBronze: P({ color: 0x4e3a2c, metalness: 1, roughness: 0.48, roughnessMap: grunge(0.04), envMapIntensity: 1.6 }),
    KnobSteel: P({ color: 0x8e9196, metalness: 1, roughness: 0.4, roughnessMap: grunge(0.04), envMapIntensity: 1.8 }),
    KnobTopBrushed: P({ color: 0xd8c08a, metalness: 1, roughness: 0.32, roughnessMap: brushedR(0.02), normalMap: brushedN(0.02), normalScale: new THREE.Vector2(0.35, 0.35), envMapIntensity: 1.8 }),
    LedGreen: S({ color: 0x0a2a06, emissive: 0x46ff2a, emissiveIntensity: 5 }),
    Mica: S({ color: 0xd8d2c4, roughness: 0.5, transparent: true, opacity: 0.8 }),
    VentBackDark: S({ color: 0x050507, roughness: 0.9 }),
  });
  M.HexMeshDark = M.HexMesh.clone(); M.HexMeshDark.color = new THREE.Color(0x2a2b2f); M.HexMeshDark.roughness = 0.55;
  M.HexMeshPurple = M.HexMesh.clone(); M.HexMeshPurple.color = new THREE.Color(0x7a4ac0);
  M.Emblem = new THREE.MeshStandardMaterial({ map: emblemTexture("#c9a25a", "rgba(0,0,0,0)"), transparent: true, metalness: 1, roughness: 0.45, envMapIntensity: 1.5, polygonOffset: true, polygonOffsetFactor: -2 });
  M.Glass.userData.glass = true;
  return { M, tex };
}

// Biohazard-style hazard emblem drawn with circles (original drawing, no artwork copied).
export function drawHazard(g, cx, cy, R, color) {
  const c = document.createElement("canvas"); c.width = c.height = Math.ceil(R * 2.4); const k = c.getContext("2d");
  const o = c.width / 2;
  k.fillStyle = color;
  for (let i = 0; i < 3; i++) {
    const a = -Math.PI / 2 + i * Math.PI * 2 / 3;
    k.beginPath(); k.arc(o + Math.cos(a) * R * 0.42, o + Math.sin(a) * R * 0.42, R * 0.52, 0, Math.PI * 2); k.fill();
  }
  k.globalCompositeOperation = "destination-out";
  for (let i = 0; i < 3; i++) {
    const a = -Math.PI / 2 + i * Math.PI * 2 / 3;
    k.beginPath(); k.arc(o + Math.cos(a) * R * 0.62, o + Math.sin(a) * R * 0.62, R * 0.38, 0, Math.PI * 2); k.fill();
  }
  k.beginPath(); k.arc(o, o, R * 0.14, 0, Math.PI * 2); k.fill();
  k.lineWidth = R * 0.06;
  for (let i = 0; i < 3; i++) { const a = Math.PI / 2 + i * Math.PI * 2 / 3; k.beginPath(); k.moveTo(o, o); k.lineTo(o + Math.cos(a) * R * 0.5, o + Math.sin(a) * R * 0.5); k.stroke(); }
  k.globalCompositeOperation = "source-over";
  k.strokeStyle = color; k.lineWidth = R * 0.07; k.beginPath(); k.arc(o, o, R * 0.33, 0, Math.PI * 2); k.stroke();
  k.globalCompositeOperation = "destination-out"; k.lineWidth = R * 0.1;
  for (let i = 0; i < 3; i++) { const a = Math.PI / 2 + i * Math.PI * 2 / 3; k.beginPath(); k.moveTo(o + Math.cos(a) * R * 0.25, o + Math.sin(a) * R * 0.25); k.lineTo(o + Math.cos(a) * R * 0.42, o + Math.sin(a) * R * 0.42); k.stroke(); }
  g.drawImage(c, cx - o, cy - o);
}
function emblemTexture(color, bg) {
  const c = document.createElement("canvas"); c.width = c.height = 256; const g = c.getContext("2d");
  g.fillStyle = bg; g.fillRect(0, 0, 256, 256); drawHazard(g, 128, 128, 104, color);
  const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; return t;
}
