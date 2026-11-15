# ToxicRig "Overlord" - modelled from the concept image (proportions measured on it:
# 1 px = 0.589 mm, front face 1155 x 525 px). Head on a matching black 2x12.
import sys, math
sys.path.insert(0, "/home/claude/amp3d")
from rig3d import *

reset()
top = cab_2x12(CW=0.72, CH=0.44, CD=0.30, wheels=False, badge_mat="AntiqueBrass", badge_text="EngraveDark")
PX = 0.000589
W, HH, HD = 0.68, 0.31, 0.26
z0 = top + 0.012
fy = -HD / 2
zc = z0 + HH / 2                      # vertical centre of the front
X = lambda px: (px - 677.5) * PX       # image x -> metres
Z = lambda py: zc + (337.5 - py) * PX  # image y -> metres

for x in (-W / 2 + 0.07, W / 2 - 0.07):
    for y in (-HD / 2 + 0.05, HD / 2 - 0.05):
        cyl("HeadFoot", 0.018, 0.016, (x, y, z0 - 0.008), "Rubber", rot=(0, 0, 0), bevel=0.003)
# Shell: tolex box with a recessed front (the fascia sits 12 mm inside the tolex frame).
shell = box("Head", (W, HD, HH), (0, 0, zc), "Tolex", bevel=0.016, segments=4)
fasc = box("FasciaCut", (X(1225) - X(135), 0.05, Z(140) - Z(592)), (0, fy, (Z(140) + Z(592)) / 2), "Baffle")
mod = shell.modifiers.new("cut", "BOOLEAN"); mod.operation = "DIFFERENCE"; mod.object = fasc; mod.solver = "EXACT"
bpy.context.view_layer.objects.active = shell; bpy.ops.object.modifier_apply(modifier="cut")
bpy.data.objects.remove(fasc)
FY = fy + 0.012                       # fascia plane
box("FasciaBack", (X(1225) - X(135) + 0.004, 0.01, Z(140) - Z(592) + 0.004), (0, FY + 0.006, (Z(140) + Z(592)) / 2), "BlackMetal")
# Corner protectors (rounded) and handle.
for x in (-W / 2, W / 2):
    for z in (z0 + 0.006, z0 + HH - 0.006):
        for y in (-HD / 2, HD / 2):
            box("Corner", (0.034, 0.034, 0.034), (x * 0.993, y * 0.983, z), "CornerMetal", bevel=0.006, segments=3)
box("Handle", (0.25, 0.036, 0.016), (0, 0, z0 + HH + 0.022), "Leather", bevel=0.007, segments=4)
for x in (-0.145, 0.145):
    box("HandleCap", (0.034, 0.05, 0.024), (x, 0, z0 + HH + 0.01), "CornerMetal", bevel=0.007, segments=4)

# ---------------- upper section: hex grille across the whole width, tube window on the right
GX0, GX1, GZ0, GZ1 = X(140), X(1222), Z(415), Z(143)
box("GrilleFrame", (GX1 - GX0 + 0.012, 0.008, GZ1 - GZ0 + 0.012), ((GX0 + GX1) / 2, FY - 0.001, (GZ0 + GZ1) / 2), "CornerMetal", bevel=0.003)
gcut = box("GrilleCut", (GX1 - GX0, 0.05, GZ1 - GZ0), ((GX0 + GX1) / 2, FY, (GZ0 + GZ1) / 2), "Baffle")
o = bpy.data.objects["GrilleFrame"]; m2 = o.modifiers.new("cut", "BOOLEAN"); m2.operation = "DIFFERENCE"; m2.object = gcut; m2.solver = "EXACT"
bpy.context.view_layer.objects.active = o; bpy.ops.object.modifier_apply(modifier="cut"); bpy.data.objects.remove(gcut)
box("VentBack", (X(772) - GX0, 0.004, GZ1 - GZ0), ((GX0 + X(772)) / 2, FY + 0.03, (GZ0 + GZ1) / 2), "VentBackDark")
box("VentBack2", (GX1 - X(772), 0.004, Z(352) - GZ0), ((X(772) + GX1) / 2, FY + 0.03, (GZ0 + Z(352)) / 2), "VentBackDark")
bpy.ops.mesh.primitive_plane_add(size=1, location=((GX0 + GX1) / 2, FY - 0.003, (GZ0 + GZ1) / 2), rotation=(math.pi / 2, 0, 0))
g = bpy.context.active_object; g.name = "VentHex"; g.scale = (GX1 - GX0, GZ1 - GZ0, 1); bpy.ops.object.transform_apply(scale=True)
g.data.materials.append(mat("HexMeshDark"))
for (px, py) in ((150, 150), (1212, 150), (150, 405), (1212, 405)):
    cyl("GrilleScrew", 0.0026, 0.002, (X(px), FY - 0.006, Z(py)), "Nickel", verts=20, bevel=0.0006)
# Badge (brushed dark brass plate with engraved/raised art, painted by the engine)
BX, BZ, BW, BH = X(472), Z(277), 0.186, 0.068
box("BadgeBase", (BW + 0.004, 0.006, BH + 0.004), (BX, FY - 0.006, BZ), "BurnishedBrass", bevel=0.004, segments=3)
art_plane("Art_badge", (BX, FY - 0.0092, BZ), BW - 0.004, BH - 0.004)
for sx in (-1, 1):
    for sz in (-1, 1):
        cyl("BadgeScrew", 0.0021, 0.0018, (BX + sx * (BW / 2 - 0.007), FY - 0.0098, BZ + sz * (BH / 2 - 0.007)), "AntiqueBrass", verts=20, bevel=0.0005)
# Tube window
WX0, WX1, WZ0, WZ1 = X(772), X(1222), Z(352), Z(143)
WX, WZ, WW, WH = (WX0 + WX1) / 2, (WZ0 + WZ1) / 2, WX1 - WX0, WZ1 - WZ0
frame = box("WinFrame", (WW, 0.012, WH), (WX, FY - 0.004, WZ), "CornerMetal", bevel=0.006, segments=4)
wc = box("WinCut", (WW - 0.02, 0.06, WH - 0.02), (WX, FY, WZ), "Baffle")
deep = box("WinDeep", (WW - 0.02, 0.16, WH - 0.02), (WX, FY + 0.075, WZ), "Baffle")
for t in ("Head", "FasciaBack"):
    ob = bpy.data.objects[t]; mm = ob.modifiers.new("deep", "BOOLEAN"); mm.operation = "DIFFERENCE"; mm.object = deep; mm.solver = "EXACT"
    bpy.context.view_layer.objects.active = ob; bpy.ops.object.modifier_apply(modifier="deep")
bpy.data.objects.remove(deep)
for t in ("WinFrame", "VentHex"):
    ob = bpy.data.objects[t]; mm = ob.modifiers.new("cut", "BOOLEAN"); mm.operation = "DIFFERENCE"; mm.object = wc; mm.solver = "EXACT"
    bpy.context.view_layer.objects.active = ob; bpy.ops.object.modifier_apply(modifier="cut")
bpy.data.objects.remove(wc)
box("WinBack", (WW - 0.02, 0.004, WH - 0.02), (WX, FY + 0.09, WZ), "VentBackDark")
box("WinCeil", (WW - 0.02, 0.1, 0.004), (WX, FY + 0.04, WZ1 - 0.012), "BlackMetal")
for side in (-1, 1):
    box("WinSide", (0.004, 0.08, WH - 0.02), (WX + side * (WW / 2 - 0.012), FY + 0.035, WZ), "BlackMetal")
box("WinFloor", (WW - 0.02, 0.08, 0.004), (WX, FY + 0.035, WZ0 + 0.012), "BlackMetal")
box("WinLedBar", (WW - 0.05, 0.012, 0.005), (WX, FY + 0.012, WZ1 - 0.016), "BlackMetal", bevel=0.001)
box("WinLed", (WW - 0.055, 0.004, 0.0018), (WX, FY + 0.012, WZ1 - 0.0185), "TubeLightGreen")
for sx in (-1, 1):
    for sz in (-1, 1):
        cyl("WinScrew", 0.0026, 0.002, (WX + sx * (WW / 2 - 0.009), FY - 0.0105, WZ + sz * (WH / 2 - 0.009)), "Nickel", verts=20, bevel=0.0006)
for i, px in enumerate((855, 945, 1043, 1140)):
    power_tube("tube%d" % i, X(px), FY + 0.04, WZ0 + 0.011, h=0.074, r=0.0128)
box("WinGlass", (WW - 0.02, 0.0015, WH - 0.02), (WX, FY - 0.006, WZ), "Glass")
marker("WinLight", (WX, FY + 0.012, WZ1 - 0.02), w=WW - 0.06, h=0.01)

# ---------------- control panel: engraved antique bronze plate
PX0, PX1, PZ0, PZ1 = X(135), X(1225), Z(592), Z(425)
PW, PH, PCX, PCZ = PX1 - PX0, PZ1 - PZ0, (PX0 + PX1) / 2, (PZ0 + PZ1) / 2
box("PanelPlate", (PW, 0.006, PH), (PCX, FY - 0.002, PCZ), "PanelBronze", bevel=0.002)
pf = FY - 0.0052
KZ = Z(492)
knobs = [("master", "VOLUME", 262, "KnobBrass", "LedGreen"), ("gain", "GAIN", 333, "KnobBrass", "LedGreen"),
         ("attack", "ATTACK", 466, "KnobCopper", None), ("bass", "BASS", 541, "KnobDarkBronze", None),
         ("treble", "TREBLE", 620, "KnobBrass", None), ("presence", "PRESENCE", 705, "KnobCopper", None),
         ("resonance", "DEPTH", 787, "KnobSteel", "LedPurple")]
layout = {"w": PW, "h": PH, "cx": PCX, "cz": PCZ, "knobs": [], "toggles": [], "sliders": [], "jacks": [], "labels": []}
for cid, lab, px, m, led in knobs:
    knurl_knob(cid, X(px), KZ, pf, 0.0158, m, "KnobTopBrushed", led)
    layout["knobs"].append({"id": cid, "x": X(px) - PCX, "z": KZ - PCZ, "r": 0.0158, "label": lab})
toggle("tight", X(403), KZ, pf, None)
layout["toggles"].append({"id": "tight", "x": X(403) - PCX, "z": KZ - PCZ, "label": "TIGHT"})
jack("input", X(190), KZ, pf)
layout["jacks"].append({"x": X(190) - PCX, "z": KZ - PCZ, "top": "TOXICZONE", "bottom": "OVERLORD"})
EQX0, EQDX = X(885), 30 * PX
for b in range(8):
    slider("eq%d" % (b + 1), EQX0 + EQDX * b, Z(505), pf, travel=0.034, m="KnobBrass")
    layout["sliders"].append({"id": "eq%d" % (b + 1), "x": EQX0 + EQDX * b - PCX, "z": Z(505) - PCZ, "travel": 0.034})
cyl("Jewel", 0.0095, 0.012, (X(1162), pf - 0.007, Z(473)), "Jewel", verts=48, bevel=0.003)
cyl("JewelRing", 0.0125, 0.005, (X(1162), pf - 0.002, Z(473)), "Chrome", verts=48, bevel=0.0012)
jack("fxloop", X(1162), Z(536), pf)
layout["jacks"].append({"x": X(1162) - PCX, "z": Z(536) - PCZ, "top": "", "bottom": "FX LOOP"})
layout["jewel"] = {"x": X(1162) - PCX, "z": Z(473) - PCZ}
art_plane("Art_panel", (PCX, FY - 0.0051, PCZ), PW - 0.002, PH - 0.002, layout)
# remove the slider tick meshes from rig3d.slider (the engraving draws the scales)
for ob in [o for o in bpy.data.objects if o.name.startswith("tick")]: bpy.data.objects.remove(ob)
marker("ShotPanel", (0, pf, (Z(150) + Z(590)) / 2), w=W - 0.01, h=Z(140) - Z(595) + 0.02)

export("/home/claude/stage3d/public/models/overlord.json",
       ["master", "gain", "attack", "bass", "treble", "presence", "resonance"], ["tight"], ["eq%d" % (i + 1) for i in range(8)])
