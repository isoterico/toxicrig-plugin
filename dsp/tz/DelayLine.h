// Circular delay line with 4-point Hermite interpolation for fractional (and
// smoothly changing) delay times. Memory is allocated in prepare() only.
#pragma once
#include "Core.h"

namespace tz
{
class DelayLine
{
public:
    void prepare (int maxDelaySamples)
    {
        int size = 1;
        while (size < maxDelaySamples + 4) size <<= 1;
        buf.assign ((size_t) size, 0.0f);
        mask = size - 1;
        w = 0;
    }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }
    int capacity() const { return mask - 3; }

    inline void push (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }

    // Delay measured from the most recently pushed sample (1 = previous sample).
    inline float read (double delay) const
    {
        delay = clamp (delay, 2.0, (double) (mask - 3));
        const double rp = (double) w - delay;
        const int i = (int) std::floor (rp);
        const float f = (float) (rp - i);
        const float y0 = buf[(size_t) ((i - 1) & mask)], y1 = buf[(size_t) (i & mask)];
        const float y2 = buf[(size_t) ((i + 1) & mask)], y3 = buf[(size_t) ((i + 2) & mask)];
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + y1;
    }
    inline float readInt (int delay) const { return buf[(size_t) ((w - delay) & mask)]; }

private:
    std::vector<float> buf { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    int mask = 7, w = 0;
};
} // namespace tz
