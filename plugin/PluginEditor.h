// Functional v1 interface: header (presets, A/B, oversampling, input/tuner/output),
// a signal-chain strip (click = edit, drag = reorder / move between board and loop,
// LED = on/off) and a detail panel for the selected block. Everything is drawn as
// vectors at a fixed design size and scaled as a whole, so any window size stays
// sharp and keeps its proportions.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "../dsp/tz/Tuner.h"

namespace ui
{
namespace col
{
    const juce::Colour bg { 0xff0b0b10 }, panel { 0xff15151d }, panel2 { 0xff1d1d28 }, line { 0xff2c2c3a };
    const juce::Colour green { 0xff39ff14 }, purple { 0xff8a2be2 }, text { 0xffe6e6ef }, dim { 0xff8a8a9c };
}
inline juce::Font font (float h, bool bold = false) { return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain)); }

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return font (13.0f); }
    juce::Font getPopupMenuFont() override { return font (14.0f); }
    juce::Font getLabelFont (juce::Label&) override { return font (12.0f); }
};

// Knob = slider + caption, bound to a parameter.
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, const juce::String& caption);
    void resized() override;
    void setCaption (const juce::String& c) { label.setText (c, juce::dontSendNotification); }
    juce::Slider slider;
    juce::Label label;
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class Meter : public juce::Component
{
public:
    void set (float l, float r) { lv = l; rv = r; repaint(); }
    void paint (juce::Graphics&) override;
private:
    float lv = 0, rv = 0;
};

class TunerDisplay : public juce::Component
{
public:
    void set (int midi, double cents) { note = midi; c = cents; repaint(); }
    void paint (juce::Graphics&) override;
private:
    int note = -1; double c = 0;
};

// The signal chain overview.
class ChainStrip : public juce::Component
{
public:
    explicit ChainStrip (ToxicRigProcessor& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    std::function<void (int)> onSelect; // pedal index, or kAmp / kCab
    static constexpr int kAmp = 100, kCab = 101;
    int selected = kAmp;
private:
    struct Item { int id; juce::Rectangle<float> r; };
    std::vector<Item> layoutItems() const;
    int hitTest (juce::Point<float>) const;
    ToxicRigProcessor& proc;
    int dragging = -1;
    juce::Point<float> dragPos;
    bool dragMoved = false;
};

class PedalPanel : public juce::Component
{
public:
    PedalPanel (ToxicRigProcessor& p, int pedalIndex);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    int pedal;
    std::vector<std::unique_ptr<juce::Component>> controls;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> btnAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> boxAtt;
    juce::ToggleButton onButton { "ON" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;
};

class AmpPanel : public juce::Component, private juce::Timer
{
public:
    explicit AmpPanel (ToxicRigProcessor& p);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    struct Side
    {
        juce::ComboBox model;
        juce::ComboBox channel;
        juce::ToggleButton bright { "Bright" };
        std::vector<std::unique_ptr<Knob>> knobs;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modelAtt, channelAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> brightAtt;
    };
    ToxicRigProcessor& proc;
    std::array<Side, 2> sides;
    juce::ToggleButton bOn { "Ampli B ON" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bOnAtt;
    std::unique_ptr<Knob> mix, width;
};

class MicPad : public juce::Component
{
public:
    MicPad (juce::AudioProcessorValueTreeState& s, char slot);
    ~MicPad() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { mouseDrag (e); }
    void mouseDrag (const juce::MouseEvent&) override;
private:
    juce::AudioProcessorValueTreeState& state;
    juce::RangedAudioParameter* pos; juce::RangedAudioParameter* dist;
    juce::ParameterAttachment posAtt, distAtt;
};

class CabPanel : public juce::Component
{
public:
    explicit CabPanel (ToxicRigProcessor& p);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    struct Side
    {
        juce::ToggleButton on { "Cassa attiva" };
        juce::ComboBox cab, mic;
        juce::TextButton load { "Carica IR..." };
        juce::Label file;
        std::unique_ptr<MicPad> pad;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cabAtt, micAtt;
    };
    ToxicRigProcessor& proc;
    std::array<Side, 2> sides;
    std::unique_ptr<juce::FileChooser> chooser;
};

class Root : public juce::Component, private juce::Timer
{
public:
    explicit Root (ToxicRigProcessor& p);
    ~Root() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    static constexpr int W = 1280, H = 780;
    void showForSnapshot (int id) { select (id); }
private:
    void timerCallback() override;
    void select (int id);
    void refreshPresetBox();
    ToxicRigProcessor& proc;
    LookAndFeel lnf;
    juce::ComboBox presetBox, osBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, saveBtn { "Salva" }, aBtn { "A" }, bBtn { "B" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osAtt;
    std::unique_ptr<Knob> inKnob, outKnob;
    Meter inMeter, outMeter;
    TunerDisplay tuner;
    ChainStrip chain;
    std::vector<std::unique_ptr<PedalPanel>> pedalPanels;
    AmpPanel ampPanel;
    CabPanel cabPanel;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Array<juce::File> userPresets;
    tz::PitchDetector detector;
    std::vector<float> tunerWindow;
    float inLevel = 0, outL = 0, outR = 0;
    double lastTunerRate = 0;
    int tick = 0;
};
} // namespace ui

class ToxicRigEditor : public juce::AudioProcessorEditor
{
public:
    explicit ToxicRigEditor (ToxicRigProcessor&);
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (ui::col::bg); }
private:
    ui::Root root;
};
