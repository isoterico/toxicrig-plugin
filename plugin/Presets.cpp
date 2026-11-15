#include "Presets.h"

namespace rig
{
const std::vector<FactoryPreset>& factoryPresets()
{
    // Choice indices: model 0 British, 1 American, 2 Modern; channel 0 Clean, 1 Crunch, 2 Lead;
    // cab 0 British 2x12, 1 American 1x12, 2 Modern 4x12; mic 0 dynamic, 1 ribbon, 2 condenser.
    static const std::vector<FactoryPreset> presets = {
        { "Clean",
          { { "a_model", 1 }, { "a_channel", 0 }, { "a_gain", 0.45f }, { "a_level", 1.0f }, { "a_bass", 0.72f }, { "a_mid", 0.55f }, { "a_treble", 0.48f },
            { "a_bright", 0 }, { "a_master", 0.5f }, { "a_presence", 0.4f }, { "a_cab", 1 }, { "a_mic", 1 }, { "a_mic_pos", 0.45f }, { "a_mic_dist", 0.3f },
            { "gate_on", 0 }, { "comp_on", 1 }, { "comp_sustain", 0.35f }, { "comp_blend", 0.7f },
            { "chorus_on", 1 }, { "chorus_rate", 0.6f }, { "chorus_depth", 0.4f }, { "chorus_mix", 0.35f },
            { "reverb_on", 1 }, { "reverb_mix", 0.25f }, { "reverb_decay", 2.4f } } },
        { "Blues",
          { { "a_model", 1 }, { "a_channel", 1 }, { "a_gain", 0.42f }, { "a_level", 2.0f }, { "a_bass", 0.5f }, { "a_mid", 0.55f }, { "a_treble", 0.55f },
            { "a_master", 0.55f }, { "a_sag", 0.7f }, { "a_cab", 1 }, { "a_mic", 0 }, { "a_mic_pos", 0.4f },
            { "gate_on", 0 }, { "od_on", 1 }, { "od_drive", 0.25f }, { "od_tone", 0.55f }, { "od_level", 4.0f },
            { "delay_on", 1 }, { "delay_time", 320.0f }, { "delay_feedback", 0.25f }, { "delay_mix", 0.18f },
            { "reverb_on", 1 }, { "reverb_mix", 0.2f } } },
        { "Rock",
          { { "a_model", 0 }, { "a_channel", 1 }, { "a_gain", 0.7f }, { "a_level", 6.0f }, { "a_bass", 0.5f }, { "a_mid", 0.5f }, { "a_treble", 0.55f },
            { "a_master", 0.6f }, { "a_presence", 0.55f }, { "a_cab", 0 }, { "a_mic", 0 }, { "a_mic_pos", 0.3f },
            { "gate_thresh", -66.0f }, { "delay_on", 1 }, { "delay_time", 120.0f }, { "delay_feedback", 0.1f }, { "delay_mix", 0.15f },
            { "reverb_on", 1 }, { "reverb_mix", 0.15f } } },
        { "Metal",
          { { "a_model", 2 }, { "a_channel", 2 }, { "a_gain", 0.7f }, { "a_bass", 0.55f }, { "a_mid", 0.45f }, { "a_treble", 0.6f },
            { "a_presence", 0.6f }, { "a_resonance", 0.55f }, { "a_master", 0.55f }, { "a_cab", 2 }, { "a_mic", 0 }, { "a_mic_pos", 0.25f },
            { "gate_on", 1 }, { "gate_thresh", -55.0f }, { "gate_release", 60.0f },
            { "od_on", 1 }, { "od_drive", 0.0f }, { "od_tone", 0.6f }, { "od_level", 6.0f },
            { "reverb_on", 1 }, { "reverb_mix", 0.1f }, { "reverb_decay", 1.5f } } },
        { "Djent",
          { { "a_model", 2 }, { "a_channel", 2 }, { "a_gain", 0.6f }, { "a_bass", 0.45f }, { "a_mid", 0.6f }, { "a_treble", 0.55f },
            { "a_presence", 0.7f }, { "a_resonance", 0.6f }, { "a_master", 0.5f }, { "a_cab", 2 }, { "a_mic", 0 }, { "a_mic_pos", 0.2f }, { "a_mic_dist", 0.1f },
            { "gate_on", 1 }, { "gate_thresh", -48.0f }, { "gate_release", 40.0f },
            { "od_on", 1 }, { "od_drive", 0.0f }, { "od_tone", 0.7f }, { "od_level", 8.0f },
            { "eq_on", 1 }, { "eq_b2", -3.0f }, { "eq_b4", 2.0f }, { "eq_b7", -2.0f } } },
        { "Red Queen",
          { { "a_model", 0 }, { "a_channel", 1 }, { "a_gain", 0.75f }, { "a_level", 4.0f }, { "a_bass", 0.45f }, { "a_mid", 0.5f }, { "a_treble", 0.6f },
            { "a_presence", 0.45f }, { "a_master", 0.7f }, { "a_sag", 0.6f }, { "a_cab", 0 }, { "a_mic", 0 }, { "a_mic_pos", 0.35f },
            { "gate_on", 0 }, { "boost_on", 1 }, { "boost_amount", 0.75f }, { "boost_range", 0 },
            { "delay_on", 1 }, { "delay_time", 800.0f }, { "delay_feedback", 0.35f }, { "delay_mix", 0.3f },
            { "reverb_on", 1 }, { "reverb_mix", 0.2f } } },
    };
    return presets;
}
} // namespace rig
