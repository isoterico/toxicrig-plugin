# Shared Blender helpers for ToxicRig real-time amp models.
# Materials are named placeholders: the real-time engine replaces them by name with
# its own physically based materials (textures, clearcoat, wear), so the look of every
# amp is consistent and controlled in one place.
import bpy, bmesh, math, os, json, struct, base64
from mathutils import Vector

FONT_SERIF = "/home/claude/toxicrig/assets/fonts/Cormorant-SemiBoldItalic.ttf"
FONT_LABEL = "/home/claude/toxicrig/assets/fonts/Rajdhani-Bold.ttf"
FONT_TECH = "/home/claude/toxicrig/assets/fonts/BarlowCondensed-Medium.ttf"
FONT_LOGO = "/home/claude/toxicrig/assets/fonts/SpaceGrotesk-Bold.ttf"

def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)

_M = {}
def mat(name):
    if name not in _M:
        m = bpy.data.materials.new(name); m.use_nodes = True; _M[name] = m
    return _M[name]

def link(o): bpy.context.collection.objects.link(o); return o

def box(name, size, loc, m, bevel=0.0, segments=3):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    o = bpy.context.active_object; o.name = name
    o.scale = size; bpy.ops.object.transform_apply(scale=True)
    if bevel > 0:
        mod = o.modifiers.new("bev", "BEVEL"); mod.width = bevel; mod.segments = segments; mod.limit_method = "ANGLE"
    o.data.materials.append(mat(m)); bpy.ops.object.shade_auto_smooth()
    return o

def cyl(name, r, depth, loc, m, rot=(math.pi / 2, 0, 0), verts=48, bevel=0.0, r2=None):
    if r2 is None:
        bpy.ops.mesh.primitive_cylinder_add(radius=r, depth=depth, location=loc, rotation=rot, vertices=verts)
    else:
        bpy.ops.mesh.primitive_cone_add(radius1=r, radius2=r2, depth=depth, location=loc, rotation=rot, vertices=verts)
    o = bpy.context.active_object; o.name = name
    if bevel > 0:
        mod = o.modifiers.new("bev", "BEVEL"); mod.width = bevel; mod.segments = 3; mod.limit_method = "ANGLE"
    o.data.materials.append(mat(m)); bpy.ops.object.shade_auto_smooth()
    return o

def sphere(name, r, loc, m, scale=(1, 1, 1)):
    bpy.ops.mesh.primitive_uv_sphere_add(radius=r, location=loc, segments=32, ring_count=16)
    o = bpy.context.active_object; o.name = name; o.scale = scale
    o.data.materials.append(mat(m)); bpy.ops.object.shade_smooth(); return o

def text(name, s, size, loc, m, font=FONT_LABEL, extrude=0.0002, align="CENTER", rot=(math.pi / 2, 0, 0)):
    bpy.ops.object.text_add(location=loc, rotation=rot)
    o = bpy.context.active_object; o.name = name
    o.data.body = s; o.data.size = size; o.data.extrude = extrude
    o.data.align_x = align; o.data.align_y = "CENTER"
    o.data.font = bpy.data.fonts.load(font, check_existing=True)
    o.data.resolution_u = 2 if size < 0.008 else (3 if size < 0.02 else 6)
    o.data.materials.append(mat(m))
    return o

def parent(child, par):
    child.parent = par; child.matrix_parent_inverse = par.matrix_world.inverted()

def lathe(name, profile, loc, m, rot=(math.pi / 2, 0, 0), steps=64):
    """profile: list of (radius, height) points, revolved around local Z."""
    bm = bmesh.new()
    rings = []
    for k in range(steps):
        a = 2 * math.pi * k / steps
        rings.append([bm.verts.new((r * math.cos(a), r * math.sin(a), h)) for r, h in profile])
    for k in range(steps):
        r0, r1 = rings[k], rings[(k + 1) % steps]
        for i in range(len(profile) - 1):
            bm.faces.new((r0[i], r1[i], r1[i + 1], r0[i + 1]))
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new(name, me); link(o)
    o.location = loc; o.rotation_euler = rot
    me.materials.append(mat(m))
    for p in me.polygons: p.use_smooth = True
    return o

def marker(name, loc, **props):
    bpy.ops.object.empty_add(location=loc); e = bpy.context.active_object; e.name = name
    for k, v in props.items(): e[k] = v
    return e

def speaker(prefix, x, z, fy, radius, depth=0.07):
    """A guitar speaker seen from the front: basket rim, paper cone, dust cap. Faces -Y."""
    r = radius
    lathe(prefix + "_cone", [(r * 0.93, 0.0), (r * 0.8, -0.01), (r * 0.55, -0.032), (r * 0.3, -0.05), (r * 0.2, -0.055)],
          (x, fy, z), "Cone", rot=(math.pi / 2, 0, 0))
    lathe(prefix + "_surround", [(r, 0.0), (r * 0.965, 0.006), (r * 0.93, 0.0)], (x, fy, z), "Rubber", rot=(math.pi / 2, 0, 0))
    lathe(prefix + "_cap", [(r * 0.22, -0.05), (r * 0.18, -0.038), (r * 0.1, -0.031), (0.0, -0.029)], (x, fy, z), "DustCap", rot=(math.pi / 2, 0, 0))
    cyl(prefix + "_rim", r * 1.04, 0.006, (x, fy + 0.003, z), "BasketMetal", verts=64)
    marker(prefix + "_center", (x, fy - 0.03, z), radius=r)

# ------------------------------------------------------------------ controls
def knob_label_ring(x, z, fy, rad, m="EngraveWhite", numbers=True, size=0.0042):
    for k in range(11):
        a = math.radians(-135 + 27 * k)
        if numbers:
            text("num", str(k), size, (x + math.sin(a) * rad, fy - 0.0004, z + math.cos(a) * rad), m, FONT_TECH, extrude=0.0001)
        else:
            box("tick", (0.0009, 0.0006, 0.004), (x + math.sin(a) * rad, fy - 0.0004, z + math.cos(a) * rad), m)
            bpy.context.active_object.rotation_euler = (0, -a, 0)

def knob_chicken(cid, x, z, fy, cap_mat="KnobBlack", insert="Nickel", r=0.0115):
    """Modern knob: knurled body, metal insert, white pointer line."""
    skirt = cyl(cid + "_skirt", r * 1.12, 0.004, (x, fy - 0.002, z), cap_mat, verts=64, bevel=0.0008)
    cap = cyl(cid + "_cap", r, 0.018, (x, fy - 0.013, z), cap_mat, verts=40, bevel=0.0015, r2=r * 0.9)
    knurl = cyl(cid + "_knurl", r * 1.015, 0.012, (x, fy - 0.012, z), cap_mat, verts=40)
    # knurl ridges as a displaced copy: alternate vertices pushed out
    top = cyl(cid + "_top", r * 0.78, 0.0016, (x, fy - 0.0225, z), insert, verts=64, bevel=0.0005)
    ptr = box(cid + "_ptr", (r * 0.14, 0.0012, r * 0.85), (x, fy - 0.0234, z + r * 0.42), "PointerWhite")
    for o in (skirt, knurl, top, ptr): parent(o, cap)
    return cap

def knob_skirted(cid, x, z, fy, r=0.0105):
    """Vintage American knob: black skirt, round top, silver pointer."""
    skirt = cyl(cid + "_skirt", r * 1.45, 0.004, (x, fy - 0.003, z), "KnobBlack", verts=64, bevel=0.001)
    cap = cyl(cid + "_cap", r, 0.016, (x, fy - 0.012, z), "KnobBlack", verts=48, bevel=0.003, r2=r * 0.86)
    ptr = box(cid + "_ptr", (r * 0.16, 0.0012, r * 0.9), (x, fy - 0.0052, z + r * 1.0), "Nickel")
    top = cyl(cid + "_top", r * 0.6, 0.0012, (x, fy - 0.0205, z), "KnobBlack", verts=48, bevel=0.0004)
    for o in (skirt, ptr, top): parent(o, cap)
    return cap

def toggle(cid, x, z, fy, label=None, label_mat="EngraveWhite", label_dz=0.017, size=0.0058):
    cyl(cid + "_nut", 0.0068, 0.003, (x, fy - 0.0015, z), "Chrome", verts=6)
    cyl(cid + "_bush", 0.004, 0.007, (x, fy - 0.006, z), "Chrome", verts=32)
    bpy.ops.object.empty_add(location=(x, fy - 0.009, z)); pivot = bpy.context.active_object; pivot.name = cid + "_pivot"
    lever = cyl(cid + "_lever", 0.0019, 0.018, (x, fy - 0.018, z), "Chrome", verts=24, r2=0.0014)
    tip = sphere(cid + "_tip", 0.0029, (x, fy - 0.027, z), "Chrome")
    parent(lever, pivot); parent(tip, pivot)
    if label: text(cid + "_label", label, size, (x, fy - 0.0004, z + label_dz), label_mat, FONT_LABEL)
    return pivot

def jack(name, x, z, fy):
    cyl(name + "_nut", 0.0082, 0.004, (x, fy - 0.002, z), "Nickel", verts=6, bevel=0.0005)
    cyl(name + "_ring", 0.0058, 0.005, (x, fy - 0.0035, z), "Nickel", verts=32)
    cyl(name + "_hole", 0.0038, 0.006, (x, fy - 0.0036, z), "Bakelite", verts=32)

def tube(prefix, x, y, z, height=0.06, r=0.0135):
    tz = z
    cyl(prefix + "_base", r * 1.05, 0.012, (x, y, tz - height / 2 - 0.004), "Bakelite", rot=(0, 0, 0))
    cyl(prefix + "_glass", r, height * 0.75, (x, y, tz - height * 0.1), "Glass", rot=(0, 0, 0))
    sphere(prefix + "_dome", r, (x, y, tz + height * 0.27), "Glass", scale=(1, 1, 1.3))
    cyl(prefix + "_plate", r * 0.55, height * 0.5, (x, y, tz - height * 0.05), "TubePlate", rot=(0, 0, 0), verts=6)
    cyl(prefix + "_getter", r * 0.75, 0.002, (x, y, tz + height * 0.3), "Getter", rot=(0, 0, 0), verts=24)
    cyl(prefix + "_heater", 0.0028, height * 0.42, (x, y, tz - height * 0.05), "Heater", rot=(0, 0, 0), verts=10)

def corners(W, D, z_lo, z_hi, m="CornerMetal", s=0.026):
    for x in (-W / 2, W / 2):
        for z in (z_lo, z_hi):
            for y in (-D / 2, D / 2):
                box("Corner", (s, s, s), (x * 0.993, y * 0.975, z), m, bevel=s * 0.28)

# ------------------------------------------------------------------ export
def bake_ao(knob_ids, slider_ids, size=2048, samples=48):
    """Join every static part into one mesh, give it a second UV set (lightmap) and bake
    ambient occlusion into it with Cycles. The real-time engine multiplies this into the
    lighting: every crevice, every knob base, every recess gets the soft darkening that real
    objects have and that real-time lighting can't compute. That is most of the 'CG look'."""
    scene = bpy.context.scene
    interactive = set(k + "_cap" for k in knob_ids) | set(k + "_cap" for k in slider_ids)
    def is_glassy(o): return any(m and m.name in ("Glass",) for m in o.data.materials)
    static = [o for o in bpy.data.objects if o.type == "MESH" and o.parent is None and o.name not in interactive
              and not o.get("keep_uv") and not is_glassy(o) and not o.name.endswith("_hole")]
    bpy.ops.object.select_all(action="DESELECT")
    for o in static: o.select_set(True)
    bpy.context.view_layer.objects.active = static[0]
    bpy.ops.object.join()
    body = bpy.context.active_object; body.name = "Static"
    lm = body.data.uv_layers.new(name="Lightmap"); body.data.uv_layers.active = lm
    bpy.ops.object.mode_set(mode="EDIT"); bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.004, area_weight=0.0, correct_aspect=True, scale_to_bounds=True)
    bpy.ops.object.mode_set(mode="OBJECT")
    img = bpy.data.images.new("AO", size, size, alpha=False, float_buffer=False)
    for m in body.data.materials:
        nt = m.node_tree; n = nt.nodes.new("ShaderNodeTexImage"); n.image = img; n.name = "BakeTarget"
        for x in nt.nodes: x.select = False
        n.select = True; nt.nodes.active = n
    hidden = [o for o in bpy.data.objects if o.type == "MESH" and is_glassy(o)]
    for o in hidden: o.hide_render = True
    scene.render.engine = "CYCLES"; scene.cycles.device = "CPU"; scene.cycles.samples = samples
    scene.render.bake.margin = 6
    bpy.ops.object.select_all(action="DESELECT"); body.select_set(True); bpy.context.view_layer.objects.active = body
    bpy.ops.object.bake(type="AO")
    for o in hidden: o.hide_render = False
    for m in body.data.materials: m.node_tree.nodes.remove(m.node_tree.nodes["BakeTarget"])
    return img

def export(path, knob_ids, toggle_ids, slider_ids=(), ao=True):
    """Freeze, box-map UVs in world metres, write embedded glTF JSON (served as .json)."""
    for cid in knob_ids:
        o = bpy.data.objects[cid + "_cap"]; o.rotation_euler = (math.pi / 2, math.radians(-135), 0)
    for cid in toggle_ids:
        bpy.data.objects[cid + "_pivot"].rotation_euler = (0, 0, 0)
    bpy.context.view_layer.update()
    for o in [o for o in bpy.data.objects if o.type == "FONT"]:
        bpy.ops.object.select_all(action="DESELECT"); o.select_set(True)
        bpy.context.view_layer.objects.active = o; bpy.ops.object.convert(target="MESH")
    for o in [o for o in bpy.data.objects if o.type == "MESH"]:
        bpy.context.view_layer.objects.active = o
        for m in list(o.modifiers): bpy.ops.object.modifier_apply(modifier=m.name)
    for o in [o for o in bpy.data.objects if o.type == "MESH" and not o.get("keep_uv")]:
        bm = bmesh.new(); bm.from_mesh(o.data)
        uv = bm.loops.layers.uv.verify(); mw = o.matrix_world; nmat = mw.to_3x3().inverted().transposed()
        for f in bm.faces:
            n = (nmat @ f.normal).normalized(); ax = max(range(3), key=lambda i: abs(n[i]))
            for l in f.loops:
                w = mw @ l.vert.co
                l[uv].uv = (w.y, w.z) if ax == 0 else ((w.x, w.z) if ax == 1 else (w.x, w.y))
        bm.to_mesh(o.data); bm.free()
    if ao:
        img = bake_ao(knob_ids, slider_ids)
        png = path.replace(".json", "_ao.png")
        img.filepath_raw = png; img.file_format = "PNG"; img.save()
        print("baked", png)
    tmp = path + ".glb"
    bpy.ops.export_scene.gltf(filepath=tmp, export_format="GLB", export_apply=True, export_lights=False,
                              export_cameras=False, export_yup=True, export_extras=True)
    b = open(tmp, "rb").read()
    l = struct.unpack("<I", b[12:16])[0]; j = json.loads(b[20:20 + l])
    off = 20 + l; bl = struct.unpack("<I", b[off:off + 4])[0]; binc = b[off + 8:off + 8 + bl]
    j["buffers"] = [{"byteLength": len(binc), "uri": "data:application/octet-stream;base64," + base64.b64encode(binc).decode()}]
    json.dump(j, open(path, "w"), separators=(",", ":"))
    os.remove(tmp)
    print("exported", path, os.path.getsize(path))

# ------------------------------------------------------------------ shared 2x12 cab
def cab_2x12(CW=0.70, CH=0.46, CD=0.30, wheels=True, tolex="Tolex", corner="CornerMetal", grille="HexMesh",
             badge_mat="SteelPlate", badge_text="EngraveDark", frame=None):
    """Closed-back 2x12 with a perforated front, two speakers behind it and a badge.
    Returns the cab top height."""
    WH = 0.06 if wheels else 0.016
    cz0 = WH
    box("Cab", (CW, CD, CH), (0, 0, cz0 + CH / 2), tolex, bevel=0.014)
    for (sx, sz, lx, lz) in [(CW - 0.036, 0.016, 0, cz0 + CH - 0.026), (CW - 0.036, 0.016, 0, cz0 + 0.026),
                             (0.016, CH - 0.036, -CW / 2 + 0.026, cz0 + CH / 2), (0.016, CH - 0.036, CW / 2 - 0.026, cz0 + CH / 2)]:
        box("CabFrame", (sx, 0.012, sz), (lx, -CD / 2 + 0.002, lz), frame or corner, bevel=0.003)
    box("CabBaffle", (CW - 0.06, 0.01, CH - 0.06), (0, -CD / 2 + 0.03, cz0 + CH / 2), "Baffle")
    for i, x in enumerate((-0.165, 0.165)):
        speaker("spk%d" % i, x, cz0 + CH / 2 - 0.005, -CD / 2 + 0.024, 0.148)
    bpy.ops.mesh.primitive_plane_add(size=1, location=(0, -CD / 2 - 0.002, cz0 + CH / 2), rotation=(math.pi / 2, 0, 0))
    g = bpy.context.active_object; g.name = "CabGrille"; g.scale = (CW - 0.05, CH - 0.05, 1); bpy.ops.object.transform_apply(scale=True)
    g.data.materials.append(mat(grille))
    box("CabBadge", (0.17, 0.006, 0.042), (0.17, -CD / 2 - 0.006, cz0 + 0.1), badge_mat, bevel=0.004)
    text("CabBadgeText", "ToxicRig", 0.026, (0.18, -CD / 2 - 0.0095, cz0 + 0.099), badge_text, FONT_LOGO, extrude=0.0004)
    emblem("CabEmblem", (0.103, -CD / 2 - 0.0094, cz0 + 0.1), 0.03)
    marker("ShotCab", (0, -CD / 2, cz0 + CH / 2), w=CW, h=CH)
    corners(CW, CD, cz0 + 0.004, cz0 + CH - 0.004, m=corner)
    if wheels:
        for x in (-CW / 2 + 0.06, CW / 2 - 0.06):
            for y in (-CD / 2 + 0.06, CD / 2 - 0.06):
                box("CasterPlate", (0.05, 0.05, 0.004), (x, y, cz0 - 0.002), "CornerMetal", bevel=0.002)
                cyl("CasterStem", 0.006, 0.02, (x, y, cz0 - 0.014), "Chrome", rot=(0, 0, 0))
                cyl("CasterWheel", 0.024, 0.016, (x, y + 0.018, 0.024), "Rubber", rot=(0, math.pi / 2, 0), verts=40, bevel=0.003)
                cyl("CasterHub", 0.01, 0.018, (x, y + 0.018, 0.024), "Chrome", rot=(0, math.pi / 2, 0), verts=24)
    else:
        for x in (-CW / 2 + 0.07, CW / 2 - 0.07):
            for y in (-CD / 2 + 0.05, CD / 2 - 0.05):
                cyl("Foot", 0.016, 0.016, (x, y, cz0 / 2), "Rubber", rot=(0, 0, 0), bevel=0.003)
    return cz0 + CH

def emblem(name, loc, size, m="Emblem"):
    """A square decal facing -Y; the engine paints the hazard emblem on it."""
    bpy.ops.mesh.primitive_plane_add(size=size, location=loc, rotation=(math.pi / 2, 0, 0))
    o = bpy.context.active_object; o.name = name; o.data.materials.append(mat(m))
    # UVs 0..1 across the decal (kept by export: only box-mapped when not named Emblem)
    o["keep_uv"] = 1
    return o

def slider(cid, x, z, fy, travel=0.032, m="BrassKnob"):
    """Vertical fader: slot in the panel, cap centred at 0 dB. Moves along world Z."""
    box(cid + "_slot", (0.0026, 0.002, travel + 0.008), (x, fy - 0.0008, z), "Baffle")
    cap = box(cid + "_cap", (0.009, 0.009, 0.0055), (x, fy - 0.0055, z), m, bevel=0.0012)
    box(cid + "_line", (0.0092, 0.0004, 0.0008), (x, fy - 0.0102, z), "EngraveDark")
    parent(bpy.data.objects[cid + "_line"], cap)
    for k in range(9):
        zz = z - travel / 2 + travel * k / 8
        box("tick", (0.003 if k % 4 else 0.005, 0.0004, 0.0006), (x + 0.0065, fy - 0.0004, zz), "EngraveWhite")
    return cap

def knob_acrylic(cid, x, z, fy, r=0.0118):
    """Clear acrylic knob lit from inside (purple), metal shaft cap, pointer line."""
    skirt = cyl(cid + "_skirt", r * 1.1, 0.004, (x, fy - 0.002, z), "AcrylicRing", verts=64, bevel=0.0008)
    cap = cyl(cid + "_cap", r, 0.018, (x, fy - 0.013, z), "Acrylic", verts=48, bevel=0.002, r2=r * 0.92)
    core = cyl(cid + "_core", r * 0.42, 0.014, (x, fy - 0.011, z), "AcrylicCore", verts=24)
    top = cyl(cid + "_top", r * 0.5, 0.0012, (x, fy - 0.0226, z), "Nickel", verts=48, bevel=0.0004)
    ptr = box(cid + "_ptr", (r * 0.12, 0.0012, r * 0.55), (x, fy - 0.0228, z + r * 0.62), "PointerWhite")
    for o in (skirt, core, top, ptr): parent(o, cap)
    return cap

def knurl_knob(cid, x, z, fy, r, body_mat, top_mat="KnobTopBrushed", led_mat=None, teeth=36, h=0.019):
    """Knurled metal knob (real teeth), brushed top insert, optional LED dot. Faces -Y."""
    bm = bmesh.new()
    n = teeth * 2
    ring = []
    for i in range(n):
        a = 2 * math.pi * i / n
        rr = r if i % 2 == 0 else r * 0.955
        ring.append((rr * math.cos(a), rr * math.sin(a)))
    bot = [bm.verts.new((px, py, 0)) for px, py in ring]
    top = [bm.verts.new((px * 0.97, py * 0.97, h)) for px, py in ring]
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((bot[i], bot[j], top[j], top[i]))
    bm.faces.new(list(reversed(bot))); bm.faces.new(top)
    me = bpy.data.meshes.new(cid + "_cap"); bm.to_mesh(me); bm.free()
    cap = bpy.data.objects.new(cid + "_cap", me); link(cap)
    cap.location = (x, fy - 0.0005, z); cap.rotation_euler = (math.pi / 2, 0, 0)
    me.materials.append(mat(body_mat))
    bpy.context.view_layer.objects.active = cap
    mod = cap.modifiers.new("bev", "BEVEL"); mod.width = 0.0009; mod.segments = 2; mod.limit_method = "ANGLE"
    bpy.ops.object.shade_auto_smooth()
    # bmesh local +Z = knob axis; rotation (pi/2,0,0) maps +Z to -Y (towards the player)
    top_disc = cyl(cid + "_top", r * 0.78, 0.0014, (x, fy - 0.0005 - h - 0.0005, z), top_mat, verts=64, bevel=0.0005)
    skirt = cyl(cid + "_skirt", r * 1.12, 0.0025, (x, fy - 0.0018, z), body_mat, verts=64, bevel=0.0008)
    parent(top_disc, cap); parent(skirt, cap)
    if led_mat:
        led = cyl(cid + "_led", r * 0.13, 0.001, (x, fy - h - 0.0016, z + r * 0.55), led_mat, verts=16)
        parent(led, cap)
    else:
        ptr = box(cid + "_ptr", (r * 0.1, 0.0008, r * 0.45), (x, fy - h - 0.0016, z + r * 0.5), "EngraveDark")
        parent(ptr, cap)
    return cap

def power_tube(prefix, x, y, z, h=0.1, r=0.0135):
    """EL34-style bottle: base, straight body, shoulder, dome; plates, micas, getter inside."""
    cyl(prefix + "_base", r * 1.02, 0.016, (x, y, z + 0.008), "Bakelite", rot=(0, 0, 0), verts=40, bevel=0.0015)
    cyl(prefix + "_key", r * 0.25, 0.006, (x, y, z - 0.002), "Bakelite", rot=(0, 0, 0), verts=12)
    prof = [(r * 0.98, 0.0), (r, 0.004), (r, h * 0.55), (r * 1.06, h * 0.68), (r * 1.0, h * 0.82), (r * 0.82, h * 0.93), (r * 0.45, h * 0.99), (0.0, h * 1.0)]
    lathe(prefix + "_glass", prof, (x, y, z + 0.016), "Glass", rot=(0, 0, 0), steps=48)
    cyl(prefix + "_pip", 0.0018, 0.004, (x, y, z + 0.016 + h + 0.0015), "Glass", rot=(0, 0, 0), verts=12)
    zc = z + 0.016 + h * 0.42
    for sx in (-1, 1):
        box(prefix + "_plate", (r * 0.75, 0.0012, h * 0.42), (x + sx * r * 0.18, y, zc), "TubePlate")
    box(prefix + "_plateSide", (0.0012, r * 0.8, h * 0.4), (x, y, zc), "TubePlate")
    for dz in (-h * 0.23, h * 0.23):
        cyl(prefix + "_mica", r * 0.88, 0.0008, (x, y, zc + dz), "Mica", rot=(0, 0, 0), verts=32)
    cyl(prefix + "_heater", 0.0018, h * 0.36, (x, y, zc), "Heater", rot=(0, 0, 0), verts=10)
    sphere(prefix + "_getter", r * 0.7, (x, y, z + 0.016 + h * 0.86), "Getter", scale=(1, 1, 0.55))
    for k in range(4):
        a = k * math.pi / 2 + 0.4
        cyl(prefix + "_rod", 0.0005, h * 0.55, (x + math.cos(a) * r * 0.6, y + math.sin(a) * r * 0.6, zc), "Nickel", rot=(0, 0, 0), verts=6)

def art_plane(name, center, w, h, layout=None, y_off=0.0):
    """Faceplate area painted by the engine (engraving, labels, scales). Faces -Y."""
    bpy.ops.mesh.primitive_plane_add(size=1, location=center, rotation=(math.pi / 2, 0, 0))
    o = bpy.context.active_object; o.name = name; o.scale = (w, h, 1); bpy.ops.object.transform_apply(scale=True)
    o.data.materials.append(mat(name)); o["keep_uv"] = 1
    o["w"] = w; o["h"] = h
    if layout is not None: o["layout"] = json.dumps(layout)
    return o
