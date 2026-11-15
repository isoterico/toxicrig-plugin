// Complete amp heads built from the tube blocks. Three voicings, three channels each.
// Runs at the oversampled rate. All knob changes are smoothed; topology changes
// (model / channel) are crossfaded between two engines by AmpSlot.
#pragma once
#include "Tube.h"

namespace tz
{
enum class AmpVoicing { British = 0, American = 1, Modern = 2 };
enum class AmpChannel { Clean = 0, Crunch = 1, Lead = 2 };

struct AmpKnobs
{
    double gain = 0.5, bass = 0.5, mid = 0.5, treble = 0.5;
    double presence = 0.5, resonance = 0.5, master = 0.5, sag = 0.5;
    bool bright = false;
};

class AmpEngine
{
public:
    // Shared, read-only after construction: one triode table for everybody.
    static const TriodeTable& triode()
    {
        static const TriodeTable t = [] { TriodeTable tt; tt.build(); return tt; }();
        return t;
    }

    // resetState = false keeps the filter/tube state (used when only the sample
    // rate changes, e.g. new oversampling factor): restarting from zero would make
    // an audible "power-on" transient.
    void configure (double fs, AmpVoicing v, AmpChannel ch, const AmpKnobs& k, bool resetState = true)
    {
        rate = fs; voicing = v; channel = ch;
        const auto* T = &triode();
        TubeStageConfig s1, s2, s3, s4;
        PowerAmpConfig pa;
        ToneStackParts parts = stacks::fenderBlackface;
        int nStages = 1;
        stackPosition = 1; // tone stack after this many stages
        potAfter = 1;      // gain pot after this many stages
        gainRange = 1.0;
        outputTrim = 1.0;
        tightHz = 20.0;
        switch (v)
        {
            case AmpVoicing::British:
                parts = stacks::britishTopBoost;
                s1 = { 20, 0, 0, 0.0, 16000, 0.4 };
                s2 = { 35, 0, 0, 0.0, 12000, 0.4 };
                s3 = { 110, 0, 0, -0.5, 9000, 0.3 };
                pa = { 0.55, 1.4, 0.0, 110, 3500, 0.015, 0.30, 0.55, 0.10 };
                nStages = ch == AmpChannel::Clean ? 1 : (ch == AmpChannel::Crunch ? 2 : 3);
                stackPosition = nStages; potAfter = 1;
                gainRange = ch == AmpChannel::Clean ? 0.8 : 1.0;
                outputTrim = ch == AmpChannel::Clean ? 4.0 : 1.0;
                break;
            case AmpVoicing::American:
                parts = stacks::fenderBlackface;
                s1 = { 20, 0, 0, 0.0, 15000, 0.4 };
                s2 = { 25, 0, 0, 0.0, 12000, 0.4 };
                s3 = { 90, 0, 0, -0.35, 9000, 0.3 };
                pa = { 0.22, 1.6, 0.45, 90, 4000, 0.010, 0.20, 0.35, 0.06 };
                nStages = ch == AmpChannel::Lead ? 3 : 2;
                stackPosition = 1; potAfter = 1;
                gainRange = ch == AmpChannel::Clean ? 0.5 : (ch == AmpChannel::Crunch ? 1.6 : 1.0);
                outputTrim = ch == AmpChannel::Clean ? 1.3 : 1.0;
                break;
            case AmpVoicing::Modern:
                parts = stacks::modernHiGain;
                tightHz = ch == AmpChannel::Lead ? 160.0 : (ch == AmpChannel::Crunch ? 110.0 : 40.0);
                s1 = { tightHz, 450, -6.0, 0.0, 16000, 0.4 };
                s2 = { 60, 0, 0, -0.6, 10000, 0.3 };
                s3 = { 80, 0, 0, -0.4, 8000, 0.3 };
                s4 = { 40, 0, 0, 0.0, 7000, 0.4 };
                pa = { 0.20, 1.6, 0.40, 85, 3500, 0.008, 0.15, 0.20, 0.05 };
                nStages = ch == AmpChannel::Clean ? 2 : (ch == AmpChannel::Crunch ? 3 : 4);
                stackPosition = nStages;
                potAfter = ch == AmpChannel::Clean ? 1 : 2;
                gainRange = 1.0;
                outputTrim = ch == AmpChannel::Clean ? 1.4 : 1.0;
                break;
        }
        // Loudness balance between channels: saturated channels are naturally much
        // louder (compressed); bring everything near the same perceived level so
        // switching channel or model doesn't jump in volume.
        outputTrim *= ch == AmpChannel::Clean ? 0.9 : 0.355;
        numStages = nStages;
        stages[0].prepare (T, fs, s1, resetState);
        stages[1].prepare (T, fs, s2, resetState);
        stages[2].prepare (T, fs, s3, resetState);
        stages[3].prepare (T, fs, s4, resetState);
        stack.prepare (fs, parts);
        stack.setKnobs (k.treble, k.mid, k.bass, true);
        if (resetState) stack.reset();
        power.prepare (fs, pa, resetState);
        isBritish = (v == AmpVoicing::British);
        for (auto* s : { &sGain, &sBright, &sCut, &sDepth, &sLevel }) s->prepare (fs, 0.03);
        setKnobs (k, true);
        updateCoefficients (0);
        dcOut.prepare (fs, 10.0);
        if (resetState) reset();
    }

    void reset()
    {
        for (auto& s : stages) s.reset();
        stack.reset(); power.reset(); brightShelf.reset(); cutLp.reset(); depthShelf.reset(); dcOut.reset();
    }

    void setKnobs (const AmpKnobs& k, bool immediate = false)
    {
        knobs = k;
        // Audio-taper gain pot.
        const double g = std::pow (clamp (k.gain, 0.0, 1.0), 2.0) * gainRange + 0.004;
        const double brightDb = k.bright ? 9.0 * (1.0 - clamp (k.gain, 0.0, 1.0)) + 2.0 : 0.0;
        const double m = clamp (k.master, 0.0, 1.0);
        const double drive = 0.04 + 1.6 * m * m;
        if (immediate)
        {
            sGain.setImmediate (g); sBright.setImmediate (brightDb);
            sCut.setImmediate (k.presence); sDepth.setImmediate (k.resonance);
            sLevel.setImmediate (drive);
        }
        else
        {
            sGain.setTarget (g); sBright.setTarget (brightDb);
            sCut.setTarget (k.presence); sDepth.setTarget (k.resonance);
            sLevel.setTarget (drive);
        }
        stack.setKnobs (k.treble, k.mid, k.bass, immediate);
        if (isBritish)
            power.setControls (1.0, 0.0, 0.0, k.sag, immediate); // no NFB: presence = Cut, resonance = Depth
        else
            power.setControls (1.0, k.presence, k.resonance, k.sag, immediate);
    }

    // x: mono buffer at oversampled rate, in "volts at the input jack". In place.
    void process (float* x, int n)
    {
        int done = 0;
        while (done < n)
        {
            const int len = std::min (kSub, n - done);
            updateCoefficients (len);
            const double gain = sGain.skip (len);
            const double drive = sLevel.skip (len);
            for (int i = 0; i < len; ++i)
            {
                double v = x[done + i];
                const double sup = power.supply();
                for (int s = 0; s < numStages; ++s)
                {
                    v = stages[s].process (v, s == numStages - 1 ? sup : 1.0);
                    // Interstage: plate swing (~±100 V) into the next grid through
                    // the plate load / grid divider (~ /2) and the gain pot.
                    if (s + 1 == potAfter)
                    {
                        v = brightShelf.process (v);
                        v *= gain * 0.5;
                    }
                    else if (s + 1 < numStages)
                        v *= 0.35;
                    if (s + 1 == stackPosition)
                        v = stack.process (v);
                }
                // Into the phase inverter / power section.
                v = v * (1.0 / 12.0) * drive;
                v = power.process (v);
                if (isBritish)
                {
                    v = cutLp.lp (v);
                    v = depthShelf.process (v);
                }
                v = dcOut.process (v) * outputTrim * 0.5;
                x[done + i] = (float) v;
            }
            done += len;
        }
    }

    AmpVoicing voicing = AmpVoicing::British;
    AmpChannel channel = AmpChannel::Crunch;

private:
    void updateCoefficients (int len)
    {
        stack.update (len);
        const double bd = sBright.skip (0);
        if (bd != lastBright) { brightShelf.setHighShelf (rate, 2200.0, 0.6, bd); lastBright = bd; }
        if (isBritish)
        {
            const double cut = sCut.skip (0), depth = sDepth.skip (0);
            if (cut != lastCut) { cutLp.setCutoff (rate, 1800.0 * std::pow (11.0, cut)); lastCut = cut; }
            if (depth != lastDepth) { depthShelf.setLowShelf (rate, 120.0, 0.7, (depth - 0.5) * 10.0); lastDepth = depth; }
        }
        sBright.skip (len); sCut.skip (len); sDepth.skip (len);
    }

    static constexpr int kSub = 16;
    double rate = 48000;
    std::array<TubeStage, 4> stages;
    int numStages = 2, stackPosition = 1, potAfter = 1;
    double gainRange = 1, outputTrim = 1, tightHz = 20;
    bool isBritish = false;
    ToneStack stack;
    PowerAmp power;
    Biquad brightShelf, depthShelf;
    OnePole cutLp;
    DCBlocker dcOut;
    AmpKnobs knobs;
    Smoother sGain, sBright, sCut, sDepth, sLevel;
    double lastBright = -999, lastCut = -999, lastDepth = -999;
};

// An amp slot holds two engines so that changing model or channel crossfades
// (30 ms) instead of switching abruptly - an instant topology switch would click.
class AmpSlot
{
public:
    // maxBlock = largest block at the highest rate this slot will ever run at.
    void prepare (double fs, int maxBlock)
    {
        scratch.assign ((size_t) maxBlock, 0.0f);
        setRate (fs);
    }
    // Switch sample rate (e.g. new oversampling factor) without allocating.
    void setRate (double fs)
    {
        rate = fs;
        engines[0].configure (fs, voicing, channel, knobs, false);
        engines[1].configure (fs, voicing, channel, knobs, false);
        fadeLen = std::max (1, (int) (0.03 * fs));
        fadePos = -1;
    }

    // Called once per block from the audio thread. Never allocates.
    void set (AmpVoicing v, AmpChannel ch, const AmpKnobs& k)
    {
        knobs = k;
        if ((v != voicing || ch != channel) && fadePos < 0)
        {
            voicing = v; channel = ch;
            const int idle = 1 - active;
            engines[idle].configure (rate, v, ch, k); // coefficient maths only, no allocation
            fadePos = 0;
        }
        engines[active].setKnobs (k);
        if (fadePos >= 0) engines[1 - active].setKnobs (k);
    }

    void reset() { engines[0].reset(); engines[1].reset(); }

    void process (float* x, int n)
    {
        if (fadePos < 0) { engines[active].process (x, n); return; }
        std::copy (x, x + n, scratch.begin());
        engines[active].process (x, n);
        engines[1 - active].process (scratch.data(), n);
        for (int i = 0; i < n; ++i)
        {
            const double t = std::min (1.0, (double) (fadePos + i) / fadeLen);
            // Equal-power crossfade: the two engines are uncorrelated mid-fade.
            x[i] = (float) (x[i] * std::cos (t * kPi * 0.5) + scratch[(size_t) i] * std::sin (t * kPi * 0.5));
        }
        fadePos += n;
        if (fadePos >= fadeLen) { active = 1 - active; fadePos = -1; }
    }

private:
    double rate = 48000;
    AmpEngine engines[2];
    int active = 0, fadeLen = 1000, fadePos = -1;
    AmpVoicing voicing = AmpVoicing::British;
    AmpChannel channel = AmpChannel::Crunch;
    AmpKnobs knobs;
    std::vector<float> scratch;
};
} // namespace tz
