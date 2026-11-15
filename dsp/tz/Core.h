// ToxicZone DSP library - core utilities shared by every plugin in the bundle.
// Rules for everything in this library:
//  - prepare()/setRate() may allocate; process() never allocates, locks or does I/O.
//  - All recursive filter state is double precision (low cutoffs at 8x oversampled
//    rates lose accuracy in float and drift into denormals).
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace tz
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

inline double dbToGain (double db) { return std::pow (10.0, db / 20.0); }
inline double gainToDb (double g) { return 20.0 * std::log10 (std::max (g, 1e-12)); }
template <typename T> T clamp (T v, T lo, T hi) { return std::min (std::max (v, lo), hi); }

// Flush tiny values to zero. Used on feedback state in addition to the CPU FTZ/DAZ
// flags, because some hosts reset MXCSR between callbacks.
inline double flushDenormal (double v) { return std::abs (v) < 1e-20 ? 0.0 : v; }

// Linear ramp smoother: reaches the target in a fixed time, so a knob jump becomes
// a short glide instead of a step (a step in gain = an audible click/zip).
class Smoother
{
public:
    void prepare (double fs, double rampSeconds)
    {
        steps = std::max (1, (int) std::lround (fs * rampSeconds));
    }
    void setImmediate (double v) { cur = target = v; remaining = 0; }
    void setTarget (double v)
    {
        if (v == target) return;
        target = v;
        remaining = steps;
        inc = (target - cur) / (double) steps;
    }
    double next()
    {
        if (remaining > 0) { cur += inc; if (--remaining == 0) cur = target; }
        return cur;
    }
    // Advance n samples at once (for coefficient-rate updates).
    double skip (int n)
    {
        if (remaining <= 0) return cur;
        if (n >= remaining) { cur = target; remaining = 0; }
        else { cur += inc * n; remaining -= n; }
        return cur;
    }
    bool isSmoothing() const { return remaining > 0; }
    double current() const { return cur; }
    double getTarget() const { return target; }
private:
    double cur = 0, target = 0, inc = 0;
    int steps = 1, remaining = 0;
};

// Cheap deterministic noise (no std::random on the audio thread).
struct FastRandom
{
    uint32_t s = 0x12345678u;
    float nextBipolar()
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (float) ((double) s / 2147483648.0 - 1.0);
    }
};
} // namespace tz
