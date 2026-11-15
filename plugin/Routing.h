// Pedal order, packed into one 64-bit word so the UI can change it and the audio
// thread can read it atomically (no locks): 5 bits per pedal = zone bit + position.
#pragma once
#include "../dsp/tz/Pedals.h"
#include <juce_core/juce_core.h>

namespace rig
{
struct Routing
{
    // board = before the amp (oversampled), loop = after amp+cab (stereo).
    std::array<int, tz::kNumPedals> board {}, loop {};
    int numBoard = 0, numLoop = 0;

    static Routing defaults()
    {
        using P = tz::PedalId;
        Routing r;
        for (auto p : { P::Gate, P::Comp, P::Boost, P::Overdrive, P::Fuzz }) r.board[(size_t) r.numBoard++] = (int) p;
        for (auto p : { P::EQ, P::Chorus, P::Phaser, P::Tremolo, P::Delay, P::Reverb }) r.loop[(size_t) r.numLoop++] = (int) p;
        return r;
    }

    uint64_t encode() const
    {
        uint64_t code = 0;
        for (int i = 0; i < numBoard; ++i) code |= (uint64_t) (i & 15) << (5 * board[(size_t) i]);
        for (int i = 0; i < numLoop; ++i) code |= (uint64_t) (16 | (i & 15)) << (5 * loop[(size_t) i]);
        return code;
    }

    static Routing decode (uint64_t code)
    {
        Routing r;
        std::array<int, tz::kNumPedals> posB, posL;
        posB.fill (-1); posL.fill (-1);
        for (int p = 0; p < tz::kNumPedals; ++p)
        {
            const int v = (int) ((code >> (5 * p)) & 31);
            if (v & 16) posL[(size_t) p] = v & 15; else posB[(size_t) p] = v & 15;
        }
        // Stable ordering by stored position (insertion sort; 11 items, no allocation).
        auto build = [] (const std::array<int, tz::kNumPedals>& pos, std::array<int, tz::kNumPedals>& out, int& count)
        {
            count = 0;
            for (int p = 0; p < tz::kNumPedals; ++p)
                if (pos[(size_t) p] >= 0)
                {
                    int j = count++;
                    while (j > 0 && pos[(size_t) out[(size_t) j - 1]] > pos[(size_t) p]) { out[(size_t) j] = out[(size_t) j - 1]; --j; }
                    out[(size_t) j] = p;
                }
        };
        build (posB, r.board, r.numBoard);
        build (posL, r.loop, r.numLoop);
        return r;
    }

    bool inLoop (int pedal) const
    {
        for (int i = 0; i < numLoop; ++i) if (loop[(size_t) i] == pedal) return true;
        return false;
    }

    juce::String toString() const { return juce::String::toHexString ((juce::int64) encode()); }
    static Routing fromString (const juce::String& s)
    {
        if (s.isEmpty()) return defaults();
        return decode ((uint64_t) s.getHexValue64());
    }
};
} // namespace rig
