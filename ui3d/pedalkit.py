# Shared parts for stompboxes built lying flat (top face = +Z, knobs stand along +Z).
# The knob object named <id> is the rotating part: its local +Z is the shaft axis, which becomes
# +Y in glTF, so the scene turns it with rotation.y. Skirt/washer stay fixed on the enclosure.
import bpy, bmesh, math
from rig3d import *


def _knurl_mesh(name, r, h, teeth=40, taper=0.94):
    bm = bmesh.new(); n = teeth * 2
    ring = [((r if i % 2 == 0 else r * 0.955) * math.cos(2 * math.pi * i / n),
             (r if i % 2 == 0 else r * 0.955) * math.sin(2 * math.pi * i / n)) for i in range(n)]
    bot = [bm.verts.new((x, y, 0)) for x, y in ring]
    top = [bm.verts.new((x * taper, y * taper, h)) for x, y in ring]
    for i in range(n):
        j = (i + 1) % n; bm.faces.new((bot[i], bot[j], top[j], top[i]))
    bm.faces.new(list(reversed(bot))); bm.faces.new(top)
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new(name, me); bpy.context.collection.objects.link(o)
    return o


def pedal_knob(cid, x, y, z0, r=0.0115, h=0.015, body="KnobBlack", top="KnobTopBrushed", skirt="Chrome"):
    """Knurled knob standing on the top plate at height z0, with brushed insert and a white pointer."""
    # Fixed parts: threaded bushing nut and chrome washer
    cyl(cid + "_nut", r * 0.62, 0.002, (x, y, z0 + 0.001), "Chrome", rot=(0, 0, 0), verts=6, bevel=0.0003)
    cyl(cid + "_skirt", r * 1.18, 0.0012, (x, y, z0 + 0.0006), skirt, rot=(0, 0, 0), verts=64, bevel=0.0004)
    k = _knurl_mesh(cid, r, h)
    k.location = (x, y, z0 + 0.0022)
    k.data.materials.append(mat(body))
    bpy.context.view_layer.objects.active = k
    mod = k.modifiers.new("bev", "BEVEL"); mod.width = 0.0007; mod.segments = 2; mod.limit_method = "ANGLE"
    bpy.ops.object.shade_auto_smooth()
    # Brushed metal insert and pointer line, children of the knob so they turn with it
    t = cyl(cid + "_top", r * 0.8, 0.0012, (x, y, z0 + 0.0022 + h + 0.0004), top, rot=(0, 0, 0), verts=64, bevel=0.0004)
    p = box(cid + "_ptr", (0.0011, r * 0.72, 0.0006), (x, y + r * 0.42, z0 + 0.0022 + h + 0.0011), "PointerWhite")
    for c in (t, p):
        c.parent = k; c.matrix_parent_inverse = k.matrix_world.inverted()
    return k


def enclosure(W, D, H, paint, plate=None):
    """Die-cast body: rounded box, recessed base plate, rubber feet, side jacks with nuts."""
    box("Body", (W, D, H - 0.003), (0, 0, (H - 0.003) / 2 + 0.003), paint, bevel=0.0045, segments=5)
    box("BasePlate", (W - 0.004, D - 0.004, 0.003), (0, 0, 0.0015), "BaseMetal", bevel=0.0008)
    for sx in (-1, 1):
        for sy in (-1, 1):
            cyl("foot", 0.0045, 0.002, (sx * (W / 2 - 0.009), sy * (D / 2 - 0.009), -0.001), "Rubber", rot=(0, 0, 0), verts=24, bevel=0.0006)
    # Input / output jacks mounted on top at the back edge (towards the amp), with hex nuts
    jx = W / 2 - (0.011 if W < 0.06 else 0.016)
    for sx in (-1, 1):
        cyl("jack_nut", 0.0062, 0.0022, (sx * jx, D / 2 - 0.009, H + 0.0011), "Chrome", rot=(0, 0, 0), verts=6, bevel=0.0003)
        cyl("jack_hole", 0.0034, 0.0026, (sx * jx, D / 2 - 0.009, H + 0.0014), "JackHole", rot=(0, 0, 0), verts=24)
    # DC jack on the back
    box("dc_jack", (0.011, 0.004, 0.009), (0, D / 2 + 0.0016, H * 0.62), "Bakelite", bevel=0.0008)
    if plate:
        box("TopPlate", (W - 0.01, D - 0.01, 0.0008), (0, 0, H + 0.0002), plate, bevel=0.0004)
    for sx in (-1, 1):
        for sy in (-1, 1):
            cyl("screw", 0.0021, 0.0009, (sx * (W / 2 - 0.0055), sy * (D / 2 - 0.0055), H + 0.0004), "Chrome", rot=(0, 0, 0), verts=20, bevel=0.0003)


def footswitch(x, y, z0):
    cyl("footswitch_nut", 0.0095, 0.0024, (x, y, z0 + 0.0012), "Chrome", rot=(0, 0, 0), verts=6, bevel=0.0004)
    cyl("footswitch_bezel", 0.0068, 0.006, (x, y, z0 + 0.0054), "Chrome", rot=(0, 0, 0), verts=40, bevel=0.0005)
    cyl("footswitch", 0.0058, 0.0035, (x, y, z0 + 0.0098), "Chrome", rot=(0, 0, 0), verts=40, bevel=0.0012)


def led(x, y, z0):
    cyl("led_bezel", 0.0034, 0.0012, (x, y, z0 + 0.0006), "Chrome", rot=(0, 0, 0), verts=24, bevel=0.0003)
    sphere("led", 0.0024, (x, y, z0 + 0.0012), "LedGreen", scale=(1, 1, 0.7))


def label(name, s, size, x, y, z0, m="EngraveLight", font=None):
    return text(name, s, size, (x, y, z0 + 0.0003), m, font or FONT_TECH, extrude=0.0002, rot=(0, 0, 0))
