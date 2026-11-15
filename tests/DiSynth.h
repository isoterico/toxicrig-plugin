// Synthetic DI guitar for offline tests: extended Karplus-Strong strings (pick
// position, frequency-dependent decay, fractional tuning) through a single-coil
// pickup model. Not a replacement for a real DI, but it has the things that
// stress an amp sim: sharp pick attacks, palm mutes, chords, high notes.
#pragma once
#include "../dsp/tz/Filters.h"
#include <vector>

namespace test
{
struct Note { double start, freq, dur, vel; bool mute; };

inline std::vector<float> renderDI (double fs, double seconds, const std::vector<Note>& notes)
{
    using namespace tz;
    const int total = (int) (fs * seconds);
    std::vector<double> mix ((size_t) total, 0.0);
    FastRandom rnd;
    for (const auto& n : notes)
    {
        const double period = fs / n.freq;
        // Loop delay = N samples + 0.5 (loss filter) + d (allpass). Keeping d in
        // [0.5, 1.5) keeps the tuning allpass coefficient small and the loop stable.
        const int N = (int) std::floor (period - 1.0);
        const double d = period - 0.5 - N;
        const double ap = (1.0 - d) / (1.0 + d);
        std::vector<double> line ((size_t) N + 2, 0.0);
        // Excitation: noise burst shaped by velocity (brightness) and pick position.
        OnePole exLp; exLp.setCutoff (fs, 700.0 + 2200.0 * n.vel); // finger/pick softness
        const int pick = std::max (1, (int) (N * 0.13));
        std::vector<double> ex ((size_t) N, 0.0);
        for (int i = 0; i < N; ++i) ex[(size_t) i] = exLp.lp (rnd.nextBipolar());
        for (int i = N - 1; i >= pick; --i) ex[(size_t) i] -= ex[(size_t) (i - pick)];
        for (int i = 0; i < N; ++i) line[(size_t) i] = ex[(size_t) i] * n.vel;
        const double t60 = n.mute ? 0.12 : std::min (6.0, 3.5 * std::pow (110.0 / n.freq, 0.6) + 0.6);
        const double g = std::pow (10.0, -3.0 / (n.freq * t60));
        const double bright = n.mute ? 0.25 : 0.5;
        int w = 0;
        double prev = 0, apState = 0;
        const int start = (int) (n.start * fs);
        const int end = std::min (total, start + (int) ((n.dur + (n.mute ? 0.2 : 1.5)) * fs));
        const int offAt = start + (int) (n.dur * fs);
        double damp = 1.0;
        for (int i = start; i < end; ++i)
        {
            const double cur = line[(size_t) w];
            double y = g * damp * ((1.0 - bright) * cur + bright * prev);
            prev = cur;
            const double apOut = ap * y + apState; // fractional tuning allpass
            apState = y - ap * apOut;
            line[(size_t) w] = apOut;
            w = (w + 1) % N;
            if (i > offAt) damp = std::max (0.95, damp * 0.9995); // finger releases the string
            mix[(size_t) i] += cur;
        }
    }
    // Single-coil pickup: resonant peak ~3.8 kHz (coil inductance + cable capacitance),
    // gentle highpass for the coupling.
    Biquad res, hp, lp;
    res.setPeak (fs, 3800.0, 1.6, 3.5);
    hp.setHighpass (fs, 70.0, 0.7);
    lp.setLowpass (fs, 5200.0, 0.7);
    std::vector<float> out ((size_t) total);
    double peak = 1e-9;
    for (int i = 0; i < total; ++i) { mix[(size_t) i] = lp.process (res.process (hp.process (mix[(size_t) i]))); peak = std::max (peak, std::abs (mix[(size_t) i])); }
    const double norm = dbToGain (-9.0) / peak; // typical interface DI level: peaks around -9 dBFS
    for (int i = 0; i < total; ++i) out[(size_t) i] = (float) (mix[(size_t) i] * norm);
    return out;
}

// A 16-second test riff in E standard: palm-muted power chords, open chords,
// a single-note lick up high, and a long high note (the aliasing stress test).
inline std::vector<Note> testRiff()
{
    auto hz = [] (int midi) { return 440.0 * std::pow (2.0, (midi - 69) / 12.0); };
    std::vector<Note> n;
    const double e8 = 0.25; // 8th note at 120 bpm
    double t = 0.2;
    auto power = [&] (int root, double start, double dur, bool mute, double vel)
    {
        for (int k : { 0, 7, 12 }) n.push_back ({ start + k * 0.002, hz (root + k), dur, vel, mute });
    };
    for (int bar = 0; bar < 2; ++bar)
    {
        for (int i = 0; i < 6; ++i) power (40, t + i * e8 * 0.5 * 2, e8 * 0.9, true, 0.8);
        t += 6 * e8;
        power (43, t, e8 * 0.9, false, 0.9); t += e8;
        power (45, t, e8 * 2.0, false, 0.95); t += e8 * 2;
    }
    // Open chords, strummed (E major, A major).
    const int eMaj[] = { 40, 47, 52, 56, 59, 64 }, aMaj[] = { 45, 52, 57, 61, 64 };
    for (int i = 0; i < 6; ++i) n.push_back ({ t + i * 0.012, hz (eMaj[i]), 1.8, 0.7, false });
    t += 2.0;
    for (int i = 0; i < 5; ++i) n.push_back ({ t + i * 0.012, hz (aMaj[i]), 1.8, 0.7, false });
    t += 2.0;
    // Lick on the top strings (E minor pentatonic, 12th fret area).
    const int lick[] = { 76, 79, 81, 79, 76, 74, 76, 79, 81, 83, 84, 83, 81, 79, 76 };
    for (int m : lick) { n.push_back ({ t, hz (m), 0.22, 0.85, false }); t += 0.2; }
    t += 0.2;
    // Long high note: E6 (1318 Hz) - with max gain this is the aliasing torture test.
    n.push_back ({ t, hz (88), 2.5, 0.9, false });
    return n;
}
} // namespace test
