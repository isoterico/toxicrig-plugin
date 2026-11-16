# AcidClean "Vapor-30": brushed aluminium head with amethyst corners, glowing acrylic knobs,
# neon "Vapor" screen and purple tubes, on a matching 2x12.
# Real controls only:
#   row 1: INPUT, GAIN, BRIGHT, TREBLE          right of screen: DEPTH, PRESENCE
#   row 2: FX LOOP, VOLUME, MIDDLE, BASS, [VAPOR: AMOUNT, BLOOM], [D/R: MIX]
#   strip under the tubes: WIDTH, STANDBY, POWER (+ power jewel)
import sys, math
sys.path.insert(0, "/home/claude/amp3d")
from rig3d import *

reset()
top = cab_2x12(CW=0.72, CH=0.44, CD=0.30, wheels=False, tolex="AluBrushed", corner="CornerPurple", grille="HexMeshPurple",
               badge_mat="PurplePlate", badge_text="EngraveLight", frame="CornerPurple")
W, HH, HD = 0.72, 0.30, 0.26
z0 = top + 0.012
fy = -HD / 2
for x in (-W / 2 + 0.07, W / 2 - 0.07):
    for y in (-HD / 2 + 0.05, HD / 2 - 0.05):
        cyl("HeadFoot", 0.016, 0.014, (x, y, z0 - 0.007), "Rubber", rot=(0, 0, 0), bevel=0.002)
box("Head", (W, HD, HH), (0, 0, z0 + HH / 2), "AluVapor", bevel=0.012)
corners(W, HD, z0 + 0.004, z0 + HH - 0.004, m="CornerPurple", s=0.032)
box("Handle", (0.24, 0.034, 0.014), (0, 0, z0 + HH + 0.02), "Rubber", bevel=0.006)
for x in (-0.14, 0.14):
    box("HandleCap", (0.03, 0.046, 0.022), (x, 0, z0 + HH + 0.009), "CornerPurple", bevel=0.005)

# Front panel (upper 2/3): brushed alu inset with screws.
PZ = z0 + HH - 0.098; PH = 0.17
box("Panel", (W - 0.036, 0.008, PH), (0, fy - 0.002, PZ), "AluVapor", bevel=0.002)
pf = fy - 0.006
for sx in (-1, 1):
    for sz in (-1, 1):
        cyl("PanelScrew", 0.0026, 0.002, (sx * (W / 2 - 0.03), pf - 0.0005, PZ + sz * (PH / 2 - 0.008)), "Nickel", verts=16, bevel=0.0006)
text("model_label", "ACIDCLEAN VAPOR-30", 0.0125, (-W / 2 + 0.03, pf - 0.0003, PZ + 0.062), "EngraveLight", FONT_TECH, align="LEFT")
text("model_sub", "Ambient / Shoegaze\nCrystal Clean", 0.0078, (-W / 2 + 0.03, pf - 0.0003, PZ + 0.04), "EngraveLight", FONT_TECH, align="LEFT")

LX0, LDX = -0.268, 0.052
R1, R2 = PZ + 0.004, PZ - 0.048
def akn(cid, x, z, lab):
    knob_acrylic(cid, x, z, pf)
    text(cid + "_label", lab, 0.0066, (x, pf - 0.0003, z - 0.025), "EngraveLight", FONT_TECH)
jack("input", LX0, R1, pf); text("input_label", "INPUT", 0.0066, (LX0, pf - 0.0003, R1 - 0.025), "EngraveLight", FONT_TECH)
akn("gain", LX0 + LDX, R1, "GAIN")
toggle("bright", LX0 + 2 * LDX, R1, pf, None)
text("bright_label", "BRIGHT", 0.0066, (LX0 + 2 * LDX, pf - 0.0003, R1 - 0.025), "EngraveLight", FONT_TECH)
cyl("bright_led", 0.0022, 0.002, (LX0 + 2 * LDX, pf - 0.001, R1 + 0.02), "LedPurple", verts=16)
akn("treble", LX0 + 3 * LDX, R1, "TREBLE")
jack("fxloop", LX0, R2, pf); text("loop_label", "FX LOOP", 0.0066, (LX0, pf - 0.0003, R2 - 0.025), "EngraveLight", FONT_TECH)
akn("master", LX0 + LDX, R2, "VOLUME")
akn("mid", LX0 + 2 * LDX, R2, "MIDDLE")
akn("bass", LX0 + 3 * LDX, R2, "BASS")
akn("vapor", LX0 + 4 * LDX, R2, "DEPTH")
akn("bloom", LX0 + 5 * LDX, R2, "RESONANCE")
akn("amb", LX0 + 6 * LDX, R2, "PRESENCE")
# Section brackets (engraved lines with titles)
def bracket(x0, x1, z, title):
    box("br", (x1 - x0, 0.0004, 0.0009), ((x0 + x1) / 2, pf - 0.0003, z), "EngraveDark")
    for x in (x0, x1): box("br", (0.0009, 0.0004, 0.006), (x, pf - 0.0003, z - 0.003), "EngraveDark")
    box("brmask", (0.032, 0.0006, 0.004), ((x0 + x1) / 2, pf - 0.0005, z), "AluVapor")
    text("brt", title, 0.0064, ((x0 + x1) / 2, pf - 0.0008, z - 0.0003), "EngraveLight", FONT_TECH)
bracket(LX0 + 4 * LDX - 0.018, LX0 + 5 * LDX + 0.018, R2 + 0.026, "VAPOR")
bracket(LX0 + 6 * LDX - 0.016, LX0 + 6 * LDX + 0.016, R2 + 0.026, "D/R")
# Neon screen
SX, SZ = -0.02, PZ + 0.035
box("ScreenBezel", (0.15, 0.008, 0.09), (SX, pf - 0.002, SZ), "CornerPurple", bevel=0.004)
bpy.ops.mesh.primitive_plane_add(size=1, location=(SX, pf - 0.0065, SZ), rotation=(math.pi / 2, 0, 0))
s = bpy.context.active_object; s.name = "Screen"; s.scale = (0.136, 0.076, 1); bpy.ops.object.transform_apply(scale=True)
s.data.materials.append(mat("Screen")); s["keep_uv"] = 1
# Tube window (right)
WX, WZ, WW, WHH = 0.235, PZ + 0.022, 0.2, 0.11
cutter = box("WinCutter", (WW, 0.06, WHH), (WX, fy, WZ), "Baffle")
for t in ("Panel", "Head"):
    o = bpy.data.objects[t]; mod = o.modifiers.new("hole", "BOOLEAN"); mod.operation = "DIFFERENCE"; mod.object = cutter; mod.solver = "EXACT"
    bpy.context.view_layer.objects.active = o; bpy.ops.object.modifier_apply(modifier="hole")
bpy.data.objects.remove(cutter)
box("WinBack", (WW + 0.01, 0.004, WHH + 0.01), (WX, fy + 0.07, WZ), "WindowBackPurple")
box("WinTopLight", (WW - 0.02, 0.01, 0.004), (WX, fy + 0.03, WZ + WHH / 2 - 0.006), "TubeLightPurple")
for (sx, sz, lx, lz) in [(WW + 0.014, 0.008, WX, WZ + WHH / 2 + 0.003), (WW + 0.014, 0.008, WX, WZ - WHH / 2 - 0.003),
                         (0.008, WHH + 0.014, WX - WW / 2 - 0.003, WZ), (0.008, WHH + 0.014, WX + WW / 2 + 0.003, WZ)]:
    box("WinBezel", (sx, 0.01, sz), (lx, fy - 0.006, lz), "AluVapor", bevel=0.002)
for sx in (-1, 1):
    for sz in (-1, 1):
        cyl("WinScrew", 0.0024, 0.002, (WX + sx * (WW / 2 + 0.003), fy - 0.0115, WZ + sz * (WHH / 2 + 0.003)), "CornerPurple", verts=16)
for i in range(4):
    tube("tube%d" % i, WX - 0.072 + 0.048 * i, fy + 0.03, WZ - 0.004, height=0.085, r=0.0145)
box("WinGlass", (WW, 0.0015, WHH), (WX, fy - 0.003, WZ), "Glass")
text("win_label", "ACIDCLEAN VAPOR-30  -  Crystal Clear Tube Amplifier", 0.0058, (WX, pf - 0.0003, WZ - WHH / 2 - 0.012), "EngraveLight", FONT_TECH)
# Strip under the tubes: WIDTH, STANDBY, POWER, jewel
BZ = PZ - 0.058
akn("amb_width", WX - 0.07, BZ, "WIDTH")
toggle("standby", WX + 0.0, BZ, pf, None); text("sb_label", "STANDBY", 0.0062, (WX, pf - 0.0003, BZ - 0.025), "EngraveLight", FONT_TECH)
toggle("power", WX + 0.05, BZ, pf, None); text("pw_label", "POWER", 0.0062, (WX + 0.05, pf - 0.0003, BZ - 0.025), "EngraveLight", FONT_TECH)
cyl("Jewel", 0.0068, 0.008, (WX + 0.085, pf - 0.006, BZ), "JewelPurple", verts=32, bevel=0.002)
cyl("JewelRing", 0.0098, 0.004, (WX + 0.085, pf - 0.002, BZ), "Chrome", verts=48, bevel=0.001)
marker("ShotPanel", (0, pf, PZ), w=W - 0.01, h=PH + 0.02)

# Lower hex vent with the purple ToxicRig plate.
VZ = z0 + 0.04; VH = 0.06
box("VentBack", (W - 0.05, 0.004, VH), (0, fy + 0.02, VZ), "VentGlow")
box("VentFrame", (W - 0.03, 0.012, VH + 0.012), (0, fy + 0.004, VZ), "AluVapor", bevel=0.003)
bpy.ops.mesh.primitive_plane_add(size=1, location=(0, fy - 0.003, VZ), rotation=(math.pi / 2, 0, 0))
g = bpy.context.active_object; g.name = "VentHex"; g.scale = (W - 0.045, VH - 0.004, 1); bpy.ops.object.transform_apply(scale=True)
g.data.materials.append(mat("HexMeshPurple"))
box("VentPlate", (0.22, 0.006, 0.05), (0, fy - 0.007, VZ), "PurplePlate", bevel=0.004)
emblem("VentEmblem", (-0.072, fy - 0.0105, VZ + 0.001), 0.044, m="EmblemPlate")
text("VentText", "ToxicRig", 0.024, (0.02, fy - 0.0112, VZ - 0.0045), "EngraveLight", FONT_LOGO, extrude=0.0014)

export("/home/claude/stage3d/public/models/vapor30.json",
       ["gain", "treble", "master", "mid", "bass", "vapor", "bloom", "amb", "amb_width"], ["bright", "standby", "power"])
