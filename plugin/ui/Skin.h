// Visual skins. All three draw the same controls bound to the same parameters:
// only the look changes, so switching skin never changes the sound.
#pragma once
#include <juce_gui_extra/juce_gui_extra.h>

namespace ui
{
enum class SkinId { ToxicLab = 0, StudioNoir = 1, Schematic = 2 };
inline const juce::StringArray skinNames { "Toxic Lab", "Studio Noir", "Schematic" };

struct Palette
{
    juce::Colour bg, panel, panel2, line, accent, accent2, text, dim, knobCap, knobTrack;
};

const Palette& palette (SkinId s);

// Embedded fonts (all SIL Open Font License).
enum class FontRole { Title, Label, Body, Mono, Serif };
juce::Font skinFont (SkinId s, FontRole role, float height);
juce::Font plainFont (float height, bool bold = false);

// Machine-wide settings (default skin and window size for new instances).
juce::PropertiesFile& settings();
} // namespace ui
