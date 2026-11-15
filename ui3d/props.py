# Stage props that make the room feel lived-in: an electric guitar on its stand (original
# offset body), a road case, a floor monitor and a drum kit. Positions are final (metres).
import sys, math
sys.path.insert(0, "/home/claude/amp3d")
from rig3d import *
import bmesh

reset()

def extrude_outline(name, pts, depth, m, loc=(0, 0, 0), rot=(0, 0, 0), bevel=0.003):
    bm = bmesh.new()
    vs = [bm.verts.new((x, y, 0)) for x, y in pts]
    f = bm.faces.new(vs)
    r = bmesh.ops.extrude_face_region(bm, geom=[f])
    for v in [e for e in r["geom"] if isinstance(e, bmesh.types.BMVert)]: v.co.z += depth
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new(name, me); link(o); o.location = loc; o.rotation_euler = rot
    me.materials.append(mat(m))
    if bevel:
        bpy.context.view_layer.objects.active = o
        mod = o.modifiers.new("bev", "BEVEL"); mod.width = bevel; mod.segments = 4; mod.limit_method = "ANGLE"
    for p in me.polygons: p.use_smooth = True
    o.modifiers.new("ws", "WEIGHTED_NORMAL") if hasattr(o.modifiers, "new") else None
    return o

def body_outline(n=140):
    """Original offset double-cutaway outline (metres), built from a few smooth lobes."""
    pts = []
    for i in range(n):
        t = 2 * math.pi * i / n
        x = 0.17 * math.cos(t)
        y = 0.215 * math.sin(t)
        # waist + offset lower bout + two horns
        x *= 1.0 - 0.16 * math.exp(-((math.sin(t)) ** 2) * 4) * (1 if abs(math.cos(t)) > 0.2 else 0)
        if y > 0.07:  # upper bout: horns
            horn = 0.06 * math.exp(-((x - 0.07) / 0.035) ** 2) + 0.045 * math.exp(-((x + 0.08) / 0.035) ** 2)
            y += horn
            y -= 0.05 * math.exp(-(x / 0.04) ** 2)  # neck pocket notch between the horns
        if y < 0: x += 0.012 * math.sin(t * 2)    # offset lower bout
        pts.append((x, y))
    return pts

def guitar(base):
    bx, by, bz = base
    tilt = math.radians(-14)  # leaning back on the stand
    g = []
    body = extrude_outline("GtrBody", body_outline(), 0.044, "GuitarPaint", (0, 0, 0), (0, 0, 0), bevel=0.006)
    g.append(body)
    # pickguard
    pg = extrude_outline("GtrGuard", [(x * 0.62 + 0.02, y * 0.55 + 0.03) for x, y in body_outline(80)], 0.0025, "GuardPearl", (0, 0, 0.044), bevel=0.0008)
    g.append(pg)
    # neck + fretboard + frets
    neck = box("GtrNeck", (0.05, 0.43, 0.022), (0, 0.42, 0.028), "Maple", bevel=0.006); g.append(neck)
    fb = box("GtrBoard", (0.052, 0.43, 0.006), (0, 0.42, 0.042), "Rosewood", bevel=0.001); g.append(fb)
    for k in range(22):
        y = 0.635 - 0.43 * (1 - 2 ** (-k / 12.0)) * 1.12
        if y < 0.21: break
        g.append(box("Fret", (0.052, 0.0016, 0.0012), (0, y, 0.0455), "Nickel"))
        if k in (3, 5, 7, 9, 15, 17): g.append(cyl("Dot", 0.0028, 0.0004, (0, y + 0.012, 0.0452), "GuardPearl", rot=(0, 0, 0), verts=16))
    # headstock (6 in line) + tuners
    head = extrude_outline("GtrHead", [(-0.022, 0), (0.03, 0.0), (0.045, 0.07), (0.04, 0.17), (0.01, 0.19), (-0.03, 0.17), (-0.034, 0.02)], 0.016, "GuitarPaint", (0, 0.632, 0.02), bevel=0.003)
    g.append(head)
    for k in range(6):
        y = 0.645 + 0.026 * k
        g.append(cyl("Tuner", 0.004, 0.014, (-0.04, y, 0.03), "Chrome", rot=(0, math.pi / 2, 0), verts=12))
        g.append(box("TunerKey", (0.006, 0.012, 0.004), (-0.05, y, 0.03), "Chrome", bevel=0.001))
        g.append(cyl("Post", 0.003, 0.012, (-0.02, y, 0.041), "Chrome", rot=(0, 0, 0), verts=12))
    # pickups, bridge, knobs, jack, strings
    for y in (0.12, 0.02):
        g.append(box("Pickup", (0.07, 0.019, 0.01), (0.002, y, 0.051), "PickupBlack", bevel=0.002))
        for i in range(6): g.append(cyl("Pole", 0.0022, 0.002, (-0.026 + i * 0.0104, y, 0.0565), "Nickel", rot=(0, 0, 0), verts=12))
    g.append(box("Bridge", (0.072, 0.014, 0.008), (0, -0.06, 0.05), "Chrome", bevel=0.002))
    g.append(box("Tail", (0.08, 0.01, 0.01), (0, -0.095, 0.05), "Chrome", bevel=0.003))
    for i, (x, y) in enumerate(((0.08, -0.07), (0.1, -0.11), (0.065, -0.13))):
        g.append(cyl("GtrKnob", 0.009, 0.014, (x, y, 0.052), "KnobGold", rot=(0, 0, 0), verts=24, bevel=0.002))
    g.append(toggle_switch := cyl("Sel", 0.002, 0.016, (-0.11, 0.1, 0.05), "Chrome", rot=(0.3, 0, 0), verts=12))
    for i in range(6):
        x0 = -0.026 + i * 0.0104; x1 = -0.0175 + i * 0.007
        bpy.ops.mesh.primitive_cylinder_add(radius=0.00045 + 0.0001 * i, depth=1, vertices=6)
        st = bpy.context.active_object; st.name = "String"
        a = Vector((x0, -0.06, 0.056)); b = Vector((-0.02, 0.645 + 0.026 * (5 - i), 0.046)) if False else Vector((x1 * 0.6, 0.64, 0.047))
        d = b - a; st.scale = (1, 1, d.length); st.location = (a + b) / 2
        st.rotation_euler = d.to_track_quat("Z", "Y").to_euler(); st.data.materials.append(mat("StringSteel")); g.append(st)
    # group: rotate and place the whole guitar on the stand
    bpy.ops.object.empty_add(location=(0, 0, 0)); root = bpy.context.active_object; root.name = "GuitarRoot"
    for o in g: o.parent = root
    root.rotation_euler = (math.radians(90) + tilt, 0, math.radians(18))
    root.location = (bx, by, bz + 0.24)
    # A-frame stand
    sm = "StandBlack"
    for sx in (-1, 1):
        a = Vector((bx + sx * 0.13, by - 0.12, 0.0)); b = Vector((bx + sx * 0.05, by + 0.05, 0.62))
        bpy.ops.mesh.primitive_cylinder_add(radius=0.0075, depth=1, vertices=16); t = bpy.context.active_object; t.name = "StandLeg"
        d = b - a; t.scale = (1, 1, d.length); t.location = (a + b) / 2; t.rotation_euler = d.to_track_quat("Z", "Y").to_euler(); t.data.materials.append(mat(sm))
    bpy.ops.mesh.primitive_cylinder_add(radius=0.0075, depth=0.42, vertices=16, location=(bx, by + 0.17, 0.2), rotation=(math.radians(-30), 0, 0))
    bpy.context.active_object.data.materials.append(mat(sm))
    box("StandCradle", (0.3, 0.06, 0.012), (bx, by - 0.08, 0.21), "Rubber", bevel=0.005)
    cyl("StandYoke", 0.012, 0.05, (bx, by + 0.05, 0.64), "Rubber", rot=(0, math.pi / 2, 0), verts=16)

def road_case(c, w=0.62, d=0.42, h=0.46, rotz=0.0):
    x, y = c
    box("CaseBody", (w, d, h), (x, y, h / 2), "CaseBlack", bevel=0.004)
    for z in (0.01, h - 0.01, h * 0.62):
        box("CaseEdge", (w + 0.004, d + 0.004, 0.018), (x, y, z), "Aluminium", bevel=0.002)
    for sx in (-1, 1):
        for sy in (-1, 1):
            for z in (0.022, h - 0.022):
                box("CaseCorner", (0.05, 0.05, 0.05), (x + sx * (w / 2 - 0.02), y + sy * (d / 2 - 0.02), z), "Chrome", bevel=0.012)
    for sx in (-0.18, 0.18):
        box("Latch", (0.05, 0.012, 0.035), (x + sx, y - d / 2 - 0.006, h * 0.62), "Chrome", bevel=0.003)
    box("Handle", (0.11, 0.02, 0.03), (x, y - d / 2 - 0.01, h * 0.32), "Chrome", bevel=0.006)
    text("CaseStencil", "TOXICRIG  ·  ROAD", 0.04, (x, y - d / 2 - 0.0025, h * 0.2), "StencilWhite", FONT_TECH, extrude=0.0002)

def wedge(c):
    x, y = c
    bm = bmesh.new()
    pts = [(-0.28, -0.18, 0), (0.28, -0.18, 0), (0.28, 0.18, 0), (-0.28, 0.18, 0), (-0.28, 0.12, 0.3), (0.28, 0.12, 0.3), (0.28, -0.18, 0.12), (-0.28, -0.18, 0.12)]
    v = [bm.verts.new(p) for p in pts]
    for f in ((0, 1, 2, 3), (3, 2, 5, 4), (0, 7, 6, 1), (7, 4, 5, 6), (0, 3, 4, 7), (1, 6, 5, 2)): bm.faces.new([v[i] for i in f])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new("Wedge"); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new("Wedge", me); link(o); o.location = (x, y, 0); o.rotation_euler = (0, 0, math.pi)
    me.materials.append(mat("Tolex"))
    bpy.context.view_layer.objects.active = o; mod = o.modifiers.new("bev", "BEVEL"); mod.width = 0.008; mod.segments = 3

def drum_kit(c):
    x, y = c
    def drum(name, r, depth, loc, rot):
        cyl(name + "Shell", r, depth, loc, "DrumWrap", rot=rot, verts=64)
        for s in (-1, 1):
            off = Vector((0, 0, s * depth / 2)); m = bpy.context.active_object.matrix_world.to_3x3()
            p = Vector(loc) + m @ off
            cyl(name + "Hoop", r * 1.02, 0.012, tuple(p), "Chrome", rot=rot, verts=64)
            cyl(name + "Head", r * 0.995, 0.002, tuple(p + m @ Vector((0, 0, s * 0.004))), "DrumHead", rot=rot, verts=64)
    drum("Kick", 0.28, 0.4, (x, y, 0.3), (math.pi / 2, 0, 0))
    drum("Snare", 0.18, 0.13, (x - 0.38, y + 0.25, 0.62), (0.12, 0, 0))
    drum("Tom", 0.13, 0.16, (x - 0.12, y - 0.05, 0.78), (0.5, 0, 0))
    drum("Floor", 0.21, 0.36, (x + 0.42, y + 0.3, 0.4), (0, 0, 0))
    for (cx, cy, h, r) in ((x - 0.5, y - 0.15, 1.15, 0.24), (x + 0.4, y - 0.25, 1.25, 0.28), (x - 0.62, y + 0.35, 0.92, 0.18)):
        lathe("Cymbal", [(0.0, 0.012), (0.03, 0.012), (0.05, 0.004), (r, -0.01)], (cx, cy, h), "Bronze", rot=(0.1, 0.05, 0), steps=64)
        cyl("CymStand", 0.008, h, (cx, cy, h / 2), "Chrome", rot=(0, 0, 0), verts=12)
        for k in range(3):
            a = k * 2 * math.pi / 3
            bpy.ops.mesh.primitive_cylinder_add(radius=0.006, depth=0.3, vertices=8, location=(cx + math.cos(a) * 0.12, cy + math.sin(a) * 0.12, 0.08),
                                                rotation=(math.sin(a) * 0.9, -math.cos(a) * 0.9, 0))
            bpy.context.active_object.data.materials.append(mat("Chrome"))
    cyl("Throne", 0.17, 0.08, (x - 0.1, y + 0.62, 0.5), "Rubber", rot=(0, 0, 0), verts=48, bevel=0.02)
    cyl("ThronePost", 0.015, 0.46, (x - 0.1, y + 0.62, 0.23), "Chrome", rot=(0, 0, 0), verts=12)

# guitar removed until it meets the quality bar
road_case((0.98, -0.05))
box("CaseTop", (0.0001, 0.0001, 0.0001), (0, 0, -1), "CaseBlack")
# wedge removed
# drums removed
export("/home/claude/stage3d/public/models/props.json", [], [])
