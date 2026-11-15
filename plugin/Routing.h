// Pedal order, packed into one 64-bit word so the UI can change it and the audio
// thread can read it atomically (no locks): see encode() for the bit layout.
#pragma once
#include "../dsp/tz/Pedals.h"
#include <juce_core/juce_core.h>
#include <vector>

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
        for (auto p : { P::Gate, P::Comp, P::BioBoost, P::Boost, P::Overdrive, P::Fuzz, P::NuclearFuzz }) r.board[(size_t) r.numBoard++] = (int) p;
        for (auto p : { P::EQ, P::Chorus, P::MutantMod, P::Phaser, P::Tremolo, P::Delay, P::Reverb }) r.loop[(size_t) r.numLoop++] = (int) p;
        return r;
    }

    // v2 format (14 pedals): the signal order as a permutation, 4 bits per pedal
    // (56 bits), plus the split point between board and loop (4 bits), plus a
    // format flag in bit 63. Every pedal always has a place: on/off is a parameter.
    static constexpr uint64_t kV2 = 1ull << 63;
    uint64_t encode() const
    {
        uint64_t code = kV2 | ((uint64_t) (numBoard & 15) << 56);
        int k = 0;
        for (int i = 0; i < numBoard; ++i) code |= (uint64_t) (board[(size_t) i] & 15) << (4 * k++);
        for (int i = 0; i < numLoop; ++i) code |= (uint64_t) (loop[(size_t) i] & 15) << (4 * k++);
        return code;
    }

    static Routing decode (uint64_t code)
    {
        if (code & kV2)
        {
            Routing r;
            const int split = (int) ((code >> 56) & 15);
            std::array<bool, tz::kNumPedals> seen {};
            for (int k = 0; k < tz::kNumPedals; ++k)
            {
                const int p = (int) ((code >> (4 * k)) & 15);
                if (p >= tz::kNumPedals || seen[(size_t) p]) return defaults(); // corrupt: never trust it
                seen[(size_t) p] = true;
                if (k < split) r.board[(size_t) r.numBoard++] = p; else r.loop[(size_t) r.numLoop++] = p;
            }
            return r;
        }
        return decodeV1 (code);
    }

    // v0.2 sessions (11 pedals, 5 bits each: zone bit + position). The three new
    // pedals are appended where the defaults put them.
    static Routing decodeV1 (uint64_t code)
    {
        constexpr int kOld = 11;
        Routing r;
        std::array<int, kOld> posB, posL;
        posB.fill (-1); posL.fill (-1);
        for (int p = 0; p < kOld; ++p)
        {
            const int v = (int) ((code >> (5 * p)) & 31);
            if (v & 16) posL[(size_t) p] = v & 15; else posB[(size_t) p] = v & 15;
        }
        auto build = [] (const std::array<int, kOld>& pos, std::array<int, tz::kNumPedals>& out, int& count)
        {
            count = 0;
            for (int p = 0; p < kOld; ++p)
                if (pos[(size_t) p] >= 0)
                {
                    int j = count++;
                    while (j > 0 && pos[(size_t) out[(size_t) j - 1]] > pos[(size_t) p]) { out[(size_t) j] = out[(size_t) j - 1]; --j; }
                    out[(size_t) j] = p;
                }
        };
        build (posB, r.board, r.numBoard);
        build (posL, r.loop, r.numLoop);
        using P = tz::PedalId;
        r.board[(size_t) r.numBoard++] = (int) P::BioBoost;
        r.board[(size_t) r.numBoard++] = (int) P::NuclearFuzz;
        r.loop[(size_t) r.numLoop++] = (int) P::MutantMod;
        return r;
    }

    bool inLoop (int pedal) const
    {
        for (int i = 0; i < numLoop; ++i) if (loop[(size_t) i] == pedal) return true;
        return false;
    }

    // Move a pedal to position 'index' of the board or the loop. Delay and reverb
    // always land in the loop (they only make sense after the amp).
    Routing moved (int pedal, bool toLoop, int index) const
    {
        if (pedal == (int) tz::PedalId::Delay || pedal == (int) tz::PedalId::Reverb) toLoop = true;
        std::vector<int> b, l;
        for (int i = 0; i < numBoard; ++i) if (board[(size_t) i] != pedal) b.push_back (board[(size_t) i]);
        for (int i = 0; i < numLoop; ++i) if (loop[(size_t) i] != pedal) l.push_back (loop[(size_t) i]);
        auto& dst = toLoop ? l : b;
        index = juce::jlimit (0, (int) dst.size(), index);
        dst.insert (dst.begin() + index, pedal);
        Routing r;
        for (int p : b) r.board[(size_t) r.numBoard++] = p;
        for (int p : l) r.loop[(size_t) r.numLoop++] = p;
        return r;
    }

    juce::String toString() const { return juce::String::toHexString ((juce::int64) encode()); }
    static Routing fromString (const juce::String& s)
    {
        if (s.isEmpty()) return defaults();
        return decode ((uint64_t) s.getHexValue64());
    }
};
} // namespace rig
