// Speaker cabinet: built-in impulse responses generated from a filter model of
// each cab (so they are 100% ours - no third-party licence), plus a "virtual mic"
// stage that lets you move the microphone on ANY IR, including the user's own.
#pragma once
#include "DelayLine.h"
#include "Filters.h"

namespace tz
{
enum class CabType { British2x12 = 0, American1x12 = 1, Modern4x12 = 2, User = 3 };
enum class MicType { Dynamic = 0, Ribbon = 1, Condenser = 2 };

// Generates a minimum-phase cab IR at the given rate. Allocates: call off the
// audio thread only.
inline std::vector<float> makeCabIR (CabType type, double fs, int length = 0)
{
    if (length <= 0) length = (int) std::lround (fs * 0.08);
    struct Band { int kind; double f, q, db; }; // kind: 0 HP, 1 LP, 2 peak, 3 notch
    std::vector<Band> bands;
    std::vector<std::pair<double, double>> reflections; // (ms, gain): internal cabinet reflections
    switch (type)
    {
        case CabType::British2x12: // open-ish 2x12 with alnico-style "chime"
            bands = { { 0, 75, 0.7, 0 }, { 2, 115, 1.4, 4.5 }, { 2, 420, 1.0, -3.0 }, { 2, 1200, 1.2, 2.0 },
                      { 2, 2600, 2.0, 6.0 }, { 2, 4100, 3.0, 3.0 }, { 1, 5600, 0.7, 0 }, { 1, 6200, 0.6, 0 }, { 3, 7400, 4, 0 } };
            reflections = { { 0.45, 0.18 }, { 1.1, -0.10 }, { 2.3, 0.05 } };
            break;
        case CabType::American1x12: // open-back 1x12: lighter low end, scooped, sparkly
            bands = { { 0, 70, 0.7, 0 }, { 2, 95, 1.2, 2.5 }, { 2, 650, 0.9, -2.5 }, { 2, 2200, 1.5, 4.0 },
                      { 2, 3600, 2.5, 2.5 }, { 1, 6200, 0.7, 0 }, { 1, 7000, 0.6, 0 }, { 3, 8200, 4, 0 } };
            reflections = { { 0.35, 0.12 }, { 0.9, -0.06 } };
            break;
        case CabType::Modern4x12: // closed-back 4x12 with V30-style upper-mid bite
            bands = { { 0, 82, 0.8, 0 }, { 2, 105, 1.8, 6.0 }, { 2, 500, 1.0, -4.0 }, { 2, 1500, 1.4, 3.0 },
                      { 2, 2800, 2.0, 5.0 }, { 2, 4300, 3.0, 3.0 }, { 1, 5000, 0.7, 0 }, { 1, 5600, 0.6, 0 }, { 3, 6900, 4, 0 } };
            reflections = { { 0.55, 0.22 }, { 1.4, -0.14 }, { 2.9, 0.07 }, { 4.1, -0.04 } };
            break;
        case CabType::User: break;
    }
    std::vector<Biquad> chain (bands.size());
    for (size_t i = 0; i < bands.size(); ++i)
    {
        const auto& b = bands[i];
        if (b.kind == 0) chain[i].setHighpass (fs, b.f, b.q);
        else if (b.kind == 1) chain[i].setLowpass (fs, b.f, b.q);
        else if (b.kind == 2) chain[i].setPeak (fs, b.f, b.q, b.db);
        else chain[i].setNotch (fs, b.f, b.q);
    }
    std::vector<float> ir ((size_t) length, 0.0f);
    // Impulse plus internal reflections (the box resonance that makes a cab "boxy").
    std::vector<double> excite ((size_t) length, 0.0);
    excite[0] = 1.0;
    for (auto& r : reflections)
    {
        const auto idx = (size_t) std::lround (r.first * 0.001 * fs);
        if (idx < excite.size()) excite[idx] += r.second;
    }
    FastRandom rnd;
    OnePole tailLp; tailLp.setCutoff (fs, 3000.0);
    for (int i = 0; i < length; ++i)
    {
        // Short diffuse tail (cone + cabinet + close room), ~-40 dB, 8 ms decay.
        const double t = i / fs;
        const double tail = tailLp.lp (rnd.nextBipolar()) * 0.012 * std::exp (-t / 0.008);
        double v = excite[(size_t) i] + (i > 2 ? tail : 0.0);
        for (auto& b : chain) v = b.process (v);
        ir[(size_t) i] = (float) v;
    }
    // Fade the end to avoid a truncation click.
    const int fade = length / 8;
    for (int i = 0; i < fade; ++i) ir[(size_t) (length - 1 - i)] *= (float) i / fade;
    // Normalise on the average in-band response (150 Hz - 4 kHz), so every cab and
    // every sample rate gives the same loudness. (Normalising the IR energy instead
    // would make 96 kHz ~3 dB louder than 48 kHz.)
    double acc = 0; int cnt = 0;
    for (double f = 150.0; f <= 4000.0; f *= 1.1, ++cnt)
    {
        double re = 0, im = 0;
        const double w = kTwoPi * f / fs;
        for (int i = 0; i < length; ++i) { re += ir[(size_t) i] * std::cos (w * i); im -= ir[(size_t) i] * std::sin (w * i); }
        acc += re * re + im * im;
    }
    const float g = (float) (1.0 / std::sqrt (std::max (acc / cnt, 1e-12)));
    for (auto& s : ir) s *= g;
    return ir;
}

// Virtual microphone: type (EQ signature), position across the cone (center =
// bright & direct, edge = darker & rounder), distance (proximity bass when close,
// floor-reflection comb + air loss when far).
class MicFilter
{
public:
    void prepare (double rate)
    {
        fs = rate;
        comb.prepare ((int) (0.01 * fs));
        for (auto* s : { &sPos, &sDist }) s->prepare (fs, 0.05);
        for (auto& s : sSig) s.prepare (fs, 0.05);
        dirty = true;
    }
    void reset() { for (auto& b : eq) b.reset(); comb.reset(); }
    void set (MicType t, double position, double distance, bool immediate = false)
    {
        // Each mic "signature" is a set of gains on the same six filters, so
        // changing mic type glides between signatures instead of switching filters.
        static constexpr double sig[3][6] = {
            //  peak5k2  lowshelf60  hishelf6k  lowshelf180  hishelf8k5  peak3k
            {    4.0,    -6.0,       0.0,       0.0,         0.0,        0.0 },  // dynamic
            {    0.0,     0.0,      -7.0,       2.0,         0.0,        0.0 },  // ribbon
            {    0.0,     0.0,       0.0,       0.0,         2.5,        1.0 } }; // condenser
        const int ti = clamp ((int) t, 0, 2);
        directional = t != MicType::Condenser;
        for (int i = 0; i < 6; ++i)
        {
            if (immediate) sSig[(size_t) i].setImmediate (sig[ti][i]); else sSig[(size_t) i].setTarget (sig[ti][i]);
        }
        if (immediate) { sPos.setImmediate (position); sDist.setImmediate (distance); dirty = true; }
        else { sPos.setTarget (position); sDist.setTarget (distance); }
    }
    void process (float* x, int n)
    {
        int done = 0;
        while (done < n)
        {
            const int len = std::min (32, n - done);
            bool moving = sPos.isSmoothing() || sDist.isSmoothing() || dirty;
            for (auto& s : sSig) moving |= s.isSmoothing();
            sPos.skip (len); sDist.skip (len);
            for (auto& s : sSig) s.skip (len);
            if (moving) design();
            const double dist = sDist.current();
            const double combDelay = (0.6 + 2.4 * dist) * 0.001 * fs;
            const double combGain = 0.35 * dist;
            for (int i = 0; i < len; ++i)
            {
                double v = x[done + i];
                comb.push ((float) v);
                v = v + combGain * comb.read (combDelay);
                for (auto& b : eq) v = b.process (v);
                x[done + i] = (float) v;
            }
            done += len;
        }
    }
private:
    void design()
    {
        const double pos = sPos.current(), dist = sDist.current();
        eq[0].setPeak (fs, 5200, 1.4, sSig[0].current());
        eq[1].setLowShelf (fs, 60, 0.7, sSig[1].current());
        eq[2].setHighShelf (fs, 6000, 0.6, sSig[2].current());
        eq[3].setLowShelf (fs, 180, 0.7, sSig[3].current());
        eq[4].setHighShelf (fs, 8500, 0.7, sSig[4].current());
        eq[5].setPeak (fs, 3000, 1.0, sSig[5].current());
        // Position across the cone: centre = bright and direct, edge = darker.
        eq[6].setHighShelf (fs, 2400, 0.6, 2.0 - 11.0 * pos);
        eq[7].setPeak (fs, 3400, 1.6, 3.0 * (1.0 - pos) - 1.0);
        // Distance: proximity bass when close (directional mics), air loss when far.
        eq[8].setLowShelf (fs, 220, 0.7, (directional ? 5.0 : 2.0) * (1.0 - dist) - 2.0);
        eq[9].setHighShelf (fs, 7000, 0.7, -5.0 * dist);
        dirty = false;
    }
    double fs = 48000;
    bool directional = true;
    std::array<Biquad, 10> eq;
    std::array<Smoother, 6> sSig;
    DelayLine comb;
    Smoother sPos, sDist;
    bool dirty = true;
};
} // namespace tz
