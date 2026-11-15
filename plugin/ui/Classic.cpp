#include "Classic.h"

namespace ui
{
// ============================================================================ ChainStrip
ChainStrip::ChainStrip (ToxicRigProcessor& p) : proc (p) { startTimerHz (8); }

std::vector<ChainStrip::Item> ChainStrip::layoutItems() const
{
    const auto routing = proc.getRouting();
    std::vector<int> seq { -1 };
    for (int i = 0; i < routing.numBoard; ++i) seq.push_back (routing.board[(size_t) i]);
    seq.push_back (kAmp); seq.push_back (kCab);
    for (int i = 0; i < routing.numLoop; ++i) seq.push_back (routing.loop[(size_t) i]);
    seq.push_back (-2);
    const float gap = 10.0f;
    const float w = std::min (82.0f, ((float) getWidth() - gap * (float) (seq.size() + 1)) / (float) seq.size());
    const float total = w * (float) seq.size() + gap * (float) (seq.size() - 1);
    float x = ((float) getWidth() - total) * 0.5f;
    std::vector<Item> items;
    for (int id : seq) { items.push_back ({ id, juce::Rectangle<float> (x, 26.0f, w, (float) getHeight() - 36.0f) }); x += w + gap; }
    return items;
}

int ChainStrip::hitTest (juce::Point<float> p) const
{
    for (auto& it : layoutItems()) if (it.r.contains (p)) return it.id;
    return -99;
}

void ChainStrip::paint (juce::Graphics& g)
{
    const auto sk = skinOf (*this);
    const auto& p = palette (sk);
    const auto items = layoutItems();
    const auto& specs = rig::pedalSpecs();
    auto& s = proc.apvts;
    const auto routing = proc.getRouting();
    const bool schem = sk == SkinId::Schematic;

    auto zone = [&] (int firstId, int lastId, const juce::String& text, juce::Colour c)
    {
        juce::Rectangle<float> a, b;
        for (auto& it : items) { if (it.id == firstId) a = it.r; if (it.id == lastId) b = it.r; }
        if (a.isEmpty()) return;
        auto r = a.getUnion (b).expanded (5.0f, 0).withY (2.0f).withHeight ((float) getHeight() - 4.0f);
        if (schem) { g.setColour (c.withAlpha (0.5f)); g.drawRect (r, 1.0f); }
        else { g.setColour (c.withAlpha (0.07f)); g.fillRoundedRectangle (r, 6.0f); }
        g.setColour (c.withAlpha (0.9f));
        g.setFont (skinFont (sk, FontRole::Label, 13.0f));
        g.drawText (text, r.withHeight (20.0f), juce::Justification::centred);
    };
    if (routing.numBoard > 0) zone (routing.board[0], routing.board[(size_t) routing.numBoard - 1], schem ? "// pre-amp" : "PEDALIERA (prima dell'ampli)", p.accent);
    if (routing.numLoop > 0) zone (routing.loop[0], routing.loop[(size_t) routing.numLoop - 1], schem ? "// fx loop" : "FX LOOP (dopo ampli e cassa)", p.accent2);

    g.setColour (schem ? p.accent.withAlpha (0.6f) : p.line);
    for (size_t i = 1; i < items.size(); ++i)
        g.drawLine (items[i - 1].r.getRight(), items[i].r.getCentreY(), items[i].r.getX(), items[i].r.getCentreY(), schem ? 1.5f : 2.0f);

    for (auto& it : items)
    {
        if (it.id == dragging && dragMoved) continue;
        auto r = it.r;
        juce::String name;
        bool on = true;
        const bool isPedal = it.id >= 0 && it.id < tz::kNumPedals;
        if (isPedal) { name = specs[(size_t) it.id].displayName; on = s.getRawParameterValue (rig::pedalOnId (it.id))->load() > 0.5f; }
        else if (it.id == kAmp) name = "AMP";
        else if (it.id == kCab) name = "CAB";
        else name = it.id == -1 ? "IN" : "OUT";
        const bool sel = it.id == selected;
        const float rad = schem ? 2.0f : 6.0f;
        g.setColour (schem ? p.bg : (isPedal ? p.panel2 : p.panel));
        g.fillRoundedRectangle (r, rad);
        g.setColour (sel ? p.accent : (schem ? (on ? p.accent.withAlpha (0.6f) : p.line) : p.line));
        g.drawRoundedRectangle (r, rad, sel ? 2.0f : 1.0f);
        if (schem && it.id == kAmp)
        {
            // Two triode symbols.
            for (int k = 0; k < 2; ++k)
            {
                const auto c = juce::Point<float> (r.getX() + r.getWidth() * (0.3f + 0.4f * (float) k), r.getCentreY() - 6.0f);
                const float rr = std::min (r.getWidth() * 0.17f, 12.0f);
                g.setColour (p.accent);
                g.drawEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2, 1.2f);
                g.drawLine (c.x - rr * 0.5f, c.y - rr * 0.4f, c.x + rr * 0.5f, c.y - rr * 0.4f, 1.2f);
                g.drawLine (c.x - rr * 0.5f, c.y + rr * 0.4f, c.x + rr * 0.5f, c.y + rr * 0.4f, 1.0f);
            }
        }
        if (isPedal)
        {
            const auto led = juce::Rectangle<float> (10.0f, 10.0f).withCentre ({ r.getCentreX(), r.getY() + 14.0f });
            if (on) { g.setColour (p.accent.withAlpha (0.3f)); g.fillEllipse (led.expanded (4.0f)); }
            g.setColour (on ? p.accent : p.panel2.brighter (0.15f));
            if (schem) g.fillRect (led.reduced (1.0f)); else g.fillEllipse (led);
        }
        g.setColour (on ? p.text : p.dim);
        g.setFont (skinFont (sk, FontRole::Label, schem ? 11.5f : 15.0f));
        g.drawFittedText (sk == SkinId::StudioNoir ? name.toUpperCase() : name,
                          r.reduced (4.0f).withTrimmedTop (isPedal ? 20.0f : (schem && it.id == kAmp ? 34.0f : 0.0f)).toNearestInt(),
                          schem && it.id == kAmp ? juce::Justification::centredBottom : juce::Justification::centred, 3);
    }
    if (dragging >= 0 && dragMoved)
    {
        auto r = juce::Rectangle<float> (70.0f, (float) getHeight() - 36.0f).withCentre (dragPos);
        g.setColour (p.accent2.withAlpha (0.85f));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (p.text);
        g.setFont (skinFont (sk, FontRole::Label, 14.0f));
        g.drawFittedText (specs[(size_t) dragging].displayName, r.reduced (4.0f).toNearestInt(), juce::Justification::centred, 3);
    }
}

void ChainStrip::mouseDown (const juce::MouseEvent& e)
{
    const int id = hitTest (e.position);
    dragging = (id >= 0 && id < tz::kNumPedals) ? id : -1;
    dragMoved = false;
    dragPos = e.position;
}

void ChainStrip::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging < 0) return;
    if (e.getDistanceFromDragStart() > 5) dragMoved = true;
    dragPos = e.position;
    repaint();
}

void ChainStrip::mouseUp (const juce::MouseEvent& e)
{
    const int id = hitTest (e.position);
    if (! dragMoved)
    {
        if (id >= 0 && id < tz::kNumPedals)
            for (auto& it : layoutItems())
                if (it.id == id && e.position.y < it.r.getY() + 26.0f)
                {
                    auto* prm = proc.apvts.getParameter (rig::pedalOnId (id));
                    prm->beginChangeGesture();
                    prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.0f : 1.0f);
                    prm->endChangeGesture();
                    dragging = -1;
                    repaint();
                    return;
                }
        if ((id >= 0 && id < tz::kNumPedals) || id == kAmp || id == kCab) { selected = id; if (onSelect) onSelect (id); }
        dragging = -1;
        repaint();
        return;
    }
    // Drop: count the pedals left of the drop point in the target zone.
    const int moved = dragging;
    dragging = -1;
    bool afterAmp = false, toLoop = false;
    int index = 0, ampX = 0;
    juce::ignoreUnused (ampX);
    for (auto& it : layoutItems())
    {
        if (it.id == kAmp && dragPos.x > it.r.getCentreX()) toLoop = true;
    }
    for (auto& it : layoutItems())
    {
        if (it.id == kAmp) afterAmp = true;
        if (it.id == moved || ! (it.id >= 0 && it.id < tz::kNumPedals)) continue;
        if (afterAmp == toLoop && it.r.getCentreX() < dragPos.x) ++index;
    }
    proc.setRouting (proc.getRouting().moved (moved, toLoop, index));
    repaint();
}

// ============================================================================ PedalPanel
PedalPanel::PedalPanel (ToxicRigProcessor& p, int pedalIndex)
    : pedal (pedalIndex), onButton (p.apvts, rig::pedalOnId (pedalIndex), "ON", ParamToggle::Look::Pill)
{
    auto& s = p.apvts;
    const auto& spec = rig::pedalSpecs()[(size_t) pedal];
    for (int i = 0; i < (int) spec.params.size(); ++i)
    {
        const auto& ps = spec.params[(size_t) i];
        const auto id = rig::pedalParamId (pedal, i);
        if (ps.isBool) controls.push_back (std::make_unique<ParamToggle> (s, id, ps.name, ParamToggle::Look::Pill));
        else if (! ps.choices.isEmpty()) controls.push_back (std::make_unique<ChoiceButton> (s, id));
        else controls.push_back (std::make_unique<CaptionKnob> (s, id, ps.name));
        addAndMakeVisible (*controls.back());
    }
    addAndMakeVisible (onButton);
}

void PedalPanel::paint (juce::Graphics& g)
{
    const auto sk = skinOf (*this);
    const auto& p = palette (sk);
    const auto& spec = rig::pedalSpecs()[(size_t) pedal];
    auto r = getLocalBounds().toFloat();
    g.setColour (p.panel);
    g.fillRoundedRectangle (r, sk == SkinId::Schematic ? 3.0f : 8.0f);
    g.setColour (p.line);
    g.drawRoundedRectangle (r.reduced (0.5f), sk == SkinId::Schematic ? 3.0f : 8.0f, 1.0f);
    g.setColour (p.text);
    g.setFont (skinFont (sk, FontRole::Title, sk == SkinId::StudioNoir ? 40.0f : 28.0f));
    g.drawText (sk == SkinId::Schematic ? "// " + juce::String (spec.key) : juce::String (spec.displayName), r.reduced (24.0f, 16.0f).removeFromTop (40.0f), juce::Justification::centredLeft);
    g.setColour (p.dim);
    g.setFont (skinFont (sk, FontRole::Body, 17.0f));
    g.drawFittedText (spec.description, r.reduced (24.0f, 0).withTrimmedTop (64.0f).removeFromTop (44.0f).toNearestInt(), juce::Justification::topLeft, 2);
}

void PedalPanel::resized()
{
    onButton.setBounds (getWidth() - 120, 22, 96, 32);
    const int n = (int) controls.size();
    const int cw = 116;
    int x = (getWidth() - n * cw) / 2;
    for (auto& c : controls)
    {
        if (dynamic_cast<CaptionKnob*> (c.get()) != nullptr) c->setBounds (x + 6, 140, cw - 12, 140);
        else c->setBounds (x + 8, 196, cw - 16, 30);
        x += cw;
    }
}

// ============================================================================ AmpPanel
AmpPanel::AmpPanel (ToxicRigProcessor& p)
    : proc (p), bOn (p.apvts, rig::ids::ampBOn, "AMPLI B ON", ParamToggle::Look::Pill)
{
    auto& s = p.apvts;
    for (int i = 0; i < 2; ++i)
    {
        const char slot = i == 0 ? 'a' : 'b';
        auto& sd = sides[(size_t) i];
        sd.model = std::make_unique<ChoiceButton> (s, rig::ampId (slot, "model"));
        sd.channel = std::make_unique<ChoiceButton> (s, rig::ampId (slot, "channel"));
        sd.bright = std::make_unique<ParamToggle> (s, rig::ampId (slot, "bright"), "Bright", ParamToggle::Look::Pill);
        addAndMakeVisible (*sd.model); addAndMakeVisible (*sd.channel); addAndMakeVisible (*sd.bright);
        const std::pair<const char*, const char*> k[] = { { "gain", "Gain" }, { "bass", "Bass" }, { "mid", "Middle" }, { "treble", "Treble" },
                                                          { "presence", "Presence" }, { "resonance", "Resonance" }, { "master", "Master" }, { "sag", "Sag" } };
        for (auto& [id, cap] : k)
        {
            sd.knobs.push_back (std::make_unique<CaptionKnob> (s, rig::ampId (slot, id), cap));
            addAndMakeVisible (*sd.knobs.back());
        }
    }
    addAndMakeVisible (bOn);
    levelA = std::make_unique<CaptionKnob> (s, rig::ampId ('a', "level"), "Livello A");
    levelB = std::make_unique<CaptionKnob> (s, rig::ampId ('b', "level"), "Livello B");
    mix = std::make_unique<CaptionKnob> (s, rig::ids::abMix, "Mix A/B");
    width = std::make_unique<CaptionKnob> (s, rig::ids::width, "Width");
    for (auto* c : { levelA.get(), levelB.get(), mix.get(), width.get() }) addAndMakeVisible (c);
    addChildComponent (scope);
    startTimerHz (30);
}

void AmpPanel::timerCallback()
{
    for (int i = 0; i < 2; ++i)
    {
        const bool british = (int) proc.apvts.getRawParameterValue (rig::ampId (i == 0 ? 'a' : 'b', "model"))->load() == 0;
        sides[(size_t) i].knobs[4]->setCaption (british ? "Cut" : "Presence");
        sides[(size_t) i].knobs[5]->setCaption (british ? "Depth" : "Resonance");
    }
    const bool bEnabled = bOn.getToggleState();
    for (auto* c : std::initializer_list<juce::Component*> { sides[1].model.get(), sides[1].channel.get(), sides[1].bright.get(), levelB.get(), mix.get(), width.get() })
        c->setAlpha (bEnabled ? 1.0f : 0.4f);
    for (auto& k : sides[1].knobs) k->setAlpha (bEnabled ? 1.0f : 0.4f);
    const bool schem = skinOf (*this) == SkinId::Schematic;
    scope.setVisible (schem);
    if (schem)
    {
        std::array<float, 1024> buf;
        proc.engine.copyScope (buf.data(), (int) buf.size());
        scope.set (buf.data(), (int) buf.size());
    }
}

void AmpPanel::paint (juce::Graphics& g)
{
    const auto sk = skinOf (*this);
    const auto& p = palette (sk);
    auto r = getLocalBounds().toFloat();
    g.setColour (p.panel);
    g.fillRoundedRectangle (r, sk == SkinId::Schematic ? 3.0f : 8.0f);
    if (sk == SkinId::StudioNoir)
    {
        // Brushed aluminium faceplates.
        const float sideW = (r.getWidth() - 170.0f) * 0.5f;
        for (int i = 0; i < 2; ++i)
        {
            auto fp = juce::Rectangle<float> (i == 0 ? 8.0f : sideW + 162.0f, 8.0f, sideW, r.getHeight() - 16.0f);
            juce::ColourGradient al (juce::Colour (0xff6f6a62), fp.getX(), fp.getY(), juce::Colour (0xff2c2a27), fp.getX(), fp.getBottom(), false);
            al.addColour (0.08, juce::Colour (0xff4b4741));
            g.setGradientFill (al);
            g.fillRoundedRectangle (fp, 8.0f);
            g.setColour (juce::Colours::black);
            g.drawRoundedRectangle (fp, 8.0f, 1.0f);
            for (auto sc : { juce::Point<float> (fp.getX() + 10, fp.getY() + 10), juce::Point<float> (fp.getRight() - 10, fp.getY() + 10),
                             juce::Point<float> (fp.getX() + 10, fp.getBottom() - 10), juce::Point<float> (fp.getRight() - 10, fp.getBottom() - 10) })
            {
                g.setColour (juce::Colour (0xff1a1917)); g.fillEllipse (sc.x - 4, sc.y - 4, 8, 8);
                g.setColour (juce::Colour (0xff8a857c)); g.drawLine (sc.x - 2.5f, sc.y - 2.5f, sc.x + 2.5f, sc.y + 2.5f, 1.0f);
            }
        }
    }
    const float sideW = (r.getWidth() - 170.0f) * 0.5f;
    g.setFont (skinFont (sk, FontRole::Title, sk == SkinId::StudioNoir ? 34.0f : 22.0f));
    g.setColour (p.accent);
    g.drawText (sk == SkinId::Schematic ? "amp_a" : "AMPLI A", juce::Rectangle<float> (24, 14, 200, 32), juce::Justification::centredLeft);
    g.setColour (p.accent2);
    g.drawText (sk == SkinId::Schematic ? "amp_b" : "AMPLI B", juce::Rectangle<float> (sideW + 170 + 24, 14, 200, 32), juce::Justification::centredLeft);
    if (sk != SkinId::StudioNoir)
    {
        g.setColour (p.line);
        g.drawVerticalLine ((int) (sideW + 10), 20.0f, r.getBottom() - 20.0f);
        g.drawVerticalLine ((int) (sideW + 160), 20.0f, r.getBottom() - 20.0f);
    }
}

void AmpPanel::resized()
{
    const int sideW = (getWidth() - 170) / 2;
    for (int i = 0; i < 2; ++i)
    {
        auto& sd = sides[(size_t) i];
        const int x0 = i == 0 ? 0 : sideW + 170;
        sd.model->setBounds (x0 + 170, 16, 130, 30);
        sd.channel->setBounds (x0 + 308, 16, 100, 30);
        sd.bright->setBounds (x0 + 416, 16, 84, 30);
        const int kw = (sideW - 40) / 4;
        for (int k = 0; k < (int) sd.knobs.size(); ++k)
            sd.knobs[(size_t) k]->setBounds (x0 + 20 + (k % 4) * kw, 70 + (k / 4) * 220, kw, 190);
    }
    const int cx = sideW + 18;
    bOn.setBounds (cx, 20, 134, 32);
    levelA->setBounds (cx, 64, 64, 110);
    levelB->setBounds (cx + 70, 64, 64, 110);
    mix->setBounds (cx, 180, 64, 110);
    width->setBounds (cx + 70, 180, 64, 110);
    scope.setBounds (cx, 310, 134, 120);
}

// ============================================================================ MicPad
MicPad::MicPad (juce::AudioProcessorValueTreeState& s, char slot)
    : pos (s.getParameter (rig::ampId (slot, "mic_pos"))),
      dist (s.getParameter (rig::ampId (slot, "mic_dist"))),
      posAtt (*pos, [this] (float) { repaint(); }, nullptr),
      distAtt (*dist, [this] (float) { repaint(); }, nullptr)
{
    posAtt.sendInitialUpdate();
    distAtt.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
}

void MicPad::paint (juce::Graphics& g)
{
    const auto sk = skinOf (*this);
    const auto& p = palette (sk);
    auto r = getLocalBounds().toFloat();
    g.setColour (p.panel2);
    g.fillRoundedRectangle (r, 6.0f);
    auto sp = r.reduced (18.0f).withTrimmedBottom (18.0f);
    const float rad = std::min (sp.getWidth(), sp.getHeight()) * 0.48f;
    const auto c = juce::Point<float> (sp.getX() + rad + 6.0f, sp.getCentreY());
    g.setColour (juce::Colour (0xff101015));
    g.fillEllipse (juce::Rectangle<float> (rad * 2, rad * 2).withCentre (c));
    g.setColour (p.line);
    for (float k : { 1.0f, 0.75f, 0.5f }) g.drawEllipse (juce::Rectangle<float> (rad * 2 * k, rad * 2 * k).withCentre (c), 1.0f);
    g.setColour (p.accent2.withAlpha (0.7f));
    g.fillEllipse (juce::Rectangle<float> (rad * 0.5f, rad * 0.5f).withCentre (c));
    const float px = c.x + pos->convertFrom0to1 (pos->getValue()) * rad;
    const float d = dist->convertFrom0to1 (dist->getValue());
    const float mr = 16.0f - 7.0f * d;
    g.setColour (p.accent.withAlpha (0.25f));
    g.fillEllipse (juce::Rectangle<float> (mr * 3, mr * 3).withCentre ({ px, c.y }));
    g.setColour (p.accent);
    g.fillEllipse (juce::Rectangle<float> (mr, mr).withCentre ({ px, c.y }));
    g.setColour (p.dim);
    g.setFont (skinFont (sk, FontRole::Body, 14.0f));
    g.drawText ("centro", juce::Rectangle<float> (c.x - 30, sp.getBottom() + 2, 60, 16), juce::Justification::centred);
    g.drawText ("bordo", juce::Rectangle<float> (c.x + rad - 30, sp.getBottom() + 2, 60, 16), juce::Justification::centred);
    auto info = r.withTrimmedLeft (c.x + rad + 20.0f - r.getX()).reduced (8.0f);
    g.setColour (p.text);
    g.setFont (skinFont (sk, FontRole::Body, 15.0f));
    g.drawFittedText ("Trascina a destra/sinistra: posizione sul cono.\nSu/giu': distanza.\n\nPosizione: " + juce::String ((int) (pos->convertFrom0to1 (pos->getValue()) * 100))
                      + "%\nDistanza: " + juce::String ((int) (d * 100)) + "%", info.toNearestInt(), juce::Justification::centredLeft, 8);
}

void MicPad::mouseDown (const juce::MouseEvent& e) { posAtt.beginGesture(); distAtt.beginGesture(); mouseDrag (e); }

void MicPad::mouseDrag (const juce::MouseEvent& e)
{
    auto sp = getLocalBounds().toFloat().reduced (18.0f).withTrimmedBottom (18.0f);
    const float rad = std::min (sp.getWidth(), sp.getHeight()) * 0.48f;
    const float cx = sp.getX() + rad + 6.0f;
    posAtt.setValueAsPartOfGesture (juce::jlimit (0.0f, 1.0f, (e.position.x - cx) / rad));
    distAtt.setValueAsPartOfGesture (juce::jlimit (0.0f, 1.0f, (e.position.y - sp.getY()) / sp.getHeight()));
}

void MicPad::mouseUp (const juce::MouseEvent&) { posAtt.endGesture(); distAtt.endGesture(); }

// ============================================================================ CabPanel
CabPanel::CabPanel (ToxicRigProcessor& p) : proc (p)
{
    auto& s = p.apvts;
    for (int i = 0; i < 2; ++i)
    {
        const char slot = i == 0 ? 'a' : 'b';
        auto& sd = sides[(size_t) i];
        sd.on = std::make_unique<ParamToggle> (s, rig::ampId (slot, "cab_on"), "CASSA ON", ParamToggle::Look::Pill);
        sd.cab = std::make_unique<ChoiceButton> (s, rig::ampId (slot, "cab"));
        sd.mic = std::make_unique<ChoiceButton> (s, rig::ampId (slot, "mic"));
        sd.pad = std::make_unique<MicPad> (s, slot);
        sd.file.setColour (juce::Label::textColourId, palette (SkinId::ToxicLab).dim);
        const auto f = p.getUserIR (i);
        sd.file.setText (f.existsAsFile() ? f.getFileName() : "nessuna IR caricata", juce::dontSendNotification);
        sd.load.onClick = [this, i]
        {
            chooser = std::make_unique<juce::FileChooser> ("Scegli una IR (.wav)", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.wav;*.aif;*.aiff");
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this, i] (const juce::FileChooser& fc)
            {
                const auto f = fc.getResult();
                if (! f.existsAsFile()) return;
                proc.setUserIR (i, f);
                sides[(size_t) i].file.setText (f.getFileName(), juce::dontSendNotification);
            });
        };
        for (juce::Component* c : std::initializer_list<juce::Component*> { sd.on.get(), sd.cab.get(), sd.mic.get(), &sd.load, &sd.file, sd.pad.get() })
            addAndMakeVisible (c);
    }
}

void CabPanel::paint (juce::Graphics& g)
{
    const auto sk = skinOf (*this);
    const auto& p = palette (sk);
    auto r = getLocalBounds().toFloat();
    g.setColour (p.panel);
    g.fillRoundedRectangle (r, sk == SkinId::Schematic ? 3.0f : 8.0f);
    g.setFont (skinFont (sk, FontRole::Title, sk == SkinId::StudioNoir ? 34.0f : 22.0f));
    g.setColour (p.accent);
    g.drawText (sk == SkinId::Schematic ? "cab_a" : "CASSA A", juce::Rectangle<float> (24, 14, 200, 32), juce::Justification::centredLeft);
    g.setColour (p.accent2);
    g.drawText (sk == SkinId::Schematic ? "cab_b" : "CASSA B", juce::Rectangle<float> (r.getWidth() * 0.5f + 24, 14, 200, 32), juce::Justification::centredLeft);
    g.setColour (p.line);
    g.drawVerticalLine (getWidth() / 2, 20.0f, r.getBottom() - 20.0f);
}

void CabPanel::resized()
{
    const int half = getWidth() / 2;
    for (int i = 0; i < 2; ++i)
    {
        auto& sd = sides[(size_t) i];
        const int x0 = i * half;
        sd.on->setBounds (x0 + 170, 16, 120, 30);
        sd.cab->setBounds (x0 + 24, 60, 180, 30);
        sd.load.setBounds (x0 + 214, 60, 120, 30);
        sd.file.setBounds (x0 + 340, 60, half - 360, 30);
        sd.mic->setBounds (x0 + 24, 98, 180, 30);
        sd.pad->setBounds (x0 + 24, 140, half - 48, getHeight() - 160);
    }
}

// ============================================================================ ClassicView
ClassicView::ClassicView (ToxicRigProcessor& p) : proc (p), chain (p), ampPanel (p), cabPanel (p)
{
    addAndMakeVisible (chain);
    chain.onSelect = [this] (int id) { select (id); };
    for (int i = 0; i < tz::kNumPedals; ++i)
    {
        pedalPanels.push_back (std::make_unique<PedalPanel> (p, i));
        addChildComponent (*pedalPanels.back());
    }
    addChildComponent (ampPanel);
    addChildComponent (cabPanel);
    select (ChainStrip::kAmp);
}

void ClassicView::select (int id)
{
    chain.selected = id;
    for (int i = 0; i < (int) pedalPanels.size(); ++i) pedalPanels[(size_t) i]->setVisible (i == id);
    ampPanel.setVisible (id == ChainStrip::kAmp);
    cabPanel.setVisible (id == ChainStrip::kCab);
    chain.repaint();
}

void ClassicView::resized()
{
    chain.setBounds (10, 8, getWidth() - 20, 132);
    const juce::Rectangle<int> detail (16, 150, getWidth() - 32, getHeight() - 166);
    for (auto& pp : pedalPanels) pp->setBounds (detail);
    ampPanel.setBounds (detail);
    cabPanel.setBounds (detail);
}
} // namespace ui
