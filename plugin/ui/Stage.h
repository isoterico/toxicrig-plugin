// "Toxic Lab" skin: the rig as a stage. The amp you choose stands on the floor and
// its own knobs are the controls (every knob drawn is a real control, nothing
// fake); the pedals sit on the floor in signal order, wired with cables; click a
// pedal to open it big, drag it to reorder, stomp its footswitch to turn it on.
#pragma once
#include "Widgets.h"
#include "../PluginProcessor.h"

namespace ui
{
class AmpStage : public juce::Component, private juce::Timer
{
public:
    explicit AmpStage (ToxicRigProcessor& p);
    void setSlot (int s);
    int getSlot() const { return slot; }
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    int model() const;
    juce::Rectangle<float> imageArea() const;
    ToxicRigProcessor& proc;
    int slot = 0, lastModel = -1;
    std::array<std::array<std::unique_ptr<ParamKnob>, 8>, 2> knobs;
    std::array<std::unique_ptr<ParamToggle>, 2> bright;
    std::array<juce::Image, 3> images;
};

class AmpSide : public juce::Component, private juce::Timer
{
public:
    AmpSide (ToxicRigProcessor& p, AmpStage& stage);
    void paint (juce::Graphics&) override;
    void resized() override;
    std::function<void()> onOpenCab;
private:
    void timerCallback() override;
    void bindSlot();
    ToxicRigProcessor& proc;
    AmpStage& stage;
    juce::TextButton tabA { "AMPLI A" }, tabB { "AMPLI B" }, prev { "<" }, next { ">" }, cabBtn { "CASSA E MICROFONO" };
    std::array<juce::TextButton, 3> channels;
    ParamToggle bOn;
    std::unique_ptr<CaptionKnob> levelA, levelB, mix, width;
    std::unique_ptr<juce::ParameterAttachment> modelAtt, channelAtt;
    int modelIdx = 0, channelIdx = 0;
};

class PedalView : public juce::Component, public juce::SettableTooltipClient
{
public:
    PedalView (ToxicRigProcessor& p, int pedal, float scale);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    static int baseWidth (int pedal);
    static constexpr int kBaseHeight = 150;
    const int pedal;
    std::function<void (int)> onOpen;
    std::function<void (int, const juce::MouseEvent&)> onDrag, onDrop;
private:
    juce::Rectangle<float> footswitch() const;
    bool isOn() const;
    ToxicRigProcessor& proc;
    float k;
    bool dragging = false;
    std::vector<std::unique_ptr<juce::Component>> controls;
    std::vector<std::pair<juce::String, juce::Point<float>>> captions;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAtts;
};

class FloorView : public juce::Component, private juce::Timer
{
public:
    explicit FloorView (ToxicRigProcessor& p);
    void paint (juce::Graphics&) override;
    void resized() override { layout (false); }
    std::function<void (int)> onOpenPedal;
private:
    void timerCallback() override;
    void layout (bool animate);
    juce::Rectangle<int> ampSlot;
    ToxicRigProcessor& proc;
    std::vector<std::unique_ptr<PedalView>> pedals;
    uint64_t lastRouting = 0;
    int draggingPedal = -1;
};

class Overlay : public juce::Component
{
public:
    Overlay();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void resized() override;
    juce::Rectangle<int> card;
    std::function<void()> onClose;
protected:
    juce::TextButton close { "X" };
};

class PedalOverlay : public Overlay
{
public:
    PedalOverlay (ToxicRigProcessor& p, int pedal);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    int pedal;
    PedalView big;
    ParamToggle onBtn;
};

class CabOverlay : public Overlay, private juce::Timer
{
public:
    explicit CabOverlay (ToxicRigProcessor& p, int slot);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    void timerCallback() override { repaint(); }
    void bind (int slot);
    juce::Rectangle<float> cone() const;
    ToxicRigProcessor& proc;
    int slot = 0;
    juce::TextButton tabA { "CASSA A" }, tabB { "CASSA B" }, load { "CARICA LA TUA IR (.wav)" };
    std::array<juce::TextButton, 4> cabBtns;
    std::array<juce::ImageButton, 3> micBtns;
    std::unique_ptr<ParamToggle> onBtn;
    std::unique_ptr<juce::Slider> distSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> distAtt;
    std::unique_ptr<juce::ParameterAttachment> cabAtt, micAtt, posAtt, dAtt;
    int cabIdx = 0, micIdx = 0;
    float pos = 0.3f, dist = 0.2f;
    bool draggingMic = false;
    juce::Image speaker;
    std::array<juce::Image, 3> micImgs;
    std::unique_ptr<juce::FileChooser> chooser;
};

class StageView : public juce::Component
{
public:
    explicit StageView (ToxicRigProcessor& p);
    void paint (juce::Graphics&) override;
    void resized() override;
    void openPedal (int pedal);
    void openCab();
    void closeOverlay();
private:
    ToxicRigProcessor& proc;
    juce::Image stageImage;
    AmpStage amp;
    AmpSide side;
    FloorView floor;
    std::unique_ptr<Overlay> overlay;
};
} // namespace ui
