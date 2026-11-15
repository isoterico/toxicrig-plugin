// Factory presets. Values are in real units (dB, 0..1, choice index); anything a
// preset does not mention goes back to its default, so presets never "inherit"
// leftovers from the previous sound.
#pragma once
#include "Routing.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace rig
{
struct FactoryPreset
{
    juce::String name;
    std::vector<std::pair<juce::String, float>> values;
    Routing routing = Routing::defaults();
};

const std::vector<FactoryPreset>& factoryPresets();
} // namespace rig
