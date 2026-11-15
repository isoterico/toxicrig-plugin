# ToxicRig "American Twin" 1x12 combo, black panel style with silver grille cloth.
# Real controls only: GAIN BASS MIDDLE TREBLE PRESENCE DEPTH SAG MASTER, BRIGHT (2), CHANNEL (3).
import sys, math
sys.path.insert(0, "/home/claude/amp3d")
from rig3d import *

reset()
W, H, D = 0.68, 0.50, 0.265
z0 = 0.016
fy = -D / 2
for x in (-W / 2 + 0.07, W / 2 - 0.07):
    for y in (-D / 2 + 0.05, D / 2 - 0.05):
        cyl("Foot", 0.016, 0.016, (x, y, z0 / 2), "Rubber", rot=(0, 0, 0), bevel=0.003)
box("Body", (W, D, H), (0, 0, z0 + H / 2), "Tolex", bevel=0.016)
corners(W, D, z0 + 0.004, z0 + H - 0.004, m="Chrome", s=0.03)
# Handle (leather strap with chrome end caps)
box("Handle", (0.22, 0.03, 0.012), (0, 0, z0 + H + 0.024), "Leather", bevel=0.005)
for x in (-0.13, 0.13):
    box("HandleCap", (0.032, 0.042, 0.024), (x, 0, z0 + H + 0.01), "Chrome", bevel=0.006)

# Control panel along the top of the front.
PZ = z0 + H - 0.05; PH = 0.066
box("PanelTrim", (W - 0.032, 0.008, PH + 0.008), (0, fy + 0.0015, PZ), "Chrome", bevel=0.002)
box("Panel", (W - 0.04, 0.008, PH), (0, fy - 0.002, PZ), "PanelBlack", bevel=0.0015)
pf = fy - 0.006
jack("input", -W / 2 + 0.045, PZ - 0.004, pf)
text("input_label", "INPUT", 0.0058, (-W / 2 + 0.045, pf - 0.0003, PZ + 0.016), "EngraveWhite")
toggle("bright", -W / 2 + 0.083, PZ - 0.004, pf, "BRIGHT", label_dz=0.02)
text("bright_onoff", "ON\n\nOFF", 0.0034, (-W / 2 + 0.096, pf - 0.0003, PZ - 0.004), "EngraveWhite")
ctl = [("gain", "VOLUME"), ("bass", "BASS"), ("mid", "MIDDLE"), ("treble", "TREBLE"),
       ("presence", "PRESENCE"), ("resonance", "DEPTH"), ("sag", "SAG"), ("master", "MASTER")]
x0, dx = -0.2, 0.05
for i, (cid, lab) in enumerate(ctl):
    x = x0 + dx * i + (0.012 if i >= 4 else 0); z = PZ - 0.006
    knob_skirted(cid, x, z, pf)
    knob_label_ring(x, z, pf, 0.0192, numbers=True, size=0.0036)
    text(cid + "_label", lab, 0.0054, (x, pf - 0.0003, z + 0.0275), "EngraveWhite")
toggle("channel", 0.235, PZ - 0.004, pf, "CHANNEL", label_dz=0.02)
text("ch_names", "LEAD\nCRUNCH\nCLEAN", 0.0032, (0.252, pf - 0.0003, PZ - 0.004), "EngraveWhite", align="LEFT")
jack("fxloop", 0.282, PZ - 0.004, pf)
text("loop_label", "FX LOOP", 0.0052, (0.282, pf - 0.0003, PZ + 0.016), "EngraveWhite")
cyl("Jewel", 0.0075, 0.009, (W / 2 - 0.034, pf - 0.006, PZ - 0.004), "Jewel", verts=32, bevel=0.0025)
cyl("JewelRing", 0.0105, 0.004, (W / 2 - 0.034, pf - 0.002, PZ - 0.004), "Chrome", verts=48, bevel=0.001)
marker("ShotPanel", (0, pf, PZ), w=W - 0.02, h=PH + 0.03)

# Grille: chrome-piped silver cloth over a baffle with one 12" speaker behind.
GZ = z0 + (H - PH - 0.02) / 2 + 0.006; GH = H - PH - 0.06
box("Baffle", (W - 0.04, 0.01, GH + 0.01), (0, fy + 0.03, GZ), "Baffle")
speaker("spk0", 0, GZ, fy + 0.024, 0.15)
bpy.ops.mesh.primitive_plane_add(size=1, location=(0, fy - 0.001, GZ), rotation=(math.pi / 2, 0, 0))
g = bpy.context.active_object; g.name = "SilverCloth"; g.scale = (W - 0.05, GH, 1); bpy.ops.object.transform_apply(scale=True)
g.data.materials.append(mat("SilverCloth"))
for (sx, sz, lx, lz) in [(W - 0.046, 0.004, 0, GZ + GH / 2), (W - 0.046, 0.004, 0, GZ - GH / 2),
                         (0.004, GH + 0.004, -(W - 0.05) / 2, GZ), (0.004, GH + 0.004, (W - 0.05) / 2, GZ)]:
    box("Piping", (sx, 0.006, sz), (lx, fy - 0.002, lz), "Chrome", bevel=0.0018)
text("Script", "American Twin", 0.05, (-0.2, fy - 0.006, GZ + GH / 2 - 0.06), "Chrome", FONT_SERIF, extrude=0.0025)
box("Badge", (0.1, 0.004, 0.03), (W / 2 - 0.1, fy - 0.004, GZ - GH / 2 + 0.035), "PanelBlack", bevel=0.003)
text("BadgeText", "ToxicRig", 0.017, (W / 2 - 0.1, fy - 0.0065, GZ - GH / 2 + 0.034), "Chrome", FONT_LOGO, extrude=0.0003)
marker("ShotCab", (0, fy, GZ), w=W - 0.04, h=GH)

export("/home/claude/stage3d/public/models/american.json", [c for c, _ in ctl], ["channel", "bright"])
