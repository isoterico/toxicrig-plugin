#include "Stage.h"
#include "BinaryData.h"

namespace ui
{
namespace
{
const juce::Colour kGreen { 0xff39ff14 }, kPurple { 0xff9b4dff };

juce::Image img (const void* d, int n) { return juce::ImageCache::getFromMemory (d, n); }

// --- Amp faceplates: knob centres measured on the artwork (image pixels, 1272x660).
struct AmpLayout { std::array<juce::Point<float>, 8> knobs; float knobSize; KnobStyle style; juce::Rectangle<float> bright; juce::Colour labelCol; float labelY; };

const AmpLayout& ampLayout (int model)
{
    static const AmpLayout british { { { { 161, 111 }, { 262, 111 }, { 362, 111 }, { 462, 111 }, { 811, 111 }, { 911, 111 }, { 1011, 111 }, { 1112, 111 } } },
                                     96.0f, KnobStyle::ChickenHead, { 680, 82, 46, 62 }, juce::Colour (0xff1d1a16), 156.0f };
    static const AmpLayout round { { { { 157, 111 }, { 264, 111 }, { 371, 111 }, { 478, 111 }, { 585, 111 }, { 692, 111 }, { 799, 111 }, { 906, 111 } } },
                                   82.0f, KnobStyle::AmpRound, { 988, 76, 50, 72 }, juce::Colour (0xffcfcfda), 157.0f };
    return model == 0 ? british : round;
}

const char* const ampKnobIds[8] = { "gain", "bass", "mid", "treble", "presence", "resonance", "master", "sag" };

juce::String ampKnobLabel (int model, int i)
{
    static const char* const std8[8] = { "GAIN", "BASS", "MIDDLE", "TREBLE", "PRESENCE", "RESONANCE", "MASTER", "SAG" };
    if (model == 0 && i == 4) return "CUT";
    if (model == 0 && i == 5) return "DEPTH";
    return std8[i];
}

const char* const modelNames[3] = { "BRITISH 30", "AMERICAN TWIN", "MODERN 5050" };
const char* const modelBlurb[3] = {
    "Classe A, valvole EL84 senza controreazione: brillante, 'chime', satura ricco. Ispirato al Top Boost britannico.",
    "Pulito cristallino, 6L6 in classe AB con tanta headroom e tone stack scavato. Ispirato ai combo americani anni '60.",
    "Alto guadagno, quattro stadi a cascata e bassi stretti per riff veloci e palm mute. Ispirato alle testate moderne." };

// --- Pedals: size, colours and where each control sits (base units, 150 high).
struct Ctl { int param; float x, y, size; };
struct PedalLook { int w; juce::Colour top, bottom, ink; int shape; std::vector<Ctl> ctls; };

const PedalLook& look (int pedal)
{
    static const std::vector<PedalLook> looks = {
        { 70,  juce::Colour (0xff26262c), juce::Colour (0xff0e0e12), juce::Colour (0xffe8e8f0), 0, { { 0, 35, 26, 28 }, { 1, 35, 58, 20 } } },
        { 92,  juce::Colour (0xff4a4d55), juce::Colour (0xff22242a), juce::Colour (0xffeeeeee), 0, { { 0, 26, 24, 26 }, { 1, 66, 24, 26 }, { 2, 26, 57, 20 }, { 3, 66, 57, 20 } } },
        { 76,  juce::Colour (0xffb4b4bc), juce::Colour (0xff5d5d64), juce::Colour (0xff1a1a1e), 1, { { 0, 24, 30, 26 }, { 2, 54, 30, 22 }, { 1, 38, 60, 56 } } },
        { 92,  juce::Colour (0xff6a33a6), juce::Colour (0xff2c1150), juce::Colour (0xffefe6ff), 0, { { 0, 24, 26, 28 }, { 2, 68, 26, 28 }, { 1, 46, 57, 22 } } },
        { 104, juce::Colour (0xff1c1c20), juce::Colour (0xff050506), juce::Colour (0xffd9c8ff), 2, { { 0, 30, 30, 28 }, { 2, 74, 30, 28 }, { 1, 30, 60, 20 }, { 3, 74, 60, 20 } } },
        { 124, juce::Colour (0xff1e1e24), juce::Colour (0xff0a0a0d), juce::Colour (0xffe8e8f0), 3, { { 7, 110, 38, 20 } } },
        { 92,  juce::Colour (0xff7a4ac0), juce::Colour (0xff3a1d66), juce::Colour (0xfff3ecff), 0, { { 0, 24, 26, 26 }, { 1, 68, 26, 26 }, { 2, 46, 57, 22 } } },
        { 84,  juce::Colour (0xff2e2e36), juce::Colour (0xff131317), juce::Colour (0xffe8e8f0), 0, { { 0, 42, 28, 34 }, { 1, 16, 60, 16 }, { 2, 42, 63, 16 }, { 3, 68, 60, 16 } } },
        { 92,  juce::Colour (0xffd8d0bc), juce::Colour (0xff9b9482), juce::Colour (0xff2a2520), 0, { { 0, 24, 26, 26 }, { 1, 68, 26, 26 }, { 2, 46, 57, 22 } } },
        { 124, juce::Colour (0xff5a5e66), juce::Colour (0xff2a2d33), juce::Colour (0xffeef2f6), 0, { { 0, 22, 24, 24 }, { 3, 52, 24, 22 }, { 4, 80, 24, 22 }, { 5, 106, 24, 18 },
                                                                                                    { 1, 20, 58, 22 }, { 2, 62, 58, 46 }, { 6, 104, 58, 22 } } },
        { 112, juce::Colour (0xff1b1b22), juce::Colour (0xff08080b), juce::Colour (0xffd9c8ff), 4, { { 0, 22, 24, 24 }, { 1, 56, 24, 24 }, { 2, 90, 24, 24 }, { 3, 36, 57, 20 }, { 4, 78, 57, 20 } } },
    };
    return looks[(size_t) pedal];
}
} // namespace

// ============================================================================ AmpStage
AmpStage::AmpStage (ToxicRigProcessor& p) : proc (p)
{
    images[0] = img (BinaryData::amp_british_png, BinaryData::amp_british_pngSize);
    images[1] = img (BinaryData::amp_american_png, BinaryData::amp_american_pngSize);
    images[2] = img (BinaryData::amp_modern_png, BinaryData::amp_modern_pngSize);
    for (int s = 0; s < 2; ++s)
    {
        const char c = s == 0 ? 'a' : 'b';
        for (int i = 0; i < 8; ++i)
        {
            knobs[(size_t) s][(size_t) i] = std::make_unique<ParamKnob> (p.apvts, rig::ampId (c, ampKnobIds[i]), KnobStyle::AmpRound);
            addChildComponent (*knobs[(size_t) s][(size_t) i]);
        }
        bright[(size_t) s] = std::make_unique<ParamToggle> (p.apvts, rig::ampId (c, "bright"), "BRIGHT", ParamToggle::Look::Bat);
        addChildComponent (*bright[(size_t) s]);
    }
    setSlot (0);
    startTimerHz (10);
}

int AmpStage::model() const
{
    return juce::jlimit (0, 2, (int) proc.apvts.getRawParameterValue (rig::ampId (slot == 0 ? 'a' : 'b', "model"))->load());
}

void AmpStage::setSlot (int s)
{
    slot = s;
    for (int k = 0; k < 2; ++k)
    {
        for (auto& kn : knobs[(size_t) k]) kn->setVisible (k == slot);
        bright[(size_t) k]->setVisible (k == slot);
    }
    lastModel = -1;
    timerCallback();
}

void AmpStage::timerCallback()
{
    const int m = model();
    if (m != lastModel) { lastModel = m; resized(); repaint(); }
}

juce::Rectangle<float> AmpStage::imageArea() const
{
    const auto b = getLocalBounds().toFloat();
    const float w = std::min (b.getWidth(), b.getHeight() * 1272.0f / 660.0f);
    return juce::Rectangle<float> (w, w * 660.0f / 1272.0f).withCentre (b.getCentre()).withBottomY (b.getBottom());
}

void AmpStage::resized()
{
    const int m = model();
    const auto& L = ampLayout (m);
    const auto a = imageArea();
    const float s = a.getWidth() / 1272.0f;
    for (int i = 0; i < 8; ++i)
    {
        auto& kn = knobs[(size_t) slot][(size_t) i];
        kn->style = L.style;
        const auto c = a.getTopLeft() + L.knobs[(size_t) i] * s;
        kn->setBounds (juce::Rectangle<float> (L.knobSize * s, L.knobSize * s).withCentre (c).toNearestInt());
        kn->repaint();
    }
    bright[(size_t) slot]->setBounds ((L.bright * s + a.getTopLeft()).toNearestInt());
}

void AmpStage::paint (juce::Graphics& g)
{
    const int m = model();
    const auto a = imageArea();
    const float s = a.getWidth() / 1272.0f;
    // Floor shadow, then the amp.
    for (int i = 6; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.10f));
        g.fillEllipse (a.getX() - (float) i * 6.0f, a.getBottom() - 18.0f - (float) i, a.getWidth() + (float) i * 12.0f, 26.0f + (float) i * 3.0f);
    }
    g.setOpacity (1.0f); // drawImage uses the current colour's alpha
    g.drawImage (images[(size_t) m], a, juce::RectanglePlacement::stretchToFit);
    const auto& L = ampLayout (m);
    // Engraved labels under every knob (same count as the knobs: nothing fake).
    g.setFont (skinFont (SkinId::ToxicLab, FontRole::Label, std::max (9.5f, 19.0f * s)));
    for (int i = 0; i < 8; ++i)
    {
        const auto c = a.getTopLeft() + juce::Point<float> (L.knobs[(size_t) i].x, L.labelY) * s;
        const auto r = juce::Rectangle<float> (110.0f * s, 16.0f * s).withCentre (c);
        if (m == 0) { g.setColour (juce::Colours::white.withAlpha (0.25f)); g.drawText (ampKnobLabel (m, i), r.translated (0, 1), juce::Justification::centred); }
        g.setColour (L.labelCol);
        g.drawText (ampKnobLabel (m, i), r, juce::Justification::centred);
    }
    if (m == 0)
    {
        // Nameplate: model name engraved, bright switch on the right of it.
        const auto plate = juce::Rectangle<float> (548, 84, 132, 58) * s + a.getTopLeft();
        g.setColour (juce::Colour (0xff1d1a16));
        g.setFont (skinFont (SkinId::ToxicLab, FontRole::Title, 17.0f * s));
        g.drawText ("TOXICRIG", plate.withTrimmedBottom (plate.getHeight() * 0.45f), juce::Justification::centred);
        g.setFont (skinFont (SkinId::ToxicLab, FontRole::Label, 15.0f * s));
        g.drawText ("BRITISH 30", plate.withTrimmedTop (plate.getHeight() * 0.5f), juce::Justification::centred);
    }
    else
    {
        // Logo badge on the grille.
        const auto badge = (m == 2 ? juce::Rectangle<float> (521, 186, 230, 54) : juce::Rectangle<float> (120, 200, 230, 54)) * s + a.getTopLeft();
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (badge, 6.0f * s);
        g.setColour (juce::Colour (0xffb8b8c4).withAlpha (0.6f));
        g.drawRoundedRectangle (badge, 6.0f * s, 1.2f);
        g.setColour (kGreen);
        g.setFont (skinFont (SkinId::ToxicLab, FontRole::Title, 22.0f * s));
        g.drawText ("TOXICRIG", badge.withTrimmedBottom (badge.getHeight() * 0.42f), juce::Justification::centred);
        g.setColour (juce::Colour (0xffcfcfda));
        g.setFont (skinFont (SkinId::ToxicLab, FontRole::Label, 15.0f * s));
        g.drawText (modelNames[m], badge.withTrimmedTop (badge.getHeight() * 0.55f), juce::Justification::centred);
        // The pilot jewel glows when this amp is actually in use.
        const bool active = slot == 0 || proc.apvts.getRawParameterValue (rig::ids::ampBOn)->load() > 0.5f;
        if (! active)
        {
            g.setColour (juce::Colours::black.withAlpha (0.7f));
            g.fillEllipse (juce::Rectangle<float> (62, 62).withCentre ({ 1118, 111 }) * s + a.getTopLeft());
        }
    }
    if (slot == 1 && proc.apvts.getRawParameterValue (rig::ids::ampBOn)->load() < 0.5f)
    {
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRect (a);
        g.setColour (kPurple.brighter (0.4f));
        g.setFont (skinFont (SkinId::ToxicLab, FontRole::Title, 26.0f));
        g.drawText ("AMPLI B SPENTO", a.withTrimmedTop (a.getHeight() * 0.4f), juce::Justification::centredTop);
    }
}

// ============================================================================ AmpSide
AmpSide::AmpSide (ToxicRigProcessor& p, AmpStage& st)
    : proc (p), stage (st), bOn (p.apvts, rig::ids::ampBOn, "B ON", ParamToggle::Look::Pill)
{
    for (auto* b : { &tabA, &tabB, &prev, &next, &cabBtn }) addAndMakeVisible (b);
    tabA.onClick = [this] { stage.setSlot (0); bindSlot(); };
    tabB.onClick = [this] { stage.setSlot (1); bindSlot(); };
    const char* chNames[3] = { "CLEAN", "CRUNCH", "LEAD" };
    for (int i = 0; i < 3; ++i)
    {
        channels[(size_t) i].setButtonText (chNames[i]);
        channels[(size_t) i].onClick = [this, i] { if (channelAtt) channelAtt->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (channels[(size_t) i]);
    }
    prev.onClick = [this] { if (modelAtt) modelAtt->setValueAsCompleteGesture ((float) ((modelIdx + 2) % 3)); };
    next.onClick = [this] { if (modelAtt) modelAtt->setValueAsCompleteGesture ((float) ((modelIdx + 1) % 3)); };
    cabBtn.onClick = [this] { if (onOpenCab) onOpenCab(); };
    addAndMakeVisible (bOn);
    levelA = std::make_unique<CaptionKnob> (p.apvts, rig::ampId ('a', "level"), "Livello A");
    levelB = std::make_unique<CaptionKnob> (p.apvts, rig::ampId ('b', "level"), "Livello B");
    mix = std::make_unique<CaptionKnob> (p.apvts, rig::ids::abMix, "Mix A/B");
    width = std::make_unique<CaptionKnob> (p.apvts, rig::ids::width, "Width");
    for (auto* c : { levelA.get(), levelB.get(), mix.get(), width.get() }) addAndMakeVisible (c);
    bindSlot();
    startTimerHz (8);
}

void AmpSide::bindSlot()
{
    const char c = stage.getSlot() == 0 ? 'a' : 'b';
    modelAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (rig::ampId (c, "model")), [this] (float v) { modelIdx = (int) std::lround (v); repaint(); }, nullptr);
    channelAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (rig::ampId (c, "channel")), [this] (float v)
    {
        channelIdx = (int) std::lround (v);
        for (int i = 0; i < 3; ++i) channels[(size_t) i].setToggleState (i == channelIdx, juce::dontSendNotification);
    }, nullptr);
    modelAtt->sendInitialUpdate();
    channelAtt->sendInitialUpdate();
    tabA.setToggleState (stage.getSlot() == 0, juce::dontSendNotification);
    tabB.setToggleState (stage.getSlot() == 1, juce::dontSendNotification);
    repaint();
}

void AmpSide::timerCallback()
{
    const bool b = proc.apvts.getRawParameterValue (rig::ids::ampBOn)->load() > 0.5f;
    for (auto* c : std::initializer_list<juce::Component*> { levelB.get(), mix.get(), width.get() }) c->setAlpha (b ? 1.0f : 0.4f);
    repaint();
}

void AmpSide::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xd80c0c12));
    g.fillRoundedRectangle (r, 14.0f);
    g.setColour (kPurple.withAlpha (0.35f));
    g.drawRoundedRectangle (r.reduced (0.5f), 14.0f, 1.0f);
    // Model selector.
    auto modelRow = juce::Rectangle<float> (16, 60, r.getWidth() - 32, 44);
    g.setColour (kGreen);
    g.setFont (skinFont (SkinId::ToxicLab, FontRole::Title, 19.0f));
    g.drawText (modelNames[modelIdx], modelRow.reduced (40, 0), juce::Justification::centred);
    g.setColour (juce::Colour (0xffb0b0c2));
    g.setFont (skinFont (SkinId::ToxicLab, FontRole::Body, 14.5f));
    g.drawFittedText (modelBlurb[modelIdx], juce::Rectangle<int> (18, 106, (int) r.getWidth() - 36, 40), juce::Justification::centredTop, 3);
    g.setColour (juce::Colour (0xff6e6e84));
    g.setFont (skinFont (SkinId::ToxicLab, FontRole::Label, 13.0f));
    g.drawText ("CANALE", 18, 148, 200, 16, juce::Justification::centredLeft);
    g.drawText ("MIXER A/B", 18, 214, 200, 16, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff2e2e3c));
    g.drawHorizontalLine (208, 16.0f, r.getWidth() - 16.0f);
    g.drawHorizontalLine ((int) r.getHeight() - 70, 16.0f, r.getWidth() - 16.0f);
    // Cab summary.
    const char c = stage.getSlot() == 0 ? 'a' : 'b';
    const int cab = (int) proc.apvts.getRawParameterValue (rig::ampId (c, "cab"))->load();
    const int mic = (int) proc.apvts.getRawParameterValue (rig::ampId (c, "mic"))->load();
    g.setColour (juce::Colour (0xffb0b0c2));
    g.setFont (skinFont (SkinId::ToxicLab, FontRole::Body, 14.0f));
    g.drawText (rig::cabNames[cab] + "  -  mic " + rig::micNames[mic].toLowerCase(), juce::Rectangle<int> (18, (int) r.getHeight() - 64, (int) r.getWidth() - 36, 16),
                juce::Justification::centred);
}

void AmpSide::resized()
{
    const int w = getWidth();
    tabA.setBounds (16, 14, (w - 112) / 2, 34);
    tabB.setBounds (16 + (w - 112) / 2 + 6, 14, (w - 112) / 2, 34);
    bOn.setBounds (w - 84, 16, 68, 30);
    prev.setBounds (16, 64, 36, 36);
    next.setBounds (w - 52, 64, 36, 36);
    const int cw = (w - 32 - 12) / 3;
    for (int i = 0; i < 3; ++i) channels[(size_t) i].setBounds (16 + i * (cw + 6), 166, cw, 32);
    const int kw = (w - 32) / 4;
    levelA->setBounds (16, 232, kw, 96);
    levelB->setBounds (16 + kw, 232, kw, 96);
    mix->setBounds (16 + kw * 2, 232, kw, 96);
    width->setBounds (16 + kw * 3, 232, kw, 96);
    cabBtn.setBounds (16, getHeight() - 44, w - 32, 34);
}

// ============================================================================ PedalView
int PedalView::baseWidth (int p) { return look (p).w; }

PedalView::PedalView (ToxicRigProcessor& p, int ped, float scale) : pedal (ped), proc (p), k (scale)
{
    const auto& L = look (pedal);
    const auto& spec = rig::pedalSpecs()[(size_t) pedal];
    setSize ((int) std::lround (L.w * k), (int) std::lround (kBaseHeight * k));
    const bool big = k > 1.5f;
    for (const auto& c : L.ctls)
    {
        const auto& ps = spec.params[(size_t) c.param];
        const auto id = rig::pedalParamId (pedal, c.param);
        std::unique_ptr<juce::Component> comp;
        juce::Rectangle<float> r;
        if (ps.isBool)
        {
            comp = std::make_unique<ParamToggle> (p.apvts, id, big ? juce::String (ps.name).toUpperCase() : juce::String(), ParamToggle::Look::Bat);
            r = juce::Rectangle<float> (c.size, c.size * 1.25f).withCentre ({ c.x, c.y + 2 });
        }
        else if (! ps.choices.isEmpty())
        {
            auto cb = std::make_unique<ChoiceButton> (p.apvts, id);
            r = juce::Rectangle<float> (c.size, 14.0f).withCentre ({ c.x, c.y });
            comp = std::move (cb);
            if (big) captions.push_back ({ juce::String (ps.name).toUpperCase(), { c.x, c.y + 13.0f } });
        }
        else
        {
            comp = std::make_unique<ParamKnob> (p.apvts, id, big ? KnobStyle::PedalBig : KnobStyle::Pedal);
            r = juce::Rectangle<float> (c.size, c.size).withCentre ({ c.x, c.y });
            if (big) captions.push_back ({ juce::String (ps.name).toUpperCase(), { c.x, c.y + c.size * 0.5f + 5.0f } });
        }
        comp->setBounds ((r * k).toNearestInt());
        addAndMakeVisible (*comp);
        controls.push_back (std::move (comp));
    }
    if (pedal == (int) tz::PedalId::EQ)
    {
        // Seven vertical faders.
        for (int b = 0; b < 7; ++b)
        {
            auto sl = std::make_unique<juce::Slider> (juce::Slider::LinearVertical, juce::Slider::NoTextBox);
            sl->setPopupDisplayEnabled (true, true, nullptr, 900);
            sl->setDoubleClickReturnValue (true, 0.0);
            sl->setTooltip (juce::String ("EQ ") + spec.params[(size_t) b].name + " Hz");
            sliderAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, rig::pedalParamId (pedal, b), *sl));
            sl->setBounds ((juce::Rectangle<float> (11.0f, 50.0f).withCentre ({ 13.0f + (float) b * 13.5f, 38.0f }) * k).toNearestInt());
            addAndMakeVisible (*sl);
            if (big) captions.push_back ({ spec.params[(size_t) b].name, { 13.0f + (float) b * 13.5f, 68.0f } });
            controls.push_back (std::move (sl));
        }
        if (big) captions.push_back ({ "LEVEL", { 110.0f, 52.0f } });
    }
    setTooltip (juce::String (spec.displayName) + ": clic per aprirlo, trascina per spostarlo, pedale in basso = on/off");
    setRepaintsOnMouseActivity (true);
}

bool PedalView::isOn() const { return proc.apvts.getRawParameterValue (rig::pedalOnId (pedal))->load() > 0.5f; }

juce::Rectangle<float> PedalView::footswitch() const
{
    return juce::Rectangle<float> (30.0f * k, 30.0f * k).withCentre ({ getWidth() * 0.5f, 124.0f * k });
}

void PedalView::paint (juce::Graphics& g)
{
    const auto& L = look (pedal);
    const auto& spec = rig::pedalSpecs()[(size_t) pedal];
    const float W = (float) getWidth(), H = (float) getHeight();
    auto body = juce::Rectangle<float> (2.0f * k, 2.0f * k, W - 4.0f * k, H - 6.0f * k);
    juce::Path shape;
    if (L.shape == 1) // wedge: slanted top
    {
        shape.startNewSubPath (body.getX() + 6 * k, body.getY() + 10 * k);
        shape.lineTo (body.getRight() - 6 * k, body.getY() + 10 * k);
        shape.lineTo (body.getRight(), body.getBottom());
        shape.lineTo (body.getX(), body.getBottom());
        shape.closeSubPath();
        shape = shape.createPathWithRoundedCorners (8.0f * k);
    }
    else if (L.shape == 2) // dome top
        shape.addRoundedRectangle (body.getX(), body.getY(), body.getWidth(), body.getHeight(), 36.0f * k, 36.0f * k, true, true, false, false);
    else
        shape.addRoundedRectangle (body, 10.0f * k);
    // Drop shadow on the floor.
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillPath (shape, juce::AffineTransform::translation (3.0f * k, 6.0f * k));
    juce::ColourGradient grad (L.top, 0, body.getY(), L.bottom, 0, body.getBottom(), false);
    g.setGradientFill (grad);
    g.fillPath (shape);
    // Finishes: sparkle (reverb), hammered (booster), shimmer (chorus).
    if (L.shape == 4 || pedal == (int) tz::PedalId::Chorus)
    {
        tz::FastRandom rnd; rnd.s = (uint32_t) (pedal * 977 + 13);
        g.saveState(); g.reduceClipRegion (shape);
        for (int i = 0; i < 70; ++i)
        {
            const float x = body.getX() + (rnd.nextBipolar() * 0.5f + 0.5f) * body.getWidth();
            const float y = body.getY() + (rnd.nextBipolar() * 0.5f + 0.5f) * body.getHeight();
            g.setColour ((i % 3 == 0 ? kPurple : juce::Colours::white).withAlpha (0.18f + 0.12f * (rnd.nextBipolar() * 0.5f + 0.5f)));
            g.fillEllipse (x, y, 1.3f * k, 1.3f * k);
        }
        g.restoreState();
    }
    g.setColour (juce::Colours::white.withAlpha (0.14f));
    g.strokePath (shape, juce::PathStrokeType (1.0f), juce::AffineTransform::translation (0, 1.0f));
    g.setColour (juce::Colours::black);
    g.strokePath (shape, juce::PathStrokeType (1.6f));
    // Side jacks.
    g.setColour (juce::Colour (0xff3a3a42));
    g.fillRect (juce::Rectangle<float> (3.0f * k, 10.0f * k).withCentre ({ 1.5f * k, 40.0f * k }));
    g.fillRect (juce::Rectangle<float> (3.0f * k, 10.0f * k).withCentre ({ W - 1.5f * k, 40.0f * k }));
    // Name.
    g.setColour (L.ink);
    static const char* const shortNames[] = { "NOISE GATE", "SUSTAINER", "TREBLE BOOST", "GREEN DRIVE", "TOXIC FUZZ", "GRAPHIC EQ",
                                              "CHORUS", "PHASER", "TREMOLO", "DELAY", "REVERB" };
    juce::ignoreUnused (spec);
    g.setFont (skinFont (SkinId::ToxicLab, FontRole::Label, 12.5f * k));
    g.drawFittedText (shortNames[pedal], juce::Rectangle<float> (8.0f * k, 76.0f * k, W - 16.0f * k, 16.0f * k).toNearestInt(),
                      juce::Justification::centred, 1, 0.5f);
    // Captions (big view only).
    g.setFont (skinFont (SkinId::ToxicLab, FontRole::Label, 5.2f * k));
    g.setColour (L.ink.withAlpha (0.85f));
    for (auto& [txt, pt] : captions)
        g.drawText (txt, juce::Rectangle<float> (40.0f * k, 7.0f * k).withCentre (pt * k), juce::Justification::centred);
    // LED.
    const bool on = isOn();
    const auto led = juce::Rectangle<float> (7.0f * k, 7.0f * k).withCentre ({ W * 0.5f, 100.0f * k });
    if (on)
        for (int i = 3; i >= 1; --i) { g.setColour (kGreen.withAlpha (0.12f)); g.fillEllipse (led.expanded ((float) i * 2.5f * k)); }
    g.setColour (on ? kGreen : juce::Colour (0xff1f2a1c));
    g.fillEllipse (led);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawEllipse (led, 0.8f);
    // Footswitch: hex nut + domed cap.
    const auto fs = footswitch();
    juce::Path hex;
    for (int i = 0; i < 6; ++i)
    {
        const float a = juce::MathConstants<float>::pi / 3.0f * (float) i;
        const auto pt = fs.getCentre() + juce::Point<float> (std::cos (a), std::sin (a)) * fs.getWidth() * 0.5f;
        if (i == 0) hex.startNewSubPath (pt); else hex.lineTo (pt);
    }
    hex.closeSubPath();
    g.setColour (juce::Colour (0xff2b2b30));
    g.fillPath (hex);
    g.setColour (juce::Colours::black);
    g.strokePath (hex, juce::PathStrokeType (1.0f));
    const auto capR = fs.reduced (fs.getWidth() * 0.2f);
    juce::ColourGradient chrome (juce::Colour (0xffe9e9ee), capR.getX(), capR.getY(), juce::Colour (0xff4e4e56), capR.getRight(), capR.getBottom(), false);
    g.setGradientFill (chrome);
    g.fillEllipse (capR);
    if (isMouseOver() && fs.contains (getMouseXYRelative().toFloat()))
    {
        g.setColour (kGreen.withAlpha (0.5f));
        g.drawEllipse (fs, 1.2f);
    }
}

void PedalView::mouseDown (const juce::MouseEvent&) { dragging = false; }

void PedalView::mouseDrag (const juce::MouseEvent& e)
{
    if (k > 1.5f) return; // the big pedal in the overlay doesn't move
    if (! dragging && e.getDistanceFromDragStart() > 6) dragging = true;
    if (dragging && onDrag) onDrag (pedal, e);
}

void PedalView::mouseUp (const juce::MouseEvent& e)
{
    if (dragging) { dragging = false; if (onDrop) onDrop (pedal, e); return; }
    if (footswitch().expanded (4.0f).contains (e.position))
    {
        auto* prm = proc.apvts.getParameter (rig::pedalOnId (pedal));
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.0f : 1.0f);
        prm->endChangeGesture();
        repaint();
        return;
    }
    if (onOpen) onOpen (pedal);
}

// ============================================================================ FloorView
FloorView::FloorView (ToxicRigProcessor& p) : proc (p)
{
    for (int i = 0; i < tz::kNumPedals; ++i)
    {
        pedals.push_back (std::make_unique<PedalView> (p, i, 1.0f));
        auto& pv = *pedals.back();
        pv.onOpen = [this] (int id) { if (onOpenPedal) onOpenPedal (id); };
        pv.onDrag = [this] (int id, const juce::MouseEvent& e)
        {
            draggingPedal = id;
            auto& v = *pedals[(size_t) id];
            const auto pos = e.getEventRelativeTo (this).position;
            v.setTopLeftPosition ((int) (pos.x - v.getWidth() * 0.5f), (int) (pos.y - v.getHeight() * 0.5f));
            v.toFront (false);
            repaint();
        };
        pv.onDrop = [this] (int id, const juce::MouseEvent& e)
        {
            draggingPedal = -1;
            const float x = e.getEventRelativeTo (this).position.x;
            const bool toLoop = x > (float) ampSlot.getCentreX();
            const auto r = proc.getRouting();
            int index = 0;
            const int n = toLoop ? r.numLoop : r.numBoard;
            for (int i = 0; i < n; ++i)
            {
                const int other = toLoop ? r.loop[(size_t) i] : r.board[(size_t) i];
                if (other != id && (float) pedals[(size_t) other]->getBounds().getCentreX() < x) ++index;
            }
            proc.setRouting (r.moved (id, toLoop, index));
            layout (true);
        };
        addAndMakeVisible (pv);
    }
    startTimerHz (12);
}

void FloorView::layout (bool animate)
{
    const auto r = proc.getRouting();
    lastRouting = r.encode();
    const int gap = 12, ampW = 64;
    int total = ampW + gap;
    for (int i = 0; i < r.numBoard; ++i) total += PedalView::baseWidth (r.board[(size_t) i]) + gap;
    for (int i = 0; i < r.numLoop; ++i) total += PedalView::baseWidth (r.loop[(size_t) i]) + gap;
    const float scale = std::min (1.0f, (float) (getWidth() - 20) / (float) total);
    juce::ignoreUnused (scale);
    int x = std::max (10, (getWidth() - total) / 2);
    const int y = 26 + (getHeight() - 26 - PedalView::kBaseHeight) / 2;
    auto place = [&] (int id)
    {
        auto& v = *pedals[(size_t) id];
        const juce::Rectangle<int> b (x, y, v.getWidth(), v.getHeight());
        if (id != draggingPedal)
        {
            if (animate) juce::Desktop::getInstance().getAnimator().animateComponent (&v, b, 1.0f, 160, false, 0.0, 0.0);
            else v.setBounds (b);
        }
        x += v.getWidth() + gap;
    };
    for (int i = 0; i < r.numBoard; ++i) place (r.board[(size_t) i]);
    ampSlot = { x, y + 20, ampW, 86 };
    x += ampW + gap;
    for (int i = 0; i < r.numLoop; ++i) place (r.loop[(size_t) i]);
    repaint();
}

void FloorView::timerCallback()
{
    if (proc.getRouting().encode() != lastRouting && draggingPedal < 0) layout (true);
    for (auto& p : pedals) p->repaint();
}

void FloorView::paint (juce::Graphics& g)
{
    const auto r = proc.getRouting();
    std::vector<juce::Rectangle<float>> chainRects;
    for (int i = 0; i < r.numBoard; ++i) chainRects.push_back (pedals[(size_t) r.board[(size_t) i]]->getBounds().toFloat());
    chainRects.push_back (ampSlot.toFloat());
    for (int i = 0; i < r.numLoop; ++i) chainRects.push_back (pedals[(size_t) r.loop[(size_t) i]]->getBounds().toFloat());
    // Cables (behind the pedals): from each output jack to the next input jack.
    for (size_t i = 1; i < chainRects.size(); ++i)
    {
        const auto a = juce::Point<float> (chainRects[i - 1].getRight(), chainRects[i - 1].getY() + 40.0f);
        const auto b = juce::Point<float> (chainRects[i].getX(), chainRects[i].getY() + 40.0f);
        juce::Path cable;
        cable.startNewSubPath (a);
        const float sag = 26.0f + std::abs (b.x - a.x) * 0.15f;
        cable.cubicTo (a + juce::Point<float> (12.0f, sag), b + juce::Point<float> (-12.0f, sag), b);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.strokePath (cable, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), juce::AffineTransform::translation (2, 4));
        g.setColour (juce::Colour (0xff111116));
        g.strokePath (cable, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour ((i <= (size_t) r.numBoard ? kGreen : kPurple).withAlpha (0.35f));
        g.strokePath (cable, juce::PathStrokeType (1.2f), juce::AffineTransform::translation (0, -1.2f));
    }
    // Amp patch box: where the board ends and the effects loop begins.
    auto a = ampSlot.toFloat();
    g.setColour (juce::Colour (0xee0c0c12));
    g.fillRoundedRectangle (a, 8.0f);
    g.setColour (kPurple.withAlpha (0.6f));
    g.drawRoundedRectangle (a, 8.0f, 1.2f);
    g.setColour (juce::Colour (0xffcfcfda));
    g.setFont (skinFont (SkinId::ToxicLab, FontRole::Label, 13.0f));
    g.drawFittedText ("AMP\n+\nCAB", a.reduced (4.0f).toNearestInt(), juce::Justification::centred, 3);
    g.setColour (kGreen.withAlpha (0.8f));
    g.setFont (skinFont (SkinId::ToxicLab, FontRole::Label, 13.0f));
    if (r.numBoard > 0)
        g.drawText ("PRIMA DELL'AMPLI", juce::Rectangle<float> (chainRects.front().getX(), chainRects.front().getY() - 24.0f, a.getX() - chainRects.front().getX(), 16.0f), juce::Justification::centred);
    g.setColour (kPurple.brighter (0.3f));
    if (r.numLoop > 0)
        g.drawText ("FX LOOP", juce::Rectangle<float> (a.getRight(), chainRects.front().getY() - 24.0f, chainRects.back().getRight() - a.getRight(), 16.0f), juce::Justification::centred);
}

// ============================================================================ Overlays
Overlay::Overlay()
{
    addAndMakeVisible (close);
    close.setTooltip ("Chiudi");
    close.onClick = [this] { if (onClose) onClose(); };
}

void Overlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.62f));
    auto c = card.toFloat();
    g.setColour (juce::Colour (0xf2101017));
    g.fillRoundedRectangle (c, 16.0f);
    g.setColour (kPurple.withAlpha (0.5f));
    g.drawRoundedRectangle (c.reduced (0.5f), 16.0f, 1.2f);
}

void Overlay::mouseDown (const juce::MouseEvent& e)
{
    if (! card.contains (e.getPosition()) && onClose) onClose();
}

void Overlay::resized()
{
    card = getLocalBounds().withSizeKeepingCentre (860, 560);
    close.setBounds (card.getRight() - 50, card.getY() + 14, 36, 36);
}

PedalOverlay::PedalOverlay (ToxicRigProcessor& p, int ped)
    : pedal (ped), big (p, ped, 2.6f), onBtn (p.apvts, rig::pedalOnId (ped), "ON / OFF", ParamToggle::Look::Pill)
{
    addAndMakeVisible (big);
    addAndMakeVisible (onBtn);
}

void PedalOverlay::resized()
{
    Overlay::resized();
    big.setTopLeftPosition (card.getX() + 40 + (300 - big.getWidth()) / 2, card.getY() + (card.getHeight() - big.getHeight()) / 2);
    onBtn.setBounds (card.getX() + 380, card.getBottom() - 70, 150, 36);
}

void PedalOverlay::paint (juce::Graphics& g)
{
    Overlay::paint (g);
    const auto& spec = rig::pedalSpecs()[(size_t) pedal];
    auto text = juce::Rectangle<int> (card.getX() + 380, card.getY() + 40, card.getWidth() - 430, card.getHeight() - 120);
    g.setColour (kGreen);
    g.setFont (skinFont (SkinId::ToxicLab, FontRole::Title, 30.0f));
    g.drawText (juce::String (spec.displayName).toUpperCase(), text.removeFromTop (44), juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xffdcdce6));
    g.setFont (skinFont (SkinId::ToxicLab, FontRole::Body, 19.0f));
    g.drawFittedText (spec.description, text.removeFromTop (70), juce::Justification::topLeft, 4);
    text.removeFromTop (16);
    g.setColour (juce::Colour (0xff8f8fa6));
    g.setFont (skinFont (SkinId::ToxicLab, FontRole::Body, 16.0f));
    g.drawFittedText ("Trascina una manopola su/giu' per regolarla.\nDoppio clic: torna al valore iniziale.\nRotella del mouse: regolazione fine.\nClic sul pedale in basso: acceso / spento.",
                      text.removeFromTop (120), juce::Justification::topLeft, 6);
}

CabOverlay::CabOverlay (ToxicRigProcessor& p, int s) : proc (p)
{
    speaker = img (BinaryData::speaker_png, BinaryData::speaker_pngSize);
    micImgs[0] = img (BinaryData::mic_dynamic_png, BinaryData::mic_dynamic_pngSize);
    micImgs[1] = img (BinaryData::mic_ribbon_png, BinaryData::mic_ribbon_pngSize);
    micImgs[2] = img (BinaryData::mic_condenser_png, BinaryData::mic_condenser_pngSize);
    for (auto* b : { &tabA, &tabB, &load }) addAndMakeVisible (b);
    tabA.onClick = [this] { bind (0); };
    tabB.onClick = [this] { bind (1); };
    for (int i = 0; i < 4; ++i)
    {
        cabBtns[(size_t) i].setButtonText (rig::cabNames[i].toUpperCase());
        cabBtns[(size_t) i].onClick = [this, i]
        {
            if (i == (int) tz::CabType::User && ! proc.getUserIR (slot).existsAsFile()) { load.triggerClick(); return; }
            if (cabAtt) cabAtt->setValueAsCompleteGesture ((float) i);
        };
        addAndMakeVisible (cabBtns[(size_t) i]);
    }
    for (int i = 0; i < 3; ++i)
    {
        auto& b = micBtns[(size_t) i];
        b.setImages (false, true, true, micImgs[(size_t) i], 0.75f, {}, micImgs[(size_t) i], 1.0f, {}, micImgs[(size_t) i], 1.0f, {});
        b.setTooltip ("Microfono " + rig::micNames[i].toLowerCase());
        b.onClick = [this, i] { if (micAtt) micAtt->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }
    load.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Scegli una IR (.wav)", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.wav;*.aif;*.aiff");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f.existsAsFile()) { proc.setUserIR (slot, f); repaint(); }
        });
    };
    bind (s);
    startTimerHz (10);
}

void CabOverlay::bind (int s)
{
    slot = s;
    const char c = s == 0 ? 'a' : 'b';
    auto& st = proc.apvts;
    onBtn = std::make_unique<ParamToggle> (st, rig::ampId (c, "cab_on"), "CASSA ON", ParamToggle::Look::Pill);
    addAndMakeVisible (*onBtn);
    cabAtt = std::make_unique<juce::ParameterAttachment> (*st.getParameter (rig::ampId (c, "cab")), [this] (float v)
    {
        cabIdx = (int) std::lround (v);
        for (int i = 0; i < 4; ++i) cabBtns[(size_t) i].setToggleState (i == cabIdx, juce::dontSendNotification);
    }, nullptr);
    micAtt = std::make_unique<juce::ParameterAttachment> (*st.getParameter (rig::ampId (c, "mic")), [this] (float v) { micIdx = (int) std::lround (v); repaint(); }, nullptr);
    posAtt = std::make_unique<juce::ParameterAttachment> (*st.getParameter (rig::ampId (c, "mic_pos")), [this] (float v) { pos = v; repaint(); }, nullptr);
    dAtt = std::make_unique<juce::ParameterAttachment> (*st.getParameter (rig::ampId (c, "mic_dist")), [this] (float v) { dist = v; repaint(); }, nullptr);
    for (auto* a : { cabAtt.get(), micAtt.get(), posAtt.get(), dAtt.get() }) a->sendInitialUpdate();
    distAtt.reset();
    distSlider = std::make_unique<juce::Slider> (juce::Slider::LinearVertical, juce::Slider::NoTextBox);
    distSlider->setPopupDisplayEnabled (true, true, nullptr, 900);
    distSlider->setTooltip ("Distanza del microfono dalla cassa");
    distAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, rig::ampId (c, "mic_dist"), *distSlider);
    addAndMakeVisible (*distSlider);
    tabA.setToggleState (s == 0, juce::dontSendNotification);
    tabB.setToggleState (s == 1, juce::dontSendNotification);
    resized();
    repaint();
}

juce::Rectangle<float> CabOverlay::cone() const
{
    return juce::Rectangle<float> ((float) card.getX() + 40.0f, (float) card.getY() + 70.0f, 440.0f, 440.0f);
}

void CabOverlay::resized()
{
    Overlay::resized();
    tabA.setBounds (card.getX() + 40, card.getY() + 18, 120, 34);
    tabB.setBounds (card.getX() + 166, card.getY() + 18, 120, 34);
    if (onBtn) onBtn->setBounds (card.getX() + 300, card.getY() + 18, 130, 34);
    const int x = card.getX() + 540, w = card.getRight() - 40 - x;
    for (int i = 0; i < 4; ++i) cabBtns[(size_t) i].setBounds (x + (i % 2) * (w / 2 + 3), card.getY() + 92 + (i / 2) * 40, w / 2 - 3, 34);
    load.setBounds (x, card.getY() + 176, w, 34);
    for (int i = 0; i < 3; ++i) micBtns[(size_t) i].setBounds (x + i * (w / 3), card.getY() + 280, w / 3 - 6, 90);
    if (distSlider) distSlider->setBounds (card.getX() + 490, card.getY() + 90, 30, 400);
}

void CabOverlay::paint (juce::Graphics& g)
{
    Overlay::paint (g);
    const auto cr = cone();
    g.setOpacity (1.0f);
    g.drawImage (speaker, cr, juce::RectanglePlacement::centred);
    // Speaker centre in the artwork: (226, 232) of 470x470, usable cone radius ~175.
    const float s = cr.getWidth() / 470.0f;
    const juce::Point<float> c (cr.getX() + 226.0f * s, cr.getY() + 232.0f * s);
    const float R = 175.0f * s;
    const juce::Point<float> m (c.x + pos * R, c.y);
    // Mic seen from the front: closer = bigger.
    const float mr = (34.0f - 18.0f * dist) * s * 1.4f;
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (juce::Rectangle<float> (mr * 2, mr * 2).withCentre (m).translated (4, 7));
    juce::ColourGradient grille (juce::Colour (0xff4a4a54), m.x - mr, m.y - mr, juce::Colour (0xff0e0e12), m.x + mr, m.y + mr, false);
    g.setGradientFill (grille);
    g.fillEllipse (juce::Rectangle<float> (mr * 2, mr * 2).withCentre (m));
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    for (float k = 0.3f; k < 1.0f; k += 0.23f) g.drawEllipse (juce::Rectangle<float> (mr * 2 * k, mr * 2 * k).withCentre (m), 1.0f);
    g.setColour (kGreen);
    g.drawEllipse (juce::Rectangle<float> (mr * 2, mr * 2).withCentre (m), draggingMic ? 3.0f : 2.0f);
    g.setColour (juce::Colour (0xffb0b0c2));
    g.setFont (skinFont (SkinId::ToxicLab, FontRole::Label, 14.0f));
    g.drawText ("CENTRO", juce::Rectangle<float> (c.x - 40, cr.getBottom() - 4, 80, 18), juce::Justification::centred);
    g.drawText ("BORDO", juce::Rectangle<float> (c.x + R - 40, cr.getBottom() - 4, 80, 18), juce::Justification::centred);
    g.drawText ("LONTANO", card.getX() + 470, card.getY() + 70, 70, 16, juce::Justification::centred);
    g.drawText ("VICINO", card.getX() + 470, card.getY() + 494, 70, 16, juce::Justification::centred);
    const int x = card.getX() + 540;
    g.setColour (juce::Colour (0xff8f8fa6));
    g.drawText ("TIPO DI CASSA", x, card.getY() + 70, 300, 18, juce::Justification::centredLeft);
    g.drawText ("MICROFONO", x, card.getY() + 256, 300, 18, juce::Justification::centredLeft);
    const auto f = proc.getUserIR (slot);
    g.setFont (skinFont (SkinId::ToxicLab, FontRole::Body, 14.0f));
    g.drawText (f.existsAsFile() ? "IR utente: " + f.getFileName() : "Nessuna IR utente caricata", x, card.getY() + 214, card.getRight() - 40 - x, 18, juce::Justification::centredLeft);
    // Highlight the selected mic.
    const auto mb = micBtns[(size_t) micIdx].getBounds().toFloat();
    g.setColour (kGreen);
    g.drawRoundedRectangle (mb.expanded (2.0f), 8.0f, 2.0f);
    g.setColour (juce::Colour (0xffdcdce6));
    g.setFont (skinFont (SkinId::ToxicLab, FontRole::Label, 15.0f));
    for (int i = 0; i < 3; ++i)
        g.drawText (rig::micNames[i].toUpperCase(), micBtns[(size_t) i].getBounds().translated (0, 94).withHeight (18), juce::Justification::centred);
    g.setColour (juce::Colour (0xff8f8fa6));
    g.setFont (skinFont (SkinId::ToxicLab, FontRole::Body, 15.0f));
    g.drawFittedText ("Trascina il microfono sul cono: al centro suona piu' brillante e diretto, verso il bordo piu' scuro e morbido. "
                      "La barra a sinistra sposta il microfono vicino/lontano.",
                      juce::Rectangle<int> (x, card.getY() + 410, card.getRight() - 40 - x, 90), juce::Justification::topLeft, 5);
}

void CabOverlay::mouseDown (const juce::MouseEvent& e)
{
    if (cone().contains (e.position)) { draggingMic = true; posAtt->beginGesture(); mouseDrag (e); return; }
    Overlay::mouseDown (e);
}

void CabOverlay::mouseDrag (const juce::MouseEvent& e)
{
    if (! draggingMic) return;
    const auto cr = cone();
    const float s = cr.getWidth() / 470.0f;
    const float cx = cr.getX() + 226.0f * s, R = 175.0f * s;
    posAtt->setValueAsPartOfGesture (juce::jlimit (0.0f, 1.0f, (e.position.x - cx) / R));
}

void CabOverlay::mouseUp (const juce::MouseEvent&)
{
    if (draggingMic) { draggingMic = false; posAtt->endGesture(); repaint(); }
}

// ============================================================================ StageView
StageView::StageView (ToxicRigProcessor& p) : proc (p), amp (p), side (p, amp), floor (p)
{
    stageImage = img (BinaryData::stage_jpg, BinaryData::stage_jpgSize);
    addAndMakeVisible (amp);
    addAndMakeVisible (side);
    addAndMakeVisible (floor);
    floor.onOpenPedal = [this] (int id) { openPedal (id); };
    side.onOpenCab = [this] { openCab(); };
}

void StageView::paint (juce::Graphics& g)
{
    g.setOpacity (1.0f);
    g.drawImage (stageImage, getLocalBounds().toFloat(), juce::RectanglePlacement::fillDestination);
    juce::ColourGradient vignette (juce::Colours::transparentBlack, getWidth() * 0.5f, getHeight() * 0.45f,
                                   juce::Colours::black.withAlpha (0.55f), 0.0f, 0.0f, true);
    g.setGradientFill (vignette);
    g.fillRect (getLocalBounds());
}

void StageView::resized()
{
    amp.setBounds (14, 8, 880, 470);
    side.setBounds (906, 14, 360, 462);
    floor.setBounds (0, 486, getWidth(), getHeight() - 486);
    if (overlay) overlay->setBounds (getLocalBounds());
}

void StageView::openPedal (int pedal)
{
    auto o = std::make_unique<PedalOverlay> (proc, pedal);
    juce::Component::SafePointer<StageView> safe (this); // closed asynchronously: the overlay can't delete itself mid-click
    o->onClose = [safe] { juce::MessageManager::callAsync ([safe] { if (safe != nullptr) safe->closeOverlay(); }); };
    overlay = std::move (o);
    addAndMakeVisible (*overlay);
    overlay->setBounds (getLocalBounds());
}

void StageView::openCab()
{
    auto o = std::make_unique<CabOverlay> (proc, amp.getSlot());
    juce::Component::SafePointer<StageView> safe (this); // closed asynchronously: the overlay can't delete itself mid-click
    o->onClose = [safe] { juce::MessageManager::callAsync ([safe] { if (safe != nullptr) safe->closeOverlay(); }); };
    overlay = std::move (o);
    addAndMakeVisible (*overlay);
    overlay->setBounds (getLocalBounds());
}

void StageView::closeOverlay() { overlay.reset(); }
} // namespace ui
