// "Classic" layout used by the Studio Noir and Schematic skins: a signal-chain
// strip on top (click = edit, drag = reorder, LED = on/off) and a detail panel.
#pragma once
#include "Widgets.h"
#include "../PluginProcessor.h"

namespace ui
{
class ChainStrip : public juce::Component, private juce::Timer
{
public:
    explicit ChainStrip (ToxicRigProcessor& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    std::function<void (int)> onSelect;
    static constexpr int kAmp = 100, kCab = 101;
    int selected = kAmp;
private:
    void timerCallback() override { repaint(); }
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
    ParamToggle onButton;
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
        std::unique_ptr<ChoiceButton> model, channel;
        std::unique_ptr<ParamToggle> bright;
        std::vector<std::unique_ptr<CaptionKnob>> knobs;
    };
    ToxicRigProcessor& proc;
    std::array<Side, 2> sides;
    ParamToggle bOn;
    std::unique_ptr<CaptionKnob> levelA, levelB, mix, width;
    ScopeDisplay scope;
};

class MicPad : public juce::Component
{
public:
    MicPad (juce::AudioProcessorValueTreeState& s, char slot);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    juce::RangedAudioParameter* pos;
    juce::RangedAudioParameter* dist;
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
        std::unique_ptr<ParamToggle> on;
        std::unique_ptr<ChoiceButton> cab, mic;
        juce::TextButton load { "Carica IR..." };
        juce::Label file;
        std::unique_ptr<MicPad> pad;
    };
    ToxicRigProcessor& proc;
    std::array<Side, 2> sides;
    std::unique_ptr<juce::FileChooser> chooser;
};

class ClassicView : public juce::Component
{
public:
    explicit ClassicView (ToxicRigProcessor& p);
    void resized() override;
    void select (int id);
private:
    ToxicRigProcessor& proc;
    ChainStrip chain;
    std::vector<std::unique_ptr<PedalPanel>> pedalPanels;
    AmpPanel ampPanel;
    CabPanel cabPanel;
};
} // namespace ui
