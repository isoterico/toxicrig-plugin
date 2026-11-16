import sys; sys.path.insert(0, "/home/claude/amp3d")
from pedalkit import *
reset()
W, D, H = 0.15, 0.12, 0.052
enclosure(W, D, H, "MutantPaint")
z = H
# Black anodised top half with a chrome trim line
box("TopBlack", (W - 0.006, D * 0.46, 0.0008), (0, D * 0.25, z + 0.0002), "MutantBlack", bevel=0.0003)
box("Trim", (W - 0.006, 0.0016, 0.0009), (0, D * 0.02, z + 0.0003), "Chrome", bevel=0.0002)
# LFO tube in a window with a brushed bezel
box("WinBezel", (0.05, 0.05, 0.003), (0.042, 0.022, z + 0.0015), "AluBrushed", bevel=0.0012)
box("WinWell", (0.044, 0.044, 0.0032), (0.042, 0.022, z + 0.0018), "JackHole")
tube("tube0", 0.042, 0.022, z + 0.032, height=0.05, r=0.0105)
pedal_knob("mode", -0.04, 0.026, z, r=0.0125)
label("ml_chorus", "CHORUS", 0.0048, -0.04, 0.047, z + 0.0007, m="PointerWhite")
label("ml_flanger", "FLANGER", 0.0048, -0.008, 0.034, z + 0.0007, m="PointerWhite")
label("ml_vibrato", "VIBRATO", 0.0048, -0.066, 0.017, z + 0.0007, m="PointerWhite")
pedal_knob("speed", -0.04, -0.018, z, r=0.0125)
label("speed_label", "SPEED", 0.0042, -0.04, -0.037, z)
label("title", "MUTANT MOD", 0.0100, 0.036, -0.014, z, font=FONT_LABEL)
label("sub", "LFO TUBE", 0.0050, 0.036, -0.025, z)
footswitch(0.036, -0.048, z)
led(0.064, -0.048, z)
export("/home/claude/stage3d/public/models/mutant.json", [], [], ao=False)
