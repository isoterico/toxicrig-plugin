// Pitch detector (YIN) for the tuner. Runs on the UI thread on a copy of the input
// that the audio thread pushes into a lock-free FIFO - never on the audio thread.
#pragma once
#include "Core.h"

namespace tz
{
class PitchDetector
{
public:
    // Works on a decimated signal (~11 kHz) for low CPU; low E (82 Hz) needs
    // a window of ~2 periods at minimum, we use ~70 ms.
    void prepare (double rate)
    {
        fs = rate;
        window = (int) (0.07 * fs);
        d.assign ((size_t) window / 2, 0.0);
    }
    // Returns Hz, or 0 when no confident pitch.
    double detect (const float* x, int n)
    {
        if (n < window) return 0.0;
        const int W = window / 2;
        double energy = 0;
        for (int i = 0; i < window; ++i) energy += (double) x[i] * x[i];
        if (energy / window < 1e-7) return 0.0;
        const int minLag = (int) (fs / 1400.0), maxLag = std::min (W - 1, (int) (fs / 60.0));
        d[0] = 1.0;
        double run = 0;
        for (int tau = 1; tau <= maxLag; ++tau)
        {
            double s = 0;
            for (int j = 0; j < W; ++j) { const double df = (double) x[j] - x[j + tau]; s += df * df; }
            run += s;
            d[(size_t) tau] = run > 0 ? s * tau / run : 1.0; // cumulative mean normalised difference
        }
        int best = -1;
        for (int tau = std::max (2, minLag); tau < maxLag; ++tau)
            if (d[(size_t) tau] < 0.12)
            {
                while (tau + 1 < maxLag && d[(size_t) tau + 1] < d[(size_t) tau]) ++tau;
                best = tau;
                break;
            }
        if (best < 0) return 0.0;
        // Parabolic interpolation for sub-sample accuracy (needed for ±1 cent).
        const double a = d[(size_t) best - 1], b = d[(size_t) best], c = d[(size_t) best + 1];
        const double den = a - 2 * b + c;
        const double shift = std::abs (den) > 1e-12 ? 0.5 * (a - c) / den : 0.0;
        return fs / (best + shift);
    }
    int windowSize() const { return window; }
private:
    double fs = 11025;
    int window = 770;
    std::vector<double> d;
};

struct NoteInfo { int midi = -1; double cents = 0; };
inline NoteInfo hzToNote (double hz, double a4 = 440.0)
{
    if (hz <= 0) return {};
    const double m = 69.0 + 12.0 * std::log2 (hz / a4);
    const int r = (int) std::lround (m);
    return { r, (m - r) * 100.0 };
}
} // namespace tz
