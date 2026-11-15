// Offline verification of the shipping processor code. Writes a report and audio
// renders to the folder given as argv[1] (default ./test-out).
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DiSynth.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <chrono>

namespace
{
juce::File outDir;
juce::String report;
int failures = 0;

void log (const juce::String& s) { std::cout << s << std::endl; report << s << "\n"; }
void check (bool ok, const juce::String& what) { if (! ok) { ++failures; log ("  !! FAIL: " + what); } }

void setParam (ToxicRigProcessor& p, const juce::String& id, float value)
{
    if (auto* prm = p.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (value));
    else { log ("unknown param " + id); ++failures; }
}

void allPedalsOff (ToxicRigProcessor& p)
{
    for (int i = 0; i < tz::kNumPedals; ++i) setParam (p, rig::pedalOnId (i), 0.0f);
}

struct Rendered { juce::AudioBuffer<float> audio; double seconds = 0; };

// Feeds a mono signal through the processor in blocks; returns stereo output.
Rendered run (ToxicRigProcessor& p, const std::vector<float>& in, int block,
              std::function<void (int, ToxicRigProcessor&)> perBlock = {})
{
    const int n = (int) in.size();
    Rendered r;
    r.audio.setSize (2, n);
    juce::AudioBuffer<float> buf (2, block);
    const auto t0 = std::chrono::high_resolution_clock::now();
    for (int pos = 0, b = 0; pos < n; pos += block, ++b)
    {
        if (perBlock) perBlock (b, p);
        const int len = std::min (block, n - pos);
        buf.setSize (2, len, false, false, true);
        for (int c = 0; c < 2; ++c) buf.copyFrom (c, 0, in.data() + pos, len); // stereo input, same signal
        juce::MidiBuffer midi;
        p.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c) r.audio.copyFrom (c, pos, buf, c, 0, len);
    }
    r.seconds = std::chrono::duration<double> (std::chrono::high_resolution_clock::now() - t0).count();
    return r;
}

void prepare (ToxicRigProcessor& p, double fs, int block)
{
    p.setRateAndBufferSizeDetails (fs, block);
    p.prepareToPlay (fs, block);
    juce::Thread::sleep (400); // convolution IRs load on a background thread
}

void writeWav (const juce::String& name, const juce::AudioBuffer<float>& b, double fs)
{
    auto f = outDir.getChildFile (name);
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (f.createOutputStream().release());
    if (auto* w = wav.createWriterFor (os.get(), fs, (unsigned) b.getNumChannels(), 24, {}, 0))
    {
        os.release();
        std::unique_ptr<juce::AudioFormatWriter> writer (w);
        writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    }
}

struct Stats { float peak = 0; double rms = 0; bool finite = true; int nonFinite = 0; };
Stats stats (const juce::AudioBuffer<float>& b, int from = 0)
{
    Stats s; double sum = 0; int cnt = 0;
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = from; i < b.getNumSamples(); ++i)
        {
            const float v = b.getSample (c, i);
            if (! std::isfinite (v)) { s.finite = false; ++s.nonFinite; continue; }
            s.peak = std::max (s.peak, std::abs (v)); sum += (double) v * v; ++cnt;
        }
    s.rms = std::sqrt (sum / std::max (1, cnt));
    return s;
}
juce::String db (double g) { return juce::String (juce::Decibels::gainToDecibels ((float) g, -120.0f), 1) + " dB"; }

// Aliasing: power at frequencies that are NOT harmonics of f0, relative to total.
// Harmonics land exactly on k*f0; anything folded back from above Nyquist lands
// in between - that's what aliasing sounds like (inharmonic, "digital" fizz).
double aliasingDb (const float* x, int n, double fs, double f0)
{
    const int order = 15, N = 1 << order;
    jassert (n >= N);
    juce::dsp::FFT fft (order);
    std::vector<float> data ((size_t) N * 2, 0.0f);
    for (int i = 0; i < N; ++i)
    {
        const double w = 0.35875 - 0.48829 * std::cos (2 * juce::MathConstants<double>::pi * i / (N - 1))
                       + 0.14128 * std::cos (4 * juce::MathConstants<double>::pi * i / (N - 1))
                       - 0.01168 * std::cos (6 * juce::MathConstants<double>::pi * i / (N - 1)); // Blackman-Harris
        data[(size_t) i] = (float) (x[n - N + i] * w);
    }
    fft.performFrequencyOnlyForwardTransform (data.data());
    double harm = 0, other = 0;
    const double binHz = fs / N;
    for (int k = 2; k < N / 2; ++k)
    {
        const double f = k * binHz;
        if (f > 20000.0 || f < 30.0) continue;
        const double pw = (double) data[(size_t) k] * data[(size_t) k];
        const double h = f / f0;
        const bool isHarm = std::abs (h - std::round (h)) * f0 < 6.0 * binHz;
        (isHarm ? harm : other) += pw;
    }
    return 10.0 * std::log10 ((other + 1e-30) / (harm + other + 1e-30));
}

// Largest jump in the 2nd difference: a click shows up as a spike here.
double clickMetric (const juce::AudioBuffer<float>& b, int from, int to)
{
    double m = 0;
    for (int c = 0; c < b.getNumChannels(); ++c)
    {
        const float* x = b.getReadPointer (c);
        for (int i = std::max (2, from); i < to; ++i) m = std::max (m, (double) std::abs (x[i] - 2 * x[i - 1] + x[i - 2]));
    }
    return m;
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    outDir = juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "test-out");
    outDir.createDirectory();
    const auto riff = test::testRiff();
    const double riffLen = 16.5;

    if (argc > 2 && juce::String (argv[2]) == "snapshot")
    {
        // Renders the editor (each main view) to PNG files, to check the UI headless.
        ToxicRigProcessor p;
        p.applyFactoryPreset (5);
        prepare (p, 48000.0, 256);
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        ed->setSize (ui::Root::W, ui::Root::H);
        auto save = [&] (const juce::String& name)
        {
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            juce::PNGImageFormat png;
            auto f = outDir.getChildFile (name);
            f.deleteFile();
            if (auto os = f.createOutputStream()) png.writeImageToStream (img, *os);
        };
        auto& root = dynamic_cast<ToxicRigEditor&> (*ed).root;
        const char* names[] = { "toxiclab", "studionoir", "schematic" };
        for (int sk = 0; sk < 3; ++sk)
        {
            root.setSkin ((ui::SkinId) sk);
            save (juce::String ("ui_") + names[sk] + "_main.png");
            if (auto* st = root.stage())
            {
                st->openPedal ((int) tz::PedalId::Delay); save ("ui_toxiclab_pedal.png");
                st->openCab(); save ("ui_toxiclab_cab.png");
                st->closeOverlay();
            }
            if (auto* cl = root.classic())
            {
                cl->select (101); save (juce::String ("ui_") + names[sk] + "_cab.png");
                cl->select ((int) tz::PedalId::Overdrive); save (juce::String ("ui_") + names[sk] + "_pedal.png");
            }
        }
        root.setSkin (ui::SkinId::ToxicLab);
        // British amp with amp B turned on, to check the knobs follow the artwork.
        setParam (p, "a_model", 1.0f);
        root.setSkin (ui::SkinId::ToxicLab);
        save ("ui_toxiclab_american.png");
        setParam (p, "a_model", 2.0f);
        root.setSkin (ui::SkinId::ToxicLab);
        save ("ui_toxiclab_modern.png");
        return 0;
    }
    log ("=== ToxicRig offline tests ===");
    const bool onlyClicks = argc > 2 && juce::String (argv[2]) == "clicks";
    if (! onlyClicks) {

    // ------------------------------------------------------------------ 1. presets x rates
    log ("\n[1] Factory presets at 44.1 / 48 / 96 kHz (block 256), synthetic DI riff");
    const bool onlyRenders = argc > 2 && juce::String (argv[2]) == "renders";
    for (double fs : { 44100.0, 48000.0, 96000.0 })
    {
        if (onlyRenders && fs != 48000.0) continue;
        const auto di = test::renderDI (fs, riffLen, riff);
        if (fs == 48000.0)
        {
            juce::AudioBuffer<float> d (1, (int) di.size());
            std::copy (di.begin(), di.end(), d.getWritePointer (0));
            writeWav ("00_DI_input.wav", d, fs);
        }
        for (int pi = 0; pi < (int) rig::factoryPresets().size(); ++pi)
        {
            ToxicRigProcessor p;
            p.applyFactoryPreset (pi);
            prepare (p, fs, 256);
            auto r = run (p, di, 256);
            const auto s = stats (r.audio);
            const auto name = rig::factoryPresets()[(size_t) pi].name;
            log ("  " + juce::String (fs / 1000.0, 1) + " kHz  " + name.paddedRight (' ', 10) + " peak " + db (s.peak) + "  rms " + db (s.rms)
                 + "  RT x" + juce::String (riffLen / r.seconds, 1));
            check (s.finite, name + ": non-finite samples");
            check (s.peak < 1.5f, name + ": output too hot");
            check (s.rms > 0.003, name + ": output (almost) silent");
            if (fs == 48000.0) writeWav ("0" + juce::String (pi + 1) + "_" + name.replace (" ", "_") + ".wav", r.audio, fs);
        }
    }

    if (onlyRenders) return 0;
    // ------------------------------------------------------------------ 2. block sizes
    log ("\n[2] Buffer sizes 32..1024 (Metal + amp B + every pedal on, 4x)");
    for (double fs : { 44100.0, 48000.0, 96000.0 })
    {
        const auto di = test::renderDI (fs, 8.0, riff);
        juce::AudioBuffer<float> ref;
        for (int block : { 1024, 512, 256, 128, 64, 32 })
        {
            ToxicRigProcessor p;
            p.applyFactoryPreset (3);
            setParam (p, rig::ids::ampBOn, 1.0f);
            for (int i = 0; i < tz::kNumPedals; ++i) setParam (p, rig::pedalOnId (i), 1.0f);
            prepare (p, fs, block);
            auto r = run (p, di, block);
            const auto s = stats (r.audio);
            double diff = 0;
            if (ref.getNumSamples() == 0) ref = r.audio;
            else
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < r.audio.getNumSamples(); ++i)
                        diff = std::max (diff, (double) std::abs (r.audio.getSample (c, i) - ref.getSample (c, i)));
            log ("  " + juce::String (fs / 1000.0, 1) + " kHz block " + juce::String (block).paddedLeft (' ', 4) + "  peak " + db (s.peak)
                 + "  max diff vs 1024 " + db (diff) + "  RT x" + juce::String (8.0 / r.seconds, 1));
            check (s.finite && s.peak < 2.0f && s.rms > 0.003, "block " + juce::String (block) + " output invalid");
        }
    }

    // ------------------------------------------------------------------ 3. aliasing
    log ("\n[3] Aliasing, Modern Lead gain 100% + overdrive, sine -6 dBFS (inharmonic energy, lower = better)");
    log ("    measured before the cab (worst case) and after the cab (what you hear)");
    for (double fs : { 44100.0, 48000.0 })
        for (double f0 : { 1318.5, 2637.0, 3951.1 })
        {
            juce::String line = "  " + juce::String (fs / 1000.0, 1) + " kHz  " + juce::String (f0, 0).paddedLeft (' ', 5) + " Hz:";
            for (int os = 0; os < 3; ++os)
                for (int cab = 1; cab >= 0; --cab)
                {
                    ToxicRigProcessor p;
                    p.applyFactoryPreset (3);
                    allPedalsOff (p);
                    setParam (p, "od_on", 1.0f); setParam (p, "od_drive", 0.6f);
                    setParam (p, "a_gain", 1.0f);
                    setParam (p, rig::ids::oversampling, (float) os);
                    setParam (p, "a_cab_on", (float) cab);
                    prepare (p, fs, 512);
                    std::vector<float> sine ((size_t) (fs * 1.5));
                    for (size_t i = 0; i < sine.size(); ++i) sine[i] = (float) (0.5 * std::sin (2 * juce::MathConstants<double>::pi * f0 * (double) i / fs));
                    auto r = run (p, sine, 512);
                    const double a = aliasingDb (r.audio.getReadPointer (0), r.audio.getNumSamples(), fs, f0);
                    line << "  " << rig::osNames[os] << (cab ? " cab " : " raw ") << juce::String (a, 1);
                    if (os == 1) check (a < -45.0, "aliasing at 4x too high (" + juce::String (a, 1) + " dB)");
                }
            log (line);
        }

    }
    // ------------------------------------------------------------------ 4. smoothing / clicks
    log ("\n[4] Clicks / zipper noise (2nd-difference spike, relative to the same audio without changes)");
    {
        const double fs = 48000.0;
        std::vector<float> sine ((size_t) (fs * 8.0));
        for (size_t i = 0; i < sine.size(); ++i) sine[i] = (float) (0.25 * std::sin (2 * juce::MathConstants<double>::pi * 220.0 * (double) i / fs));
        // A click is a sample-to-sample jump bigger than anything the sound itself
        // does. So: compare the 2nd-difference peak right around each change with
        // the peak in the steady parts of the SAME render (all settings involved).
        auto measure = [&] (const juce::String& label, std::function<bool (int, ToxicRigProcessor&)> mover)
        {
            ToxicRigProcessor p;
            p.applyFactoryPreset (0);
            prepare (p, fs, 64);
            std::vector<int> events;
            const auto r = run (p, sine, 64, [&] (int b, ToxicRigProcessor& pp) { if (mover (b, pp)) events.push_back (b * 64); });
            const int total = r.audio.getNumSamples(), from = (int) (0.3 * fs);
            const int before = 64, after = (int) (0.05 * fs), guard = (int) (0.1 * fs);
            double nearPeak = 0, steady = 0; int nearAt = 0;
            for (int i = std::max (2, from); i < total; ++i)
            {
                bool inWin = false, inGuard = false;
                for (int e : events) { if (i >= e - before && i < e + after) inWin = true; if (i >= e - guard && i < e + guard) inGuard = true; }
                double v = 0;
                for (int c = 0; c < 2; ++c) { const float* x = r.audio.getReadPointer (c); v = std::max (v, (double) std::abs (x[i] - 2 * x[i - 1] + x[i - 2])); }
                if (inWin) { if (v > nearPeak) { nearPeak = v; nearAt = i; } } else if (! inGuard) steady = std::max (steady, v);
            }
            const double ratio = nearPeak / std::max (1e-9, steady);
            int off = 0; for (int e : events) if (nearAt >= e - before && nearAt < e + after) off = nearAt - e;
            log ("  " + label.paddedRight (' ', 40) + " " + juce::String ((int) events.size()) + " changes, worst jump " + juce::String (ratio, 2)
                 + "x the steady sound (at +" + juce::String (off * 1000.0 / fs, 1) + " ms)");
            return ratio;
        };
        const double r1 = measure ("Gain knob jumps 0 <-> 100%", [] (int b, ToxicRigProcessor& p)
        { if (b > 300 && b % 400 == 0) { setParam (p, "a_gain", (b / 400) % 2 ? 1.0f : 0.0f); return true; } return false; });
        double r2 = 0;
        for (auto id : { "a_bass", "a_mid", "a_treble", "a_presence", "a_master" })
            r2 = std::max (r2, measure (juce::String ("Knob ") + id + " jumping min <-> max", [id] (int b, ToxicRigProcessor& p)
            { if (b > 300 && b % 400 == 0) { setParam (p, id, (b / 400) % 2 ? 1.0f : 0.0f); return true; } return false; }));
        const double r3 = measure ("Pedals on/off (chorus, comp, delay)", [] (int b, ToxicRigProcessor& p)
        { if (b > 300 && b % 400 == 0) { const float v = (b / 400) % 2 ? 0.0f : 1.0f; setParam (p, "chorus_on", v); setParam (p, "comp_on", v); setParam (p, "delay_on", v); return true; } return false; });
        const double r4 = measure ("Amp model / channel switch", [] (int b, ToxicRigProcessor& p)
        { if (b > 300 && b % 400 == 200) { setParam (p, "a_model", (float) ((b / 400) % 3)); setParam (p, "a_channel", (float) ((b / 400 + 1) % 3)); return true; } return false; });
        const double r5 = measure ("Reorder pedals + oversampling change", [] (int b, ToxicRigProcessor& p)
        {
            if (b > 300 && b % 400 == 200)
            {
                auto r = rig::Routing::defaults();
                std::swap (r.board[0], r.board[1]);
                if ((b / 400) % 2) p.setRouting (r); else p.setRouting (rig::Routing::defaults());
                setParam (p, rig::ids::oversampling, (float) ((b / 400) % 3));
                return true;
            }
            return false;
        });
        const double r5a = measure ("Reorder only", [] (int b, ToxicRigProcessor& p)
        { if (b > 300 && b % 400 == 200) { auto r = rig::Routing::defaults(); std::swap (r.board[0], r.board[1]); if ((b / 400) % 2) p.setRouting (r); else p.setRouting (rig::Routing::defaults()); return true; } return false; });
        const double r5b = measure ("Oversampling change only", [] (int b, ToxicRigProcessor& p)
        { if (b > 300 && b % 400 == 200) { setParam (p, rig::ids::oversampling, (float) ((b / 400) % 3)); return true; } return false; });
        const double r6 = measure ("Preset changes", [] (int b, ToxicRigProcessor& p)
        { if (b > 300 && b % 400 == 0) { p.applyFactoryPreset ((b / 400) % (int) rig::factoryPresets().size()); return true; } return false; });
        check (r1 < 1.5 && r2 < 1.5 && r3 < 1.5, "zipper noise on knob / pedal changes");
        check (r4 < 1.5, "click on amp model / channel switch");
        // Reorder / oversampling / preset changes use a 12 ms fade; what remains is
        // below ~-60 dBFS, but this metric is extremely sensitive, so the bar is 5x.
        check (r5 < 5.0 && r5a < 5.0 && r5b < 5.0 && r6 < 5.0, "click on reorder / oversampling / preset change");
    }

    if (! onlyClicks) {
    // ------------------------------------------------------------------ 5. latency
    log ("\n[5] Latency: reported vs measured, same for every oversampling factor");
    for (double fs : { 44100.0, 48000.0, 96000.0 })
    {
        juce::String line = "  " + juce::String (fs / 1000.0, 1) + " kHz:";
        for (int os = 0; os < 3; ++os)
        {
            ToxicRigProcessor p;
            p.applyFactoryPreset (0);
            allPedalsOff (p);
            setParam (p, "a_cab_on", 0.0f);
            setParam (p, "a_gain", 0.2f);
            setParam (p, rig::ids::oversampling, (float) os);
            prepare (p, fs, 128);
            std::vector<float> click ((size_t) (fs * 0.5), 0.0f);
            const int at = (int) (fs * 0.2);
            click[(size_t) at] = 0.3f;
            auto r = run (p, click, 128);
            int best = 0; float bv = 0;
            for (int i = 0; i < r.audio.getNumSamples(); ++i) if (std::abs (r.audio.getSample (0, i)) > bv) { bv = std::abs (r.audio.getSample (0, i)); best = i; }
            line << "  " << rig::osNames[os] << " reported " << p.getLatencySamples() << " peak at +" << (best - at);
        }
        log (line);
    }

    // ------------------------------------------------------------------ 6. CPU
    log ("\n[6] CPU (this cloud machine, 1 core): real-time factor = seconds of audio per second of CPU");
    for (int os = 0; os < 3; ++os)
    {
        const double fs = 48000.0;
        const auto di = test::renderDI (fs, 8.0, riff);
        ToxicRigProcessor p;
        p.applyFactoryPreset (3);
        setParam (p, rig::ids::ampBOn, 1.0f);
        for (int i = 0; i < tz::kNumPedals; ++i) setParam (p, rig::pedalOnId (i), 1.0f);
        setParam (p, rig::ids::oversampling, (float) os);
        prepare (p, fs, 128);
        auto r = run (p, di, 128);
        log ("  48 kHz, 2 amps + 11 pedals + 2 cabs, " + rig::osNames[os] + ":  x" + juce::String (8.0 / r.seconds, 1) + " real time ("
             + juce::String (100.0 * r.seconds / 8.0, 1) + "% of one core)");
    }

    // ------------------------------------------------------------------ 7. state
    log ("\n[7] Save/restore state");
    {
        ToxicRigProcessor a, b;
        a.applyFactoryPreset (5);
        auto r = rig::Routing::defaults(); std::swap (r.loop[0], r.loop[4]); a.setRouting (r);
        juce::MemoryBlock mb; a.getStateInformation (mb);
        b.setStateInformation (mb.getData(), (int) mb.getSize());
        const bool same = a.getRouting().encode() == b.getRouting().encode()
                       && std::abs (a.apvts.getRawParameterValue ("a_gain")->load() - b.apvts.getRawParameterValue ("a_gain")->load()) < 1e-6f
                       && a.apvts.getRawParameterValue ("boost_on")->load() == b.apvts.getRawParameterValue ("boost_on")->load();
        log (juce::String ("  state round-trip: ") + (same ? "OK" : "MISMATCH"));
        check (same, "state round trip");
    }

    }
    log ("\n=== " + juce::String (failures) + " failure(s) ===");
    outDir.getChildFile ("report.txt").replaceWithText (report);
    return failures == 0 ? 0 : 1;
}
