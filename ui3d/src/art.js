// Engraved faceplates and badges, painted at runtime at 4096 px across.
// Why textures and not 3D letters: real panels are printed/engraved, so the letters must be
// razor sharp at any zoom and share the metal's lighting. We draw two layers:
//   - colour (metal + inlaid paint) and
//   - height (engraved = low, raised = high), turned into a normal map,
// plus roughness/metalness so paint and bare metal reflect differently.
import * as THREE from "three";
import { drawHazard } from "./materials.js";

const TW = 4096;
function mk(w, h) { const c = document.createElement("canvas"); c.width = w; c.height = h; return [c, c.getContext("2d")]; }

export function heightToNormal(src, strength = 2.0) {
  const w = src.width, h = src.height, s = src.getContext("2d").getImageData(0, 0, w, h).data;
  const [c, g] = mk(w, h); const out = g.createImageData(w, h), d = out.data;
  const H = (x, y) => s[((Math.min(h - 1, Math.max(0, y)) * w + Math.min(w - 1, Math.max(0, x))) << 2)] / 255;
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
    const dx = (H(x + 1, y) - H(x - 1, y)) * strength, dy = (H(x, y + 1) - H(x, y - 1)) * strength;
    const l = Math.hypot(dx, dy, 1), i = (y * w + x) << 2;
    d[i] = (-dx / l * 0.5 + 0.5) * 255; d[i + 1] = (dy / l * 0.5 + 0.5) * 255; d[i + 2] = (1 / l * 0.5 + 0.5) * 255; d[i + 3] = 255;
  }
  g.putImageData(out, 0, 0); return c;
}
function noiseFill(g, w, h, alpha, scaleX = 1, scaleY = 1, seed = 1) {
  // brushed / patina streaks
  let a = seed; const r = () => ((a = (a * 16807) % 2147483647) / 2147483647);
  for (let i = 0; i < w * h / 900; i++) {
    g.fillStyle = `rgba(${r() > 0.5 ? 255 : 0},${r() > 0.5 ? 255 : 0},${r() > 0.5 ? 255 : 0},${alpha * r()})`;
    g.fillRect(r() * w, r() * h, (20 + r() * 260) * scaleX, (1 + r() * 2) * scaleY);
  }
}
function tex(c, srgb) { const t = new THREE.CanvasTexture(c); t.flipY = false; t.anisotropy = 16; if (srgb) t.colorSpace = THREE.SRGBColorSpace; t.needsUpdate = true; return t; }

// --- Overlord control panel ------------------------------------------------------------
export function overlordPanel(L) {
  const k = TW / L.w, W = TW, H = Math.round(L.h * k);
  const P = (x, z) => [(x + L.w / 2) * k, (L.h / 2 - z) * k];   // panel metres -> pixels
  const [col, g] = mk(W, H), [hei, hg] = mk(W, H), [rgh, rg] = mk(W, H);
  // Base: dark antique bronze, brushed, darker towards the edges (handling patina).
  const grd = g.createLinearGradient(0, 0, 0, H); grd.addColorStop(0, "#3a2d1b"); grd.addColorStop(0.5, "#46371f"); grd.addColorStop(1, "#33281a");
  g.fillStyle = grd; g.fillRect(0, 0, W, H); noiseFill(g, W, H, 0.05, 1, 1, 3);
  const vg = g.createRadialGradient(W / 2, H / 2, H * 0.3, W / 2, H / 2, W * 0.6); vg.addColorStop(0, "rgba(0,0,0,0)"); vg.addColorStop(1, "rgba(0,0,0,0.35)"); g.fillStyle = vg; g.fillRect(0, 0, W, H);
  hg.fillStyle = "#808080"; hg.fillRect(0, 0, W, H);
  rg.fillStyle = "#6a6a6a"; rg.fillRect(0, 0, W, H); noiseFill(rg, W, H, 0.12, 1, 1, 9);
  const ink = "#cdb27a";            // light brass inlay (labels, border)
  // Engrave helper: paint the same shape on colour (inlay), height (recess) and roughness.
  const both = (fn) => { g.save(); hg.save(); rg.save(); g.fillStyle = g.strokeStyle = ink; hg.fillStyle = hg.strokeStyle = "#3a3a3a"; rg.fillStyle = rg.strokeStyle = "#9a9a9a"; fn(g); fn(hg); fn(rg); g.restore(); hg.restore(); rg.restore(); };
  // Ornate double border with corner scrolls
  both((c) => {
    c.lineWidth = 6; c.strokeRect(28, 28, W - 56, H - 56); c.lineWidth = 2.5; c.strokeRect(46, 46, W - 92, H - 92);
    for (const [cx, cy, sx, sy] of [[46, 46, 1, 1], [W - 46, 46, -1, 1], [46, H - 46, 1, -1], [W - 46, H - 46, -1, -1]]) {
      c.lineWidth = 3; c.beginPath();
      c.moveTo(cx, cy + sy * 60); c.bezierCurveTo(cx + sx * 10, cy + sy * 20, cx + sx * 20, cy + sy * 10, cx + sx * 60, cy);
      c.moveTo(cx + sx * 14, cy + sy * 40); c.quadraticCurveTo(cx + sx * 30, cy + sy * 30, cx + sx * 40, cy + sy * 14); c.stroke();
      c.beginPath(); c.arc(cx + sx * 26, cy + sy * 26, 7, 0, Math.PI * 2); c.fill();
    }
    // centre scrolls on the long edges
    for (const yy of [46, H - 46]) { c.lineWidth = 3; c.beginPath(); c.moveTo(W * 0.62 - 90, yy); c.bezierCurveTo(W * 0.62 - 40, yy + (yy < H / 2 ? 26 : -26), W * 0.62 + 40, yy + (yy < H / 2 ? 26 : -26), W * 0.62 + 90, yy); c.stroke(); }
  });
  const label = (t, x, z, size = 0.0072, font = "BarlowCondensed") => both((c) => { const [px, py] = P(x, z); c.font = `500 ${Math.round(size * k)}px ${font}`; c.textAlign = "center"; c.textBaseline = "middle"; c.fillText(t, px, py); });
  // Knobs: name above and below (as on the reference), scale ring with 11 ticks
  for (const kn of L.knobs) {
    label(kn.label, kn.x, kn.z + 0.031); label(kn.label, kn.x, kn.z - 0.031);
    both((c) => { const [cx, cy] = P(kn.x, kn.z); c.lineWidth = 3;
      for (let i = 0; i <= 10; i++) { const a = (-135 + 27 * i - 90) * Math.PI / 180, r1 = (kn.r + 0.0032) * k, r2 = r1 + (i % 5 === 0 ? 0.0045 : 0.0028) * k;
        c.beginPath(); c.moveTo(cx + Math.cos(a) * r1, cy + Math.sin(a) * r1); c.lineTo(cx + Math.cos(a) * r2, cy + Math.sin(a) * r2); c.stroke(); }
      c.lineWidth = 2; c.beginPath(); c.arc(cx, cy, (kn.r + 0.0032) * k, (-225) * Math.PI / 180, 45 * Math.PI / 180); c.stroke(); });
  }
  for (const t of L.toggles) { label(t.label, t.x, t.z + 0.031); label(t.label, t.x, t.z - 0.031); label("ON", t.x, t.z + 0.014, 0.0042); label("OFF", t.x, t.z - 0.015, 0.0042); }
  for (const j of L.jacks) { if (j.top) label(j.top, j.x, j.z + 0.031); if (j.bottom) label(j.bottom, j.x, j.z - 0.031); }
  // EQ: title with rule, slider scales, legends
  const s0 = L.sliders[0], s7 = L.sliders[L.sliders.length - 1], tr = s0.travel;
  label("EQ", (s0.x + s7.x) / 2, s0.z + tr / 2 + 0.017, 0.0068);
  both((c) => { const [a1, y1] = P(s0.x - 0.004, s0.z + tr / 2 + 0.017), [a2] = P(s7.x + 0.004, 0); c.lineWidth = 2.5;
    c.beginPath(); c.moveTo(a1, y1); c.lineTo((a1 + a2) / 2 - 40, y1); c.moveTo((a1 + a2) / 2 + 40, y1); c.lineTo(a2, y1); c.stroke();
    for (const s of L.sliders) for (let i = 0; i <= 8; i++) { const [px, py] = P(s.x + 0.0062, s.z - tr / 2 + tr * i / 8); c.fillRect(px - 2, py - 1.5, i % 4 === 0 ? 22 : 13, 3); } });
  label("+12", s0.x - 0.012, s0.z + tr / 2, 0.0046); label("0", s0.x - 0.012, s0.z, 0.0046); label("-12", s0.x - 0.012, s0.z - tr / 2, 0.0046);
  label("150", s0.x, s0.z - tr / 2 - 0.0095, 0.0042); label("MIDRANGE", (s0.x + s7.x) / 2, s0.z - tr / 2 - 0.0105, 0.0066); label("4k", s7.x, s7.z - tr / 2 - 0.0095, 0.0042);
  label("POWER", L.jewel.x, L.jewel.z + 0.019, 0.0052);
  return materialFrom(col, hei, rgh, 2.4, 0.95);
}

// --- Overlord badge: burnished brass, raised polished art ------------------------------
export function overlordBadge(w, h) {
  const k = TW / 2 / w, W = Math.round(w * k), H = Math.round(h * k);
  const [col, g] = mk(W, H), [hei, hg] = mk(W, H), [rgh, rg] = mk(W, H);
  const grd = g.createLinearGradient(0, 0, W, H); grd.addColorStop(0, "#3b2c17"); grd.addColorStop(0.5, "#4a3820"); grd.addColorStop(1, "#33260f");
  g.fillStyle = grd; g.fillRect(0, 0, W, H); noiseFill(g, W, H, 0.06, 1.4, 1, 5);
  hg.fillStyle = "#606060"; hg.fillRect(0, 0, W, H);
  rg.fillStyle = "#8a8a8a"; rg.fillRect(0, 0, W, H); noiseFill(rg, W, H, 0.15, 1.4, 1, 7);
  const polished = "#e2c582";
  const raise = (fn) => { g.save(); hg.save(); rg.save(); g.fillStyle = g.strokeStyle = polished; hg.fillStyle = hg.strokeStyle = "#e6e6e6"; rg.fillStyle = rg.strokeStyle = "#383838"; fn(g); fn(hg); fn(rg); g.restore(); hg.restore(); rg.restore(); };
  // raised rim
  raise((c) => { c.lineWidth = 10; c.strokeRect(16, 16, W - 32, H - 32); });
  // hazard emblem
  const [ec, eg] = mk(H, H); drawHazard(eg, H / 2, H / 2, H * 0.34, "#ffffff");
  for (const [cx, c, color] of [[0, g, polished], [0, hg, "#e6e6e6"], [0, rg, "#383838"]]) { const [tc, tg] = mk(H, H); tg.drawImage(ec, 0, 0); tg.globalCompositeOperation = "source-in"; tg.fillStyle = color; tg.fillRect(0, 0, H, H); c.drawImage(tc, W * 0.04, -H * 0.04); }
  raise((c) => {
    c.textBaseline = "middle"; c.textAlign = "left";
    c.font = `700 ${Math.round(H * 0.4)}px SpaceGrotesk`; c.fillText("ToxicRig", W * 0.3, H * 0.4);
    c.font = `500 ${Math.round(H * 0.17)}px BarlowCondensed`; c.textAlign = "center"; c.fillText("O V E R L O R D", W * 0.6, H * 0.77);
    c.lineWidth = 4; for (const s of [-1, 1]) { const x0 = W * 0.6 + s * W * 0.15; c.beginPath(); c.moveTo(x0, H * 0.77); c.bezierCurveTo(x0 + s * 30, H * 0.7, x0 + s * 60, H * 0.84, x0 + s * 95, H * 0.77); c.stroke(); }
  });
  return materialFrom(col, hei, rgh, 3.0, 1.0);
}

function materialFrom(col, hei, rgh, nStrength, metal) {
  const m = new THREE.MeshPhysicalMaterial({
    map: tex(col, true), normalMap: tex(heightToNormal(hei, nStrength)), roughnessMap: tex(rgh), roughness: 1, metalness: metal,
    clearcoat: 0.15, clearcoatRoughness: 0.4, envMapIntensity: 1.6,
  });
  m.normalScale.set(1, -1);
  return m;
}
