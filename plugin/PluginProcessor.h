#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "RigEngine.h"
#include "Presets.h"

class ToxicRigProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    ToxicRigProcessor();
    ~ToxicRigProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override { return (int) rig::factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- UI helpers (message thread) ------------------------------------------
    void applyFactoryPreset (int index);
    void setRouting (const rig::Routing& r);
    rig::Routing getRouting() const { return engine.getRouting(); }
    void setUserIR (int slot, const juce::File& f);
    juce::File getUserIR (int slot) const { return userIR[(size_t) slot]; }
    bool saveUserPreset (const juce::File& f);
    bool loadUserPreset (const juce::File& f);
    static juce::File userPresetFolder();
    void storeAB (int slot);          // remember current sound in A or B
    void recallAB (int slot);         // switch to it
    int getSkin() const { return skin.load(); }   // -1 = machine default
    void setSkin (int s) { skin.store (s); }
    bool hasAB (int slot) const { return abState[(size_t) slot].isValid(); }
    int currentAB = 0;

    // Call to (re)load cab IRs if the cab choice changed. Normally driven by a timer;
    // offline tests call it directly.
    void checkCabs (bool force = false);

    juce::AudioProcessorValueTreeState apvts;
    rig::RigEngine engine { apvts };

private:
    void timerCallback() override { checkCabs(); }
    juce::ValueTree captureState();
    void restoreState (const juce::ValueTree& tree);

    std::array<int, 2> loadedCab { -1, -1 };
    std::array<juce::File, 2> userIR;
    std::array<juce::ValueTree, 2> abState;
    int currentProgram = 0;
    std::atomic<int> skin { -1 };
    double currentRate = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToxicRigProcessor)
};
