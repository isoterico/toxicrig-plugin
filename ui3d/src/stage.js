// The room: lights, reflections, club stage set, haze, beams, dust and the camera post-processing.
import * as THREE from "three";
import { EffectComposer } from "three/examples/jsm/postprocessing/EffectComposer.js";
import { RenderPass } from "three/examples/jsm/postprocessing/RenderPass.js";
import { UnrealBloomPass } from "three/examples/jsm/postprocessing/UnrealBloomPass.js";
import { OutputPass } from "three/examples/jsm/postprocessing/OutputPass.js";
import { GTAOPass } from "three/examples/jsm/postprocessing/GTAOPass.js";
import { BokehPass } from "three/examples/jsm/postprocessing/BokehPass.js";
import { ShaderPass } from "three/examples/jsm/postprocessing/ShaderPass.js";

export function buildStage(renderer, scene, camera, tex, M) {
  scene.background = new THREE.Color(0x040406);
  scene.fog = new THREE.FogExp2(0x07060a, 0.16);

  // ---------------- reflections: a small virtual club (softbox, neon strips, par cans)
  const env = new THREE.Scene();
  const bm = (r, g, b) => new THREE.MeshBasicMaterial({ color: new THREE.Color(r, g, b), side: THREE.DoubleSide });
  const room = new THREE.Mesh(new THREE.BoxGeometry(12, 6, 12), bm(0.012, 0.011, 0.016)); room.material.side = THREE.BackSide; room.position.y = 2.5; env.add(room);
  const panel = (w, h, mat, pos, look) => { const m = new THREE.Mesh(new THREE.PlaneGeometry(w, h), mat); m.position.set(...pos); m.lookAt(...look); env.add(m); };
  panel(2.2, 1.4, bm(3.2, 2.8, 2.3), [-2.5, 3.2, 2.6], [0, 0.4, 0]);    // warm key softbox
  panel(4.0, 0.12, bm(0.9, 0.35, 2.4), [2.8, 1.6, -2.2], [0, 0.5, 0]);  // violet strip
  panel(4.0, 0.12, bm(0.35, 2.2, 0.25), [-2.8, 1.2, -2.4], [0, 0.5, 0]);// green strip
  panel(0.5, 0.5, bm(2.5, 2.4, 2.2), [0.6, 4.2, 0.8], [0, 0, 0]);       // top par
  panel(8, 0.3, bm(0.25, 0.22, 0.2), [0, 0.15, 4], [0, 0.4, 0]);        // faint front fill
  const pmrem = new THREE.PMREMGenerator(renderer);
  scene.environment = pmrem.fromScene(env, 0.02).texture;
  scene.environmentIntensity = 0.55;

  // ---------------- lights
  const L = {};
  L.key = new THREE.SpotLight(0xffd9b8, 0, 9, 0.5, 0.75, 1.6);
  L.key.position.set(-1.6, 2.6, 2.3); L.key.target.position.set(0, 0.35, 0.25);
  L.key.castShadow = true; L.key.shadow.mapSize.set(2048, 2048); L.key.shadow.bias = -0.0001; L.key.shadow.normalBias = 0.01;
  L.key.shadow.camera.near = 1; L.key.shadow.camera.far = 7;
  L.rimV = new THREE.SpotLight(0x8b46ff, 0, 9, 0.5, 0.7, 1.4); L.rimV.position.set(2.0, 1.9, -1.2); L.rimV.target.position.set(0, 0.4, 0.2);
  L.rimG = new THREE.SpotLight(0x39ff14, 0, 9, 0.5, 0.7, 1.4); L.rimG.position.set(-2.0, 1.6, -1.1); L.rimG.target.position.set(0, 0.35, 0.2);
  L.parV = new THREE.SpotLight(0x9b4dff, 0, 10, 0.32, 0.8, 1.2); L.parV.position.set(1.15, 2.55, -0.35); L.parV.target.position.set(0.45, 0, 0.55);
  L.parG = new THREE.SpotLight(0x46ff2a, 0, 10, 0.3, 0.8, 1.2); L.parG.position.set(-1.25, 2.55, -0.35); L.parG.target.position.set(-0.5, 0, 0.6);
  L.wall = new THREE.SpotLight(0xff8a4a, 0, 9, 1.0, 1, 1.0); L.wall.position.set(0, 0.1, -0.5); L.wall.target.position.set(0, 2.2, -1.6);
  L.fill = new THREE.HemisphereLight(0x6a64ff, 0x100c08, 0);
  for (const k in L) { scene.add(L[k]); if (L[k].target) scene.add(L[k].target); }
  const LEVELS = { key: 55, rimV: 22, rimG: 12, parV: 26, parG: 14, wall: 9, fill: 0.15 };
  // Soft light that travels with the camera: like the fill a photographer puts next to the lens for close-ups.
  L.cam = new THREE.SpotLight(0xfff1e2, 0, 3, 0.6, 1, 2); scene.add(L.cam, L.cam.target);
  const setLights = (t) => { for (const k in L) if (k !== "cam") L[k].intensity = LEVELS[k] * t; };
  setLights(0.2);

  // ---------------- set
  const floor = new THREE.Mesh(new THREE.PlaneGeometry(10, 10), new THREE.MeshStandardMaterial({
    map: tex("stage_col.jpg", true, 2), roughnessMap: tex("stage_rough.jpg", false, 2), normalMap: tex("stage_nrm.jpg", false, 2),
    normalScale: new THREE.Vector2(0.4, 0.4), roughness: 1, color: 0xbab0a6 }));
  for (const k of ["map", "roughnessMap", "normalMap"]) floor.material[k].repeat.set(5, 5);
  floor.rotation.x = -Math.PI / 2; floor.position.y = -0.012; floor.receiveShadow = true; scene.add(floor);

  const rug = new THREE.Mesh(new THREE.BoxGeometry(1.75, 0.006, 1.25), new THREE.MeshStandardMaterial({
    map: tex("rug_col.jpg", true, 1.75, 1.25), normalMap: tex("rug_nrm.jpg", false, 1.75, 1.25), normalScale: new THREE.Vector2(0.8, 0.8), roughness: 0.97, color: 0x5a4c48 }));
  // the rug texture is mapped once over the whole rug: box UVs per face are 0..1
  for (const k of ["map", "normalMap"]) rug.material[k].repeat.set(1, 1);
  rug.position.set(0.04, -0.009, 0.32); rug.rotation.y = 0.05; rug.receiveShadow = true; scene.add(rug);

  const brick = new THREE.Mesh(new THREE.PlaneGeometry(9, 4.5), new THREE.MeshStandardMaterial({
    map: tex("brick_col.jpg", true, 1.2), normalMap: tex("brick_nrm.jpg", false, 1.2), normalScale: new THREE.Vector2(1.2, 1.2), roughness: 0.95 }));
  brick.material.map.repeat.set(9 / 1.2, 4.5 / 1.2); brick.material.normalMap.repeat.set(9 / 1.2, 4.5 / 1.2);
  brick.position.set(0, 2.2, -1.55); brick.receiveShadow = true; scene.add(brick);

  // Background speaker stacks left and right (dim silhouettes give the room depth).
  const stackMat = M.Tolex, hex = M.HexMesh.clone(); hex.map = M.HexMesh.map.clone(); hex.map.needsUpdate = true;
  hex.map.repeat.set(1 / 0.03, 1 / (0.03 * Math.sqrt(3) / 3));
  for (const sx of [-1, 1]) {
    for (let k = 0; k < 2; k++) {
      const g = new THREE.Group();
      const b = new THREE.Mesh(new THREE.BoxGeometry(0.78, 0.78, 0.38), stackMat); b.castShadow = b.receiveShadow = true;
      const f = new THREE.Mesh(new THREE.PlaneGeometry(0.7, 0.7), hex); f.position.z = 0.191;
      const back = new THREE.Mesh(new THREE.PlaneGeometry(0.7, 0.7), M.Baffle); back.position.z = 0.17;
      g.add(b, f, back); g.position.set(sx * 1.75, 0.39 + k * 0.785, -1.25); g.rotation.y = -sx * 0.25; scene.add(g);
    }
  }
  // Lighting truss with par cans.
  const trussMat = new THREE.MeshStandardMaterial({ color: 0x55575c, metalness: 1, roughness: 0.4 });
  const truss = new THREE.Group(); truss.position.set(0, 2.6, -0.35); scene.add(truss);
  const tube = (a, b, r) => {
    const d = new THREE.Vector3().subVectors(b, a); const m = new THREE.Mesh(new THREE.CylinderGeometry(r, r, d.length(), 8), trussMat);
    m.position.copy(a).addScaledVector(d, 0.5); m.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), d.normalize()); truss.add(m);
  };
  const s = 0.15;
  for (const [y, z] of [[0, 0], [s, 0], [0, s], [s, s]]) tube(new THREE.Vector3(-2.6, y, z), new THREE.Vector3(2.6, y, z), 0.012);
  for (let x = -2.6; x < 2.6; x += 0.3) {
    tube(new THREE.Vector3(x, 0, 0), new THREE.Vector3(x + 0.3, s, 0), 0.005); tube(new THREE.Vector3(x, 0, s), new THREE.Vector3(x + 0.3, s, s), 0.005);
    tube(new THREE.Vector3(x, 0, 0), new THREE.Vector3(x + 0.3, 0, s), 0.005); tube(new THREE.Vector3(x, s, 0), new THREE.Vector3(x + 0.3, s, s), 0.005);
  }
  const canMat = new THREE.MeshStandardMaterial({ color: 0x111113, metalness: 0.7, roughness: 0.5 });
  const beams = [];
  for (const [light, col] of [[L.parV, 0x9b4dff], [L.parG, 0x46ff2a]]) {
    const can = new THREE.Group();
    const body = new THREE.Mesh(new THREE.CylinderGeometry(0.09, 0.075, 0.26, 24, 1, true), canMat); body.rotation.x = Math.PI / 2;
    const lens = new THREE.Mesh(new THREE.CircleGeometry(0.085, 24), new THREE.MeshBasicMaterial({ color: new THREE.Color(col).multiplyScalar(4) }));
    lens.position.z = 0.13; can.add(body, lens);
    can.position.copy(light.position); can.lookAt(light.target.position); scene.add(can);
    beams.push(makeBeam(light.position, light.target.position, col, 0.62));
  }
  for (const b of beams) scene.add(b);

  // Haze: soft sprites drifting slowly, lit in the colours of the room.
  const smokeTex = new THREE.TextureLoader().load("tex/smoke.png");
  const haze = [];
  for (let i = 0; i < 14; i++) {
    const m = new THREE.SpriteMaterial({ map: smokeTex, color: [0x6a3cc0, 0x2a8a20, 0x8c6a5a][i % 3], transparent: true, opacity: 0.05 + 0.04 * Math.random(), depthWrite: false, fog: false });
    const sp = new THREE.Sprite(m); const sc = 1.2 + Math.random() * 1.8; sp.scale.set(sc * 1.6, sc, 1);
    sp.position.set((Math.random() - 0.5) * 4.5, 0.4 + Math.random() * 1.6, -1.3 + Math.random() * 1.4);
    sp.userData = { vx: (Math.random() - 0.5) * 0.02, ph: Math.random() * 6 }; scene.add(sp); haze.push(sp);
  }
  // Dust floating in the light.
  const N = 900, pos = new Float32Array(N * 3), seed = new Float32Array(N);
  for (let i = 0; i < N; i++) { pos[i * 3] = (Math.random() - 0.5) * 3.2; pos[i * 3 + 1] = Math.random() * 2.4; pos[i * 3 + 2] = -1 + Math.random() * 2.2; seed[i] = Math.random(); }
  const dg = new THREE.BufferGeometry(); dg.setAttribute("position", new THREE.BufferAttribute(pos, 3)); dg.setAttribute("seed", new THREE.BufferAttribute(seed, 1));
  const dust = new THREE.Points(dg, new THREE.ShaderMaterial({
    transparent: true, depthWrite: false, blending: THREE.AdditiveBlending,
    uniforms: { time: { value: 0 }, px: { value: renderer.getPixelRatio() } },
    vertexShader: `attribute float seed; uniform float time; uniform float px; varying float vA;
      void main(){ vec3 p = position; p.x += sin(time*0.11 + seed*40.0)*0.06; p.y = mod(p.y + time*0.012*(0.3+seed), 2.4); p.z += cos(time*0.09 + seed*23.0)*0.05;
        vec4 mv = modelViewMatrix * vec4(p,1.0); gl_Position = projectionMatrix * mv;
        gl_PointSize = (1.2 + seed*2.2) * px * (2.0 / -mv.z); vA = 0.25 + 0.75*fract(seed*7.3); }`,
    fragmentShader: `varying float vA; void main(){ float d = length(gl_PointCoord-0.5); float a = smoothstep(0.5,0.0,d)*vA*0.55; gl_FragColor = vec4(vec3(1.0,0.92,0.85)*a, a); }`,
  }));
  scene.add(dust);

  // ---------------- post-processing
  const composer = new EffectComposer(renderer);
  composer.addPass(new RenderPass(scene, camera));
  const gtao = new GTAOPass(scene, camera, 1, 1);
  gtao.updateGtaoMaterial({ radius: 0.12, distanceExponent: 1.4, thickness: 1.2, scale: 1.0, samples: 12 });
  gtao.updatePdMaterial({ lumaPhi: 10, depthPhi: 2, normalPhi: 3, radius: 6, rings: 2, samples: 12 });
  gtao.blendIntensity = 0.9;
  composer.addPass(gtao);
  const bokeh = new BokehPass(scene, camera, { focus: 2, aperture: 0.0025, maxblur: 0.006 });
  composer.addPass(bokeh);
  const bloom = new UnrealBloomPass(new THREE.Vector2(1, 1), 0.26, 0.3, 3.2);
  composer.addPass(bloom);
  composer.addPass(new OutputPass());
  // Lens: vignette, a touch of chromatic fringing at the edges, film grain.
  const lens = new ShaderPass({
    uniforms: { tDiffuse: { value: null }, time: { value: 0 }, vig: { value: 0.42 }, ca: { value: 0.0016 }, grain: { value: 0.035 } },
    vertexShader: `varying vec2 vUv; void main(){ vUv = uv; gl_Position = projectionMatrix*modelViewMatrix*vec4(position,1.0); }`,
    fragmentShader: `uniform sampler2D tDiffuse; uniform float time, vig, ca, grain; varying vec2 vUv;
      float h(vec2 p){ return fract(sin(dot(p, vec2(12.9898,78.233)))*43758.5453); }
      void main(){ vec2 c = vUv-0.5; float r2 = dot(c,c);
        vec3 col; col.r = texture2D(tDiffuse, vUv - c*ca*r2*8.0).r; col.g = texture2D(tDiffuse, vUv).g; col.b = texture2D(tDiffuse, vUv + c*ca*r2*8.0).b;
        col *= 1.0 - vig * smoothstep(0.08, 0.62, r2*1.6);
        col += (h(vUv*1000.0 + time) - 0.5) * grain;
        gl_FragColor = vec4(col, 1.0); }`,
  });
  composer.addPass(lens);

  function update(t) {
    dust.material.uniforms.time.value = t;
    lens.uniforms.time.value = t % 100;
    for (const sp of haze) { sp.position.x += sp.userData.vx * 0.016; if (Math.abs(sp.position.x) > 2.6) sp.userData.vx *= -1; sp.material.rotation = Math.sin(t * 0.05 + sp.userData.ph) * 0.2; }
    for (const b of beams) b.material.uniforms.time.value = t;
  }
  return { L, setLights, composer, gtao, bokeh, bloom, lens, update, dust };
}

// Volumetric-looking light cone: brightest at the lamp, soft at the edges, faint moving haze.
function makeBeam(from, to, color, angle) {
  const len = from.distanceTo(to) * 1.15;
  const r1 = Math.tan(angle * 0.55) * len;
  const geo = new THREE.CylinderGeometry(0.07, r1, len, 48, 1, true);
  geo.translate(0, -len / 2, 0);
  const mat = new THREE.ShaderMaterial({
    transparent: true, depthWrite: false, blending: THREE.AdditiveBlending, side: THREE.DoubleSide,
    uniforms: { color: { value: new THREE.Color(color) }, len: { value: len }, time: { value: 0 }, strength: { value: 0.16 } },
    vertexShader: `varying float vAlong; varying vec3 vN; varying vec3 vV; varying vec3 vW; uniform float len;
      void main(){ vAlong = -position.y/len; vec4 mv = modelViewMatrix*vec4(position,1.0); vV = -mv.xyz; vN = normalMatrix*normal; vW = (modelMatrix*vec4(position,1.0)).xyz; gl_Position = projectionMatrix*mv; }`,
    fragmentShader: `uniform vec3 color; uniform float time, strength; varying float vAlong; varying vec3 vN; varying vec3 vV; varying vec3 vW;
      float n3(vec3 p){ return fract(sin(dot(floor(p), vec3(127.1,311.7,74.7)))*43758.5453); }
      float noise(vec3 p){ vec3 i=floor(p), f=fract(p); f=f*f*(3.0-2.0*f);
        float a=mix(mix(n3(i),n3(i+vec3(1,0,0)),f.x),mix(n3(i+vec3(0,1,0)),n3(i+vec3(1,1,0)),f.x),f.y);
        float b=mix(mix(n3(i+vec3(0,0,1)),n3(i+vec3(1,0,1)),f.x),mix(n3(i+vec3(0,1,1)),n3(i+vec3(1,1,1)),f.x),f.y); return mix(a,b,f.z); }
      void main(){ float edge = pow(abs(dot(normalize(vN), normalize(vV))), 2.2);
        float fall = pow(1.0 - clamp(vAlong,0.0,1.0), 1.6) * smoothstep(0.0, 0.05, vAlong);
        float hz = 0.55 + 0.45*noise(vW*3.0 + vec3(time*0.08, time*0.03, 0.0));
        float a = edge*fall*hz*strength; gl_FragColor = vec4(color*a, a); }`,
  });
  const m = new THREE.Mesh(geo, mat);
  m.position.copy(from);
  m.quaternion.setFromUnitVectors(new THREE.Vector3(0, -1, 0), new THREE.Vector3().subVectors(to, from).normalize());
  m.renderOrder = 5;
  return m;
}
