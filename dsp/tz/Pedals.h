// The pedalboard. Every pedal has the same shape (prepare / setRate / reset /
// process + a small array of parameter values), so the host processor can put them
// in any order. Pedals that may sit before the amp run at the oversampled rate;
// prepare() allocates for the highest rate, setRate() only recomputes coefficients.
#pragma once
#include "DelayLine.h"
#include "Filters.h"

namespace tz
{
enum class PedalId : int { Gate = 0, Comp, Boost, Overdrive, Fuzz, EQ, Chorus, Phaser, Tremolo, Delay, Reverb, Count };
constexpr int kNumPedals = (int) PedalId::Count;

// log(cosh(x)) without overflow: the antiderivative of tanh, used for ADAA.
inline double logCosh (double x) { const double a = std::abs (x); return a + std::log1p (std::exp (-2.0 * a)) - 0.69314718055994531; }

// First-order ADAA for a curve with a closed-form antiderivative.
struct Adaa1
{
    double x0 = 0;
    template <typename F, typename Fi> inline double process (double x, F&& f, Fi&& fi)
    {
        const double d = x - x0;
        const double y = std::abs (d) > 1e-6 ? (fi (x) - fi (x0)) / d : f (0.5 * (x + x0));
        x0 = x;
        return y;
    }
};

class Pedal
{
public:
    static constexpr int kMaxParams = 8;
    virtual ~Pedal() = default;
    virtual void prepare (double maxRate, int maxBlock) { (void) maxRate; (void) maxBlock; }
    virtual void setRate (double fs) = 0;
    virtual void reset() = 0;
    virtual void process (float* const* ch, int numCh, int n) = 0;
    // Values in real units (dB, ms, 0..1 ...). Read once per block.
    void setParam (int i, float v) { p[(size_t) i] = v; }
    float param (int i) const { return p[(size_t) i]; }
    // Time-based pedals (delay, reverb) only make sense after the amp.
    virtual bool loopOnly() const { return false; }
    void setTempo (double bpm) { tempo = bpm > 20 ? bpm : 120.0; }
protected:
    std::array<float, kMaxParams> p {};
    double fs = 48000, tempo = 120;
};

// ---------------------------------------------------------------------------
// Noise gate with hysteresis and hold: it opens at the threshold but closes 6 dB
// lower, so a decaying note doesn't chatter open/closed around the threshold.
class GatePedal : public Pedal
{
public:
    // p0 threshold dB, p1 release ms
    void setRate (double r) override
    {
        fs = r;
        envRel = std::exp (-1.0 / (0.03 * fs));
        atk = 1.0 - std::exp (-1.0 / (0.0005 * fs));
        holdSamples = (int) (0.015 * fs);
    }
    void reset() override { env = 0; g = 1; hold = 0; open = true; }
    void process (float* const* ch, int numCh, int n) override
    {
        const double thrOpen = dbToGain (p[0]), thrClose = thrOpen * 0.5;
        const double rel = 1.0 - std::exp (-1.0 / (std::max (5.0f, p[1]) * 0.001 * fs));
        for (int i = 0; i < n; ++i)
        {
            double a = 0;
            for (int c = 0; c < numCh; ++c) a = std::max (a, (double) std::abs (ch[c][i]));
            env = std::max (a, env * envRel);
            if (env > thrOpen) { open = true; hold = holdSamples; }
            else if (env < thrClose) { if (hold > 0) --hold; else open = false; }
            const double target = open ? 1.0 : 0.0;
            g += (target - g) * (target > g ? atk : rel);
            for (int c = 0; c < numCh; ++c) ch[c][i] = (float) (ch[c][i] * g);
        }
        env = flushDenormal (env);
    }
private:
    double env = 0, envRel = 0.999, atk = 0.1, g = 1;
    int hold = 0, holdSamples = 100;
    bool open = true;
};

// ---------------------------------------------------------------------------
// Sustainer-style compressor (one "Sustain" knob sets threshold and ratio together).
class CompPedal : public Pedal
{
public:
    // p0 sustain 0..1, p1 attack ms, p2 level dB, p3 blend 0..1
    void setRate (double r) override
    {
        fs = r; rmsCoef = 1.0 - std::exp (-1.0 / (0.005 * fs)); relCoef = 1.0 - std::exp (-1.0 / (0.15 * fs));
        for (auto* s : { &sSus, &sLevel, &sBlend }) s->prepare (fs, 0.03);
    }
    void reset() override { ms = 0; grDb = 0; }
    void process (float* const* ch, int numCh, int n) override
    {
        const double knee = 6.0;
        const double atkCoef = 1.0 - std::exp (-1.0 / (std::max (0.5f, p[1]) * 0.001 * fs));
        sSus.setTarget (clamp ((double) p[0], 0.0, 1.0)); sLevel.setTarget (p[2]); sBlend.setTarget (clamp ((double) p[3], 0.0, 1.0));
        for (int i = 0; i < n; ++i)
        {
            const double s = sSus.next(), lvlDb = sLevel.next(), blend = sBlend.next();
            const double thr = -10.0 - 30.0 * s, ratio = 2.0 + 8.0 * s;
            const double makeup = -thr * (1.0 - 1.0 / ratio) * 0.6 + lvlDb;
            double a = 0;
            for (int c = 0; c < numCh; ++c) a = std::max (a, (double) ch[c][i] * ch[c][i]);
            ms += (a - ms) * rmsCoef;
            const double lvl = 10.0 * std::log10 (ms + 1e-12);
            const double over = lvl - thr;
            double target = 0;
            if (over > knee * 0.5) target = over * (1.0 - 1.0 / ratio);
            else if (over > -knee * 0.5) target = (1.0 - 1.0 / ratio) * (over + knee * 0.5) * (over + knee * 0.5) / (2.0 * knee);
            grDb += (target - grDb) * (target > grDb ? atkCoef : relCoef);
            const double gain = dbToGain (makeup - grDb);
            for (int c = 0; c < numCh; ++c)
                ch[c][i] = (float) (ch[c][i] * (blend * gain + (1.0 - blend) * dbToGain (lvlDb)));
        }
        ms = flushDenormal (ms);
    }
private:
    Smoother sSus, sLevel, sBlend;
    double ms = 0, grDb = 0, rmsCoef = 0.01, relCoef = 0.001;
};

// ---------------------------------------------------------------------------
// Germanium treble booster (Rangemaster family): a small input capacitor only lets
// the upper mids/treble through to a single germanium transistor, which is driven
// into soft, asymmetric clipping. Pushing an AC30 with it = the classic lead tone.
class BoostPedal : public Pedal
{
public:
    // p0 boost 0..1, p1 range (0 treble, 1 mid, 2 full), p2 level dB
    void setRate (double r) override
    {
        fs = r;
        for (auto& s : st) { s.dc.prepare (fs, 15.0); s.bw.setCutoff (fs, 9000.0); }
        sBoost.prepare (fs, 0.02); sLevel.prepare (fs, 0.02);
        lastRange = -1;
    }
    void reset() override { for (auto& s : st) { s.hp.reset(); s.dc.reset(); s.bw.reset(); s.adaa = {}; } }
    void process (float* const* ch, int numCh, int n) override
    {
        const int range = (int) std::lround (p[1]);
        if (range != lastRange)
        {
            const double f = range == 0 ? 1600.0 : (range == 1 ? 550.0 : 70.0);
            for (auto& s : st) s.hp.setCutoff (fs, f);
            lastRange = range;
        }
        sBoost.setTarget (p[0]); sLevel.setTarget (dbToGain (p[2]));
        for (int i = 0; i < n; ++i)
        {
            const double gain = 2.0 + 40.0 * std::pow (sBoost.next(), 1.5);
            const double lvl = sLevel.next();
            for (int c = 0; c < numCh; ++c)
            {
                auto& s = st[(size_t) c];
                const double x = ch[c][i];
                double u = (s.hp.hp (x) + 0.08 * x) * gain;
                // Germanium: asymmetric soft clip (with ADAA against aliasing).
                u = s.adaa.process (u,
                    [] (double v) { return v >= 0 ? std::tanh (v * 1.6) / 1.6 : std::tanh (v * 2.4) / 2.4 * 0.75; },
                    [] (double v) { return v >= 0 ? logCosh (v * 1.6) / 2.56 : 0.75 * logCosh (v * 2.4) / 5.76; });
                u = s.bw.lp (s.dc.process (u));
                ch[c][i] = (float) (u * 0.9 * lvl);
            }
        }
    }
private:
    struct St { OnePole hp, bw; DCBlocker dc; Adaa1 adaa; };
    std::array<St, 2> st;
    Smoother sBoost, sLevel;
    int lastRange = -1;
};

// ---------------------------------------------------------------------------
// Green "screamer" overdrive. The clipping diodes sit in the op-amp feedback loop
// behind a 720 Hz highpass: lows pass clean, mids get boosted and clipped. That is
// why it tightens the bass and adds the famous mid hump in front of a hot amp.
class OverdrivePedal : public Pedal
{
public:
    // p0 drive 0..1, p1 tone 0..1, p2 level dB
    void prepare (double, int) override {}
    void setRate (double r) override
    {
        fs = r;
        for (auto& s : st) { s.in.setCutoff (fs, 20.0); s.hp.setCutoff (fs, 720.0); s.tone.setCutoff (fs, 723.0); s.dc.prepare (fs, 10.0); }
        sDrive.prepare (fs, 0.03); sTone.prepare (fs, 0.03); sLevel.prepare (fs, 0.03);
        lastDrive = -1;
    }
    void reset() override { for (auto& s : st) { s.in.reset(); s.hp.reset(); s.opLp.reset(); s.tone.reset(); s.dc.reset(); s.adaa = {}; } }
    void process (float* const* ch, int numCh, int n) override
    {
        sDrive.setTarget (p[0]); sTone.setTarget (p[1]); sLevel.setTarget (dbToGain (p[2]));
        for (int i = 0; i < n; ++i)
        {
            const double d = sDrive.next();
            if ((i & 15) == 0 && d != lastDrive)
            {
                // Op-amp feedback: 51k + drive pot (500k) against 4.7k and 51 pF.
                const double rf = 51e3 + d * 500e3;
                gainHi = rf / 4.7e3;
                for (auto& s : st) s.opLp.setCutoff (fs, 1.0 / (kTwoPi * rf * 51e-12));
                lastDrive = d;
            }
            const double tone = sTone.next();
            const double toneGain = 0.08 + 1.5 * tone * tone;
            const double lvl = sLevel.next();
            for (int c = 0; c < numCh; ++c)
            {
                auto& s = st[(size_t) c];
                const double x = s.in.hp (ch[c][i]);
                double v = s.hp.hp (x) * gainHi;
                v = s.adaa.process (v, [] (double a) { return 0.6 * std::tanh (a / 0.6); },   // silicon diode pair, ~0.6 V
                                       [] (double a) { return 0.36 * logCosh (a / 0.6); });
                v = s.opLp.lp (v);
                double y = x + v;
                const double lo = s.tone.lp (y);
                y = lo + (y - lo) * toneGain;
                ch[c][i] = (float) (s.dc.process (y) * lvl);
            }
        }
    }
private:
    struct St { OnePole in, hp, opLp, tone; DCBlocker dc; Adaa1 adaa; };
    std::array<St, 2> st;
    Smoother sDrive, sTone, sLevel;
    double gainHi = 11, lastDrive = -1;
};

// ---------------------------------------------------------------------------
// Fuzz: very high gain into hard, asymmetric transistor clipping. "Bias" starves
// the transistor so quiet signals fall into a dead zone = gated, sputtery fuzz.
// Tone blends a lowpass and a highpass (big scooped-mid fuzz voicing).
class FuzzPedal : public Pedal
{
public:
    // p0 fuzz 0..1, p1 tone 0..1, p2 level dB, p3 bias 0..1
    void prepare (double, int) override {}
    void setRate (double r) override
    {
        fs = r;
        for (auto& s : st) { s.in.setCutoff (fs, 60.0); s.lp.setCutoff (fs, 900.0); s.hp.setCutoff (fs, 1100.0); s.dc.prepare (fs, 15.0); s.bw.setCutoff (fs, 7000.0); }
        sFuzz.prepare (fs, 0.03); sTone.prepare (fs, 0.03); sLevel.prepare (fs, 0.03); sBias.prepare (fs, 0.03);
    }
    void reset() override { for (auto& s : st) { s.in.reset(); s.lp.reset(); s.hp.reset(); s.dc.reset(); s.bw.reset(); } }
    void process (float* const* ch, int numCh, int n) override
    {
        sFuzz.setTarget (p[0]); sTone.setTarget (p[1]); sLevel.setTarget (dbToGain (p[2])); sBias.setTarget (p[3]);
        for (int i = 0; i < n; ++i)
        {
            const double g = 8.0 + 300.0 * std::pow (sFuzz.next(), 2.0);
            const double tone = sTone.next(), lvl = sLevel.next(), bias = sBias.next();
            const double dead = 0.35 * bias;
            for (int c = 0; c < numCh; ++c)
            {
                auto& s = st[(size_t) c];
                double u = s.in.hp (ch[c][i]) * g - 0.3 * bias;
                u = u > 0 ? std::tanh (u) : -std::tanh (-u * 1.8) / 1.3;
                const double a = std::abs (u);
                u = a > dead ? (u > 0 ? 1.0 : -1.0) * (a - dead) / (1.0 - dead) : 0.0;
                u = s.bw.lp (s.dc.process (u));
                const double y = (1.0 - tone) * s.lp.lp (u) + tone * s.hp.hp (u) * 1.3;
                ch[c][i] = (float) (y * 0.6 * lvl);
            }
        }
    }
private:
    struct St { OnePole in, lp, hp, bw; DCBlocker dc; };
    std::array<St, 2> st;
    Smoother sFuzz, sTone, sLevel, sBias;
};

// ---------------------------------------------------------------------------
// 7-band graphic EQ (100 Hz ... 6.4 kHz, octave spaced, ±12 dB).
class EqPedal : public Pedal
{
public:
    // p0..p6 band gains dB, p7 level dB
    static constexpr double freqs[7] = { 100, 200, 400, 800, 1600, 3200, 6400 };
    void prepare (double, int) override {}
    void setRate (double r) override { fs = r; for (auto& s : sm) s.prepare (fs, 0.03); sLevel.prepare (fs, 0.03); dirty = true; }
    void reset() override { for (auto& c : bands) for (auto& b : c) b.reset(); }
    void process (float* const* ch, int numCh, int n) override
    {
        for (int b = 0; b < 7; ++b) sm[(size_t) b].setTarget (p[(size_t) b]);
        sLevel.setTarget (dbToGain (p[7]));
        int done = 0;
        while (done < n)
        {
            const int len = std::min (32, n - done);
            bool moving = dirty;
            for (auto& s : sm) moving |= s.isSmoothing();
            for (int b = 0; b < 7; ++b) sm[(size_t) b].skip (len);
            if (moving)
            {
                for (int b = 0; b < 7; ++b)
                    for (auto& c : bands) c[(size_t) b].setPeak (fs, freqs[b], 1.1, sm[(size_t) b].current());
                dirty = false;
            }
            for (int i = 0; i < len; ++i)
            {
                const double lvl = sLevel.next();
                for (int c = 0; c < numCh; ++c)
                {
                    double v = ch[c][done + i];
                    for (auto& bq : bands[(size_t) c]) v = bq.process (v);
                    ch[c][done + i] = (float) (v * lvl);
                }
            }
            done += len;
        }
    }
private:
    std::array<std::array<Biquad, 7>, 2> bands;
    std::array<Smoother, 7> sm;
    Smoother sLevel;
    bool dirty = true;
};

// ---------------------------------------------------------------------------
// Chorus: two voices per channel reading a short delay modulated by phase-offset
// LFOs. Right channel LFOs are shifted 90 degrees for stereo width.
class ChorusPedal : public Pedal
{
public:
    // p0 rate Hz, p1 depth 0..1, p2 mix 0..1
    void prepare (double maxRate, int) override { for (auto& d : dl) d.prepare ((int) (0.05 * maxRate)); }
    void setRate (double r) override { fs = r; sMix.prepare (fs, 0.03); sDepth.prepare (fs, 0.05); }
    void reset() override { for (auto& d : dl) d.reset(); phase = 0; }
    void process (float* const* ch, int numCh, int n) override
    {
        sMix.setTarget (p[2]); sDepth.setTarget (p[1]);
        const double inc = clamp ((double) p[0], 0.05, 10.0) / fs;
        for (int i = 0; i < n; ++i)
        {
            phase += inc; if (phase >= 1.0) phase -= 1.0;
            const double mix = sMix.next(), depth = sDepth.next();
            const double base = 0.008 * fs, dep = (0.0005 + 0.0035 * depth) * fs;
            for (int c = 0; c < numCh; ++c)
            {
                const double x = ch[c][i];
                dl[(size_t) c].push ((float) x);
                const double off = c == 0 ? 0.0 : 0.25;
                const double d1 = base + dep * std::sin (kTwoPi * (phase + off));
                const double d2 = base * 1.3 + dep * std::sin (kTwoPi * (phase + off + 0.333));
                const double wet = 0.5 * (dl[(size_t) c].read (d1) + dl[(size_t) c].read (d2));
                ch[c][i] = (float) (x * (1.0 - 0.3 * mix) + wet * mix * 0.9);
            }
        }
    }
private:
    std::array<DelayLine, 2> dl;
    Smoother sMix, sDepth;
    double phase = 0;
};

// ---------------------------------------------------------------------------
// Phaser: six first-order allpasses swept by an LFO; mixing with the dry signal
// creates moving notches. Feedback deepens the notches (more "vowel").
class PhaserPedal : public Pedal
{
public:
    // p0 rate Hz, p1 depth 0..1, p2 feedback 0..1, p3 mix 0..1
    void prepare (double, int) override {}
    void setRate (double r) override { fs = r; sMix.prepare (fs, 0.03); }
    void reset() override { for (auto& c : ap) for (auto& a : c) a.reset(); fb = { 0, 0 }; phase = 0; }
    void process (float* const* ch, int numCh, int n) override
    {
        sMix.setTarget (p[3]);
        const double inc = clamp ((double) p[0], 0.02, 10.0) / fs;
        const double depth = clamp ((double) p[1], 0.0, 1.0), feed = 0.85 * clamp ((double) p[2], 0.0, 1.0);
        for (int i = 0; i < n; ++i)
        {
            phase += inc; if (phase >= 1.0) phase -= 1.0;
            if ((i & 7) == 0)
                for (int c = 0; c < numCh; ++c)
                {
                    const double l = 0.5 + 0.5 * depth * std::sin (kTwoPi * (phase + (c ? 0.25 : 0.0)));
                    const double f = 160.0 * std::pow (2600.0 / 160.0, l);
                    for (auto& a : ap[(size_t) c]) a.setFreq (fs, f);
                }
            const double mix = sMix.next();
            for (int c = 0; c < numCh; ++c)
            {
                const double x = ch[c][i];
                double v = x + feed * fb[(size_t) c];
                for (auto& a : ap[(size_t) c]) v = a.process (v);
                fb[(size_t) c] = flushDenormal (std::tanh (v));
                ch[c][i] = (float) ((1.0 - 0.5 * mix) * x + 0.5 * mix * v);
            }
        }
    }
private:
    std::array<std::array<Allpass1, 6>, 2> ap;
    std::array<double, 2> fb { 0, 0 };
    Smoother sMix;
    double phase = 0;
};

// ---------------------------------------------------------------------------
// Tremolo: amplitude LFO, shape morphs from smooth sine (vintage optical) to
// choppy square.
class TremoloPedal : public Pedal
{
public:
    // p0 rate Hz, p1 depth 0..1, p2 shape 0..1
    void prepare (double, int) override {}
    void setRate (double r) override { fs = r; sDepth.prepare (fs, 0.03); sShape.prepare (fs, 0.05); }
    void reset() override { phase = 0; }
    void process (float* const* ch, int numCh, int n) override
    {
        sDepth.setTarget (p[1]); sShape.setTarget (p[2]);
        const double inc = clamp ((double) p[0], 0.2, 20.0) / fs;
        for (int i = 0; i < n; ++i)
        {
            phase += inc; if (phase >= 1.0) phase -= 1.0;
            const double k = 1.0 + 9.0 * sShape.next();
            const double s = std::tanh (k * std::sin (kTwoPi * phase)) / std::tanh (k);
            const double g = 1.0 - sDepth.next() * (0.5 - 0.5 * s);
            for (int c = 0; c < numCh; ++c) ch[c][i] = (float) (ch[c][i] * g);
        }
    }
private:
    Smoother sDepth, sShape;
    double phase = 0;
};

// ---------------------------------------------------------------------------
// Delay with tempo sync, darkening/saturating feedback (analog-style repeats) and
// ping-pong. Time changes glide (like turning the knob on a tape/BBD delay)
// instead of jumping, which would click.
class DelayPedal : public Pedal
{
public:
    // p0 time ms, p1 sync 0/1, p2 division idx, p3 feedback 0..0.95, p4 mix, p5 tone 0..1, p6 ping-pong 0/1
    static constexpr double divisionBeats[7] = { 2.0, 1.0, 1.5, 2.0 / 3.0, 0.5, 0.75, 0.25 };
    bool loopOnly() const override { return true; }
    void prepare (double maxRate, int) override { for (auto& d : dl) d.prepare ((int) (2.6 * maxRate)); }
    void setRate (double r) override
    {
        fs = r; sMix.prepare (fs, 0.03); sFb.prepare (fs, 0.03);
        glide = 1.0 - std::exp (-1.0 / (0.12 * fs));
        for (auto& s : st) { s.hp.setCutoff (fs, 90.0); }
        lastTone = -1;
    }
    void reset() override { for (auto& d : dl) d.reset(); for (auto& s : st) { s.lp.reset(); s.hp.reset(); s.last = 0; } curDelay = -1; }
    double targetSeconds() const
    {
        if (p[1] > 0.5)
        {
            const int idx = clamp ((int) std::lround (p[2]), 0, 6);
            return std::min (2.5, divisionBeats[idx] * 60.0 / tempo);
        }
        return clamp ((double) p[0], 1.0, 2500.0) * 0.001;
    }
    void process (float* const* ch, int numCh, int n) override
    {
        const double target = std::max (4.0, targetSeconds() * fs);
        if (curDelay < 0) curDelay = target;
        if (p[5] != lastTone) { for (auto& s : st) s.lp.setCutoff (fs, 800.0 * std::pow (15.0, (double) p[5])); lastTone = p[5]; }
        sMix.setTarget (p[4]); sFb.setTarget (clamp ((double) p[3], 0.0, 0.95));
        const bool pp = p[6] > 0.5 && numCh == 2;
        for (int i = 0; i < n; ++i)
        {
            curDelay += (target - curDelay) * glide;
            const double mix = sMix.next(), fb = sFb.next();
            std::array<double, 2> wet { 0, 0 };
            for (int c = 0; c < numCh; ++c) wet[(size_t) c] = dl[(size_t) c].read (curDelay);
            for (int c = 0; c < numCh; ++c)
            {
                auto& s = st[(size_t) c];
                const double x = ch[c][i];
                // Feedback source: own channel, or the other one for ping-pong.
                const double back = pp ? wet[(size_t) (1 - c)] : wet[(size_t) c];
                double in = pp ? (c == 0 ? 0.5 * (x + ch[numCh - 1][i]) : 0.0) : x;
                double f = s.hp.hp (s.lp.lp (back)) * fb;
                f = std::tanh (f * 1.2) / 1.2; // gentle saturation keeps high feedback musical
                dl[(size_t) c].push ((float) (in + f));
                ch[c][i] = (float) (x + mix * wet[(size_t) c]);
            }
        }
    }
private:
    struct St { OnePole lp, hp; double last = 0; };
    std::array<DelayLine, 2> dl;
    std::array<St, 2> st;
    Smoother sMix, sFb;
    double glide = 0.001, curDelay = -1;
    float lastTone = -1;
};

// ---------------------------------------------------------------------------
// Reverb: 8-line feedback delay network. Input diffusion (4 allpasses) smears the
// attack; a Hadamard matrix mixes the lines so the echo density builds up quickly;
// gentle delay modulation removes metallic ringing; per-line gains give an exact
// decay time and a lowpass in the loop makes highs die faster (like real rooms).
class ReverbPedal : public Pedal
{
public:
    // p0 mix, p1 decay s, p2 size 0..1, p3 predelay ms, p4 damping 0..1
    bool loopOnly() const override { return true; }
    void prepare (double maxRate, int) override
    {
        for (auto& l : lines) l.prepare ((int) (0.13 * maxRate));
        for (auto& a : diff) a.prepare ((int) (0.02 * maxRate));
        pre.prepare ((int) (0.25 * maxRate));
    }
    void setRate (double r) override
    {
        fs = r; sMix.prepare (fs, 0.03); sSize.prepare (fs, 0.5);
        sSize.setImmediate (p[2]);
        for (auto& d : damp) d.reset();
        lastDamp = -1;
    }
    void reset() override
    {
        for (auto& l : lines) l.reset();
        for (auto& a : diff) a.reset();
        pre.reset();
        for (auto& d : damp) d.reset();
        state.fill (0);
        lfo = 0;
    }
    void process (float* const* ch, int numCh, int n) override
    {
        static constexpr double baseMs[8] = { 29.7, 37.1, 41.1, 43.7, 53.0, 59.3, 67.9, 73.1 };
        static constexpr double diffMs[4] = { 3.2, 2.4, 8.6, 6.3 };
        sMix.setTarget (p[0]); sSize.setTarget (p[2]);
        const double rt60 = clamp ((double) p[1], 0.2, 20.0);
        if (p[4] != lastDamp)
        {
            for (auto& d : damp) d.setCutoff (fs, 18000.0 * std::pow (0.06, (double) p[4]));
            lastDamp = p[4];
        }
        const double preSamples = std::max (2.0, (double) p[3] * 0.001 * fs);
        for (int i = 0; i < n; ++i)
        {
            const double mix = sMix.next(), size = sSize.next();
            const double scale = 0.45 + 0.9 * size;
            lfo += 0.7 / fs; if (lfo >= 1.0) lfo -= 1.0;
            const double xin = numCh == 2 ? 0.5 * (ch[0][i] + ch[1][i]) : ch[0][i];
            pre.push ((float) xin);
            double v = pre.read (preSamples);
            for (int k = 0; k < 4; ++k)
            {
                const double d = diffMs[k] * 0.001 * fs * scale;
                const double delayed = diff[(size_t) k].read (d);
                const double g = 0.62;
                const double w = v + g * delayed;
                diff[(size_t) k].push ((float) w);
                v = delayed - g * w;
            }
            std::array<double, 8> out;
            for (int k = 0; k < 8; ++k)
            {
                const double mod = k < 4 ? 0.0004 * fs * std::sin (kTwoPi * (lfo * (1.0 + 0.31 * k) + k * 0.25)) : 0.0;
                const double len = baseMs[k] * 0.001 * fs * scale + mod;
                const double g = std::pow (10.0, -3.0 * len / (rt60 * fs));
                out[(size_t) k] = damp[(size_t) k].lp (lines[(size_t) k].read (len)) * g;
            }
            // Fast Hadamard transform (orthogonal, energy preserving), normalised.
            std::array<double, 8> h = out;
            for (int span = 1; span < 8; span <<= 1)
                for (int a = 0; a < 8; a += span << 1)
                    for (int b = a; b < a + span; ++b)
                    {
                        const double x0 = h[(size_t) b], x1 = h[(size_t) (b + span)];
                        h[(size_t) b] = x0 + x1; h[(size_t) (b + span)] = x0 - x1;
                    }
            static constexpr double sgn[8] = { 1, -1, 1, 1, -1, 1, -1, -1 };
            for (int k = 0; k < 8; ++k)
                lines[(size_t) k].push ((float) flushDenormal (h[(size_t) k] * 0.35355339059 + v * sgn[k] * 0.5));
            const double wl = out[0] - out[2] + out[4] + out[6] - 0.5 * out[1];
            const double wr = out[1] - out[3] + out[5] + out[7] - 0.5 * out[0];
            const double wet[2] = { wl * 0.45, wr * 0.45 };
            for (int c = 0; c < numCh; ++c)
            {
                const double w = numCh == 2 ? wet[c] : 0.5 * (wet[0] + wet[1]);
                ch[c][i] = (float) (ch[c][i] * (1.0 - 0.35 * mix) + w * mix);
            }
        }
    }
private:
    std::array<DelayLine, 8> lines;
    std::array<DelayLine, 4> diff;
    DelayLine pre;
    std::array<OnePole, 8> damp;
    std::array<double, 8> state {};
    Smoother sMix, sSize;
    double lfo = 0;
    float lastDamp = -1;
};
} // namespace tz
