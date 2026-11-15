# ToxicRig "Modern 5050" high-gain head on a 2x12 cab with casters.
# Real controls only: GAIN BASS MIDDLE TREBLE / PRESENCE DEPTH SAG MASTER, CHANNEL (3), BRIGHT (2).
import sys, math
sys.path.insert(0, "/home/claude/amp3d")
from rig3d import *

reset()
W, HH, HD = 0.66, 0.255, 0.255          # head
CW, CH, CD = 0.70, 0.46, 0.30           # cab (without casters)
WH = 0.06                               # caster height
cz0 = WH                                # cab bottom
z0 = cz0 + CH + 0.012                   # head bottom (on small feet)
fy = -HD / 2

# ---------------- cab
box("Cab", (CW, CD, CH), (0, 0, cz0 + CH / 2), "Tolex", bevel=0.014)
for (sx, sz, lx, lz) in [(CW - 0.036, 0.016, 0, cz0 + CH - 0.026), (CW - 0.036, 0.016, 0, cz0 + 0.026),
                         (0.016, CH - 0.036, -CW / 2 + 0.026, cz0 + CH / 2), (0.016, CH - 0.036, CW / 2 - 0.026, cz0 + CH / 2)]:
    box("CabFrame", (sx, 0.012, sz), (lx, -CD / 2 + 0.002, lz), "CornerMetal", bevel=0.003)
box("CabBaffle", (CW - 0.06, 0.01, CH - 0.06), (0, -CD / 2 + 0.03, cz0 + CH / 2), "Baffle")
for i, x in enumerate((-0.165, 0.165)):
    speaker("spk%d" % i, x, cz0 + CH / 2 - 0.005, -CD / 2 + 0.024, 0.148)
bpy.ops.mesh.primitive_plane_add(size=1, location=(0, -CD / 2 - 0.002, cz0 + CH / 2), rotation=(math.pi / 2, 0, 0))
g = bpy.context.active_object; g.name = "CabHex"; g.scale = (CW - 0.05, CH - 0.05, 1); bpy.ops.object.transform_apply(scale=True)
g.data.materials.append(mat("HexMesh"))
box("CabBadge", (0.17, 0.006, 0.042), (0.17, -CD / 2 - 0.006, cz0 + 0.1), "SteelPlate", bevel=0.004)
text("CabBadgeText", "ToxicRig", 0.026, (0.17, -CD / 2 - 0.0095, cz0 + 0.099), "EngraveDark", FONT_LOGO, extrude=0.0004)
for sx, sz in ((-1, -1), (-1, 1), (1, -1), (1, 1)):
    cyl("BadgeScrew", 0.0022, 0.002, (0.17 + sx * 0.077, -CD / 2 - 0.0095, cz0 + 0.1 + sz * 0.014), "Nickel", verts=16)
marker("ShotCab", (0, -CD / 2, cz0 + CH / 2), w=CW, h=CH)
corners(CW, CD, cz0 + 0.004, cz0 + CH - 0.004)
# Casters
for x in (-CW / 2 + 0.06, CW / 2 - 0.06):
    for y in (-CD / 2 + 0.06, CD / 2 - 0.06):
        box("CasterPlate", (0.05, 0.05, 0.004), (x, y, cz0 - 0.002), "CornerMetal", bevel=0.002)
        cyl("CasterStem", 0.006, 0.02, (x, y, cz0 - 0.014), "Chrome", rot=(0, 0, 0))
        cyl("CasterFork", 0.016, 0.012, (x, y + 0.012, cz0 - 0.026), "CornerMetal", rot=(0, math.pi / 2, 0), verts=6)
        cyl("CasterWheel", 0.024, 0.016, (x, y + 0.018, 0.024), "Rubber", rot=(0, math.pi / 2, 0), verts=40, bevel=0.003)
        cyl("CasterHub", 0.01, 0.018, (x, y + 0.018, 0.024), "Chrome", rot=(0, math.pi / 2, 0), verts=24)

# ---------------- head
for x in (-W / 2 + 0.07, W / 2 - 0.07):
    for y in (-HD / 2 + 0.05, HD / 2 - 0.05):
        cyl("HeadFoot", 0.014, 0.012, (x, y, z0 - 0.006), "Rubber", rot=(0, 0, 0), bevel=0.002)
box("Head", (W, HD, HH), (0, 0, z0 + HH / 2), "Tolex", bevel=0.013)
corners(W, HD, z0 + 0.004, z0 + HH - 0.004)
# Handle
box("Handle", (0.2, 0.032, 0.014), (0, 0, z0 + HH + 0.02), "Rubber", bevel=0.006)
for x in (-0.12, 0.12):
    box("HandleCap", (0.03, 0.044, 0.02), (x, 0, z0 + HH + 0.008), "CornerMetal", bevel=0.005)

# Control panel (upper half): dark brushed steel with a bevelled frame and four screws.
PZ = z0 + HH - 0.075; PH = 0.112
box("PanelFrame", (W - 0.03, 0.01, PH + 0.012), (0, fy + 0.001, PZ), "CornerMetal", bevel=0.003)
box("Panel", (W - 0.044, 0.008, PH), (0, fy - 0.003, PZ), "PanelSteel", bevel=0.002)
pf = fy - 0.007
for sx in (-1, 1):
    for sz in (-1, 1):
        cyl("PanelScrew", 0.0026, 0.002, (sx * (W / 2 - 0.035), pf - 0.0005, PZ + sz * (PH / 2 - 0.009)), "Nickel", verts=16, bevel=0.0006)
jack("input", -W / 2 + 0.06, PZ + 0.024, pf)
text("input_label", "INPUT", 0.0058, (-W / 2 + 0.06, pf - 0.0003, PZ + 0.043), "EngraveWhite")
jack("fxloop", -W / 2 + 0.06, PZ - 0.026, pf)
text("loop_label", "FX LOOP", 0.0058, (-W / 2 + 0.06, pf - 0.0003, PZ - 0.008), "EngraveWhite")
row1 = [("gain", "GAIN"), ("bass", "BASS"), ("mid", "MIDDLE"), ("treble", "TREBLE")]
row2 = [("presence", "PRESENCE"), ("resonance", "DEPTH"), ("sag", "SAG"), ("master", "MASTER")]
x0, dx = -0.205, 0.064
for i, (cid, lab) in enumerate(row1):
    x = x0 + dx * i; z = PZ + 0.022
    knob_chicken(cid, x, z, pf, "KnobBlack", "Nickel")
    text(cid + "_label", lab, 0.0062, (x, pf - 0.0003, z + 0.0225), "EngraveWhite")
    knob_label_ring(x, z, pf, 0.0165, numbers=False)
for i, (cid, lab) in enumerate(row2):
    x = x0 + dx * i; z = PZ - 0.03
    knob_chicken(cid, x, z, pf, "KnobGold", "KnobGoldTop")
    text(cid + "_label", lab, 0.0062, (x, pf - 0.0003, z + 0.0225), "EngraveWhite")
    knob_label_ring(x, z, pf, 0.0165, numbers=False)
toggle("channel", 0.06, PZ + 0.02, pf, "CHANNEL")
text("ch_names", "CLEAN   CRUNCH   LEAD", 0.0042, (0.06, pf - 0.0003, PZ + 0.004), "EngraveWhite")
toggle("bright", 0.06, PZ - 0.032, pf, "BRIGHT")
cyl("Jewel", 0.0068, 0.008, (0.105, pf - 0.006, PZ + 0.022), "Jewel", verts=32, bevel=0.002)
cyl("JewelRing", 0.0098, 0.004, (0.105, pf - 0.002, PZ + 0.022), "Chrome", verts=48, bevel=0.001)
text("pwr_label", "POWER", 0.0052, (0.105, pf - 0.0003, PZ + 0.04), "EngraveWhite")
# Tube window with four tall power tubes.
WX, WZ, WW, WHH = 0.205, PZ + 0.012, 0.17, 0.072
cutter = box("WinCutter", (WW, 0.06, WHH), (WX, fy, WZ), "Baffle")
for t in ("Panel", "PanelFrame", "Head"):
    o = bpy.data.objects[t]; mod = o.modifiers.new("hole", "BOOLEAN"); mod.operation = "DIFFERENCE"; mod.object = cutter; mod.solver = "EXACT"
    bpy.context.view_layer.objects.active = o; bpy.ops.object.modifier_apply(modifier="hole")
bpy.data.objects.remove(cutter)
box("WinBack", (WW + 0.01, 0.004, WHH + 0.01), (WX, fy + 0.05, WZ), "WindowBack")
for (sx, sz, lx, lz) in [(WW + 0.012, 0.006, WX, WZ + WHH / 2 + 0.002), (WW + 0.012, 0.006, WX, WZ - WHH / 2 - 0.002),
                         (0.006, WHH + 0.01, WX - WW / 2 - 0.002, WZ), (0.006, WHH + 0.01, WX + WW / 2 + 0.002, WZ)]:
    box("WinBezel", (sx, 0.009, sz), (lx, fy - 0.007, lz), "CornerMetal", bevel=0.0018)
for i in range(4):
    tube("tube%d" % i, WX - 0.06 + 0.04 * i, fy + 0.022, WZ - 0.002, height=0.062, r=0.0125)
box("WinGlass", (WW, 0.0015, WHH), (WX, fy - 0.004, WZ), "Glass")
box("NamePlate", (0.17, 0.005, 0.021), (WX, pf - 0.002, PZ - 0.04), "SteelPlate", bevel=0.002)
text("model_label", "MODERN 5050  ·  HIGH GAIN", 0.0078, (WX, pf - 0.005, PZ - 0.0405), "EngraveDark", FONT_TECH, extrude=0.0003)
marker("ShotPanel", (0, pf, PZ), w=W - 0.02, h=PH + 0.02)

# Lower half: hex vent with a violet glow behind and the X-shaped ToxicRig badge.
VZ = z0 + 0.065; VH = 0.09
box("VentBack", (W - 0.05, 0.004, VH), (0, fy + 0.02, VZ), "VentGlow")
box("VentFrame", (W - 0.03, 0.012, VH + 0.012), (0, fy + 0.004, VZ), "CornerMetal", bevel=0.003)
bpy.ops.mesh.primitive_plane_add(size=1, location=(0, fy - 0.003, VZ), rotation=(math.pi / 2, 0, 0))
g = bpy.context.active_object; g.name = "VentHex"; g.scale = (W - 0.045, VH - 0.004, 1); bpy.ops.object.transform_apply(scale=True)
g.data.materials.append(mat("HexMesh"))
# X badge: a plate with chamfered wings.
bm = bmesh.new()
pts = [(-0.16, 0.045), (-0.1, 0.045), (-0.075, 0.022), (0.075, 0.022), (0.1, 0.045), (0.16, 0.045),
       (0.11, 0.0), (0.16, -0.045), (0.1, -0.045), (0.075, -0.022), (-0.075, -0.022), (-0.1, -0.045), (-0.16, -0.045), (-0.11, 0.0)]
vs = [bm.verts.new((x, 0, z)) for x, z in pts]; f = bm.faces.new(vs)
r = bmesh.ops.extrude_face_region(bm, geom=[f]);
for v in [e for e in r["geom"] if isinstance(e, bmesh.types.BMVert)]: v.co.y -= 0.008
me = bpy.data.meshes.new("XBadge"); bm.to_mesh(me); bm.free()
xb = bpy.data.objects.new("XBadge", me); link(xb); xb.location = (0, fy - 0.004, VZ); me.materials.append(mat("SteelPlate"))
bpy.context.view_layer.objects.active = xb
mod = xb.modifiers.new("bev", "BEVEL"); mod.width = 0.0025; mod.segments = 3; mod.limit_method = "ANGLE"
text("XBadgeText", "ToxicRig", 0.03, (0, fy - 0.0125, VZ - 0.001), "EngraveDark", FONT_LOGO, extrude=0.0005)
for sx in (-1, 1):
    cyl("XScrew", 0.0024, 0.002, (sx * 0.135, fy - 0.0125, VZ), "Nickel", verts=16)

export("/home/claude/stage3d/public/models/modern5050.json",
       [c for c, _ in row1 + row2], ["channel", "bright"])
