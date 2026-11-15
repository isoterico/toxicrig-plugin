#include "PluginEditor.h"

namespace ui
{
// ============================================================================ LookAndFeel
LookAndFeel::LookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, col::bg);
    setColour (juce::Label::textColourId, col::text);
    setColour (juce::Slider::textBoxTextColourId, col::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId, col::panel2);
    setColour (juce::ComboBox::outlineColourId, col::line);
    setColour (juce::ComboBox::textColourId, col::text);
    setColour (juce::ComboBox::arrowColourId, col::green);
    setColour (juce::PopupMenu::backgroundColourId, col::panel);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::purple.withAlpha (0.6f));
    setColour (juce::PopupMenu::textColourId, col::text);
    setColour (juce::TextButton::buttonColourId, col::panel2);
    setColour (juce::TextButton::textColourOffId, col::text);
    setColour (juce::TextButton::textColourOnId, col::bg);
    setColour (juce::ToggleButton::textColourId, col::text);
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float r = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = start + pos * (end - start);
    const float arcR = r - 2.0f;
    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, start, end, true);
    g.setColour (col::line);
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path val;
    val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, start, angle, true);
    g.setColour (s.isEnabled() ? col::green : col::dim);
    g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const float kr = r * 0.72f;
    juce::ColourGradient grad (juce::Colour (0xff3a3a48), c.x - kr * 0.5f, c.y - kr, juce::Colour (0xff101016), c.x + kr * 0.4f, c.y + kr, false);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.2f);
    juce::Path ptr;
    ptr.addRoundedRectangle (-1.5f, -kr + 3.0f, 3.0f, kr * 0.55f, 1.5f);
    g.setColour (col::text);
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

void LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    const auto r = b.getLocalBounds().toFloat();
    const float d = std::min (14.0f, r.getHeight() - 4.0f);
    const juce::Rectangle<float> led (r.getX() + 2, r.getCentreY() - d * 0.5f, d, d);
    const bool on = b.getToggleState();
    if (on)
    {
        g.setColour (col::green.withAlpha (0.25f));
        g.fillEllipse (led.expanded (3.0f));
    }
    g.setColour (on ? col::green : juce::Colour (0xff2a2a33));
    g.fillEllipse (led);
    g.setColour (highlighted ? col::text : col::line);
    g.drawEllipse (led, 1.0f);
    g.setColour (col::text);
    g.setFont (font (13.0f));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (d + 8), juce::Justification::centredLeft);
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    g.setColour (on ? col::green : (down ? col::purple : (highlighted ? col::panel2.brighter (0.15f) : col::panel2)));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (col::line);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

// ============================================================================ Knob
Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, const juce::String& caption)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 16);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxTextColourId, col::text);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible (slider);
    label.setText (caption, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, col::dim);
    addAndMakeVisible (label);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, paramId, slider);
    slider.setDoubleClickReturnValue (true, s.getParameter (paramId)->convertFrom0to1 (s.getParameter (paramId)->getDefaultValue()));
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (16));
    slider.setBounds (r);
}

// ============================================================================ Meter / Tuner
void Meter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::panel2);
    g.fillRoundedRectangle (r, 2.0f);
    auto bar = [&] (juce::Rectangle<float> b, float v)
    {
        const float db = juce::Decibels::gainToDecibels (v, -60.0f);
        const float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        auto fill = b.withTop (b.getBottom() - b.getHeight() * frac);
        g.setColour (db > -1.0f ? juce::Colours::red : (db > -12.0f ? juce::Colour (0xffffd400) : col::green));
        g.fillRect (fill);
    };
    auto left = r.reduced (2.0f);
    auto right = left.removeFromRight (left.getWidth() * 0.5f);
    bar (left.withTrimmedRight (1.0f), lv);
    bar (right.withTrimmedLeft (1.0f), rv);
}

void TunerDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::panel2);
    g.fillRoundedRectangle (r, 4.0f);
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const bool valid = note >= 0;
    const bool inTune = valid && std::abs (c) < 3.0;
    g.setColour (valid ? (inTune ? col::green : col::text) : col::dim);
    g.setFont (font (20.0f, true));
    g.drawText (valid ? juce::String (names[note % 12]) + juce::String (note / 12 - 1) : "--", r.removeFromLeft (54), juce::Justification::centred);
    auto scale = r.reduced (6.0f, 12.0f);
    g.setColour (col::line);
    g.fillRect (scale.withHeight (2.0f).withCentre (scale.getCentre()));
    g.fillRect (juce::Rectangle<float> (1.0f, scale.getHeight()).withCentre (scale.getCentre()));
    if (valid)
    {
        const float x = scale.getCentreX() + (float) juce::jlimit (-50.0, 50.0, c) / 50.0f * scale.getWidth() * 0.5f;
        g.setColour (inTune ? col::green : col::purple.brighter (0.4f));
        g.fillRoundedRectangle (juce::Rectangle<float> (6.0f, scale.getHeight()).withCentre ({ x, scale.getCentreY() }), 2.0f);
    }
    g.setColour (col::dim);
    g.setFont (font (10.0f));
    g.drawText ("TUNER", getLocalBounds().toFloat().removeFromBottom (12.0f), juce::Justification::centred);
}

// ============================================================================ ChainStrip
ChainStrip::ChainStrip (ToxicRigProcessor& p) : proc (p) {}

std::vector<ChainStrip::Item> ChainStrip::layoutItems() const
{
    const auto routing = proc.getRouting();
    std::vector<int> seq;
    seq.push_back (-1);
    for (int i = 0; i < routing.numBoard; ++i) seq.push_back (routing.board[(size_t) i]);
    seq.push_back (kAmp);
    seq.push_back (kCab);
    for (int i = 0; i < routing.numLoop; ++i) seq.push_back (routing.loop[(size_t) i]);
    seq.push_back (-2);
    const float gap = 10.0f;
    const float w = std::min (82.0f, ((float) getWidth() - gap * (float) (seq.size() + 1)) / (float) seq.size());
    const float total = w * (float) seq.size() + gap * (float) (seq.size() - 1);
    float x = ((float) getWidth() - total) * 0.5f;
    std::vector<Item> items;
    for (int id : seq)
    {
        items.push_back ({ id, juce::Rectangle<float> (x, 26.0f, w, (float) getHeight() - 36.0f) });
        x += w + gap;
    }
    return items;
}

int ChainStrip::hitTest (juce::Point<float> p) const
{
    for (auto& it : layoutItems()) if (it.r.contains (p)) return it.id;
    return -99;
}

void ChainStrip::paint (juce::Graphics& g)
{
    const auto items = layoutItems();
    const auto& specs = rig::pedalSpecs();
    auto& s = proc.apvts;
    // Zone captions over board and loop.
    auto zone = [&] (int firstId, int lastId, const juce::String& text, juce::Colour c)
    {
        juce::Rectangle<float> a, b;
        for (auto& it : items) { if (it.id == firstId) a = it.r; if (it.id == lastId) b = it.r; }
        if (a.isEmpty()) return;
        auto r = a.getUnion (b).expanded (5.0f, 0).withY (4.0f).withHeight ((float) getHeight() - 8.0f);
        g.setColour (c.withAlpha (0.08f));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (c.withAlpha (0.8f));
        g.setFont (font (11.0f, true));
        g.drawText (text, r.withHeight (18.0f), juce::Justification::centred);
    };
    const auto routing = proc.getRouting();
    if (routing.numBoard > 0) zone (routing.board[0], routing.board[(size_t) routing.numBoard - 1], "PEDALIERA  (prima dell'ampli)", col::green);
    if (routing.numLoop > 0) zone (routing.loop[0], routing.loop[(size_t) routing.numLoop - 1], "FX LOOP  (dopo ampli e cassa)", col::purple);

    // Cable between chips.
    g.setColour (col::line);
    for (size_t i = 1; i < items.size(); ++i)
        g.drawLine (items[i - 1].r.getRight(), items[i].r.getCentreY(), items[i].r.getX(), items[i].r.getCentreY(), 2.0f);

    for (auto& it : items)
    {
        if (it.id == dragging && dragMoved) continue;
        auto r = it.r;
        juce::String name;
        bool on = true, isPedal = it.id >= 0 && it.id < tz::kNumPedals;
        if (isPedal)
        {
            name = specs[(size_t) it.id].displayName;
            on = s.getRawParameterValue (rig::pedalOnId (it.id))->load() > 0.5f;
        }
        else if (it.id == kAmp) name = "AMP";
        else if (it.id == kCab) name = "CAB";
        else if (it.id == -1) name = "IN";
        else name = "OUT";
        const bool sel = it.id == selected;
        g.setColour (isPedal ? col::panel2 : col::panel);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (sel ? col::green : col::line);
        g.drawRoundedRectangle (r, 6.0f, sel ? 2.0f : 1.0f);
        if (isPedal)
        {
            const auto led = juce::Rectangle<float> (10.0f, 10.0f).withCentre ({ r.getCentreX(), r.getY() + 14.0f });
            if (on) { g.setColour (col::green.withAlpha (0.3f)); g.fillEllipse (led.expanded (4.0f)); }
            g.setColour (on ? col::green : juce::Colour (0xff33333d));
            g.fillEllipse (led);
        }
        g.setColour (on ? col::text : col::dim);
        g.setFont (font (11.5f, true));
        g.drawFittedText (name, r.reduced (4.0f).withTrimmedTop (isPedal ? 20.0f : 0.0f).toNearestInt(), juce::Justification::centred, 3);
    }
    if (dragging >= 0 && dragMoved)
    {
        auto r = juce::Rectangle<float> (70.0f, (float) getHeight() - 36.0f).withCentre (dragPos);
        g.setColour (col::purple.withAlpha (0.85f));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (col::text);
        g.setFont (font (11.5f, true));
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
        {
            // LED area toggles the pedal; anywhere else selects it.
            for (auto& it : layoutItems())
                if (it.id == id && e.position.y < it.r.getY() + 26.0f)
                {
                    auto* p = proc.apvts.getParameter (rig::pedalOnId (id));
                    p->beginChangeGesture();
                    p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f);
                    p->endChangeGesture();
                    repaint();
                    dragging = -1;
                    return;
                }
        }
        if (id >= 0 && id < tz::kNumPedals) { selected = id; if (onSelect) onSelect (id); }
        else if (id == kAmp || id == kCab) { selected = id; if (onSelect) onSelect (id); }
        dragging = -1;
        repaint();
        return;
    }
    // Drop: rebuild the order around the drop position. Left of AMP = board.
    const int moved = dragging;
    dragging = -1;
    std::vector<int> board, loop;
    bool afterAmp = false;
    bool placed = false;
    const bool loopOnly = rig::pedalSpecs()[(size_t) moved].pedal == tz::PedalId::Delay
                       || rig::pedalSpecs()[(size_t) moved].pedal == tz::PedalId::Reverb;
    for (auto& it : layoutItems())
    {
        if (it.id == moved) continue;
        if (! placed && dragPos.x < it.r.getCentreX() && it.id != -1)
        {
            (afterAmp ? loop : board).push_back (moved);
            placed = true;
        }
        if (it.id == kAmp) afterAmp = true;
        if (it.id >= 0 && it.id < tz::kNumPedals) (afterAmp ? loop : board).push_back (it.id);
    }
    if (! placed) loop.push_back (moved);
    if (loopOnly && std::find (board.begin(), board.end(), moved) != board.end())
    {
        board.erase (std::find (board.begin(), board.end(), moved));
        loop.insert (loop.begin(), moved); // delay/reverb only live after the amp
    }
    rig::Routing r;
    for (int p : board) r.board[(size_t) r.numBoard++] = p;
    for (int p : loop) r.loop[(size_t) r.numLoop++] = p;
    proc.setRouting (r);
    repaint();
}

// ============================================================================ PedalPanel
PedalPanel::PedalPanel (ToxicRigProcessor& p, int pedalIndex) : pedal (pedalIndex)
{
    auto& s = p.apvts;
    const auto& spec = rig::pedalSpecs()[(size_t) pedal];
    for (int i = 0; i < (int) spec.params.size(); ++i)
    {
        const auto& ps = spec.params[(size_t) i];
        const auto id = rig::pedalParamId (pedal, i);
        if (ps.isBool)
        {
            auto b = std::make_unique<juce::ToggleButton> (ps.name);
            btnAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, *b));
            controls.push_back (std::move (b));
        }
        else if (! ps.choices.isEmpty())
        {
            auto c = std::make_unique<juce::ComboBox>();
            c->addItemList (ps.choices, 1);
            c->setTooltip (ps.name);
            boxAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, id, *c));
            controls.push_back (std::move (c));
        }
        else
            controls.push_back (std::make_unique<Knob> (s, id, ps.name));
        addAndMakeVisible (*controls.back());
    }
    addAndMakeVisible (onButton);
    onAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, rig::pedalOnId (pedal), onButton);
}

void PedalPanel::paint (juce::Graphics& g)
{
    const auto& spec = rig::pedalSpecs()[(size_t) pedal];
    auto r = getLocalBounds().toFloat();
    g.setColour (col::panel);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (col::text);
    g.setFont (font (26.0f, true));
    g.drawText (spec.displayName, r.reduced (24.0f, 16.0f).removeFromTop (32.0f), juce::Justification::centredLeft);
    g.setColour (col::dim);
    g.setFont (font (14.0f));
    g.drawFittedText (spec.description, r.reduced (24.0f, 0).withTrimmedTop (56.0f).removeFromTop (40.0f).toNearestInt(),
                      juce::Justification::topLeft, 2);
}

void PedalPanel::resized()
{
    onButton.setBounds (getWidth() - 120, 20, 100, 28);
    const int n = (int) controls.size();
    const int cw = 110, total = n * cw;
    int x = (getWidth() - total) / 2;
    for (auto& c : controls)
    {
        if (dynamic_cast<Knob*> (c.get()) != nullptr) c->setBounds (x + 5, 130, cw - 10, 130);
        else c->setBounds (x + 5, 180, cw - 10, 28);
        x += cw;
    }
}

// ============================================================================ AmpPanel
AmpPanel::AmpPanel (ToxicRigProcessor& p) : proc (p)
{
    auto& s = p.apvts;
    for (int i = 0; i < 2; ++i)
    {
        const char slot = i == 0 ? 'a' : 'b';
        auto& sd = sides[(size_t) i];
        sd.model.addItemList (rig::ampModels, 1);
        sd.channel.addItemList (rig::ampChannels, 1);
        sd.modelAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, rig::ampId (slot, "model"), sd.model);
        sd.channelAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, rig::ampId (slot, "channel"), sd.channel);
        sd.brightAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, rig::ampId (slot, "bright"), sd.bright);
        addAndMakeVisible (sd.model); addAndMakeVisible (sd.channel); addAndMakeVisible (sd.bright);
        const std::pair<const char*, const char*> k[] = { { "gain", "Gain" }, { "bass", "Bass" }, { "mid", "Middle" }, { "treble", "Treble" },
                                                          { "presence", "Presence" }, { "resonance", "Resonance" }, { "master", "Master" },
                                                          { "sag", "Sag" }, { "level", "Level" } };
        for (auto& [id, cap] : k)
        {
            sd.knobs.push_back (std::make_unique<Knob> (s, rig::ampId (slot, id), cap));
            addAndMakeVisible (*sd.knobs.back());
        }
    }
    bOnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, rig::ids::ampBOn, bOn);
    addAndMakeVisible (bOn);
    mix = std::make_unique<Knob> (s, rig::ids::abMix, "Mix A/B");
    width = std::make_unique<Knob> (s, rig::ids::width, "Width");
    addAndMakeVisible (*mix); addAndMakeVisible (*width);
    startTimerHz (8);
}

void AmpPanel::timerCallback()
{
    for (int i = 0; i < 2; ++i)
    {
        // The British amp has no negative feedback: its tone controls are "Cut"
        // (treble cut in the power section) and "Depth" (bass), like the real one.
        const bool british = sides[(size_t) i].model.getSelectedItemIndex() == 0;
        sides[(size_t) i].knobs[4]->setCaption (british ? "Cut" : "Presence");
        sides[(size_t) i].knobs[5]->setCaption (british ? "Depth" : "Resonance");
    }
    const bool bEnabled = bOn.getToggleState();
    for (auto* c : std::initializer_list<juce::Component*> { &sides[1].model, &sides[1].channel, &sides[1].bright, mix.get(), width.get() })
        c->setAlpha (bEnabled ? 1.0f : 0.4f);
    for (auto& k : sides[1].knobs) k->setAlpha (bEnabled ? 1.0f : 0.4f);
}

void AmpPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::panel);
    g.fillRoundedRectangle (r, 8.0f);
    const float sideW = (r.getWidth() - 150.0f) * 0.5f;
    g.setFont (font (22.0f, true));
    g.setColour (col::green);
    g.drawText ("AMPLI A", juce::Rectangle<float> (24, 14, 200, 30), juce::Justification::centredLeft);
    g.setColour (col::purple.brighter (0.3f));
    g.drawText ("AMPLI B", juce::Rectangle<float> (sideW + 150 + 24, 14, 200, 30), juce::Justification::centredLeft);
    g.setColour (col::line);
    g.drawVerticalLine ((int) (sideW + 10), 20.0f, r.getBottom() - 20.0f);
    g.drawVerticalLine ((int) (sideW + 140), 20.0f, r.getBottom() - 20.0f);
}

void AmpPanel::resized()
{
    const int sideW = (getWidth() - 150) / 2;
    for (int i = 0; i < 2; ++i)
    {
        auto& sd = sides[(size_t) i];
        const int x0 = i == 0 ? 0 : sideW + 150;
        sd.model.setBounds (x0 + 150, 16, 150, 28);
        sd.channel.setBounds (x0 + 310, 16, 110, 28);
        sd.bright.setBounds (x0 + 430, 16, 100, 28);
        const int kw = (sideW - 40) / 5;
        for (int k = 0; k < (int) sd.knobs.size(); ++k)
        {
            const int row = k < 5 ? 0 : 1, colIdx = k < 5 ? k : k - 5;
            sd.knobs[(size_t) k]->setBounds (x0 + 20 + colIdx * kw, 70 + row * 200, kw, 170);
        }
    }
    bOn.setBounds (sideW + 18, 30, 118, 44);
    mix->setBounds (sideW + 25, 110, 100, 130);
    width->setBounds (sideW + 25, 270, 100, 130);
}

// ============================================================================ MicPad / CabPanel
MicPad::MicPad (juce::AudioProcessorValueTreeState& s, char slot)
    : state (s),
      pos (s.getParameter (rig::ampId (slot, "mic_pos"))),
      dist (s.getParameter (rig::ampId (slot, "mic_dist"))),
      posAtt (*pos, [this] (float) { repaint(); }, nullptr),
      distAtt (*dist, [this] (float) { repaint(); }, nullptr)
{
    posAtt.sendInitialUpdate();
    distAtt.sendInitialUpdate();
}

MicPad::~MicPad() = default;

void MicPad::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::panel2);
    g.fillRoundedRectangle (r, 6.0f);
    // Speaker seen from the front: X = position across the cone, Y = distance.
    auto sp = r.reduced (18.0f).withTrimmedBottom (18.0f);
    const float rad = std::min (sp.getWidth(), sp.getHeight()) * 0.48f;
    const auto c = juce::Point<float> (sp.getX() + rad + 6.0f, sp.getCentreY());
    g.setColour (juce::Colour (0xff101015));
    g.fillEllipse (juce::Rectangle<float> (rad * 2, rad * 2).withCentre (c));
    g.setColour (col::line);
    for (float k : { 1.0f, 0.75f, 0.5f }) g.drawEllipse (juce::Rectangle<float> (rad * 2 * k, rad * 2 * k).withCentre (c), 1.0f);
    g.setColour (col::purple.withAlpha (0.7f));
    g.fillEllipse (juce::Rectangle<float> (rad * 0.5f, rad * 0.5f).withCentre (c));
    const float px = c.x + pos->convertFrom0to1 (pos->getValue()) * rad;
    const float d = dist->convertFrom0to1 (dist->getValue());
    // Mic marker: smaller when far away.
    const float mr = 14.0f - 6.0f * d;
    g.setColour (col::green.withAlpha (0.25f));
    g.fillEllipse (juce::Rectangle<float> (mr * 3, mr * 3).withCentre ({ px, c.y }));
    g.setColour (col::green);
    g.fillEllipse (juce::Rectangle<float> (mr, mr).withCentre ({ px, c.y }));
    g.setColour (col::dim);
    g.setFont (font (12.0f));
    g.drawText ("centro", juce::Rectangle<float> (c.x - 30, sp.getBottom() + 2, 60, 16), juce::Justification::centred);
    g.drawText ("bordo", juce::Rectangle<float> (c.x + rad - 30, sp.getBottom() + 2, 60, 16), juce::Justification::centred);
    auto info = r.withTrimmedLeft (c.x + rad + 20.0f - r.getX()).reduced (8.0f);
    g.setColour (col::text);
    g.setFont (font (13.0f));
    g.drawFittedText ("Trascina in orizzontale: posizione sul cono.\nTrascina in verticale: distanza.\n\nDistanza: " + juce::String ((int) (d * 100)) + "%",
                      info.toNearestInt(), juce::Justification::centredLeft, 6);
}

void MicPad::mouseDrag (const juce::MouseEvent& e)
{
    auto sp = getLocalBounds().toFloat().reduced (18.0f).withTrimmedBottom (18.0f);
    const float rad = std::min (sp.getWidth(), sp.getHeight()) * 0.48f;
    const float cx = sp.getX() + rad + 6.0f;
    const float newPos = juce::jlimit (0.0f, 1.0f, (e.position.x - cx) / rad);
    const float newDist = juce::jlimit (0.0f, 1.0f, (e.position.y - sp.getY()) / sp.getHeight());
    posAtt.setValueAsCompleteGesture (newPos);
    distAtt.setValueAsCompleteGesture (newDist);
}

CabPanel::CabPanel (ToxicRigProcessor& p) : proc (p)
{
    auto& s = p.apvts;
    for (int i = 0; i < 2; ++i)
    {
        const char slot = i == 0 ? 'a' : 'b';
        auto& sd = sides[(size_t) i];
        sd.cab.addItemList (rig::cabNames, 1);
        sd.mic.addItemList (rig::micNames, 1);
        sd.onAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, rig::ampId (slot, "cab_on"), sd.on);
        sd.cabAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, rig::ampId (slot, "cab"), sd.cab);
        sd.micAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, rig::ampId (slot, "mic"), sd.mic);
        sd.pad = std::make_unique<MicPad> (s, slot);
        sd.file.setColour (juce::Label::textColourId, col::dim);
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
        for (juce::Component* c : std::initializer_list<juce::Component*> { &sd.on, &sd.cab, &sd.mic, &sd.load, &sd.file, sd.pad.get() })
            addAndMakeVisible (c);
    }
}

void CabPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::panel);
    g.fillRoundedRectangle (r, 8.0f);
    g.setFont (font (22.0f, true));
    g.setColour (col::green);
    g.drawText ("CASSA A", juce::Rectangle<float> (24, 14, 200, 30), juce::Justification::centredLeft);
    g.setColour (col::purple.brighter (0.3f));
    g.drawText ("CASSA B", juce::Rectangle<float> (r.getWidth() * 0.5f + 24, 14, 200, 30), juce::Justification::centredLeft);
    g.setColour (col::line);
    g.drawVerticalLine (getWidth() / 2, 20.0f, r.getBottom() - 20.0f);
}

void CabPanel::resized()
{
    const int half = getWidth() / 2;
    for (int i = 0; i < 2; ++i)
    {
        auto& sd = sides[(size_t) i];
        const int x0 = i * half;
        sd.on.setBounds (x0 + 150, 16, 140, 28);
        sd.cab.setBounds (x0 + 24, 60, 180, 28);
        sd.load.setBounds (x0 + 214, 60, 110, 28);
        sd.file.setBounds (x0 + 330, 60, half - 350, 28);
        sd.mic.setBounds (x0 + 24, 98, 180, 28);
        sd.pad->setBounds (x0 + 24, 140, half - 48, getHeight() - 160);
    }
}

// ============================================================================ Root
Root::Root (ToxicRigProcessor& p) : proc (p), chain (p), ampPanel (p), cabPanel (p)
{
    setLookAndFeel (&lnf);
    auto& s = p.apvts;
    addAndMakeVisible (presetBox);
    refreshPresetBox();
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= 1 && id < 1000) proc.applyFactoryPreset (id - 1);
        else if (id >= 1000 && id < 9000) proc.loadUserPreset (userPresets[id - 1000]);
        else if (id == 9000)
        {
            chooser = std::make_unique<juce::FileChooser> ("Salva preset", ToxicRigProcessor::userPresetFolder().getChildFile ("Mio suono.trpreset"), "*.trpreset");
            chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [this] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f == juce::File()) return;
                proc.saveUserPreset (f.withFileExtension ("trpreset"));
                refreshPresetBox();
            });
        }
        else if (id == 9001) ToxicRigProcessor::userPresetFolder().startAsProcess();
        chain.repaint();
    };
    prevBtn.onClick = [this] { const int n = presetBox.getNumItems(); if (n > 0) presetBox.setSelectedItemIndex ((presetBox.getSelectedItemIndex() - 1 + n) % n); };
    nextBtn.onClick = [this] { const int n = presetBox.getNumItems(); if (n > 0) presetBox.setSelectedItemIndex ((presetBox.getSelectedItemIndex() + 1) % n); };
    saveBtn.onClick = [this] { presetBox.setSelectedId (9000); };
    aBtn.onClick = [this] { proc.recallAB (0); chain.repaint(); };
    bBtn.onClick = [this] { proc.recallAB (1); chain.repaint(); };
    aBtn.setTooltip ("Confronto A/B: due versioni del suono da alternare");
    for (auto* b : { &prevBtn, &nextBtn, &saveBtn, &aBtn, &bBtn }) addAndMakeVisible (b);
    osBox.addItemList (rig::osNames, 1);
    osAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, rig::ids::oversampling, osBox);
    addAndMakeVisible (osBox);
    inKnob = std::make_unique<Knob> (s, rig::ids::inGain, "Input");
    outKnob = std::make_unique<Knob> (s, rig::ids::outVol, "Output");
    addAndMakeVisible (*inKnob); addAndMakeVisible (*outKnob);
    addAndMakeVisible (inMeter); addAndMakeVisible (outMeter); addAndMakeVisible (tuner);
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
    startTimerHz (30);
}

Root::~Root() { setLookAndFeel (nullptr); }

void Root::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    const auto& f = rig::factoryPresets();
    for (int i = 0; i < (int) f.size(); ++i) presetBox.addItem (f[(size_t) i].name, i + 1);
    userPresets = ToxicRigProcessor::userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.trpreset");
    if (! userPresets.isEmpty()) presetBox.addSeparator();
    for (int i = 0; i < userPresets.size(); ++i) presetBox.addItem (userPresets[i].getFileNameWithoutExtension(), 1000 + i);
    presetBox.addSeparator();
    presetBox.addItem ("Salva come preset...", 9000);
    presetBox.addItem ("Apri cartella preset", 9001);
    presetBox.setText ("Preset", juce::dontSendNotification);
}

void Root::select (int id)
{
    chain.selected = id;
    for (int i = 0; i < (int) pedalPanels.size(); ++i) pedalPanels[(size_t) i]->setVisible (i == id);
    ampPanel.setVisible (id == ChainStrip::kAmp);
    cabPanel.setVisible (id == ChainStrip::kCab);
    chain.repaint();
}

void Root::paint (juce::Graphics& g)
{
    g.fillAll (col::bg);
    g.setColour (col::panel);
    g.fillRect (0, 0, W, 78);
    g.setColour (col::green);
    g.setFont (font (28.0f, true));
    g.drawText ("TOXICRIG", 18, 10, 200, 34, juce::Justification::centredLeft);
    g.setColour (col::purple.brighter (0.4f));
    g.setFont (font (11.0f));
    g.drawText ("by ToxicZone  -  v" JucePlugin_VersionString, 20, 44, 200, 16, juce::Justification::centredLeft);
    g.setColour (col::dim);
    g.drawText ("Oversampling", 668, 8, 90, 16, juce::Justification::centredLeft);
}

void Root::resized()
{
    presetBox.setBounds (200, 22, 230, 30);
    prevBtn.setBounds (436, 22, 30, 30);
    nextBtn.setBounds (470, 22, 30, 30);
    saveBtn.setBounds (506, 22, 60, 30);
    aBtn.setBounds (586, 22, 32, 30);
    bBtn.setBounds (622, 22, 32, 30);
    osBox.setBounds (668, 26, 80, 28);
    inKnob->setBounds (770, 2, 74, 76);
    inMeter.setBounds (846, 12, 14, 56);
    tuner.setBounds (876, 10, 230, 58);
    outKnob->setBounds (1124, 2, 74, 76);
    outMeter.setBounds (1200, 12, 14, 56);
    chain.setBounds (10, 86, W - 20, 132);
    const juce::Rectangle<int> detail (16, 228, W - 32, H - 244);
    for (auto& pp : pedalPanels) pp->setBounds (detail);
    ampPanel.setBounds (detail);
    cabPanel.setBounds (detail);
}

void Root::timerCallback()
{
    auto& e = proc.engine;
    inLevel = std::max (inLevel * 0.85f, e.inPeak.exchange (0.0f));
    outL = std::max (outL * 0.85f, e.outPeakL.exchange (0.0f));
    outR = std::max (outR * 0.85f, e.outPeakR.exchange (0.0f));
    inMeter.set (inLevel, inLevel);
    outMeter.set (outL, outR);
    aBtn.setToggleState (proc.currentAB == 0, juce::dontSendNotification);
    bBtn.setToggleState (proc.currentAB == 1, juce::dontSendNotification);

    // Tuner: drain the FIFO into a rolling window, analyse a few times a second.
    const double rate = e.tunerRate();
    if (tunerWindow.empty() || std::abs (rate - lastTunerRate) > 1.0)
    {
        detector.prepare (rate);
        tunerWindow.assign ((size_t) detector.windowSize(), 0.0f);
        lastTunerRate = rate;
    }
    {
        const int ready = e.tunerFifo.getNumReady();
        int s1, n1, s2, n2;
        e.tunerFifo.prepareToRead (ready, s1, n1, s2, n2);
        std::vector<float> fresh;
        fresh.reserve ((size_t) (n1 + n2));
        fresh.insert (fresh.end(), e.tunerBuffer.begin() + s1, e.tunerBuffer.begin() + s1 + n1);
        fresh.insert (fresh.end(), e.tunerBuffer.begin() + s2, e.tunerBuffer.begin() + s2 + n2);
        e.tunerFifo.finishedRead (n1 + n2);
        const size_t W = tunerWindow.size();
        if (fresh.size() >= W) std::copy (fresh.end() - (long) W, fresh.end(), tunerWindow.begin());
        else if (! fresh.empty())
        {
            std::move (tunerWindow.begin() + (long) fresh.size(), tunerWindow.end(), tunerWindow.begin());
            std::copy (fresh.begin(), fresh.end(), tunerWindow.end() - (long) fresh.size());
        }
    }
    if (++tick % 3 == 0)
    {
        const double hz = detector.detect (tunerWindow.data(), (int) tunerWindow.size());
        const auto ni = tz::hzToNote (hz);
        tuner.set (ni.midi, ni.cents);
    }
    if (tick % 6 == 0) chain.repaint(); // LEDs follow automation
}
} // namespace ui

// ============================================================================ Editor
ToxicRigEditor::ToxicRigEditor (ToxicRigProcessor& p) : AudioProcessorEditor (p), root (p)
{
    addAndMakeVisible (root);
    setResizable (true, true);
    setResizeLimits ((int) (ui::Root::W * 0.55), (int) (ui::Root::H * 0.55), ui::Root::W * 2, ui::Root::H * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) ui::Root::W / ui::Root::H);
    setSize ((int) (ui::Root::W * 0.85), (int) (ui::Root::H * 0.85));
}

void ToxicRigEditor::resized()
{
    const float scale = (float) getWidth() / (float) ui::Root::W;
    root.setBounds (0, 0, ui::Root::W, ui::Root::H);
    root.setTransform (juce::AffineTransform::scale (scale));
}
