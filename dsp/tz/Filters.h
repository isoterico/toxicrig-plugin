// Linear filters used everywhere: RBJ biquads, TPT one-poles, DC blocker,
// first-order allpass, and a generic 3rd-order IIR (for analog tone stacks).
#pragma once
#include "Core.h"

namespace tz
{
// Biquad in transposed direct form II (good numerical behaviour, 2 state vars).
class Biquad
{
public:
    void reset() { z1 = z2 = 0.0; }

    void setIdentity() { b0 = 1; b1 = b2 = a1 = a2 = 0; }

    // RBJ Audio-EQ-Cookbook designs.
    void setLowpass (double fs, double f, double q)   { design (fs, f, q, 0, Type::LP); }
    void setHighpass (double fs, double f, double q)  { design (fs, f, q, 0, Type::HP); }
    void setBandpass (double fs, double f, double q)  { design (fs, f, q, 0, Type::BP); }
    void setPeak (double fs, double f, double q, double db)      { design (fs, f, q, db, Type::Peak); }
    void setLowShelf (double fs, double f, double q, double db)  { design (fs, f, q, db, Type::LowShelf); }
    void setHighShelf (double fs, double f, double q, double db) { design (fs, f, q, db, Type::HighShelf); }
    void setNotch (double fs, double f, double q)     { design (fs, f, q, 0, Type::Notch); }

    inline double process (double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        z1 = flushDenormal (z1);
        z2 = flushDenormal (z2);
        return y;
    }

    // Magnitude response (used by tests/visualisation only).
    double magnitudeAt (double fs, double f) const
    {
        const double w = kTwoPi * f / fs;
        const double cr = std::cos (w), ci = -std::sin (w), c2r = std::cos (2 * w), c2i = -std::sin (2 * w);
        const double nr = b0 + b1 * cr + b2 * c2r, ni = b1 * ci + b2 * c2i;
        const double dr = 1 + a1 * cr + a2 * c2r, di = a1 * ci + a2 * c2i;
        return std::sqrt ((nr * nr + ni * ni) / (dr * dr + di * di));
    }

private:
    enum class Type { LP, HP, BP, Peak, LowShelf, HighShelf, Notch };

    void design (double fs, double f, double q, double db, Type t)
    {
        f = clamp (f, 1.0, fs * 0.49);
        q = std::max (q, 0.05);
        const double A = std::pow (10.0, db / 40.0);
        const double w0 = kTwoPi * f / fs;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / (2.0 * q);
        double B0 = 1, B1 = 0, B2 = 0, A0 = 1, A1 = 0, A2 = 0;
        switch (t)
        {
            case Type::LP: B0 = (1 - cw) / 2; B1 = 1 - cw; B2 = (1 - cw) / 2; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case Type::HP: B0 = (1 + cw) / 2; B1 = -(1 + cw); B2 = (1 + cw) / 2; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case Type::BP: B0 = alpha; B1 = 0; B2 = -alpha; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case Type::Notch: B0 = 1; B1 = -2 * cw; B2 = 1; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case Type::Peak: B0 = 1 + alpha * A; B1 = -2 * cw; B2 = 1 - alpha * A; A0 = 1 + alpha / A; A1 = -2 * cw; A2 = 1 - alpha / A; break;
            case Type::LowShelf:
            {
                const double sq = 2 * std::sqrt (A) * alpha;
                B0 = A * ((A + 1) - (A - 1) * cw + sq); B1 = 2 * A * ((A - 1) - (A + 1) * cw); B2 = A * ((A + 1) - (A - 1) * cw - sq);
                A0 = (A + 1) + (A - 1) * cw + sq; A1 = -2 * ((A - 1) + (A + 1) * cw); A2 = (A + 1) + (A - 1) * cw - sq;
                break;
            }
            case Type::HighShelf:
            {
                const double sq = 2 * std::sqrt (A) * alpha;
                B0 = A * ((A + 1) + (A - 1) * cw + sq); B1 = -2 * A * ((A - 1) + (A + 1) * cw); B2 = A * ((A + 1) + (A - 1) * cw - sq);
                A0 = (A + 1) - (A - 1) * cw + sq; A1 = 2 * ((A - 1) - (A + 1) * cw); A2 = (A + 1) - (A - 1) * cw - sq;
                break;
            }
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }

    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
};

// Topology-preserving-transform one-pole: stable under fast cutoff modulation,
// which the RBJ biquad is not (that is why modulated filters use this one).
class OnePole
{
public:
    void reset() { s = 0; }
    void setCutoff (double fs, double f)
    {
        f = clamp (f, 0.5, fs * 0.49);
        const double g = std::tan (kPi * f / fs);
        G = g / (1.0 + g);
    }
    inline double lp (double x)
    {
        const double v = (x - s) * G;
        const double y = v + s;
        s = flushDenormal (y + v);
        return y;
    }
    inline double hp (double x) { return x - lp (x); }
private:
    double G = 0.5, s = 0;
};

// DC blocker: one-pole highpass at ~10 Hz. Asymmetric tube clipping creates DC;
// leaving it in wastes headroom and makes the next nonlinearity bias wrongly.
class DCBlocker
{
public:
    void prepare (double fs, double f = 10.0) { R = std::exp (-kTwoPi * f / fs); }
    void reset() { x1 = y1 = 0; }
    inline double process (double x)
    {
        const double y = x - x1 + R * y1;
        x1 = x;
        y1 = flushDenormal (y);
        return y;
    }
private:
    double R = 0.999, x1 = 0, y1 = 0;
};

// First-order allpass (phaser building block). Coefficient from break frequency.
class Allpass1
{
public:
    void reset() { z = 0; }
    inline void setFreq (double fs, double f)
    {
        const double t = std::tan (kPi * clamp (f, 1.0, fs * 0.49) / fs);
        a = (t - 1.0) / (t + 1.0);
    }
    inline double process (double x)
    {
        const double y = a * x + z;
        z = flushDenormal (x - a * y);
        return y;
    }
private:
    double a = 0, z = 0;
};

// Generic 3rd-order IIR (TDF-II). Coefficients normalised so a0 = 1.
class Iir3
{
public:
    void reset() { z1 = z2 = z3 = 0; }
    void setCoefficients (const double* b, const double* a) // a[0] == 1
    {
        b0 = b[0]; b1 = b[1]; b2 = b[2]; b3 = b[3];
        a1 = a[1]; a2 = a[2]; a3 = a[3];
    }
    inline double process (double x)
    {
        const double y = b0 * x + z1;
        z1 = flushDenormal (b1 * x - a1 * y + z2);
        z2 = flushDenormal (b2 * x - a2 * y + z3);
        z3 = flushDenormal (b3 * x - a3 * y);
        return y;
    }
private:
    double b0 = 1, b1 = 0, b2 = 0, b3 = 0, a1 = 0, a2 = 0, a3 = 0;
    double z1 = 0, z2 = 0, z3 = 0;
};
} // namespace tz
