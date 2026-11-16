import sys; sys.path.insert(0, "/home/claude/amp3d")
from pedalkit import *
reset()
W, D, H = 0.12, 0.125, 0.052
enclosure(W, D, H, "NukePaint")
z = H
label("title", "NUCLEAR FUZZ", 0.0105, 0, 0.040, z, font=FONT_LABEL)
for cid, x, lab in [("drive", -0.034, "DRIVE"), ("gate", 0.0, "GATE"), ("volume", 0.034, "VOLUME")]:
    pedal_knob(cid, x, 0.012, z, r=0.0112)
    label(cid + "_label", lab, 0.0052, x, -0.0075, z)
label("noise_label", "NOISE REDUCTION", 0.0029, 0.0, -0.0135, z)
footswitch(0, -0.038, z)
led(0.042, -0.038, z)
label("sub", "GATED INDUSTRIAL", 0.0058, 0, -0.054, z)
export("/home/claude/stage3d/public/models/nfuzz.json", [], [], ao=False)
