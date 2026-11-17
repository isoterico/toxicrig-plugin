#include "WebEditor.h"
#include "Params.h"
#include "Presets.h"
#include "ui/Skin.h"

namespace rig
{
// ---------------------------------------------------------------------------- where the scene lives
juce::File stageArchive()
{
    // Developer override, then the folder the installers write to:
    //   Windows  C:\ProgramData\ToxicZone\ToxicRig\ToxicRig-Stage.zip
    //   macOS    /Library/Application Support/ToxicZone/ToxicRig/ToxicRig-Stage.zip
    //   Linux    /usr/share/ToxicZone/ToxicRig/ToxicRig-Stage.zip
    const auto env = juce::SystemStats::getEnvironmentVariable ("TOXICRIG_STAGE", {});
    if (env.isNotEmpty() && juce::File::isAbsolutePath (env)) return juce::File (env);
   #if JUCE_LINUX
    return juce::File ("/usr/share/ToxicZone/ToxicRig/ToxicRig-Stage.zip");
   #else
    return juce::File::getSpecialLocation (juce::File::commonApplicationDataDirectory)
               .getChildFile ("ToxicZone").getChildFile ("ToxicRig").getChildFile ("ToxicRig-Stage.zip");
   #endif
}

static juce::WebBrowserComponent::Options browserOptions()
{
    return juce::WebBrowserComponent::Options {}
       #if JUCE_WINDOWS
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)   // Edge/Chromium, WebGL 2 on the GPU
       #endif
        .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2 {}
                                     .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("ToxicRigWebView"))
                                     .withBackgroundColour (juce::Colour (0xff07070a)))
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden();
}

// Developers only: load the scene from a local web server instead of the archive (live reload of
// the scene while the engine runs). Ignored unless the variable is set.
static juce::String devStageUrl() { return juce::SystemStats::getEnvironmentVariable ("TOXICRIG_STAGE_URL", {}); }

bool canUseStageEditor()
{
    if (ui::settings().getBoolValue ("classicUi", false)) return false;
    if (devStageUrl().isEmpty() && ! stageArchive().existsAsFile()) return false;
    return juce::WebBrowserComponent::areOptionsSupported (browserOptions());
}

static const char* mimeFor (const juce::String& path)
{
    const auto ext = path.fromLastOccurrenceOf (".", false, false).toLowerCase();
    if (ext == "html") return "text/html";
    if (ext == "js") return "text/javascript";
    if (ext == "json") return "application/json";
    if (ext == "css") return "text/css";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "png") return "image/png";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "woff2") return "font/woff2";
    if (ext == "woff") return "font/woff";
    if (ext == "ttf") return "font/ttf";
    if (ext == "otf") return "font/otf";
    return "application/octet-stream";
}

// ---------------------------------------------------------------------------- editor
WebRigEditor::WebRigEditor (ToxicRigProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    zip = std::make_unique<juce::ZipFile> (stageArchive());

    for (auto* ap : proc.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap)) params.add (rp);
    dirty.reset (new std::atomic<bool>[(size_t) params.size()]);
    for (int i = 0; i < params.size(); ++i) { dirty[(size_t) i] = false; params[i]->addListener (this); }

    auto opts = browserOptions()
        .withResourceProvider ([this] (const juce::String& path) { return serve (path); })
        .withEventListener ("rig", [this] (const juce::var& v) { onMessage (v); });
    web = std::make_unique<juce::WebBrowserComponent> (opts);
    addAndMakeVisible (*web);
    web->goToURL (devStageUrl().isNotEmpty() ? devStageUrl() : juce::WebBrowserComponent::getResourceProviderRoot() + "index.html?plugin");

    setResizable (true, true);
    setResizeLimits (900, 540, 3840, 2160);
    setSize (ui::settings().getIntValue ("stageW", 1440), ui::settings().getIntValue ("stageH", 860));
    startTimerHz (30);
}

WebRigEditor::~WebRigEditor()
{
    stopTimer();
    for (auto* rp : params) rp->removeListener (this);
    ui::settings().setValue ("stageW", getWidth());
    ui::settings().setValue ("stageH", getHeight());
}

void WebRigEditor::paint (juce::Graphics& g) { g.fillAll (juce::Colour (0xff07070a)); }
void WebRigEditor::resized() { web->setBounds (getLocalBounds()); }

std::optional<WebRigEditor::Resource> WebRigEditor::serve (const juce::String& rawPath)
{
    auto path = rawPath.upToFirstOccurrenceOf ("?", false, false).trimCharactersAtStart ("/");
    if (path.isEmpty()) path = "index.html";
    path = juce::URL::removeEscapeChars (path);
    if (zip == nullptr) return std::nullopt;
    const int idx = zip->getIndexOfFileName (path, true);
    if (idx < 0) return std::nullopt;
    std::unique_ptr<juce::InputStream> in (zip->createStreamForEntry (idx));
    if (in == nullptr) return std::nullopt;
    Resource r;
    juce::MemoryBlock mb;
    in->readIntoMemoryBlock (mb);
    r.data.resize (mb.getSize());
    std::memcpy (r.data.data(), mb.getData(), mb.getSize());
    r.mimeType = mimeFor (path);
    if (juce::SystemStats::getEnvironmentVariable ("TOXICRIG_DEBUG", {}).isNotEmpty())
        std::fprintf (stderr, "[stage] %s %d bytes\n", path.toRawUTF8(), (int) r.data.size());
    return r;
}

void WebRigEditor::parameterValueChanged (int index, float)
{
    // Called on any thread (host automation can arrive on the audio thread): only flag it.
    for (int i = 0; i < params.size(); ++i)
        if (params[i]->getParameterIndex() == index) { dirty[(size_t) i] = true; anyDirty = true; return; }
}

// ---------------------------------------------------------------------------- engine -> scene
juce::var WebRigEditor::routingVar() const
{
    const auto r = proc.engine.getRouting();
    const auto& specs = pedalSpecs();
    juce::Array<juce::var> b, l;
    for (int i = 0; i < r.numBoard; ++i) b.add (juce::String (specs[(size_t) r.board[(size_t) i]].key));
    for (int i = 0; i < r.numLoop; ++i) l.add (juce::String (specs[(size_t) r.loop[(size_t) i]].key));
    auto* o = new juce::DynamicObject();
    o->setProperty ("board", b);
    o->setProperty ("loop", l);
    return juce::var (o);
}

void WebRigEditor::sendParams (bool all)
{
    auto* o = new juce::DynamicObject();
    int n = 0;
    for (int i = 0; i < params.size(); ++i)
        if (all || dirty[(size_t) i].exchange (false))
        {
            o->setProperty (params[i]->paramID, params[i]->getValue());
            ++n;
        }
    if (n == 0 && ! all) { delete o; return; }
    auto* m = new juce::DynamicObject();
    m->setProperty ("type", "params");
    m->setProperty ("values", juce::var (o));
    web->emitEventIfBrowserIsVisible ("engine", juce::var (m));
}

void WebRigEditor::sendState()
{
    for (int i = 0; i < params.size(); ++i) dirty[(size_t) i] = false;
    anyDirty = false;
    auto* m = new juce::DynamicObject();
    m->setProperty ("type", "state");
    auto* vals = new juce::DynamicObject();
    for (auto* rp : params) vals->setProperty (rp->paramID, rp->getValue());
    m->setProperty ("values", juce::var (vals));
    m->setProperty ("routing", routingVar());
    m->setProperty ("chain", proc.apvts.state.getProperty ("ui3dChain").toString());   // which pedals sit on the 3D board
    juce::Array<juce::var> names;
    for (const auto& fp : factoryPresets()) names.add (fp.name);
    for (const auto& f : ToxicRigProcessor::userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.trpreset"))
        names.add (f.getFileNameWithoutExtension() + " *");
    m->setProperty ("presets", names);
    m->setProperty ("preset", proc.getCurrentProgram());
    m->setProperty ("ab", proc.currentAB);
    m->setProperty ("version", JucePlugin_VersionString);
    lastRouting = proc.engine.getRouting().encode();
    lastAB = proc.currentAB;
    web->emitEventIfBrowserIsVisible ("engine", juce::var (m));
}

void WebRigEditor::timerCallback()
{
    if (! ready) return;
    if (proc.engine.getRouting().encode() != lastRouting || proc.currentAB != lastAB) { sendState(); return; }
    if (anyDirty.exchange (false)) sendParams (false);

    // meters (peak dBFS) and tuner, ~30 Hz
    auto& e = proc.engine;
    inLevel = std::max (inLevel * 0.85f, e.inPeak.exchange (0.0f));
    outL = std::max (outL * 0.85f, e.outPeakL.exchange (0.0f));
    outR = std::max (outR * 0.85f, e.outPeakR.exchange (0.0f));
    const double rate = e.tunerRate();
    if (tunerWindow.empty() || std::abs (rate - lastTunerRate) > 1.0)
    {
        detector.prepare (rate);
        tunerWindow.assign ((size_t) detector.windowSize(), 0.0f);
        lastTunerRate = rate;
    }
    {
        const int readyN = e.tunerFifo.getNumReady();
        int s1, n1, s2, n2;
        e.tunerFifo.prepareToRead (readyN, s1, n1, s2, n2);
        std::vector<float> fresh;
        fresh.insert (fresh.end(), e.tunerBuffer.begin() + s1, e.tunerBuffer.begin() + s1 + n1);
        fresh.insert (fresh.end(), e.tunerBuffer.begin() + s2, e.tunerBuffer.begin() + s2 + n2);
        e.tunerFifo.finishedRead (n1 + n2);
        const size_t Wn = tunerWindow.size();
        if (fresh.size() >= Wn) std::copy (fresh.end() - (long) Wn, fresh.end(), tunerWindow.begin());
        else if (! fresh.empty())
        {
            std::move (tunerWindow.begin() + (long) fresh.size(), tunerWindow.end(), tunerWindow.begin());
            std::copy (fresh.begin(), fresh.end(), tunerWindow.end() - (long) fresh.size());
        }
    }
    const auto db = [] (float x) { return juce::Decibels::gainToDecibels (x, -120.0f); };
    auto* m = new juce::DynamicObject();
    m->setProperty ("type", "meters");
    m->setProperty ("in", juce::Array<juce::var> { db (inLevel), db (inLevel) });
    m->setProperty ("out", juce::Array<juce::var> { db (outL), db (outR) });
    if (++tick % 3 == 0) m->setProperty ("hz", detector.detect (tunerWindow.data(), (int) tunerWindow.size()));
    web->emitEventIfBrowserIsVisible ("engine", juce::var (m));
}

// ---------------------------------------------------------------------------- scene -> engine
void WebRigEditor::setRoutingFrom (const juce::var& board, const juce::var& loop)
{
    // The scene gives the order of the pedals it shows; every pedal must still have a place,
    // so the others keep their current zone and relative order after them.
    const auto& specs = pedalSpecs();
    auto indexOf = [&] (const juce::String& key) {
        for (int p = 0; p < (int) specs.size(); ++p) if (key == specs[(size_t) p].key) return p;
        return -1; };
    const auto cur = proc.engine.getRouting();
    Routing r;
    std::array<bool, tz::kNumPedals> used {};
    auto put = [&] (int p, bool inLoop) {
        if (p < 0 || used[(size_t) p]) return;
        used[(size_t) p] = true;
        if (inLoop) r.loop[(size_t) r.numLoop++] = p; else r.board[(size_t) r.numBoard++] = p; };
    if (auto* a = board.getArray()) for (auto& k : *a) put (indexOf (k.toString()), false);
    if (auto* a = loop.getArray()) for (auto& k : *a) put (indexOf (k.toString()), true);
    for (int i = 0; i < cur.numBoard; ++i) put (cur.board[(size_t) i], false);
    for (int i = 0; i < cur.numLoop; ++i) put (cur.loop[(size_t) i], true);
    proc.setRouting (r);
    lastRouting = r.encode();
}

void WebRigEditor::onMessage (const juce::var& msg)
{
    const auto type = msg["type"].toString();
    if (type == "ready") { ready = true; sendState(); return; }
    if (type == "params")
    {
        if (auto* o = msg["values"].getDynamicObject())
            for (auto& kv : o->getProperties())
                if (auto* rp = proc.apvts.getParameter (kv.name.toString()))
                {
                    const float v = juce::jlimit (0.0f, 1.0f, (float) (double) kv.value);
                    if (std::abs (rp->getValue() - v) < 1.0e-5f) continue;
                    rp->beginChangeGesture();
                    rp->setValueNotifyingHost (v);
                    rp->endChangeGesture();
                }
        return;
    }
    if (type == "routing")
    {
        setRoutingFrom (msg["board"], msg["loop"]);
        proc.apvts.state.setProperty ("ui3dChain", msg["chain"].toString(), nullptr);
        return;
    }
    if (type == "preset")
    {
        const int i = (int) msg["index"], nf = (int) factoryPresets().size();
        if (i < nf) proc.setCurrentProgram (i);
        else
        {
            auto files = ToxicRigProcessor::userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.trpreset");
            if (i - nf < files.size()) proc.loadUserPreset (files[i - nf]);
        }
        proc.apvts.state.removeProperty ("ui3dChain", nullptr);   // a preset decides its own board
        sendState();
        return;
    }
    if (type == "savePreset")
    {
        const auto name = juce::File::createLegalFileName (msg["name"].toString().trim());
        if (name.isNotEmpty())
            proc.saveUserPreset (ToxicRigProcessor::userPresetFolder().getChildFile (name + ".trpreset"));
        sendState();
        return;
    }
    if (type == "ab") { proc.recallAB ((int) msg["slot"]); sendState(); return; }
    if (type == "abCopy") { proc.storeAB (1 - proc.currentAB); return; }
    if (type == "pickIR")
    {
        chooser = std::make_unique<juce::FileChooser> ("Carica una risposta all'impulso (IR)", juce::File(), "*.wav;*.aif;*.aiff");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc) {
                                  if (fc.getResult().existsAsFile()) { proc.setUserIR (0, fc.getResult()); sendState(); } });
        return;
    }
    if (type == "classicUi")
    {
        // Switch to the 2D editor: takes effect when the plugin window is reopened.
        ui::settings().setValue ("classicUi", true);
        return;
    }
}
} // namespace rig
