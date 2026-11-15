// The whole signal graph, independent of the plugin wrapper (so offline tests and
// future plugins can drive it directly).
//
//  in -> input gain/meter/tuner tap -> [UP xN] board pedals -> amp A | amp B [DOWN]
//     -> cab A | cab B (convolution + virtual mic) -> A/B mix & stereo width
//     -> FX loop pedals (stereo) -> output volume -> safety
#pragma once
#include <juce_dsp/juce_dsp.h>
#include "../dsp/tz/Amp.h"
#include "../dsp/tz/Cab.h"
#include "../dsp/tz/Pedals.h"
#include "Params.h"
#include "Routing.h"

namespace rig
{
class RigEngine
{
public:
    explicit RigEngine (juce::AudioProcessorValueTreeState& state);

    void prepare (double sampleRate, int maxBlockSize);
    void process (juce::AudioBuffer<float>& buffer, int numInputChannels, double bpm);
    void reset();

    // Constant reported latency (all oversampling factors padded to the largest),
    // so the host's delay compensation never has to change while you play.
    int getLatencySamples() const { return reportedLatency; }

    // --- thread-safe UI hooks -----------------------------------------------
    void setRouting (const Routing& r) { routingCode.store (r.encode()); }
    Routing getRouting() const { return Routing::decode (routingCode.load()); }
    // Message thread only: (re)load cab IRs. Called on cab/IR/sample-rate change.
    void loadCabIR (int slot, int cabType, const juce::File& userFile);

    std::atomic<float> inPeak { 0 }, outPeakL { 0 }, outPeakR { 0 };
    juce::AbstractFifo tunerFifo { 1 << 15 };
    std::vector<float> tunerBuffer = std::vector<float> (1 << 15);
    double tunerRate() const { return baseRate / tunerDecim; }

    static constexpr double kInputVolts = 1.5; // 0 dBFS at the interface = 1.5 V peak at the "guitar jack"

private:
    struct Values; // per-block parameter snapshot
    void readParams (Values& v) const;
    void applyRateToBoard (double osRate);
    void processOversampled (juce::dsp::AudioBlock<float>& block, const Values& v);

    juce::AudioProcessorValueTreeState& apvts;

    // Cached parameter pointers (atomic reads on the audio thread).
    std::atomic<float>* pInGain {}, * pOutVol {}, * pOs {}, * pAmpBOn {}, * pAbMix {}, * pWidth {};
    struct SlotParams { std::atomic<float>* model, * channel, * gain, * bass, * mid, * treble, * presence, * resonance,
                                             * master, * sag, * bright, * level, * cabOn, * cab, * mic, * micPos, * micDist; };
    std::array<SlotParams, 2> slotParams {};
    std::array<std::atomic<float>*, tz::kNumPedals> pedalOn {};
    std::array<std::array<std::atomic<float>*, tz::Pedal::kMaxParams>, tz::kNumPedals> pedalParams {};

    std::array<std::unique_ptr<tz::Pedal>, tz::kNumPedals> pedals;
    std::array<tz::Smoother, tz::kNumPedals> pedalMix;  // click-free bypass
    std::array<bool, tz::kNumPedals> pedalActive {};
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 3> oversamplers;
    std::array<int, 3> osLatency {};
    std::array<tz::AmpSlot, 2> amps;
    std::array<juce::dsp::Convolution, 2> cabs;
    std::array<tz::MicFilter, 2> mics;
    std::array<tz::Smoother, 2> ampLevel;
    tz::Smoother inGain, outGain, abMix, width, ampBMix;
    tz::DelayLine latencyPadL, latencyPadR;

    juce::AudioBuffer<float> monoScratch, osScratch, dryScratch;
    juce::HeapBlock<float> pedalScratch;

    // Topology changes (routing, oversampling, cab on/off) fade out -> switch -> fade in.
    std::atomic<uint64_t> routingCode { Routing::defaults().encode() };
    uint64_t activeTopology = 0;
    Routing activeRouting = Routing::defaults();
    int activeOs = 1;
    std::array<bool, 2> activeCabOn { true, true };
    double topoGain = 1.0, topoStep = 0.01;
    int topoState = 0; // 0 steady, 1 fading out, 2 fading in
    int topoHold = 0;  // silent samples before fading in

    double baseRate = 48000;
    int maxBlock = 512, reportedLatency = 0;
    int tunerDecim = 4, tunerCount = 0;
    double tunerLp1 = 0, tunerLp2 = 0, tunerAcc = 0;
    tz::Biquad dcIn;
};
} // namespace rig
