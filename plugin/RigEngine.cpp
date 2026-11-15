#include "RigEngine.h"

namespace rig
{
struct RigEngine::Values
{
    float inGainDb = 0, outVolDb = 0, abMix = 0.5f, width = 0.7f;
    int os = 1;
    bool ampBOn = false;
    struct Slot
    {
        tz::AmpVoicing model {}; tz::AmpChannel channel {}; tz::AmpKnobs knobs; float levelDb = 0;
        bool cabOn = true; int cab = 0; tz::MicType mic {}; float micPos = 0.3f, micDist = 0.2f;
    };
    std::array<Slot, 2> slot;
    std::array<bool, tz::kNumPedals> on {};
    std::array<std::array<float, tz::Pedal::kMaxParams>, tz::kNumPedals> p {};
};

static int osFactor (int index) { return 1 << (index + 1); }

RigEngine::RigEngine (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    auto get = [&] (const juce::String& id)
    {
        auto* p = apvts.getRawParameterValue (id);
        jassert (p != nullptr);
        return p;
    };
    pInGain = get (ids::inGain); pOutVol = get (ids::outVol); pOs = get (ids::oversampling);
    pAmpBOn = get (ids::ampBOn); pAbMix = get (ids::abMix); pWidth = get (ids::width);
    for (int s = 0; s < 2; ++s)
    {
        const char c = s == 0 ? 'a' : 'b';
        auto& sp = slotParams[(size_t) s];
        sp.model = get (ampId (c, "model")); sp.channel = get (ampId (c, "channel"));
        sp.gain = get (ampId (c, "gain")); sp.bass = get (ampId (c, "bass")); sp.mid = get (ampId (c, "mid"));
        sp.treble = get (ampId (c, "treble")); sp.presence = get (ampId (c, "presence"));
        sp.resonance = get (ampId (c, "resonance")); sp.master = get (ampId (c, "master")); sp.sag = get (ampId (c, "sag"));
        sp.bright = get (ampId (c, "bright")); sp.level = get (ampId (c, "level"));
        sp.cabOn = get (ampId (c, "cab_on")); sp.cab = get (ampId (c, "cab")); sp.mic = get (ampId (c, "mic"));
        sp.micPos = get (ampId (c, "mic_pos")); sp.micDist = get (ampId (c, "mic_dist"));
    }
    const auto& specs = pedalSpecs();
    for (int p = 0; p < tz::kNumPedals; ++p)
    {
        pedalOn[(size_t) p] = get (pedalOnId (p));
        for (int i = 0; i < (int) specs[(size_t) p].params.size(); ++i)
            pedalParams[(size_t) p][(size_t) i] = get (pedalParamId (p, i));
    }
    pedals[(size_t) tz::PedalId::Gate] = std::make_unique<tz::GatePedal>();
    pedals[(size_t) tz::PedalId::Comp] = std::make_unique<tz::CompPedal>();
    pedals[(size_t) tz::PedalId::Boost] = std::make_unique<tz::BoostPedal>();
    pedals[(size_t) tz::PedalId::Overdrive] = std::make_unique<tz::OverdrivePedal>();
    pedals[(size_t) tz::PedalId::Fuzz] = std::make_unique<tz::FuzzPedal>();
    pedals[(size_t) tz::PedalId::EQ] = std::make_unique<tz::EqPedal>();
    pedals[(size_t) tz::PedalId::Chorus] = std::make_unique<tz::ChorusPedal>();
    pedals[(size_t) tz::PedalId::Phaser] = std::make_unique<tz::PhaserPedal>();
    pedals[(size_t) tz::PedalId::Tremolo] = std::make_unique<tz::TremoloPedal>();
    pedals[(size_t) tz::PedalId::Delay] = std::make_unique<tz::DelayPedal>();
    pedals[(size_t) tz::PedalId::Reverb] = std::make_unique<tz::ReverbPedal>();
}

void RigEngine::readParams (Values& v) const
{
    v.inGainDb = pInGain->load(); v.outVolDb = pOutVol->load();
    v.os = juce::jlimit (0, 2, (int) pOs->load());
    v.ampBOn = pAmpBOn->load() > 0.5f; v.abMix = pAbMix->load(); v.width = pWidth->load();
    for (int s = 0; s < 2; ++s)
    {
        const auto& sp = slotParams[(size_t) s];
        auto& o = v.slot[(size_t) s];
        o.model = (tz::AmpVoicing) juce::jlimit (0, 2, (int) sp.model->load());
        o.channel = (tz::AmpChannel) juce::jlimit (0, 2, (int) sp.channel->load());
        o.knobs.gain = sp.gain->load(); o.knobs.bass = sp.bass->load(); o.knobs.mid = sp.mid->load();
        o.knobs.treble = sp.treble->load(); o.knobs.presence = sp.presence->load(); o.knobs.resonance = sp.resonance->load();
        o.knobs.master = sp.master->load(); o.knobs.sag = sp.sag->load(); o.knobs.bright = sp.bright->load() > 0.5f;
        o.levelDb = sp.level->load();
        o.cabOn = sp.cabOn->load() > 0.5f; o.cab = (int) sp.cab->load();
        o.mic = (tz::MicType) juce::jlimit (0, 2, (int) sp.mic->load());
        o.micPos = sp.micPos->load(); o.micDist = sp.micDist->load();
    }
    for (int p = 0; p < tz::kNumPedals; ++p)
    {
        v.on[(size_t) p] = pedalOn[(size_t) p]->load() > 0.5f;
        for (int i = 0; i < tz::Pedal::kMaxParams; ++i)
            if (auto* a = pedalParams[(size_t) p][(size_t) i]) v.p[(size_t) p][(size_t) i] = a->load();
    }
}

void RigEngine::prepare (double sampleRate, int maxBlockSize)
{
    baseRate = sampleRate;
    maxBlock = std::max (1, maxBlockSize);
    const double maxOsRate = sampleRate * 8.0;
    const int maxOsBlock = maxBlock * 8;

    for (int k = 0; k < 3; ++k)
    {
        oversamplers[(size_t) k] = std::make_unique<juce::dsp::Oversampling<float>> (
            2, (size_t) (k + 1), juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
        oversamplers[(size_t) k]->initProcessing ((size_t) maxBlock);
        osLatency[(size_t) k] = (int) std::lround (oversamplers[(size_t) k]->getLatencyInSamples());
    }
    reportedLatency = *std::max_element (osLatency.begin(), osLatency.end());
    latencyPadL.prepare (reportedLatency + 8);
    latencyPadR.prepare (reportedLatency + 8);

    for (int p = 0; p < tz::kNumPedals; ++p)
    {
        pedals[(size_t) p]->prepare (pedals[(size_t) p]->loopOnly() ? sampleRate : maxOsRate, maxOsBlock);
        pedalActive[(size_t) p] = false;
    }
    activeOs = juce::jlimit (0, 2, (int) pOs->load());
    activeRouting = Routing::decode (routingCode.load());
    for (int s = 0; s < 2; ++s)
    {
        amps[(size_t) s].prepare (maxOsRate, maxOsBlock);
        amps[(size_t) s].setRate (sampleRate * osFactor (activeOs));
        cabs[(size_t) s].prepare ({ sampleRate, (juce::uint32) maxBlock, 1 });
        mics[(size_t) s].prepare (sampleRate);
        ampLevel[(size_t) s].prepare (sampleRate, 0.03);
        activeCabOn[(size_t) s] = slotParams[(size_t) s].cabOn->load() > 0.5f;
    }
    applyRateToBoard (sampleRate * osFactor (activeOs));
    for (auto* s : { &inGain, &outGain, &abMix, &width, &ampBMix }) s->prepare (sampleRate, 0.03);
    inGain.setImmediate (tz::dbToGain (pInGain->load()));
    outGain.setImmediate (tz::dbToGain (pOutVol->load()));
    ampBMix.setImmediate (pAmpBOn->load() > 0.5f ? 1.0 : 0.0);
    abMix.setImmediate (pAbMix->load()); width.setImmediate (pWidth->load());

    monoScratch.setSize (2, maxBlock);
    dryScratch.setSize (2, maxOsBlock);
    pedalScratch.allocate ((size_t) maxOsBlock * 2, true);

    topoStep = 1.0 / (0.012 * sampleRate);
    topoGain = 1.0; topoState = 0;
    activeTopology = routingCode.load() ^ ((uint64_t) activeOs << 60) ^ ((uint64_t) activeCabOn[0] << 62) ^ ((uint64_t) activeCabOn[1] << 63);

    tunerDecim = std::max (1, (int) std::lround (sampleRate / 16000.0));
    dcIn.setHighpass (sampleRate, 8.0, 0.7);
    reset();
}

void RigEngine::applyRateToBoard (double osRate)
{
    for (int p = 0; p < tz::kNumPedals; ++p)
    {
        const bool loop = pedals[(size_t) p]->loopOnly() || activeRouting.inLoop (p);
        const double r = loop ? baseRate : osRate;
        for (int i = 0; i < tz::Pedal::kMaxParams; ++i)
            if (auto* a = pedalParams[(size_t) p][(size_t) i]) pedals[(size_t) p]->setParam (i, a->load());
        pedals[(size_t) p]->setRate (r); // no reset: keeping the state avoids a restart transient
        pedalMix[(size_t) p].prepare (r, 0.02);
        const double on = pedalOn[(size_t) p]->load() > 0.5f ? 1.0 : 0.0;
        pedalMix[(size_t) p].setImmediate (on);
        pedalActive[(size_t) p] = on > 0;
    }
}

void RigEngine::reset()
{
    for (auto& o : oversamplers) if (o) o->reset();
    for (auto& a : amps) a.reset();
    for (auto& c : cabs) c.reset();
    for (auto& m : mics) m.reset();
    for (auto& p : pedals) p->reset();
    latencyPadL.reset(); latencyPadR.reset();
    dcIn.reset();
}

void RigEngine::loadCabIR (int slot, int cabType, const juce::File& userFile)
{
    auto& conv = cabs[(size_t) slot];
    if (cabType == (int) tz::CabType::User)
    {
        if (userFile.existsAsFile())
            conv.loadImpulseResponse (userFile, juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::yes,
                                      0, juce::dsp::Convolution::Normalise::yes);
        return; // no file chosen yet: keep the previous IR
    }
    auto ir = tz::makeCabIR ((tz::CabType) cabType, baseRate);
    juce::AudioBuffer<float> buf (1, (int) ir.size());
    std::copy (ir.begin(), ir.end(), buf.getWritePointer (0));
    conv.loadImpulseResponse (std::move (buf), baseRate, juce::dsp::Convolution::Stereo::no,
                              juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
}

void RigEngine::process (juce::AudioBuffer<float>& buffer, int numInputChannels, double bpm)
{
    juce::ScopedNoDenormals noDenormals; // FTZ/DAZ: denormals can cost 100x CPU in feedback paths
    const int total = buffer.getNumSamples();
    const int outCh = buffer.getNumChannels();
    if (outCh == 0) return;

    Values v;
    readParams (v);
    for (int p = 0; p < tz::kNumPedals; ++p)
    {
        for (int i = 0; i < tz::Pedal::kMaxParams; ++i) pedals[(size_t) p]->setParam (i, v.p[(size_t) p][(size_t) i]);
        pedals[(size_t) p]->setTempo (bpm);
    }

    for (int start = 0; start < total; start += maxBlock)
    {
        const int n = std::min (maxBlock, total - start);
        float* outL = buffer.getWritePointer (0, start);
        float* outR = outCh > 1 ? buffer.getWritePointer (1, start) : nullptr;

        // ---- topology (routing / oversampling / cab on-off) --------------------
        const uint64_t wanted = routingCode.load() ^ ((uint64_t) v.os << 60) ^ ((uint64_t) v.slot[0].cabOn << 62) ^ ((uint64_t) v.slot[1].cabOn << 63);
        if (wanted != activeTopology && topoState == 0) topoState = 1;
        if (topoState == 1 && topoGain <= 0.0)
        {
            const Routing next = Routing::decode (routingCode.load());
            const bool osChanged = v.os != activeOs;
            activeRouting = next;
            activeOs = v.os;
            activeCabOn = { v.slot[0].cabOn, v.slot[1].cabOn };
            activeTopology = wanted;
            if (osChanged)
            {
                oversamplers[(size_t) activeOs]->reset();
                for (auto& a : amps) a.setRate (baseRate * osFactor (activeOs)); // keeps tube state
                latencyPadL.reset(); latencyPadR.reset();
            }
            applyRateToBoard (baseRate * osFactor (activeOs));
            // Stay silent a little longer while the freshly reset oversampling filters
            // settle (their start-up ringing would otherwise leak into the fade-in).
            topoHold = osChanged ? reportedLatency * 2 + (int) (0.003 * baseRate) : 0;
            topoState = 2;
        }

        // ---- input -------------------------------------------------------------
        float* mono = monoScratch.getWritePointer (0);
        float* mono2 = monoScratch.getWritePointer (1);
        inGain.setTarget (tz::dbToGain (v.inGainDb));
        float peak = 0;
        int tunerCountLocal = 0;
        float tunerTmp[1024];
        for (int i = 0; i < n; ++i)
        {
            float x = 0;
            if (numInputChannels >= 2) x = 0.5f * (buffer.getReadPointer (0, start)[i] + buffer.getReadPointer (1, start)[i]);
            else if (numInputChannels == 1) x = buffer.getReadPointer (0, start)[i];
            if (! std::isfinite (x)) x = 0.0f; // never let garbage from the host into the filters
            x = (float) dcIn.process (x * inGain.next());
            peak = std::max (peak, std::abs (x));
            // Tuner tap: 2x one-pole lowpass then decimate (~16 kHz).
            tunerLp1 += (x - tunerLp1) * 0.35; tunerLp2 += (tunerLp1 - tunerLp2) * 0.35;
            if (++tunerCount >= tunerDecim && tunerCountLocal < 1024) { tunerCount = 0; tunerTmp[tunerCountLocal++] = (float) tunerLp2; }
            mono[i] = x * (float) kInputVolts;
            mono2[i] = mono[i];
        }
        {
            int s1, n1, s2, n2;
            tunerFifo.prepareToWrite (tunerCountLocal, s1, n1, s2, n2);
            for (int i = 0; i < n1; ++i) tunerBuffer[(size_t) (s1 + i)] = tunerTmp[i];
            for (int i = 0; i < n2; ++i) tunerBuffer[(size_t) (s2 + i)] = tunerTmp[n1 + i];
            tunerFifo.finishedWrite (n1 + n2);
        }

        // ---- oversampled section: board pedals + amps ----------------------------
        juce::dsp::AudioBlock<float> inBlock (monoScratch.getArrayOfWritePointers(), 2, (size_t) n);
        auto& os = *oversamplers[(size_t) activeOs];
        auto up = os.processSamplesUp (inBlock);
        processOversampled (up, v);
        os.processSamplesDown (inBlock);

        // ---- cabs (base rate) ---------------------------------------------------
        ampBMix.setTarget (v.ampBOn ? 1.0 : 0.0);
        const bool bActive = v.ampBOn || ampBMix.isSmoothing() || ampBMix.current() > 0.0;
        for (int s = 0; s < (bActive ? 2 : 1); ++s)
        {
            float* d = monoScratch.getWritePointer (s);
            if (activeCabOn[(size_t) s])
            {
                juce::dsp::AudioBlock<float> b (&d, 1, (size_t) n);
                cabs[(size_t) s].process (juce::dsp::ProcessContextReplacing<float> (b));
                mics[(size_t) s].set (v.slot[(size_t) s].mic, v.slot[(size_t) s].micPos, v.slot[(size_t) s].micDist);
                mics[(size_t) s].process (d, n);
            }
            ampLevel[(size_t) s].setTarget (tz::dbToGain (v.slot[(size_t) s].levelDb));
            for (int i = 0; i < n; ++i) d[i] *= (float) ampLevel[(size_t) s].next();
        }

        // ---- A/B mix to stereo ----------------------------------------------------
        abMix.setTarget (v.abMix); width.setTarget (v.width);
        float* L = outL;
        float* R = outR != nullptr ? outR : dryScratch.getWritePointer (1); // mono host: compute R, then fold
        const float* a = monoScratch.getReadPointer (0);
        const float* b = monoScratch.getReadPointer (1);
        for (int i = 0; i < n; ++i)
        {
            const double bm = ampBMix.next(), m = abMix.next(), w = width.next() * bm;
            double gA = std::min (1.0, 2.0 * (1.0 - m)), gB = std::min (1.0, 2.0 * m) * bm;
            gA = gA * bm + (1.0 - bm);
            // Constant-power pan: A to the left, B to the right by 'width'.
            const double panA = (-w + 1.0) * tz::kPi * 0.25, panB = (w + 1.0) * tz::kPi * 0.25;
            const double sa = a[i] * gA, sb = bActive ? b[i] * gB : 0.0;
            L[i] = (float) (sa * std::cos (panA) * 1.41421356 + sb * std::cos (panB) * 1.41421356);
            R[i] = (float) (sa * std::sin (panA) * 1.41421356 + sb * std::sin (panB) * 1.41421356);
        }
        if (! bActive) ampBMix.setImmediate (v.ampBOn ? 1.0 : 0.0);

        // ---- FX loop (stereo, base rate) -------------------------------------------
        float* stereo[2] = { L, R };
        float* dry[2] = { pedalScratch.get(), pedalScratch.get() + maxBlock * 8 };
        for (int k = 0; k < activeRouting.numLoop; ++k)
        {
            // Pedals keep running even when bypassed: their delay lines stay full,
            // so switching one on never starts from silence (that step = a click),
            // and delay/reverb tails are already "alive". Bypass is a 8 ms crossfade.
            const int p = activeRouting.loop[(size_t) k];
            auto& mix = pedalMix[(size_t) p];
            mix.setTarget (v.on[(size_t) p] ? 1.0 : 0.0);
            std::copy (L, L + n, dry[0]); std::copy (R, R + n, dry[1]);
            pedals[(size_t) p]->process (stereo, 2, n);
            if (! mix.isSmoothing() && mix.current() <= 0.0)
            {
                std::copy (dry[0], dry[0] + n, L); std::copy (dry[1], dry[1] + n, R);
            }
            else if (mix.isSmoothing() || mix.current() < 1.0)
                for (int i = 0; i < n; ++i)
                {
                    const float g = (float) mix.next();
                    L[i] = dry[0][i] + g * (L[i] - dry[0][i]);
                    R[i] = dry[1][i] + g * (R[i] - dry[1][i]);
                }
        }

        // ---- output: volume, topology fade, latency pad, safety -----------------------
        outGain.setTarget (tz::dbToGain (v.outVolDb));
        const int pad = reportedLatency - osLatency[(size_t) activeOs];
        bool bad = false;
        float pkL = 0, pkR = 0;
        for (int i = 0; i < n; ++i)
        {
            if (topoState == 1) topoGain = std::max (0.0, topoGain - topoStep);
            else if (topoState == 2)
            {
                if (topoHold > 0) --topoHold;
                else { topoGain = std::min (1.0, topoGain + topoStep); if (topoGain >= 1.0) topoState = 0; }
            }
            // Raised-cosine fade: starts and ends with zero slope, so the fade itself
            // has no "corner" that could be heard as a tick.
            const double g = outGain.next() * (0.5 - 0.5 * std::cos (tz::kPi * topoGain));
            float l = (float) (L[i] * g), r = (float) (R[i] * g);
            if (pad > 0)
            {
                latencyPadL.push (l); latencyPadR.push (r);
                l = latencyPadL.readInt (pad); r = latencyPadR.readInt (pad);
            }
            if (! std::isfinite (l) || ! std::isfinite (r)) { bad = true; l = r = 0; }
            // Last-resort ear protection at +12 dBFS; never reached in normal use.
            L[i] = juce::jlimit (-4.0f, 4.0f, l);
            R[i] = juce::jlimit (-4.0f, 4.0f, r);
            pkL = std::max (pkL, std::abs (L[i])); pkR = std::max (pkR, std::abs (R[i]));
        }
        {
            int sp = scopePos.load (std::memory_order_relaxed);
            for (int i = 0; i < n; ++i) scopeBuf[(size_t) (sp++ & (kScope - 1))] = L[i];
            scopePos.store (sp, std::memory_order_relaxed);
        }
        if (bad) { reset(); juce::FloatVectorOperations::clear (L, n); juce::FloatVectorOperations::clear (R, n); }
        if (outR == nullptr) for (int i = 0; i < n; ++i) L[i] = 0.5f * (L[i] + R[i]);
        outPeakL.store (std::max (outPeakL.load(), pkL));
        outPeakR.store (std::max (outPeakR.load(), pkR));
        inPeak.store (std::max (inPeak.load(), peak));
    }
    for (int c = 2; c < outCh; ++c) buffer.clear (c, 0, total);
}

void RigEngine::processOversampled (juce::dsp::AudioBlock<float>& block, const Values& v)
{
    const int n = (int) block.getNumSamples();
    float* ch0 = block.getChannelPointer (0);
    float* ch1 = block.getChannelPointer (1);
    float* dry = pedalScratch.get();

    for (int k = 0; k < activeRouting.numBoard; ++k)
    {
        const int p = activeRouting.board[(size_t) k];
        if (pedals[(size_t) p]->loopOnly()) continue;
        auto& mix = pedalMix[(size_t) p];
        mix.setTarget (v.on[(size_t) p] ? 1.0 : 0.0);
        std::copy (ch0, ch0 + n, dry);
        float* chans[1] = { ch0 };
        pedals[(size_t) p]->process (chans, 1, n);
        if (! mix.isSmoothing() && mix.current() <= 0.0) std::copy (dry, dry + n, ch0);
        else if (mix.isSmoothing() || mix.current() < 1.0)
            for (int i = 0; i < n; ++i) ch0[i] = dry[i] + (float) mix.next() * (ch0[i] - dry[i]);
    }

    std::copy (ch0, ch0 + n, ch1);
    for (int s = 0; s < 2; ++s)
    {
        const auto& sv = v.slot[(size_t) s];
        amps[(size_t) s].set (sv.model, sv.channel, sv.knobs);
    }
    amps[0].process (ch0, n);
    if (v.ampBOn || ampBMix.current() > 0.0 || ampBMix.isSmoothing()) amps[1].process (ch1, n);
    else juce::FloatVectorOperations::clear (ch1, n);
}
} // namespace rig
