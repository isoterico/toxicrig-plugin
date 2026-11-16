import sys; sys.path.insert(0, "/home/claude/amp3d")
from pedalkit import *
reset()
W, D, H = 0.046, 0.1, 0.048
enclosure(W, D, H, "BioPaint")
z = H
bpy.ops.mesh.primitive_plane_add(size=0.013, location=(0, 0.031, z + 0.0004))
e = bpy.context.active_object; e.name = "BioEmblem"; e.data.materials.append(mat("EmblemDark")); e["keep_uv"] = 1
label("title", "BIO-BOOST", 0.0052, 0, -0.0465, z, m="EngraveDark")
pedal_knob("boost", 0, 0.004, z, r=0.0105, h=0.015)
label("boost_label", "BOOST", 0.0040, 0, -0.0105, z, m="EngraveDark")
pedal_knob("mode", 0, -0.020, z, r=0.0055, h=0.008)
label("clean_label", "CLEAN", 0.0034, -0.0125, -0.020, z, m="EngraveDark")
label("treble_label", "TREBLE", 0.0034, 0.0130, -0.020, z, m="EngraveDark")
footswitch(0, -0.037, z)
led(0.0, 0.041, z)
export("/home/claude/stage3d/public/models/bioboost.json", [], [], ao=False)
