// Editor: a shared header (presets, A/B, oversampling, settings, input, tuner,
// output) above the view of the selected skin. Everything is laid out at a fixed
// design size (1280x780) and scaled as a whole: any window size stays sharp and
// keeps its proportions.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "ui/Widgets.h"
#include "ui/Classic.h"
#include "ui/Stage.h"
#include "../dsp/tz/Tuner.h"

namespace ui
{
class Header : public juce::Component
{
public:
    explicit Header (ToxicRigProcessor& p);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshPresets();
    void updateMeters (float in, float outL, float outR);
    void setTuner (int midi, double cents) { tuner.set (midi, cents); }
    std::function<void (int)> onSkin;
    std::function<void (float)> onScale;
private:
    void showSettings();
    ToxicRigProcessor& proc;
    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, saveBtn { "SALVA" }, aBtn { "A" }, bBtn { "B" };
    std::unique_ptr<juce::Button> gear;
    std::unique_ptr<ChoiceButton> osBox;
    std::unique_ptr<ParamKnob> inKnob, outKnob;
    LevelMeter inMeter, outMeter;
    VuMeter vuIn { "IN" }, vuOut { "OUT" };
    TunerDisplay tuner;
    juce::Array<juce::File> userPresets;
    std::unique_ptr<juce::FileChooser> chooser;
};

class Root : public juce::Component, private juce::Timer
{
public:
    explicit Root (ToxicRigProcessor& p);
    ~Root() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void setSkin (SkinId s);
    static constexpr int W = 1280, H = 780;
    // Test hooks (offline snapshots).
    StageView* stage() { return dynamic_cast<StageView*> (view.get()); }
    ClassicView* classic() { return dynamic_cast<ClassicView*> (view.get()); }
    std::function<void (float)> onScale;
private:
    void timerCallback() override;
    ToxicRigProcessor& proc;
    std::unique_ptr<SkinLookAndFeel> lnf;
    Header header;
    std::unique_ptr<juce::Component> view;
    SkinId skin = SkinId::ToxicLab;
    tz::PitchDetector detector;
    std::vector<float> tunerWindow;
    double lastTunerRate = 0;
    float inLevel = 0, outL = 0, outR = 0;
    int tick = 0;
};
} // namespace ui

class ToxicRigEditor : public juce::AudioProcessorEditor
{
public:
    explicit ToxicRigEditor (ToxicRigProcessor&);
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black); }
    void setScale (float s);
    ui::Root root;
private:
    juce::TooltipWindow tooltips { this, 600 };
};
