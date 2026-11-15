// Parameter definitions. Every pedal type exists exactly once, so its knobs are
// fixed, meaningfully-named host parameters ("Delay Time", not "Slot 7 Param 1"):
// automation in FL Studio stays readable and survives reordering the pedals.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../dsp/tz/Pedals.h"

namespace rig
{
struct PedalParamSpec
{
    const char* id;
    const char* name;
    float min, max, def;
    float centre;                    // skew centre (0 = linear)
    const char* unit;
    juce::StringArray choices = {};  // non-empty = choice parameter
    bool isBool = false;
};

struct PedalSpec
{
    tz::PedalId pedal;
    const char* key;          // id prefix
    const char* displayName;  // shown on the pedal
    const char* description;  // one line, Italian: what it does and why
    std::vector<PedalParamSpec> params;
};

const std::vector<PedalSpec>& pedalSpecs();

inline juce::String pedalOnId (int pedal) { return juce::String (pedalSpecs()[(size_t) pedal].key) + "_on"; }
inline juce::String pedalParamId (int pedal, int index)
{
    const auto& s = pedalSpecs()[(size_t) pedal];
    return juce::String (s.key) + "_" + s.params[(size_t) index].id;
}

// Amp/cab parameter id for slot 'a' or 'b'.
inline juce::String ampId (char slot, const char* name) { return juce::String::charToString (slot) + "_" + name; }

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

namespace ids
{
    inline const juce::String inGain = "in_gain", outVol = "out_vol", oversampling = "os",
                              ampBOn = "amp_b_on", abMix = "ab_mix", width = "width";
}

inline const juce::StringArray ampModels { "British", "American", "Modern" };
inline const juce::StringArray ampChannels { "Clean", "Crunch", "Lead" };
inline const juce::StringArray cabNames { "British 2x12", "American 1x12", "Modern 4x12", "IR utente" };
inline const juce::StringArray micNames { "Dinamico", "Nastro", "Condensatore" };
inline const juce::StringArray osNames { "2x", "4x", "8x" };
} // namespace rig
