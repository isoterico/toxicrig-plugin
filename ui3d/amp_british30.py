# ToxicRig "British 30" head + 2x12 cab, modelled procedurally in Blender.
# Every control on the faceplate is a real plugin control (8 knobs, Bright, Channel).
import bpy, bmesh, math, sys, os, json
from mathutils import Vector

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
MODE = ARGS[0] if ARGS else "preview"      # preview | beauty | asset
OUT = "/home/claude/amp3d/out"
os.makedirs(OUT, exist_ok=True)
FONT_SERIF = "/home/claude/toxicrig/assets/fonts/Cormorant-SemiBoldItalic.ttf"
FONT_LABEL = "/home/claude/toxicrig/assets/fonts/Rajdhani-Bold.ttf"

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene

# ----------------------------------------------------------------- materials
def node_mat(name):
    m = bpy.data.materials.new(name); m.use_nodes = True
    nt = m.node_tree; bsdf = nt.nodes["Principled BSDF"]
    return m, nt, bsdf

def principled(name, color, metal=0.0, rough=0.5, coat=0.0, aniso=0.0, emission=None, estr=0.0, transmission=0.0, ior=1.45, alpha=1.0):
    m, nt, b = node_mat(name)
    b.inputs["Base Color"].default_value = (*color, 1)
    b.inputs["Metallic"].default_value = metal
    b.inputs["Roughness"].default_value = rough
    b.inputs["Coat Weight"].default_value = coat
    b.inputs["Anisotropic"].default_value = aniso
    b.inputs["Transmission Weight"].default_value = transmission
    b.inputs["IOR"].default_value = ior
    if emission:
        b.inputs["Emission Color"].default_value = (*emission, 1)
        b.inputs["Emission Strength"].default_value = estr
    b.inputs["Alpha"].default_value = alpha
    return m

def wood_material(name, dark=False):
    m, nt, b = node_mat(name)
    N = nt.nodes; L = nt.links
    tc = N.new("ShaderNodeTexCoord")
    mapping = N.new("ShaderNodeMapping"); mapping.inputs["Scale"].default_value = (1.0, 22.0, 22.0)
    L.new(tc.outputs["Object"], mapping.inputs["Vector"])
    noise = N.new("ShaderNodeTexNoise"); noise.inputs["Scale"].default_value = 3.0; noise.inputs["Detail"].default_value = 8
    L.new(mapping.outputs["Vector"], noise.inputs["Vector"])
    wave = N.new("ShaderNodeTexWave"); wave.wave_type = "BANDS"; wave.bands_direction = "Z"
    wave.inputs["Scale"].default_value = 2.4; wave.inputs["Distortion"].default_value = 16.0; wave.inputs["Detail"].default_value = 8; wave.inputs["Detail Scale"].default_value = 2.5
    L.new(mapping.outputs["Vector"], wave.inputs["Vector"])
    mix = N.new("ShaderNodeMix"); mix.data_type = "FLOAT"; mix.inputs["Factor"].default_value = 0.5
    L.new(wave.outputs["Fac"], mix.inputs["A"]); L.new(noise.outputs["Fac"], mix.inputs["B"])
    ramp = N.new("ShaderNodeValToRGB")
    c0, c1 = ((0.018, 0.008, 0.004), (0.085, 0.038, 0.017)) if not dark else ((0.009, 0.005, 0.003), (0.04, 0.019, 0.01))
    ramp.color_ramp.elements[0].color = (*c0, 1); ramp.color_ramp.elements[1].color = (*c1, 1)
    L.new(mix.outputs["Result"], ramp.inputs["Fac"])
    L.new(ramp.outputs["Color"], b.inputs["Base Color"])
    b.inputs["Roughness"].default_value = 0.38
    b.inputs["Coat Weight"].default_value = 0.55
    b.inputs["Coat Roughness"].default_value = 0.12
    bump = N.new("ShaderNodeBump"); bump.inputs["Strength"].default_value = 0.08
    L.new(wave.outputs["Fac"], bump.inputs["Height"]); L.new(bump.outputs["Normal"], b.inputs["Normal"])
    return m

def brushed(name, color, rough=0.26, scale=(1, 260, 1)):
    m, nt, b = node_mat(name)
    N = nt.nodes; L = nt.links
    tc = N.new("ShaderNodeTexCoord")
    mp = N.new("ShaderNodeMapping"); mp.inputs["Scale"].default_value = scale
    L.new(tc.outputs["Object"], mp.inputs["Vector"])
    nz = N.new("ShaderNodeTexNoise"); nz.inputs["Scale"].default_value = 40; nz.inputs["Detail"].default_value = 12
    L.new(mp.outputs["Vector"], nz.inputs["Vector"])
    rr = N.new("ShaderNodeMapRange"); rr.inputs["To Min"].default_value = rough - 0.08; rr.inputs["To Max"].default_value = rough + 0.08
    L.new(nz.outputs["Fac"], rr.inputs["Value"]); L.new(rr.outputs["Result"], b.inputs["Roughness"])
    b.inputs["Base Color"].default_value = (*color, 1)
    b.inputs["Metallic"].default_value = 1.0
    bump = N.new("ShaderNodeBump"); bump.inputs["Strength"].default_value = 0.03
    L.new(nz.outputs["Fac"], bump.inputs["Height"]); L.new(bump.outputs["Normal"], b.inputs["Normal"])
    return m

def grille_material(name):
    m, nt, b = node_mat(name)
    N = nt.nodes; L = nt.links
    tc = N.new("ShaderNodeTexCoord")
    mp = N.new("ShaderNodeMapping"); mp.inputs["Scale"].default_value = (620, 620, 620)
    L.new(tc.outputs["Object"], mp.inputs["Vector"])
    ck = N.new("ShaderNodeTexChecker"); ck.inputs["Scale"].default_value = 1.0
    ck.inputs["Color1"].default_value = (0.028, 0.025, 0.022, 1); ck.inputs["Color2"].default_value = (0.006, 0.0055, 0.005, 1)
    L.new(mp.outputs["Vector"], ck.inputs["Vector"])
    L.new(ck.outputs["Color"], b.inputs["Base Color"])
    b.inputs["Roughness"].default_value = 0.85
    bump = N.new("ShaderNodeBump"); bump.inputs["Strength"].default_value = 0.4
    L.new(ck.outputs["Fac"], bump.inputs["Height"]); L.new(bump.outputs["Normal"], b.inputs["Normal"])
    return m

M = {
    "wood": wood_material("Walnut"),
    "wood_dark": wood_material("WalnutDark", dark=True),
    "brass": brushed("Brass", (0.80, 0.60, 0.30), 0.24),
    "brass_knob": brushed("BrassKnob", (0.86, 0.66, 0.34), 0.22, (1, 1, 1)),
    "chrome": principled("Chrome", (0.82, 0.82, 0.84), metal=1, rough=0.12),
    "nickel": principled("Nickel", (0.62, 0.61, 0.6), metal=1, rough=0.22),
    "black_metal": principled("BlackMetal", (0.025, 0.025, 0.028), metal=0.6, rough=0.38),
    "bakelite": principled("Bakelite", (0.018, 0.017, 0.016), rough=0.28, coat=0.6),
    "paint_white": principled("PaintWhite", (0.85, 0.82, 0.74), rough=0.4),
    "engrave": principled("Engrave", (0.02, 0.018, 0.015), rough=0.6),
    "leather": principled("Leather", (0.02, 0.018, 0.017), rough=0.55, coat=0.2),
    "grille": grille_material("Grille"),
    "glass": principled("Glass", (0.95, 0.98, 0.96), rough=0.02, transmission=1.0, ior=1.45),
    "tube_plate": principled("TubePlate", (0.25, 0.25, 0.26), metal=1, rough=0.35, emission=(0.2, 0.9, 0.12), estr=1.2),
    "heater": principled("Heater", (0.2, 1.0, 0.2), emission=(0.25, 1.0, 0.12), estr=140.0),
    "window_back": principled("WindowBack", (0.01, 0.012, 0.01), rough=0.6, emission=(0.1, 0.5, 0.06), estr=0.35),
    "jewel": principled("Jewel", (0.1, 0.9, 0.1), rough=0.05, emission=(0.2, 1.0, 0.1), estr=9.0),
    "jewel_off": principled("JewelOff", (0.02, 0.08, 0.02), rough=0.05),
    "brand": brushed("BrandBrass", (0.75, 0.56, 0.26), 0.2, (1, 1, 1)),
}

# ----------------------------------------------------------------- helpers
def link(obj): bpy.context.collection.objects.link(obj); return obj

def box(name, size, loc, mat, bevel=0.0, segments=4):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    o = bpy.context.active_object; o.name = name
    o.scale = size; bpy.ops.object.transform_apply(scale=True)
    if bevel > 0:
        mod = o.modifiers.new("bev", "BEVEL"); mod.width = bevel; mod.segments = segments; mod.limit_method = "ANGLE"
    o.data.materials.append(mat)
    bpy.ops.object.shade_auto_smooth()
    return o

def cyl(name, r, depth, loc, mat, rot=(math.pi / 2, 0, 0), verts=64, bevel=0.0):
    bpy.ops.mesh.primitive_cylinder_add(radius=r, depth=depth, location=loc, rotation=rot, vertices=verts)
    o = bpy.context.active_object; o.name = name
    if bevel > 0:
        mod = o.modifiers.new("bev", "BEVEL"); mod.width = bevel; mod.segments = 3; mod.limit_method = "ANGLE"
    o.data.materials.append(mat)
    bpy.ops.object.shade_auto_smooth()
    return o

def text(name, s, size, loc, mat, font, extrude=0.0002, align="CENTER"):
    bpy.ops.object.text_add(location=loc, rotation=(math.pi / 2, 0, 0))
    o = bpy.context.active_object; o.name = name
    o.data.body = s; o.data.size = size; o.data.extrude = extrude
    o.data.align_x = align; o.data.align_y = "CENTER"
    o.data.font = bpy.data.fonts.load(font, check_existing=True); o.data.resolution_u = 2 if size < 0.008 else 3
    o.data.materials.append(mat)
    return o

def parent(child, par):
    child.parent = par
    child.matrix_parent_inverse = par.matrix_world.inverted()

# ----------------------------------------------------------------- dimensions (metres)
W = 0.64
CAB_H, CAB_D = 0.36, 0.27
HEAD_H, HEAD_D = 0.25, 0.24
z0 = CAB_H                         # head bottom
fy = -HEAD_D / 2                   # head front face (y)

# ----------------------------------------------------------------- cab
cab = box("Cab", (W, CAB_D, CAB_H), (0, 0, CAB_H / 2), M["wood"], bevel=0.012)
box("CabGrille", (W - 0.07, 0.01, CAB_H - 0.075), (0, -CAB_D / 2 + 0.003, CAB_H / 2 + 0.004), M["grille"])
# Brass piping frame around the grille.
for (sx, sz, lx, lz) in [((W - 0.064), 0.004, 0, CAB_H / 2 + 0.004 + (CAB_H - 0.075) / 2), ((W - 0.064), 0.004, 0, CAB_H / 2 + 0.004 - (CAB_H - 0.075) / 2),
                         (0.004, CAB_H - 0.071, (W - 0.068) / 2, CAB_H / 2 + 0.004), (0.004, CAB_H - 0.071, -(W - 0.068) / 2, CAB_H / 2 + 0.004)]:
    box("Piping", (sx, 0.006, sz), (lx, -CAB_D / 2 - 0.001, lz), M["brass"], bevel=0.0015)
# Brand badge on the grille.
badge = box("Badge", (0.135, 0.004, 0.042), (0.19, -CAB_D / 2 - 0.004, 0.07), M["brand"], bevel=0.004)
text("BadgeText", "ToxicRig", 0.029, (0.19, -CAB_D / 2 - 0.0065, 0.068), M["engrave"], FONT_SERIF, extrude=0.0004)
# Cab feet and corners.
for x in (-W / 2 + 0.03, W / 2 - 0.03):
    for y in (-CAB_D / 2 + 0.03, CAB_D / 2 - 0.03):
        cyl("Foot", 0.014, 0.012, (x, y, -0.006), M["black_metal"], rot=(0, 0, 0))

# ----------------------------------------------------------------- head shell
head = box("Head", (W, HEAD_D, HEAD_H), (0, 0, z0 + HEAD_H / 2), M["wood"], bevel=0.012)
# Recessed upper front panel (darker walnut) and brass control strip.
box("FrontPanel", (W - 0.05, 0.01, 0.135), (0, fy + 0.001, z0 + 0.165), M["wood_dark"], bevel=0.002)
strip_z = z0 + 0.05
box("Strip", (W - 0.06, 0.008, 0.062), (0, fy - 0.002, strip_z), M["brass"], bevel=0.0025)
# Tube window: chrome bezel, dark cavity, four glowing tubes behind glass.
win_z = z0 + 0.168
cutter = box("WinCutter", (0.205, 0.12, 0.078), (0, fy, win_z), M["window_back"])
for target in (head, bpy.data.objects["FrontPanel"]):
    mod = target.modifiers.new("hole", "BOOLEAN"); mod.operation = "DIFFERENCE"; mod.object = cutter; mod.solver = "EXACT"
    bpy.context.view_layer.objects.active = target
    bpy.ops.object.modifier_apply(modifier="hole")
bpy.data.objects.remove(cutter)
box("WinBack", (0.21, 0.004, 0.08), (0, fy + 0.05, win_z), M["window_back"])
for (sx, sz, lx, lz) in [(0.225, 0.008, 0, win_z + 0.043), (0.225, 0.008, 0, win_z - 0.043), (0.008, 0.094, 0.1085, win_z), (0.008, 0.094, -0.1085, win_z)]:
    box("WinBezel", (sx, 0.007, sz), (lx, fy - 0.004, lz), M["chrome"], bevel=0.0018)
for i, x in enumerate((-0.075, -0.025, 0.025, 0.075)):
    tz = win_z - 0.005
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.016, location=(x, fy + 0.02, tz + 0.02), segments=48, ring_count=24)
    top = bpy.context.active_object; top.scale = (1, 1, 1.25); top.data.materials.append(M["glass"]); bpy.ops.object.shade_smooth()
    cyl("TubeGlass", 0.016, 0.05, (x, fy + 0.02, tz - 0.004), M["glass"], rot=(0, 0, 0))
    cyl("TubePlate", 0.009, 0.034, (x, fy + 0.02, tz), M["tube_plate"], rot=(0, 0, 0), verts=6)
    cyl("Heater", 0.0034, 0.03, (x, fy + 0.02, tz), M["heater"], rot=(0, 0, 0), verts=12)
    cyl("TubeBase", 0.017, 0.012, (x, fy + 0.02, tz - 0.033), M["bakelite"], rot=(0, 0, 0))
glass = box("WinGlass", (0.205, 0.0015, 0.078), (0, fy - 0.002, win_z), M["glass"])
# Green glow spilling from the window.
bpy.ops.object.light_add(type="AREA", location=(0, fy - 0.004, win_z))
gl = bpy.context.active_object; gl.data.energy = 7.0; gl.data.color = (0.3, 1.0, 0.2); gl.data.size = 0.18
gl.rotation_euler = (-math.pi / 2, 0, 0); gl.name = "TubeGlow"
# Corners and handle.
for x in (-W / 2, W / 2):
    for z in (z0 + 0.004, z0 + HEAD_H - 0.004):
        for y in (-HEAD_D / 2, HEAD_D / 2):
            box("Corner", (0.024, 0.024, 0.024), (x * 0.992, y * 0.97, z), M["black_metal"], bevel=0.006)
for x in (-W / 2, W / 2):
    for z in (0.004, CAB_H - 0.004):
        for y in (-CAB_D / 2, CAB_D / 2):
            box("Corner", (0.024, 0.024, 0.024), (x * 0.992, y * 0.97, z), M["black_metal"], bevel=0.006)
box("Handle", (0.22, 0.03, 0.016), (0, 0, z0 + HEAD_H + 0.02), M["leather"], bevel=0.007)
for x in (-0.125, 0.125):
    box("HandleCap", (0.028, 0.04, 0.02), (x, 0, z0 + HEAD_H + 0.008), M["chrome"], bevel=0.005)
# Pilot jewel under the window.
jewel = cyl("Jewel", 0.0065, 0.008, (0, fy - 0.006, z0 + 0.107), M["jewel"], verts=32, bevel=0.002)
cyl("JewelRing", 0.0095, 0.004, (0, fy - 0.003, z0 + 0.107), M["chrome"], verts=48, bevel=0.001)

# ----------------------------------------------------------------- controls
controls = []   # (id, type, object, centre)

def big_knob(cid, x, z, label):
    base = cyl(cid + "_skirt", 0.027, 0.006, (x, fy - 0.005, z), M["brass_knob"], verts=96, bevel=0.0015)
    cap = cyl(cid + "_cap", 0.023, 0.022, (x, fy - 0.017, z), M["brass_knob"], verts=96, bevel=0.003)
    # Knurled edge ring and engraved pointer line.
    ring = cyl(cid + "_knurl", 0.0235, 0.012, (x, fy - 0.014, z), M["brass_knob"], verts=48)
    ptr = box(cid + "_ptr", (0.0024, 0.003, 0.016), (x, fy - 0.0285, z + 0.012), M["engrave"])
    for o in (base, ring, ptr): parent(o, cap)
    # Scale ticks around the knob (printed on the panel).
    for k in range(11):
        a = math.radians(-135 + 27 * k)
        box("tick", (0.0012, 0.001, 0.005), (x + math.sin(a) * 0.033, fy - 0.0045, z + math.cos(a) * 0.033), M["brass"])
        bpy.context.active_object.rotation_euler = (0, -a, 0)
    text(cid + "_label", label, 0.0105, (x, fy - 0.0045, z - 0.042), M["brass"], FONT_LABEL)
    controls.append((cid, "knob", cap, (x, z)))

def small_knob(cid, x, z, label):
    skirt = cyl(cid + "_skirt", 0.0125, 0.004, (x, fy - 0.008, z), M["chrome"], verts=64, bevel=0.001)
    cap = cyl(cid + "_cap", 0.0098, 0.016, (x, fy - 0.017, z), M["bakelite"], verts=24, bevel=0.0012)
    top = cyl(cid + "_top", 0.0085, 0.002, (x, fy - 0.0255, z), M["nickel"], verts=64, bevel=0.0008)
    ptr = box(cid + "_ptr", (0.0016, 0.002, 0.008), (x, fy - 0.0263, z + 0.0045), M["paint_white"])
    for o in (skirt, top, ptr): parent(o, cap)
    text(cid + "_label", label, 0.0088, (x, fy - 0.0068, z + 0.0225), M["engrave"], FONT_LABEL)
    controls.append((cid, "knob", cap, (x, z)))

def bat_toggle(cid, x, z, label, positions):
    cyl(cid + "_nut", 0.0075, 0.004, (x, fy - 0.008, z), M["chrome"], verts=6)
    bush = cyl(cid + "_bush", 0.0045, 0.008, (x, fy - 0.012, z), M["chrome"], verts=32)
    bpy.ops.object.empty_add(location=(x, fy - 0.012, z)); pivot = bpy.context.active_object; pivot.name = cid + "_pivot"
    lever = cyl(cid + "_lever", 0.0022, 0.02, (x, fy - 0.022, z), M["chrome"], rot=(math.pi / 2, 0, 0), verts=24)
    tip = bpy.ops.mesh.primitive_uv_sphere_add(radius=0.0034, location=(x, fy - 0.032, z), segments=24, ring_count=12)
    tipo = bpy.context.active_object; tipo.data.materials.append(M["chrome"]); bpy.ops.object.shade_smooth()
    parent(lever, pivot); parent(tipo, pivot)
    text(cid + "_label", label, 0.0074, (x, fy - 0.0068, z + 0.022), M["engrave"], FONT_LABEL)
    controls.append((cid, "toggle", pivot, (x, z), positions))

def jack(name, x, z):
    cyl(name + "_nut", 0.0085, 0.004, (x, fy - 0.008, z), M["nickel"], verts=6)
    cyl(name + "_hole", 0.0042, 0.006, (x, fy - 0.009, z), M["bakelite"], verts=32)

big_knob("gain", -0.215, z0 + 0.168, "GAIN")
big_knob("master", 0.215, z0 + 0.168, "MASTER")
for i, (cid, lab) in enumerate([("bass", "BASS"), ("mid", "MIDDLE"), ("treble", "TREBLE"), ("presence", "CUT"), ("resonance", "DEPTH"), ("sag", "SAG")]):
    small_knob(cid, -0.155 + 0.062 * i, strip_z - 0.006, lab)
bat_toggle("bright", -0.215, strip_z - 0.006, "BRIGHT", 2)
bat_toggle("channel", 0.215, strip_z - 0.006, "CHANNEL", 3)
jack("input", -0.268, strip_z - 0.006)
text("input_label", "INPUT", 0.0062, (-0.268, fy - 0.0068, strip_z + 0.016), M["engrave"], FONT_LABEL)
jack("fxloop", 0.268, strip_z - 0.006)
text("loop_label", "FX LOOP", 0.0062, (0.268, fy - 0.0068, strip_z + 0.016), M["engrave"], FONT_LABEL)
text("model_label", "BRITISH  30", 0.0105, (0, fy - 0.0035, z0 + 0.232), M["brass"], FONT_LABEL)

# ----------------------------------------------------------------- floor, lights, camera
bpy.ops.mesh.primitive_plane_add(size=6, location=(0, 0, -0.012))
floor = bpy.context.active_object; floor.name = "Floor"
if MODE == "beauty":
    fm = principled("Concrete", (0.05, 0.05, 0.055), rough=0.55)
    floor.data.materials.append(fm)
else:
    floor.is_shadow_catcher = True
    floor.data.materials.append(principled("Catcher", (0.5, 0.5, 0.5), rough=0.6))

def area(name, loc, energy, size, color, target=(0, 0, 0.32)):
    bpy.ops.object.light_add(type="AREA", location=loc)
    l = bpy.context.active_object; l.name = name
    l.data.energy = energy; l.data.size = size; l.data.color = color
    d = Vector(target) - Vector(loc); l.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    return l

area("Key", (-1.4, -1.6, 1.8), 110, 1.2, (1.0, 0.9, 0.8))
area("Fill", (1.6, -1.2, 0.7), 38, 1.2, (0.62, 0.45, 1.0))
area("RimL", (-0.9, 1.6, 0.9), 35, 0.5, (0.35, 1.0, 0.3))
area("RimR", (1.4, 1.0, 1.1), 180, 0.6, (0.6, 0.35, 1.0))
area("Top", (0.2, -0.6, 1.6), 45, 0.5, (1.0, 0.95, 0.9))

world = bpy.data.worlds.new("World"); scene.world = world
world.use_nodes = True
world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.012, 0.012, 0.016, 1)
world.node_tree.nodes["Background"].inputs["Strength"].default_value = 1.0

bpy.ops.object.camera_add(location=(-1.12, -2.02, 0.80))
cam = bpy.context.active_object; scene.camera = cam
cam.data.lens = 50
tgt = Vector((0.0, 0.0, 0.29))
cam.rotation_euler = (tgt - cam.location).to_track_quat("-Z", "Y").to_euler()

# ----------------------------------------------------------------- render settings
r = scene.render
r.engine = "CYCLES"
scene.cycles.device = "CPU"
scene.cycles.use_denoising = True
scene.cycles.denoiser = "OPENIMAGEDENOISE"
scene.cycles.seed = 7
scene.view_settings.view_transform = "AgX"
scene.view_settings.look = "AgX - Medium High Contrast"
r.film_transparent = MODE != "beauty"
r.image_settings.file_format = "PNG"
r.image_settings.color_mode = "RGBA"
r.use_persistent_data = True

def project(p):
    from bpy_extras.object_utils import world_to_camera_view
    co = world_to_camera_view(scene, cam, Vector(p))
    return co.x, 1.0 - co.y

if MODE == "preview":
    r.resolution_x, r.resolution_y = 900, 600
    scene.cycles.samples = 24
    r.filepath = OUT + "/preview.png"
    bpy.ops.render.render(write_still=True)
elif MODE == "beauty":
    r.resolution_x, r.resolution_y = 1800, 1200
    scene.cycles.samples = 160
    r.filepath = OUT + "/beauty.png"
    bpy.ops.render.render(write_still=True)
elif MODE == "asset":
    bpy.context.view_layer.update()  # camera/object matrices must be current before projecting
    # Full body at the controls' default positions, then one crop per control
    # state. Crops contain the panel around the control, so shadows and
    # reflections match the body exactly; the plugin feathers their edges.
    RES_X, RES_Y = int(os.environ.get("RX", "2700")), int(os.environ.get("RY", "1800"))
    r.resolution_x, r.resolution_y = RES_X, RES_Y
    scene.cycles.samples = int(ARGS[1]) if len(ARGS) > 1 else 128
    FRAMES = int(ARGS[2]) if len(ARGS) > 2 else 64
    only = ARGS[3].split(",") if len(ARGS) > 3 else None
    meta = {"width": RES_X, "height": RES_Y, "controls": []}
    def set_knob(o, v):  # v 0..1 -> -135..+135 degrees around the knob axis (y)
        o.rotation_euler = (math.pi / 2, math.radians(-135 + 270 * v), 0)
    def set_toggle(p, idx, n):
        ang = [ -0.5, 0.5 ] if n == 2 else [ -0.55, 0.0, 0.55 ]
        p.rotation_euler = (ang[idx], 0, 0) if n == 2 else (0, 0, ang[idx])
    defaults = {"gain": 0.5, "master": 0.5}
    for c in controls:
        if c[1] == "knob": set_knob(c[2], defaults.get(c[0], 0.5))
        else: set_toggle(c[2], 0, c[4])
    if only is None:
        r.use_border = False
        crop_samples = scene.cycles.samples
        scene.cycles.samples = int(os.environ.get("BODY_SAMPLES", "128"))
        r.filepath = OUT + "/body.png"
        bpy.ops.render.render(write_still=True)
        scene.cycles.samples = crop_samples
    for c in controls:
        cid = c[0]
        if only and cid not in only: continue
        x, z = c[3]
        rad = 0.036 if cid in ("gain", "master") else 0.02
        pts = [project((x + dx, fy - 0.03, z + dz)) for dx in (-rad, rad) for dz in (-rad, rad)]
        pts += [project((x + dx, fy, z + dz)) for dx in (-rad, rad) for dz in (-rad, rad)]
        u0 = max(0.0, min(p[0] for p in pts) - 0.004); u1 = min(1.0, max(p[0] for p in pts) + 0.004)
        v0 = max(0.0, min(p[1] for p in pts) - 0.006); v1 = min(1.0, max(p[1] for p in pts) + 0.006)
        r.use_border = True; r.use_crop_to_border = True
        r.border_min_x, r.border_max_x = u0, u1
        r.border_min_y, r.border_max_y = 1.0 - v1, 1.0 - v0
        n = FRAMES if c[1] == "knob" else c[4]
        os.makedirs(f"{OUT}/{cid}", exist_ok=True)
        for f in range(n):
            if c[1] == "knob": set_knob(c[2], f / (n - 1))
            else: set_toggle(c[2], f, c[4])
            r.filepath = f"{OUT}/{cid}/{f:03d}.png"
            bpy.ops.render.render(write_still=True)
        if c[1] == "knob": set_knob(c[2], defaults.get(cid, 0.5))
        else: set_toggle(c[2], 0, c[4])
        meta["controls"].append({"id": cid, "type": c[1], "frames": n,
                                 "x": round(u0 * RES_X), "y": round(v0 * RES_Y),
                                 "w": round((u1 - u0) * RES_X), "h": round((v1 - v0) * RES_Y),
                                 "cx": round(project((x, fy - 0.02, z))[0] * RES_X), "cy": round(project((x, fy - 0.02, z))[1] * RES_Y)})
    with open(OUT + ("/meta.json" if only is None else "/meta_partial.json"), "w") as fp: json.dump(meta, fp, indent=1)
elif MODE == "gltf2":
    import sys as _s; _s.path.insert(0, "/home/claude/amp3d"); import rig3d
    for o in list(bpy.data.objects):
        if o.type in ("LIGHT", "CAMERA") or o.name == "Floor": bpy.data.objects.remove(o)
    rig3d._M.update({m.name: m for m in bpy.data.materials})
    for i, x in enumerate((-0.15, 0.15)): rig3d.speaker("spk%d" % i, x, CAB_H / 2 + 0.004, -CAB_D / 2 + 0.03, 0.135)
    rig3d.marker("ShotPanel", (0, fy - 0.005, z0 + 0.12), w=W - 0.02, h=HEAD_H - 0.01)
    rig3d.marker("ShotCab", (0, -CAB_D / 2, CAB_H / 2), w=W, h=CAB_H)
    rig3d.export("/home/claude/stage3d/public/models/british30.json", [c[0] for c in controls if c[1] == "knob"], [c[0] for c in controls if c[1] == "toggle"])
elif MODE == "gltf":
    # Real-time version: the UI renders this mesh live (three.js / WebView),
    # so light, shadows and perspective are shared with pedals and floor.
    for o in list(bpy.data.objects):
        if o.type in ("LIGHT", "CAMERA") or o.name == "Floor": bpy.data.objects.remove(o)
    for c in controls:
        if c[1] == "knob": c[2].rotation_euler = (math.pi / 2, math.radians(-135), 0)   # value 0
        else: c[2].rotation_euler = (0, 0, 0)
    bpy.context.view_layer.update()
    for o in [o for o in bpy.data.objects if o.type == "FONT"]:
        bpy.ops.object.select_all(action="DESELECT"); o.select_set(True)
        bpy.context.view_layer.objects.active = o
        bpy.ops.object.convert(target="MESH")
    for o in [o for o in bpy.data.objects if o.type == "MESH"]:
        bpy.context.view_layer.objects.active = o
        for m in list(o.modifiers): bpy.ops.object.modifier_apply(modifier=m.name)
    # World-space box mapping: 1 UV unit = 1 metre, grain along the long axis.
    import bmesh
    for o in [o for o in bpy.data.objects if o.type == "MESH"]:
        bm = bmesh.new(); bm.from_mesh(o.data)
        uv = bm.loops.layers.uv.verify(); mw = o.matrix_world; nmat = mw.to_3x3().inverted().transposed()
        for f in bm.faces:
            n = (nmat @ f.normal).normalized(); ax = max(range(3), key=lambda i: abs(n[i]))
            for l in f.loops:
                w = mw @ l.vert.co
                l[uv].uv = (w.y, w.z) if ax == 0 else ((w.x, w.z) if ax == 1 else (w.x, w.y))
        bm.to_mesh(o.data); bm.free()
    bpy.ops.export_scene.gltf(filepath=OUT + "/british30.glb", export_format="GLB", export_apply=True,
                              export_lights=False, export_cameras=False, export_yup=True)
elif MODE == "project":
    bpy.context.view_layer.update()
    RX, RY = int(os.environ.get("RX", "2700")), int(os.environ.get("RY", "1800"))
    def bbox(pts):
        pp = [project(p) for p in pts]
        xs = [p[0] * RX for p in pp]; ys = [p[1] * RY for p in pp]
        return [round(min(xs)), round(min(ys)), round(max(xs) - min(xs)), round(max(ys) - min(ys))]
    hw = W / 2
    out = {
        "window": bbox([(x, fy, win_z + dz) for x in (-0.1025, 0.1025) for dz in (-0.039, 0.039)]),
        "headFront": bbox([(x, fy, z) for x in (-hw, hw) for z in (z0, z0 + HEAD_H)]),
        "amp": bbox([(x, y, z) for x in (-hw, hw) for y in (-CAB_D / 2, CAB_D / 2) for z in (-0.012, z0 + HEAD_H + 0.03)]),
        "jewel": bbox([(dx, fy, z0 + 0.107 + dz) for dx in (-0.01, 0.01) for dz in (-0.01, 0.01)]),
    }
    with open(OUT + "/layout.json", "w") as fp: json.dump(out, fp, indent=1)
    print(out)
print("DONE", MODE)
