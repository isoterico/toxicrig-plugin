#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "WebEditor.h"

ToxicRigProcessor::ToxicRigProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ToxicRig", rig::createLayout())
{
    startTimerHz (10);
}

ToxicRigProcessor::~ToxicRigProcessor() { stopTimer(); }

bool ToxicRigProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    const bool inOk = in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
    const bool outOk = out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
    return inOk && outOk;
}

void ToxicRigProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    setLatencySamples (engine.getLatencySamples());
    currentRate = sampleRate;
    checkCabs (true); // IRs are generated for the current sample rate
}

void ToxicRigProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = *b;
    engine.process (buffer, getTotalNumInputChannels(), bpm);
}

void ToxicRigProcessor::checkCabs (bool force)
{
    if (currentRate <= 0) return;
    for (int s = 0; s < 2; ++s)
    {
        const int cab = (int) apvts.getRawParameterValue (rig::ampId (s == 0 ? 'a' : 'b', "cab"))->load();
        if (force || cab != loadedCab[(size_t) s])
        {
            engine.loadCabIR (s, cab, userIR[(size_t) s]);
            loadedCab[(size_t) s] = cab;
        }
    }
}

void ToxicRigProcessor::setUserIR (int slot, const juce::File& f)
{
    userIR[(size_t) slot] = f;
    if (auto* p = apvts.getParameter (rig::ampId (slot == 0 ? 'a' : 'b', "cab")))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) tz::CabType::User));
    engine.loadCabIR (slot, (int) tz::CabType::User, f);
    loadedCab[(size_t) slot] = (int) tz::CabType::User;
}

void ToxicRigProcessor::setRouting (const rig::Routing& r) { engine.setRouting (r); }

void ToxicRigProcessor::applyFactoryPreset (int index)
{
    const auto& presets = rig::factoryPresets();
    if (index < 0 || index >= (int) presets.size()) return;
    const auto& pr = presets[(size_t) index];
    // Everything back to default first, then the preset's values. Changes are
    // smoothed by the engine, and topology changes are crossfaded.
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (rp->paramID != rig::ids::inGain && rp->paramID != rig::ids::outVol && rp->paramID != rig::ids::oversampling)
                rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, value] : pr.values)
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    engine.setRouting (pr.routing);
    currentProgram = index;
}

void ToxicRigProcessor::setCurrentProgram (int index) { applyFactoryPreset (index); }

const juce::String ToxicRigProcessor::getProgramName (int index)
{
    const auto& presets = rig::factoryPresets();
    return index >= 0 && index < (int) presets.size() ? presets[(size_t) index].name : juce::String();
}

juce::ValueTree ToxicRigProcessor::captureState()
{
    auto state = apvts.copyState();
    state.setProperty ("routing", engine.getRouting().toString(), nullptr);
    state.setProperty ("irA", userIR[0].getFullPathName(), nullptr);
    state.setProperty ("irB", userIR[1].getFullPathName(), nullptr);
    state.setProperty ("version", JucePlugin_VersionString, nullptr);
    state.setProperty ("skin", skin.load(), nullptr);
    return state;
}

void ToxicRigProcessor::restoreState (const juce::ValueTree& tree)
{
    if (! tree.isValid() || ! tree.hasType (apvts.state.getType())) return;
    apvts.replaceState (tree.createCopy());
    engine.setRouting (rig::Routing::fromString (tree.getProperty ("routing").toString()));
    for (int s = 0; s < 2; ++s)
    {
        const auto path = tree.getProperty (s == 0 ? "irA" : "irB").toString();
        userIR[(size_t) s] = path.isNotEmpty() && juce::File::isAbsolutePath (path) ? juce::File (path) : juce::File();
    }
    loadedCab = { -1, -1 }; // the timer reloads IRs on the message thread
    if (tree.hasProperty ("skin")) skin.store ((int) tree.getProperty ("skin"));
}

void ToxicRigProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = captureState().createXml()) copyXmlToBinary (*xml, destData);
}

void ToxicRigProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        restoreState (juce::ValueTree::fromXml (*xml));
}

juce::File ToxicRigProcessor::userPresetFolder()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("ToxicZone").getChildFile ("ToxicRig").getChildFile ("Presets");
    dir.createDirectory();
    return dir;
}

bool ToxicRigProcessor::saveUserPreset (const juce::File& f)
{
    if (auto xml = captureState().createXml()) return xml->writeTo (f);
    return false;
}

bool ToxicRigProcessor::loadUserPreset (const juce::File& f)
{
    if (auto xml = juce::XmlDocument::parse (f))
    {
        // Keep input/output/oversampling: those belong to your setup, not the sound.
        const float in = apvts.getRawParameterValue (rig::ids::inGain)->load();
        const float out = apvts.getRawParameterValue (rig::ids::outVol)->load();
        restoreState (juce::ValueTree::fromXml (*xml));
        if (auto* p = apvts.getParameter (rig::ids::inGain)) p->setValueNotifyingHost (p->convertTo0to1 (in));
        if (auto* p = apvts.getParameter (rig::ids::outVol)) p->setValueNotifyingHost (p->convertTo0to1 (out));
        return true;
    }
    return false;
}

void ToxicRigProcessor::storeAB (int slot) { abState[(size_t) slot] = captureState(); }

void ToxicRigProcessor::recallAB (int slot)
{
    if (currentAB == slot) return;
    abState[(size_t) currentAB] = captureState(); // keep edits made on the side we're leaving
    currentAB = slot;
    if (abState[(size_t) slot].isValid())
    {
        // Apply through the parameters (not replaceState) so the host sees the
        // changes and the engine smooths them.
        const auto& tree = abState[(size_t) slot];
        for (auto* p : getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            {
                auto child = tree.getChildWithProperty ("id", rp->paramID);
                if (child.isValid()) rp->setValueNotifyingHost (rp->convertTo0to1 ((float) child.getProperty ("value")));
            }
        engine.setRouting (rig::Routing::fromString (tree.getProperty ("routing").toString()));
    }
    else
        abState[(size_t) slot] = captureState(); // first visit: B starts as a copy of A
}

juce::AudioProcessorEditor* ToxicRigProcessor::createEditor()
{
    // The 3D stage when its files are installed and the system web view works; otherwise the 2D editor.
   #if JUCE_WEB_BROWSER
    if (rig::canUseStageEditor()) return new rig::WebRigEditor (*this);
   #endif
    return new ToxicRigEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ToxicRigProcessor(); }
