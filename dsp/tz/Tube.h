// Tube models: triode gain stage (12AX7 from the Koren equations solved on a real
// load line), Fender/Marshall-style tone stack (Yeh & Smith analytic model),
// push-pull power amp with sag and negative feedback (presence/resonance).
#pragma once
#include "Filters.h"

namespace tz
{
// ---------------------------------------------------------------------------
// Triode transfer curve. Built once (prepare-time): for every grid voltage we solve
// Vp + Ip(Vg,Vp)*Rp = B+ (the load line) with the Koren 12AX7 equations. The result
// is naturally asymmetric: towards cutoff the plate current stops abruptly (hard
// edge), towards saturation it bends softly. That asymmetry is where the even
// harmonics and the "tube" character come from - not from a generic tanh().
class TriodeTable
{
public:
    static constexpr int N = 4096;
    static constexpr double vMin = -10.0, vMax = 2.0;

    void build (double Bplus = 300.0, double Rp = 100e3, double Rk = 1.5e3)
    {
        // Koren 12AX7 parameters.
        const double mu = 100.0, ex = 1.4, kg1 = 1060.0, kp = 600.0, kvb = 300.0;
        auto ip = [&] (double vg, double vp)
        {
            const double e1 = (vp / kp) * std::log1p (std::exp (kp * (1.0 / mu + vg / std::sqrt (kvb + vp * vp))));
            return e1 > 0 ? std::pow (e1, ex) / kg1 : 0.0;
        };
        auto solveVp = [&] (double vg)
        {
            double lo = 0.0, hi = Bplus;
            for (int i = 0; i < 60; ++i)
            {
                const double mid = 0.5 * (lo + hi);
                if (mid + ip (vg, mid) * Rp > Bplus) hi = mid; else lo = mid;
            }
            return 0.5 * (lo + hi);
        };
        // Self bias: Vgk0 = -Ip0 * Rk (fixed-point iteration).
        double vg0 = -1.0;
        for (int i = 0; i < 50; ++i)
            vg0 = -ip (vg0, solveVp (vg0)) * Rk;
        bias = vg0;
        const double vpq = solveVp (vg0);
        for (int i = 0; i < N; ++i)
        {
            const double vg = vMin + (vMax - vMin) * i / (N - 1);
            table[(size_t) i] = solveVp (vg) - vpq; // output relative to quiescent (inverting)
        }
        // Small-signal gain at the operating point (for level normalisation).
        const double d = 0.01;
        smallSignalGain = -(lookup (bias + d) - lookup (bias - d)) / (2 * d);
    }

    inline double lookup (double vg) const
    {
        if (! (vg == vg)) vg = 0.0; // NaN guard: never index the table with garbage
        const double pos = (clamp (vg, vMin, vMax) - vMin) * ((N - 1) / (vMax - vMin));
        const int i = std::min ((int) pos, N - 2);
        const double f = pos - i;
        return table[(size_t) i] + f * (table[(size_t) i + 1] - table[(size_t) i]);
    }

    double bias = -1.0;           // quiescent grid-cathode voltage
    double smallSignalGain = 60;  // |dVp/dVg| at bias
private:
    std::array<double, N> table {};
};

// ---------------------------------------------------------------------------
// Antiderivative anti-aliasing (ADAA) for any static curve f(x). Instead of
// sampling f at each point (which creates harmonics above Nyquist that fold back
// as inharmonic "fizz"), we output the average of f between the previous and the
// current input: (F(x1) - F(x0)) / (x1 - x0), with F the integral of f. That
// average is a built-in lowpass on what the curve creates. Combined with
// oversampling it removes most of the remaining aliasing for ~free.
class AdaaCurve
{
public:
    static constexpr int N = 4096;
    AdaaCurve() : f ((size_t) N), F ((size_t) N) {}

    // Build from any callable over [lo, hi]. Outside the range the curve continues
    // flat (curves here saturate), and F continues linearly. No allocation.
    template <typename Fn> void build (double lo_, double hi_, Fn&& fn)
    {
        lo = lo_; hi = hi_; h = (hi - lo) / (N - 1);
        for (int i = 0; i < N; ++i) f[(size_t) i] = fn (lo + h * i);
        F[0] = 0;
        for (int i = 1; i < N; ++i) F[(size_t) i] = F[(size_t) i - 1] + 0.5 * h * (f[(size_t) i - 1] + f[(size_t) i]);
    }
    void reset() { x0 = 0; init = false; }

    inline double value (double x) const
    {
        if (! (x == x)) x = 0;
        if (x <= lo) return f[0];
        if (x >= hi) return f[(size_t) N - 1];
        const double pos = (x - lo) / h;
        const int i = std::min ((int) pos, N - 2);
        const double t = pos - i;
        return f[(size_t) i] + t * (f[(size_t) i + 1] - f[(size_t) i]);
    }
    inline double integral (double x) const
    {
        if (x <= lo) return F[0] + f[0] * (x - lo);
        if (x >= hi) return F[(size_t) N - 1] + f[(size_t) N - 1] * (x - hi);
        const double pos = (x - lo) / h;
        const int i = std::min ((int) pos, N - 2);
        const double dx = (pos - i) * h;
        const double slope = (f[(size_t) i + 1] - f[(size_t) i]) / h;
        return F[(size_t) i] + f[(size_t) i] * dx + 0.5 * slope * dx * dx;
    }
    inline double process (double x)
    {
        if (! (x == x)) x = 0;
        if (! init) { x0 = x; init = true; }
        const double d = x - x0;
        const double y = std::abs (d) > 1e-5 ? (integral (x) - integral (x0)) / d : value (0.5 * (x + x0));
        x0 = x;
        return y;
    }
private:
    std::vector<double> f, F;
    double lo = -1, hi = 1, h = 1, x0 = 0;
    bool init = false;
};

// ---------------------------------------------------------------------------
// One triode gain stage: coupling cap -> cathode bypass -> grid conduction with
// bias shift ("blocking") -> plate curve -> Miller-capacitance lowpass.
struct TubeStageConfig
{
    double couplingHz = 20.0;   // input coupling capacitor highpass
    double cathodeHz = 0.0;     // partial cathode bypass: below this freq gain drops
    double cathodeDb = 0.0;     // how much gain is lost below cathodeHz (dB, negative)
    double biasShift = 0.0;     // extra bias (V): negative = colder, more asymmetric clip
    double millerHz = 12000.0;  // Miller capacitance lowpass
    double gridLimit = 0.4;     // grid starts conducting around here (V)
};

class TubeStage
{
public:
    void prepare (const TriodeTable* t, double fs, const TubeStageConfig& c, bool resetState = true)
    {
        table = t; cfg = c; rate = fs;
        coupling.setCutoff (fs, c.couplingHz);
        if (c.cathodeHz > 0) cathode.setLowShelf (fs, c.cathodeHz, 0.6, c.cathodeDb); else cathode.setIdentity();
        miller.setCutoff (fs, c.millerHz);
        // Grid clamp + plate curve form one static curve -> one ADAA table.
        const double gl = c.gridLimit;
        curve.build (-14.0, 14.0, [t, gl] (double v)
        {
            const double over = v - gl;
            if (over > 0) v = gl + over / (1.0 + over * 4.0);
            return t->lookup (v);
        });
        // Grid current charges the coupling cap fast (via the low grid resistance)
        // and discharges slowly through the grid-leak resistor: ~2 ms / ~40 ms.
        chargeCoef = 1.0 - std::exp (-1.0 / (0.002 * fs));
        dischargeCoef = 1.0 - std::exp (-1.0 / (0.040 * fs));
        if (resetState) reset();
    }
    void reset() { coupling.reset(); cathode.reset(); miller.reset(); curve.reset(); shift = 0; }

    // x: grid signal in volts. Returns plate signal in volts (inverted, ~-60x).
    inline double process (double x, double supplyScale = 1.0)
    {
        x = coupling.hp (x);
        x = cathode.process (x);
        const double v = x + table->bias + cfg.biasShift + shift;
        const double over = v - cfg.gridLimit;
        // Grid conduction: positive peaks get clamped (soft, inside the curve) and
        // the coupling cap charges, pushing the bias colder for the next ~40 ms.
        // This is the dynamic "squash and recover" feel of a cranked preamp.
        if (over > 0) shift -= over * chargeCoef;
        shift += (0.0 - shift) * dischargeCoef;
        shift = flushDenormal (shift);
        // Supply sag lowers the plate swing available (scale) for this stage.
        const double y = curve.process (v / supplyScale) * supplyScale;
        return miller.lp (y);
    }

private:
    const TriodeTable* table = nullptr;
    TubeStageConfig cfg;
    double rate = 48000;
    OnePole coupling, miller;
    Biquad cathode;
    AdaaCurve curve;
    double shift = 0, chargeCoef = 0.01, dischargeCoef = 0.001;
};

// ---------------------------------------------------------------------------
// Passive Treble/Mid/Bass tone stack (the circuit used by Fender, Marshall and most
// high-gain amps). Yeh & Smith derived its exact 3rd-order transfer function from
// the component values; we discretise it with the bilinear transform. Because it is
// the real circuit, the knobs interact exactly like on the amp (e.g. on a Fender
// all knobs at 0 = silence; mid scoop moves with treble).
struct ToneStackParts { double C1, C2, C3, R1, R2, R3, R4; };

namespace stacks
{
    inline constexpr ToneStackParts fenderBlackface { 250e-12, 100e-9, 47e-9, 250e3, 250e3, 10e3, 100e3 };
    inline constexpr ToneStackParts marshall        { 470e-12, 22e-9, 22e-9, 220e3, 1e6, 25e3, 33e3 };
    inline constexpr ToneStackParts modernHiGain    { 250e-12, 47e-9, 20e-9, 250e3, 1e6, 25e3, 47e3 };
    inline constexpr ToneStackParts britishTopBoost { 56e-12, 22e-9, 22e-9, 1e6, 1e6, 10e3, 100e3 };
}

class ToneStack
{
public:
    void prepare (double fs, const ToneStackParts& p)
    {
        rate = fs; parts = p;
        for (auto* s : { &sT, &sM, &sB }) s->prepare (fs, 0.03);
        dirty = true;
    }
    void reset() { filt.reset(); }
    void setKnobs (double treble, double mid, double bass, bool immediate = false)
    {
        if (immediate) { sT.setImmediate (treble); sM.setImmediate (mid); sB.setImmediate (bass); dirty = true; }
        else { sT.setTarget (treble); sM.setTarget (mid); sB.setTarget (bass); }
    }
    // Call once per sub-block (e.g. every 16 samples) before processing it.
    void update (int samplesInSubBlock)
    {
        const bool moving = sT.isSmoothing() || sM.isSmoothing() || sB.isSmoothing();
        sT.skip (samplesInSubBlock); sM.skip (samplesInSubBlock); sB.skip (samplesInSubBlock);
        if (moving || dirty) { design (sT.current(), sM.current(), sB.current()); dirty = false; }
    }
    inline double process (double x) { return filt.process (x); }

    // Analog magnitude (for tests).
    void coefficients (double t, double m, double l, double* B, double* A) const { analogToDigital (t, m, l, B, A); }

private:
    void design (double t, double m, double l)
    {
        double B[4], A[4];
        analogToDigital (t, m, l, B, A);
        filt.setCoefficients (B, A);
    }

    void analogToDigital (double t, double m, double lKnob, double* B, double* A) const
    {
        const auto& p = parts;
        const double C1 = p.C1, C2 = p.C2, C3 = p.C3, R1 = p.R1, R2 = p.R2, R3 = p.R3, R4 = p.R4;
        t = clamp (t, 0.001, 0.999); m = clamp (m, 0.001, 0.999);
        // Bass pot is log taper.
        const double l = std::exp ((clamp (lKnob, 0.0, 1.0) - 1.0) * 3.4);

        const double b1 = t * C1 * R1 + m * C3 * R3 + l * (C1 * R2 + C2 * R2) + (C1 * R3 + C2 * R3);
        const double b2 = t * (C1 * C2 * R1 * R4 + C1 * C3 * R1 * R4) - m * m * (C1 * C3 * R3 * R3 + C2 * C3 * R3 * R3)
                        + m * (C1 * C3 * R1 * R3 + C1 * C3 * R3 * R3 + C2 * C3 * R3 * R3)
                        + l * (C1 * C2 * R1 * R2 + C1 * C2 * R2 * R4 + C1 * C3 * R2 * R4)
                        + l * m * (C1 * C3 * R2 * R3 + C2 * C3 * R2 * R3)
                        + (C1 * C2 * R1 * R3 + C1 * C2 * R3 * R4 + C1 * C3 * R3 * R4);
        const double b3 = l * m * (C1 * C2 * C3 * R1 * R2 * R3 + C1 * C2 * C3 * R2 * R3 * R4)
                        - m * m * (C1 * C2 * C3 * R1 * R3 * R3 + C1 * C2 * C3 * R3 * R3 * R4)
                        + m * (C1 * C2 * C3 * R1 * R3 * R3 + C1 * C2 * C3 * R3 * R3 * R4)
                        + t * C1 * C2 * C3 * R1 * R3 * R4 - t * m * C1 * C2 * C3 * R1 * R3 * R4
                        + t * l * C1 * C2 * C3 * R1 * R2 * R4;
        const double a0 = 1.0;
        const double a1 = (C1 * R1 + C1 * R3 + C2 * R3 + C2 * R4 + C3 * R4) + m * C3 * R3 + l * (C1 * R2 + C2 * R2);
        const double a2 = m * (C1 * C3 * R1 * R3 - C2 * C3 * R3 * R4 + C1 * C3 * R3 * R3 + C2 * C3 * R3 * R3)
                        + l * m * (C1 * C3 * R2 * R3 + C2 * C3 * R2 * R3) - m * m * (C1 * C3 * R3 * R3 + C2 * C3 * R3 * R3)
                        + l * (C1 * C2 * R2 * R4 + C1 * C2 * R1 * R2 + C1 * C3 * R2 * R4 + C2 * C3 * R2 * R4)
                        + (C1 * C2 * R1 * R4 + C1 * C3 * R1 * R4 + C1 * C2 * R3 * R4 + C1 * C2 * R1 * R3 + C1 * C3 * R3 * R4 + C2 * C3 * R3 * R4);
        const double a3 = l * m * (C1 * C2 * C3 * R1 * R2 * R3 + C1 * C2 * C3 * R2 * R3 * R4)
                        - m * m * (C1 * C2 * C3 * R1 * R3 * R3 + C1 * C2 * C3 * R3 * R3 * R4)
                        + m * (C1 * C2 * C3 * R3 * R3 * R4 + C1 * C2 * C3 * R1 * R3 * R3 - C1 * C2 * C3 * R1 * R3 * R4)
                        + l * C1 * C2 * C3 * R1 * R2 * R4 + C1 * C2 * C3 * R1 * R3 * R4;

        // Bilinear transform s = c (1 - z^-1) / (1 + z^-1). Multiplying through by
        // (1 + z^-1)^3, each s^k term becomes c^k (1 - z^-1)^k (1 + z^-1)^(3-k).
        const double c = 2.0 * rate;
        const double bn[4] = { 0.0, b1, b2, b3 };
        const double an[4] = { a0, a1, a2, a3 };
        double Bz[4] = { 0, 0, 0, 0 }, Az[4] = { 0, 0, 0, 0 };
        for (int k = 0; k <= 3; ++k)
        {
            double poly[4] = { 1, 0, 0, 0 };
            int len = 1;
            auto mul = [&] (double sign)
            {
                for (int i = len; i > 0; --i) poly[i] += sign * poly[i - 1];
                ++len;
            };
            for (int i = 0; i < k; ++i) mul (-1.0);
            for (int i = 0; i < 3 - k; ++i) mul (+1.0);
            const double ck = std::pow (c, k);
            for (int i = 0; i < 4; ++i) { Bz[i] += bn[k] * ck * poly[i]; Az[i] += an[k] * ck * poly[i]; }
        }
        for (int i = 0; i < 4; ++i) { B[i] = Bz[i] / Az[0]; A[i] = Az[i] / Az[0]; }
    }

    double rate = 48000;
    ToneStackParts parts = stacks::fenderBlackface;
    Smoother sT, sM, sB;
    Iir3 filt;
    bool dirty = true;
};

// ---------------------------------------------------------------------------
// Push-pull power amp. Two pentode halves driven in antiphase: their difference
// cancels even harmonics when balanced, and in class AB produces the crossover
// "grit" when one side switches off. Supply sag: the rectifier can't refill the
// filter caps instantly, so loud passages lower B+ and the headroom with it -
// that is the compression/"bloom" you feel when you dig in.
struct PowerAmpConfig
{
    double bias = 0.35;         // idle conduction: high = class A-ish (AC30), low = class AB
    double exponent = 1.5;      // pentode curve shape
    double feedback = 0.35;     // global negative feedback (0 = none, like an AC30)
    double resonanceHz = 100.0; // speaker resonance seen through the transformer
    double presenceHz = 3500.0;
    double sagAttack = 0.012, sagRelease = 0.25;
    double maxSag = 0.45;       // tube rectifier depth at sag = 1
    double piAsym = 0.08;       // phase inverter asymmetry
};

class PowerAmp
{
public:
    void prepare (double fs, const PowerAmpConfig& c, bool resetState = true)
    {
        cfg = c; rate = fs;
        inCoupling.setCutoff (fs, 12.0); // coupling cap into the phase inverter
        fbHigh.setCutoff (fs, c.presenceHz);
        fbLow.setCutoff (fs, c.resonanceHz * 1.5);
        attack = 1.0 - std::exp (-1.0 / (c.sagAttack * fs));
        release = 1.0 - std::exp (-1.0 / (c.sagRelease * fs));
        const double free = 1.0 - c.feedback; // less feedback = speaker impedance shows more
        resPeak.setPeak (fs, c.resonanceHz, 1.8, 7.0 * free);
        hiRise.setHighShelf (fs, 2500.0, 0.6, 4.0 * free);
        otLow.setCutoff (fs, 45.0);
        otHigh.setCutoff (fs, 14000.0);
        dc.prepare (fs, 8.0);
        // Normalise so small signals pass at unity gain.
        const double d = 1e-3;
        norm = 1.0;
        norm = 1.0 / ((pushPull (d, 1.0) - pushPull (-d, 1.0)) / (2 * d));
        ppCurve.build (-12.0, 12.0, [this] (double s) { return pushPull (s, 1.0); });
        for (auto* s : { &sPres, &sRes, &sSag, &sDrive }) s->prepare (fs, 0.03);
        if (resetState) reset();
    }
    void reset()
    {
        inCoupling.reset(); fbHigh.reset(); fbLow.reset(); resPeak.reset(); hiRise.reset(); otLow.reset(); otHigh.reset(); dc.reset();
        env = 0; prevOut = 0; ppCurve.reset();
    }
    void setControls (double drive, double presence, double resonance, double sag, bool immediate = false)
    {
        if (immediate) { sDrive.setImmediate (drive); sPres.setImmediate (presence); sRes.setImmediate (resonance); sSag.setImmediate (sag); }
        else { sDrive.setTarget (drive); sPres.setTarget (presence); sRes.setTarget (resonance); sSag.setTarget (sag); }
    }

    // Supply scale for upstream preamp stages (they share the sagging B+).
    double supply() const { return 1.0 - 0.35 * env; }

    inline double process (double x)
    {
        const double drive = sDrive.next(), pres = sPres.next(), res = sRes.next(), sag = sSag.next();
        // Negative feedback taken from the speaker output. Presence/resonance cut
        // the feedback in the highs/lows, so those bands come back louder.
        double fb = prevOut;
        const double hi = fb - fbHigh.lp (fb);
        const double lo = fbLow.lp (fb);
        fb = fb - pres * hi - res * lo;
        // Coupling cap: the asymmetric preamp clipping carries DC that must not
        // reach the phase inverter (it would push one side into cutoff and lock up).
        x = inCoupling.hp (x);
        double u = x * drive - cfg.feedback * fb;
        // Phase inverter: soft limiting, slightly asymmetric (one side clips earlier).
        u = u >= 0 ? std::tanh (u) : std::tanh (u * (1.0 + cfg.piAsym)) / (1.0 + cfg.piAsym);
        const double B = 1.0 - cfg.maxSag * sag * env;
        // Push-pull output stage, scaled by the sagging supply: B * PP(u / B).
        double y = B * ppCurve.process (u * 2.2 / B) * norm;
        // Rectifier/filter-cap envelope.
        const double draw = std::min (1.0, std::abs (y) * 0.8);
        env += (draw - env) * (draw > env ? attack : release);
        env = flushDenormal (env);
        // Output transformer bandwidth + speaker impedance interaction.
        y = otLow.hp (y);
        y = otHigh.lp (y);
        y = resPeak.process (y);
        y = hiRise.process (y);
        y = dc.process (y);
        prevOut = y;
        return y;
    }

private:
    inline double tube (double v) const
    {
        // Smooth rectifier (softplus) -> pentode power law -> soft current limit.
        const double a = std::log1p (std::exp (clamp (v * 6.0, -40.0, 40.0))) / 6.0;
        const double p = std::pow (a, cfg.exponent);
        return p / std::cbrt (1.0 + p * p * p);
    }
    inline double pushPull (double u, double B) const
    {
        const double s = u / B;
        return (tube (s + cfg.bias) - tube (-s + cfg.bias)) * B;
    }

    PowerAmpConfig cfg;
    double rate = 48000, attack = 0.01, release = 0.001, env = 0, prevOut = 0, norm = 1;
    OnePole inCoupling, fbHigh, fbLow, otLow, otHigh;
    Biquad resPeak, hiRise;
    DCBlocker dc;
    AdaaCurve ppCurve;
    Smoother sPres, sRes, sSag, sDrive;
};
} // namespace tz
