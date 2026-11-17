// 3D stage editor: the three.js scene runs in the platform web view (WebView2 on
// Windows, WKWebView on macOS, WebKitGTK on Linux) and talks to the processor through
// JUCE's native integration events. The audio engine stays the only source of truth:
// the scene only asks for parameter changes, and redraws whatever the engine reports.
#pragma once
#include "PluginProcessor.h"
#include "../dsp/tz/Tuner.h"
#include <juce_gui_extra/juce_gui_extra.h>

namespace rig
{
// Where the installer puts the scene (one zip with index.html, app.js, models, textures).
juce::File stageArchive();
// True if the 3D editor can open here: archive present, web view available, not disabled.
bool canUseStageEditor();
// Why the 3D stage is (not) used here, in plain words, for the settings menu of the 2D editor.
juce::String stageStatus();

#if JUCE_WEB_BROWSER   // builds without the web view (the offline test runner) use the 2D editor only
class WebRigEditor final : public juce::AudioProcessorEditor,
                           private juce::Timer,
                           private juce::AudioProcessorParameter::Listener
{
public:
    explicit WebRigEditor (ToxicRigProcessor&);
    ~WebRigEditor() override;
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    using Resource = juce::WebBrowserComponent::Resource;
    std::optional<Resource> serve (const juce::String& path);
    void onMessage (const juce::var& msg);
    void sendState();
    void sendParams (bool all);
    juce::var routingVar() const;
    void setRoutingFrom (const juce::var& board, const juce::var& loop);
    void timerCallback() override;
    void parameterValueChanged (int index, float) override;
    void parameterGestureChanged (int, bool) override {}

    ToxicRigProcessor& proc;
    std::unique_ptr<juce::ZipFile> zip;
    std::unique_ptr<juce::WebBrowserComponent> web;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Array<juce::RangedAudioParameter*> params;
    std::unique_ptr<std::atomic<bool>[]> dirty;     // set from any thread, read on the message thread
    std::atomic<bool> anyDirty { false };
    bool ready = false;
    uint64_t lastRouting = 0;
    int lastAB = -1;

    // meters and tuner (same processing as the 2D header)
    float inLevel = 0, outL = 0, outR = 0;
    tz::PitchDetector detector;
    std::vector<float> tunerWindow;
    double lastTunerRate = 0;
    int tick = 0;
};
#endif
} // namespace rig
