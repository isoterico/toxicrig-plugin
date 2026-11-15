# ToxicRig "Overlord": high-gain head (black, hex vent, green tubes, antique brass panel) on a black 2x12.
# Panel = only real controls: VOLUME GAIN TIGHT ATTACK BASS TREBLE PRESENCE DEPTH + 8-band MIDRANGE EQ.
import sys, math
sys.path.insert(0, "/home/claude/amp3d")
from rig3d import *

reset()
top = cab_2x12(CW=0.70, CH=0.44, CD=0.30, wheels=False, badge_mat="AntiqueBrass", badge_text="EngraveDark")
W, HH, HD = 0.68, 0.27, 0.26
z0 = top + 0.012
fy = -HD / 2
for x in (-W / 2 + 0.07, W / 2 - 0.07):
    for y in (-HD / 2 + 0.05, HD / 2 - 0.05):
        cyl("HeadFoot", 0.016, 0.012, (x, y, z0 - 0.006), "Rubber", rot=(0, 0, 0), bevel=0.002)
box("Head", (W, HD, HH), (0, 0, z0 + HH / 2), "Tolex", bevel=0.013)
corners(W, HD, z0 + 0.004, z0 + HH - 0.004, m="CornerMetal", s=0.03)
box("Handle", (0.22, 0.034, 0.014), (0, 0, z0 + HH + 0.02), "Rubber", bevel=0.006)
for x in (-0.13, 0.13):
    box("HandleCap", (0.03, 0.046, 0.022), (x, 0, z0 + HH + 0.009), "CornerMetal", bevel=0.005)

# ---------------- upper section: hex vent with the brass badge (left), tube window (right)
UZ = z0 + HH - 0.085; UH = 0.13
box("UpperFrame", (W - 0.03, 0.01, UH + 0.012), (0, fy + 0.002, UZ), "CornerMetal", bevel=0.003)
VX, VW = -0.115, 0.40
box("VentBack", (VW, 0.004, UH - 0.012), (VX, fy + 0.025, UZ), "VentGlowGreen")
bpy.ops.mesh.primitive_plane_add(size=1, location=(VX, fy - 0.004, UZ), rotation=(math.pi / 2, 0, 0))
g = bpy.context.active_object; g.name = "VentHex"; g.scale = (VW, UH - 0.012, 1); bpy.ops.object.transform_apply(scale=True)
g.data.materials.append(mat("HexMesh"))
box("BadgePlate", (0.2, 0.006, 0.07), (VX, fy - 0.008, UZ + 0.004), "AntiqueBrass", bevel=0.004)
box("BadgeInset", (0.188, 0.002, 0.058), (VX, fy - 0.0112, UZ + 0.004), "BurnishedBrass", bevel=0.002)
emblem("BadgeEmblem", (VX - 0.068, fy - 0.0125, UZ + 0.008), 0.046)
text("BadgeText", "ToxicRig", 0.034, (VX + 0.025, fy - 0.0125, UZ + 0.012), "AntiqueBrass", FONT_LOGO, extrude=0.0008)
text("BadgeSub", "~  OVERLORD  ~", 0.011, (VX + 0.025, fy - 0.0125, UZ - 0.017), "AntiqueBrass", FONT_TECH, extrude=0.0005)
for sx in (-1, 1):
    for sz in (-1, 1):
        cyl("BadgeScrew", 0.0022, 0.002, (VX + sx * 0.09, fy - 0.0115, UZ + 0.004 + sz * 0.028), "Nickel", verts=16)
# Tube window
WX, WW, WHH = 0.19, 0.24, 0.11
cutter = box("WinCutter", (WW, 0.06, WHH), (WX, fy, UZ), "Baffle")
for t in ("UpperFrame", "Head"):
    o = bpy.data.objects[t]; mod = o.modifiers.new("hole", "BOOLEAN"); mod.operation = "DIFFERENCE"; mod.object = cutter; mod.solver = "EXACT"
    bpy.context.view_layer.objects.active = o; bpy.ops.object.modifier_apply(modifier="hole")
bpy.data.objects.remove(cutter)
box("WinBack", (WW + 0.01, 0.004, WHH + 0.01), (WX, fy + 0.07, UZ), "WindowBack")
box("WinTopLight", (WW - 0.02, 0.01, 0.004), (WX, fy + 0.03, UZ + WHH / 2 - 0.006), "TubeLightGreen")
for (sx, sz, lx, lz) in [(WW + 0.014, 0.008, WX, UZ + WHH / 2 + 0.003), (WW + 0.014, 0.008, WX, UZ - WHH / 2 - 0.003),
                         (0.008, WHH + 0.014, WX - WW / 2 - 0.003, UZ), (0.008, WHH + 0.014, WX + WW / 2 + 0.003, UZ)]:
    box("WinBezel", (sx, 0.01, sz), (lx, fy - 0.006, lz), "CornerMetal", bevel=0.002)
for sx in (-1, 1):
    for sz in (-1, 1):
        cyl("WinScrew", 0.0024, 0.002, (WX + sx * (WW / 2 + 0.003), fy - 0.0115, UZ + sz * (WHH / 2 + 0.003)), "Nickel", verts=16)
for i in range(4):
    tube("tube%d" % i, WX - 0.084 + 0.056 * i, fy + 0.03, UZ - 0.006, height=0.085, r=0.0155)
box("WinGlass", (WW, 0.0015, WHH), (WX, fy - 0.003, UZ), "Glass")

# ---------------- control panel (lower strip): antique brass with an engraved double border
PZ = z0 + 0.052; PH = 0.074
box("PanelFrame", (W - 0.03, 0.01, PH + 0.012), (0, fy + 0.002, PZ), "CornerMetal", bevel=0.003)
box("Panel", (W - 0.042, 0.008, PH), (0, fy - 0.002, PZ), "PanelBronze", bevel=0.0015)
pf = fy - 0.006
for (sx, sz, lx, lz) in [(W - 0.052, 0.0012, 0, PZ + PH / 2 - 0.005), (W - 0.052, 0.0012, 0, PZ - PH / 2 + 0.005),
                         (0.0012, PH - 0.01, -(W - 0.052) / 2, PZ), (0.0012, PH - 0.01, (W - 0.052) / 2, PZ)]:
    box("PanelBorder", (sx, 0.0008, sz), (lx, pf - 0.0003, lz), "AntiqueBrass")
jack("input", -W / 2 + 0.048, PZ - 0.002, pf)
text("in_l1", "TOXICZONE", 0.0055, (-W / 2 + 0.048, pf - 0.0003, PZ + 0.022), "AntiqueBrass", FONT_TECH)
text("in_l2", "INPUT", 0.0055, (-W / 2 + 0.048, pf - 0.0003, PZ - 0.022), "AntiqueBrass", FONT_TECH)
knobs = [("master", "VOLUME", "KnobGold"), ("gain", "GAIN", "KnobGold"), None, ("attack", "ATTACK", "KnobCopper"),
         ("bass", "BASS", "KnobDarkCopper"), ("treble", "TREBLE", "KnobGold"), ("presence", "PRESENCE", "KnobCopper"), ("resonance", "DEPTH", "Nickel")]
x0, dx = -0.25, 0.047
for i, k in enumerate(knobs):
    x = x0 + dx * i
    if k is None:
        toggle("tight", x, PZ - 0.002, pf, "TIGHT", label_mat="AntiqueBrass", label_dz=0.024, size=0.0058)
        text("tight_l2", "TIGHT", 0.0055, (x, pf - 0.0003, PZ - 0.024), "AntiqueBrass", FONT_TECH)
        continue
    cid, lab, m = k
    knob_chicken(cid, x, PZ - 0.002, pf, m, m if m != "Nickel" else "Nickel", r=0.0112)
    cyl(cid + "_dot", 0.0013, 0.0006, (x, pf - 0.0234, PZ - 0.002 + 0.0068), "PointerGreen", verts=12)
    parent(bpy.data.objects[cid + "_dot"], bpy.data.objects[cid + "_cap"])
    text(cid + "_label", lab, 0.0058, (x, pf - 0.0003, PZ + 0.024), "AntiqueBrass", FONT_TECH)
    text(cid + "_label2", lab, 0.0052, (x, pf - 0.0003, PZ - 0.025), "AntiqueBrass", FONT_TECH)
    knob_label_ring(x, PZ - 0.002, pf, 0.0162, m="AntiqueBrass", numbers=False)
# Midrange EQ
EX0, EDX = 0.13, 0.0165
for b in range(8):
    slider("eq%d" % (b + 1), EX0 + EDX * b, PZ - 0.001, pf, travel=0.034, m="BrassKnob")
text("eq_title", "EQ", 0.006, (EX0 + EDX * 3.5, pf - 0.0003, PZ + 0.029), "AntiqueBrass", FONT_TECH)
text("eq_mid", "MIDRANGE", 0.0056, (EX0 + EDX * 3.5, pf - 0.0003, PZ - 0.029), "AntiqueBrass", FONT_TECH)
text("eq_lo", "150", 0.0042, (EX0 - 0.002, pf - 0.0003, PZ - 0.0255), "AntiqueBrass", FONT_TECH)
text("eq_hi", "4k", 0.0042, (EX0 + EDX * 7 + 0.002, pf - 0.0003, PZ - 0.0255), "AntiqueBrass", FONT_TECH)
text("eq_p", "+12", 0.0036, (EX0 - 0.011, pf - 0.0003, PZ + 0.017), "AntiqueBrass", FONT_TECH)
text("eq_0", "0", 0.0036, (EX0 - 0.011, pf - 0.0003, PZ - 0.001), "AntiqueBrass", FONT_TECH)
text("eq_m", "-12", 0.0036, (EX0 - 0.011, pf - 0.0003, PZ - 0.019), "AntiqueBrass", FONT_TECH)
cyl("Jewel", 0.0075, 0.009, (W / 2 - 0.042, pf - 0.006, PZ + 0.013), "Jewel", verts=32, bevel=0.0025)
cyl("JewelRing", 0.0105, 0.004, (W / 2 - 0.042, pf - 0.002, PZ + 0.013), "Chrome", verts=48, bevel=0.001)
jack("fxloop", W / 2 - 0.042, PZ - 0.016, pf)
text("loop_label", "FX LOOP", 0.0042, (W / 2 - 0.042, pf - 0.0003, PZ - 0.029), "AntiqueBrass", FONT_TECH)
marker("ShotPanel", (0, pf, PZ + 0.04), w=W - 0.01, h=PH + UH + 0.03)

export("/home/claude/stage3d/public/models/overlord.json",
       ["master", "gain", "attack", "bass", "treble", "presence", "resonance"], ["tight"])
