#include "Params.h"

namespace rig
{
const std::vector<PedalSpec>& pedalSpecs()
{
    static const std::vector<PedalSpec> specs = {
        { tz::PedalId::Gate, "gate", "Noise Gate",
          "Taglia il fruscio quando non suoni: si apre sopra la soglia e si richiude 6 dB sotto (niente tremolii).",
          { { "thresh", "Threshold", -90.f, -10.f, -62.f, 0.f, "dB" }, { "release", "Release", 5.f, 500.f, 80.f, 60.f, "ms" } } },
        { tz::PedalId::Comp, "comp", "Sustainer",
          "Compressore: abbassa i picchi e alza le code, le note durano di più e il suono è più uniforme.",
          { { "sustain", "Sustain", 0.f, 1.f, 0.5f, 0.f, "" }, { "attack", "Attack", 0.5f, 50.f, 8.f, 8.f, "ms" },
            { "level", "Level", -12.f, 12.f, 0.f, 0.f, "dB" }, { "blend", "Blend", 0.f, 1.f, 1.f, 0.f, "" } } },
        { tz::PedalId::Boost, "boost", "Treble Booster",
          "Booster al germanio: lascia passare solo medio-alti e li spinge nell'ampli. Il lead 'cantato' anni '70.",
          { { "amount", "Boost", 0.f, 1.f, 0.6f, 0.f, "" }, { "range", "Range", 0.f, 2.f, 0.f, 0.f, "", { "Treble", "Mid", "Full" } },
            { "level", "Level", -24.f, 12.f, 0.f, 0.f, "dB" } } },
        { tz::PedalId::Overdrive, "od", "Green Drive",
          "Overdrive tipo Tube Screamer: i bassi passano puliti, i medi vengono spinti e saturati. Stringe il suono.",
          { { "drive", "Drive", 0.f, 1.f, 0.4f, 0.f, "" }, { "tone", "Tone", 0.f, 1.f, 0.5f, 0.f, "" },
            { "level", "Level", -24.f, 12.f, 0.f, 0.f, "dB" } } },
        { tz::PedalId::Fuzz, "fuzz", "Toxic Fuzz",
          "Fuzz a transistor: guadagno enorme e clipping duro. Bias 'affama' il transistor per un fuzz sporco e gated.",
          { { "amount", "Fuzz", 0.f, 1.f, 0.7f, 0.f, "" }, { "tone", "Tone", 0.f, 1.f, 0.5f, 0.f, "" },
            { "level", "Level", -24.f, 12.f, -6.f, 0.f, "dB" }, { "bias", "Bias", 0.f, 1.f, 0.f, 0.f, "" } } },
        { tz::PedalId::EQ, "eq", "Graphic EQ",
          "Equalizzatore a 7 bande (100 Hz - 6.4 kHz). Prima dell'ampli cambia cosa satura, dopo cambia solo il timbro.",
          { { "b1", "100", -12.f, 12.f, 0.f, 0.f, "dB" }, { "b2", "200", -12.f, 12.f, 0.f, 0.f, "dB" },
            { "b3", "400", -12.f, 12.f, 0.f, 0.f, "dB" }, { "b4", "800", -12.f, 12.f, 0.f, 0.f, "dB" },
            { "b5", "1.6k", -12.f, 12.f, 0.f, 0.f, "dB" }, { "b6", "3.2k", -12.f, 12.f, 0.f, 0.f, "dB" },
            { "b7", "6.4k", -12.f, 12.f, 0.f, 0.f, "dB" }, { "level", "Level", -12.f, 12.f, 0.f, 0.f, "dB" } } },
        { tz::PedalId::Chorus, "chorus", "Chorus",
          "Copie del suono leggermente ritardate e modulate: effetto 'più chitarre insieme', allarga lo stereo.",
          { { "rate", "Rate", 0.05f, 8.f, 0.8f, 1.f, "Hz" }, { "depth", "Depth", 0.f, 1.f, 0.5f, 0.f, "" },
            { "mix", "Mix", 0.f, 1.f, 0.5f, 0.f, "" } } },
        { tz::PedalId::Phaser, "phaser", "Phaser",
          "Filtri allpass che si muovono: creano 'buchi' nello spettro che scorrono su e giù (suono che gira).",
          { { "rate", "Rate", 0.02f, 8.f, 0.4f, 1.f, "Hz" }, { "depth", "Depth", 0.f, 1.f, 0.8f, 0.f, "" },
            { "feedback", "Feedback", 0.f, 1.f, 0.4f, 0.f, "" }, { "mix", "Mix", 0.f, 1.f, 1.f, 0.f, "" } } },
        { tz::PedalId::Tremolo, "trem", "Tremolo",
          "Il volume sale e scende a tempo. Shape va da onda morbida (vintage) a quadra (stacco netto).",
          { { "rate", "Rate", 0.5f, 15.f, 5.f, 4.f, "Hz" }, { "depth", "Depth", 0.f, 1.f, 0.6f, 0.f, "" },
            { "shape", "Shape", 0.f, 1.f, 0.2f, 0.f, "" } } },
        { tz::PedalId::Delay, "delay", "Delay",
          "Eco con ripetizioni che si scuriscono come un analogico. Sync lo aggancia al tempo di FL Studio.",
          { { "time", "Time", 10.f, 2000.f, 380.f, 400.f, "ms" }, { "sync", "Sync", 0.f, 1.f, 0.f, 0.f, "", {}, true },
            { "div", "Division", 0.f, 6.f, 5.f, 0.f, "", { "1/2", "1/4", "1/4 .", "1/4 T", "1/8", "1/8 .", "1/16" } },
            { "feedback", "Feedback", 0.f, 0.95f, 0.35f, 0.f, "" }, { "mix", "Mix", 0.f, 1.f, 0.25f, 0.f, "" },
            { "tone", "Tone", 0.f, 1.f, 0.5f, 0.f, "" }, { "pingpong", "Ping-Pong", 0.f, 1.f, 0.f, 0.f, "", {}, true } } },
        { tz::PedalId::Reverb, "reverb", "Reverb",
          "Riverbero a rete di ritardi (FDN): stanza o hall. Damping scurisce le code come in un ambiente vero.",
          { { "mix", "Mix", 0.f, 1.f, 0.25f, 0.f, "" }, { "decay", "Decay", 0.2f, 12.f, 2.2f, 2.f, "s" },
            { "size", "Size", 0.f, 1.f, 0.6f, 0.f, "" }, { "predelay", "Pre-Delay", 0.f, 200.f, 20.f, 40.f, "ms" },
            { "damping", "Damping", 0.f, 1.f, 0.4f, 0.f, "" } } },
        { tz::PedalId::NuclearFuzz, "nfuzz", "Nuclear Fuzz",
          "Fuzz industriale a due stadi con gate integrato: il gate ascolta la chitarra prima del fuzz e taglia il rumore appena fermi le corde.",
          { { "drive", "Drive", 0.f, 1.f, 0.7f, 0.f, "" }, { "gate", "Gate", 0.f, 1.f, 0.4f, 0.f, "" },
            { "volume", "Volume", -24.f, 12.f, -6.f, 0.f, "dB" } } },
        { tz::PedalId::BioBoost, "bioboost", "Bio-Boost",
          "Boost pulito fino a +20 dB per spingere i soli. CLEAN spinge un po' i medi, TREBLE toglie i bassi per un boost brillante.",
          { { "boost", "Boost", 0.f, 1.f, 0.5f, 0.f, "" }, { "mode", "Mode", 0.f, 1.f, 0.f, 0.f, "", { "Clean", "Treble" } } } },
        { tz::PedalId::MutantMod, "mutant", "Mutant Mod",
          "Modulazione tre-in-uno: Chorus liquido, Flanger con feedback o Vibrato (solo segnale modulato). Speed = velocità dell'LFO.",
          { { "mode", "Mode", 0.f, 2.f, 0.f, 0.f, "", { "Chorus", "Flanger", "Vibrato" } },
            { "speed", "Speed", 0.05f, 10.f, 0.9f, 1.f, "Hz" } } },
    };
    return specs;
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto floatParam = [&] (const String& id, const String& name, float min, float max, float def, float centre, const String& unit)
    {
        NormalisableRange<float> range (min, max);
        if (centre > min && centre < max) range.setSkewForCentre (centre);
        // Display: 0..1 knobs read 0-10 like on a real amp; others show their unit.
        const bool unitKnob = min == 0.0f && max == 1.0f && unit.isEmpty();
        auto toText = [unitKnob, unit] (float v, int)
        {
            if (unitKnob) return String (v * 10.0f, 1);
            if (unit == "ms" && v >= 1000.0f) return String (v / 1000.0f, 2) + " s";
            return String (v, std::abs (v) < 10.0f ? 2 : 1) + (unit.isNotEmpty() ? " " + unit : String());
        };
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, range, def,
                                                           AudioParameterFloatAttributes().withLabel (unit).withStringFromValueFunction (toText)));
    };
    auto choiceParam = [&] (const String& id, const String& name, const StringArray& c, int def)
    { layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, c, def)); };
    // On/off switches. juce::AudioParameterBool keeps whatever in-between value a host writes (0.47...)
    // and a later state restore that says "0" may not overwrite it, so a pedal could come back in the
    // wrong state. A two-step choice always snaps to exactly 0 or 1; the saved value is still 0/1, so
    // sessions written by earlier versions load unchanged, and hosts still see a switch.
    struct SwitchParam final : AudioParameterChoice
    {
        SwitchParam (const String& id, const String& name, bool def)
            : AudioParameterChoice (ParameterID { id, 1 }, name, StringArray { "Off", "On" }, def ? 1 : 0) {}
        bool isBoolean() const override { return true; }
    };
    auto boolParam = [&] (const String& id, const String& name, bool def)
    { layout.add (std::make_unique<SwitchParam> (id, name, def)); };

    floatParam (ids::inGain, "Input Gain", -24.f, 24.f, 0.f, 0.f, "dB");
    floatParam (ids::outVol, "Output Volume", -48.f, 12.f, 0.f, 0.f, "dB");
    choiceParam (ids::oversampling, "Oversampling", osNames, 1);
    boolParam (ids::ampBOn, "Amp B On", false);
    floatParam (ids::abMix, "Amp A/B Mix", 0.f, 1.f, 0.5f, 0.f, "");
    floatParam (ids::width, "Stereo Width", 0.f, 1.f, 0.7f, 0.f, "");

    for (char slot : { 'a', 'b' })
    {
        const String S = String::charToString (slot).toUpperCase();
        choiceParam (ampId (slot, "model"), "Amp " + S + " Model", ampModels, slot == 'a' ? 0 : 2);
        choiceParam (ampId (slot, "channel"), "Amp " + S + " Channel", ampChannels, 1);
        floatParam (ampId (slot, "gain"), "Amp " + S + " Gain", 0.f, 1.f, 0.5f, 0.f, "");
        floatParam (ampId (slot, "bass"), "Amp " + S + " Bass", 0.f, 1.f, 0.5f, 0.f, "");
        floatParam (ampId (slot, "mid"), "Amp " + S + " Middle", 0.f, 1.f, 0.5f, 0.f, "");
        floatParam (ampId (slot, "treble"), "Amp " + S + " Treble", 0.f, 1.f, 0.5f, 0.f, "");
        floatParam (ampId (slot, "presence"), "Amp " + S + " Presence/Cut", 0.f, 1.f, 0.5f, 0.f, "");
        floatParam (ampId (slot, "resonance"), "Amp " + S + " Resonance/Depth", 0.f, 1.f, 0.5f, 0.f, "");
        floatParam (ampId (slot, "master"), "Amp " + S + " Master", 0.f, 1.f, 0.5f, 0.f, "");
        floatParam (ampId (slot, "sag"), "Amp " + S + " Sag", 0.f, 1.f, 0.5f, 0.f, "");
        boolParam (ampId (slot, "bright"), "Amp " + S + " Bright", false);
        floatParam (ampId (slot, "level"), "Amp " + S + " Level", -24.f, 12.f, 0.f, 0.f, "dB");
        boolParam (ampId (slot, "cab_on"), "Cab " + S + " On", true);
        choiceParam (ampId (slot, "cab"), "Cab " + S + " Type", cabNames, slot == 'a' ? 0 : 2);
        choiceParam (ampId (slot, "mic"), "Cab " + S + " Mic", micNames, 0);
        floatParam (ampId (slot, "mic_pos"), "Cab " + S + " Mic Position", 0.f, 1.f, 0.3f, 0.f, "");
        floatParam (ampId (slot, "mic_dist"), "Cab " + S + " Mic Distance", 0.f, 1.f, 0.2f, 0.f, "");
        // Model-specific controls (only the models that have them on the panel use them).
        boolParam (ampId (slot, "tight"), "Amp " + S + " Tight (Overlord)", true);
        floatParam (ampId (slot, "attack"), "Amp " + S + " Attack (Overlord)", 0.f, 1.f, 0.5f, 0.f, "");
        for (int b = 0; b < 8; ++b)
            floatParam (ampId (slot, ("eq" + String (b + 1)).toRawUTF8()), "Amp " + S + " EQ " + overlordEqNames[b] + " (Overlord)", -12.f, 12.f, 0.f, 0.f, "dB");
        floatParam (ampId (slot, "vapor"), "Amp " + S + " Depth (Vapor-30)", 0.f, 1.f, 0.4f, 0.f, "");
        floatParam (ampId (slot, "bloom"), "Amp " + S + " Resonance (Vapor-30)", 0.f, 1.f, 0.5f, 0.f, "");
        floatParam (ampId (slot, "amb"), "Amp " + S + " Presence D/R (Vapor-30)", 0.f, 1.f, 0.25f, 0.f, "");
        floatParam (ampId (slot, "amb_width"), "Amp " + S + " Width (Vapor-30)", 0.f, 1.f, 0.7f, 0.f, "");
        boolParam (ampId (slot, "standby"), "Amp " + S + " Standby (Vapor-30)", false);
        boolParam (ampId (slot, "power"), "Amp " + S + " Power (Vapor-30)", true);
    }

    const auto& specs = pedalSpecs();
    for (int p = 0; p < (int) specs.size(); ++p)
    {
        const auto& s = specs[(size_t) p];
        boolParam (pedalOnId (p), String (s.displayName) + " On", p == (int) tz::PedalId::Gate);
        for (int i = 0; i < (int) s.params.size(); ++i)
        {
            const auto& ps = s.params[(size_t) i];
            const String id = pedalParamId (p, i), name = String (s.displayName) + " " + ps.name;
            if (ps.isBool) boolParam (id, name, ps.def > 0.5f);
            else if (! ps.choices.isEmpty()) choiceParam (id, name, ps.choices, (int) ps.def);
            else floatParam (id, name, ps.min, ps.max, ps.def, ps.centre, ps.unit);
        }
    }
    return layout;
}
} // namespace rig
