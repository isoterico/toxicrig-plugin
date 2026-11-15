// Controls shared by every skin. Each one is bound to a host parameter through a
// JUCE attachment, so automation, presets, A/B and the UI always agree.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Skin.h"

namespace ui
{
// Knob drawing styles (set with ParamKnob::setStyle).
enum class KnobStyle { Panel, AmpRound, ChickenHead, Pedal, PedalBig };

class SkinLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit SkinLookAndFeel (SkinId s);
    SkinId skin;
    const Palette& pal() const { return palette (skin); }

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    void drawBubble (juce::Graphics&, juce::BubbleComponent&, const juce::Point<float>& tip, const juce::Rectangle<float>& body) override;
    void drawTooltip (juce::Graphics&, const juce::String& text, int width, int height) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return skinFont (skin, FontRole::Body, 15.0f); }
    juce::Font getPopupMenuFont() override { return skinFont (skin, FontRole::Body, 16.0f); }
    juce::Font getLabelFont (juce::Label&) override { return skinFont (skin, FontRole::Body, 14.0f); }
    juce::Font getSliderPopupFont (juce::Slider&) override { return skinFont (skin, FontRole::Label, 15.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return skinFont (skin, FontRole::Label, std::min (17.0f, h * 0.55f)); }
    int getSliderPopupPlacement (juce::Slider&) override { return juce::BubbleComponent::above; }
};

class ParamKnob : public juce::Slider
{
public:
    ParamKnob (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, KnobStyle style, bool textBox = false);
    KnobStyle style;
    juce::String paramId;
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

// Knob + caption (used by the classic panels).
class CaptionKnob : public juce::Component
{
public:
    CaptionKnob (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, const juce::String& caption, KnobStyle style = KnobStyle::Panel);
    void resized() override;
    void paint (juce::Graphics&) override;
    void setCaption (const juce::String& c) { caption = c; repaint(); }
    ParamKnob knob;
private:
    juce::String caption;
};

// On/off switch bound to a bool parameter. Look: LED button, bat toggle or text.
class ParamToggle : public juce::ToggleButton
{
public:
    enum class Look { Led, Bat, Pill };
    ParamToggle (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, const juce::String& text, Look look = Look::Led);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    Look look;
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
};

// Click to choose among a choice parameter's options (popup menu).
class ChoiceButton : public juce::Component, public juce::SettableTooltipClient
{
public:
    ChoiceButton (juce::AudioProcessorValueTreeState& s, const juce::String& paramId);
    ~ChoiceButton() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    std::function<juce::String (int)> labelFor; // optional custom label
    bool compact = false;
private:
    juce::RangedAudioParameter* param;
    std::unique_ptr<juce::ParameterAttachment> att;
    int index = 0;
};

class LevelMeter : public juce::Component
{
public:
    void set (float l, float r) { lv = l; rv = r; repaint(); }
    void paint (juce::Graphics&) override;
private:
    float lv = 0, rv = 0;
};

// Analog VU meter (Studio Noir): needle with ballistics computed by the caller.
class VuMeter : public juce::Component
{
public:
    explicit VuMeter (juce::String l) : label (std::move (l)) {}
    void set (float level);
    void paint (juce::Graphics&) override;
private:
    juce::String label;
    float needle = 0;
};

class TunerDisplay : public juce::Component
{
public:
    void set (int midi, double cents) { note = midi; c = cents; repaint(); }
    void paint (juce::Graphics&) override;
private:
    int note = -1; double c = 0;
};

class ScopeDisplay : public juce::Component
{
public:
    void set (const float* data, int n);
    void paint (juce::Graphics&) override;
private:
    std::vector<float> pts;
};

SkinId skinOf (const juce::Component& c);
} // namespace ui
