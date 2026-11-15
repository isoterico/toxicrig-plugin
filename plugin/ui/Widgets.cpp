#include "Widgets.h"

namespace ui
{
SkinId skinOf (const juce::Component& c)
{
    if (auto* l = dynamic_cast<SkinLookAndFeel*> (&c.getLookAndFeel())) return l->skin;
    return SkinId::ToxicLab;
}

// ============================================================================ LookAndFeel
SkinLookAndFeel::SkinLookAndFeel (SkinId s) : skin (s)
{
    const auto& p = pal();
    setColour (juce::ResizableWindow::backgroundColourId, p.bg);
    setColour (juce::Label::textColourId, p.text);
    setColour (juce::Slider::textBoxTextColourId, p.text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId, p.panel2);
    setColour (juce::ComboBox::outlineColourId, p.line);
    setColour (juce::ComboBox::textColourId, p.text);
    setColour (juce::ComboBox::arrowColourId, p.accent);
    setColour (juce::PopupMenu::backgroundColourId, p.panel2.withAlpha (1.0f));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, p.accent2.withAlpha (0.55f));
    setColour (juce::PopupMenu::textColourId, p.text);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextButton::buttonColourId, p.panel2);
    setColour (juce::TextButton::textColourOffId, p.text);
    setColour (juce::TextButton::textColourOnId, p.bg);
    setColour (juce::ToggleButton::textColourId, p.text);
    setColour (juce::TooltipWindow::backgroundColourId, p.panel2);
    setColour (juce::TooltipWindow::textColourId, p.text);
    setColour (juce::BubbleComponent::backgroundColourId, p.panel2);
    setColour (juce::BubbleComponent::outlineColourId, p.accent);
    setColour (juce::TextEditor::backgroundColourId, p.panel2);
    setColour (juce::TextEditor::textColourId, p.text);
    setColour (juce::AlertWindow::backgroundColourId, p.panel2);
    setColour (juce::AlertWindow::textColourId, p.text);
}

static void shadowEllipse (juce::Graphics& g, juce::Point<float> c, float r, float alpha)
{
    for (int i = 4; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (alpha / (float) (i + 1)));
        const float rr = r + (float) i * 1.6f;
        g.fillEllipse (c.x - rr + 2.0f, c.y - rr + 3.5f, rr * 2, rr * 2);
    }
}

void SkinLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
{
    const auto& p = pal();
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float r = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f - 2.0f;
    const auto c = bounds.getCentre();
    const float angle = start + pos * (end - start);
    const auto style = dynamic_cast<ParamKnob*> (&s) != nullptr ? static_cast<ParamKnob&> (s).style : KnobStyle::Panel;
    auto pointer = [&] (float from, float to, float thick, juce::Colour col)
    {
        juce::Path ptr;
        ptr.addRoundedRectangle (-thick * 0.5f, -to, thick, to - from, thick * 0.5f);
        g.setColour (col);
        g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    };

    if (style == KnobStyle::AmpRound || style == KnobStyle::Pedal || style == KnobStyle::PedalBig)
    {
        shadowEllipse (g, c, r * 0.86f, 0.5f);
        // Skirt with a fine metallic edge, then the cap.
        const float sr = r * 0.86f;
        juce::ColourGradient skirt (juce::Colour (0xff3a3a44), c.x - sr, c.y - sr, juce::Colour (0xff0c0c10), c.x + sr, c.y + sr, false);
        g.setGradientFill (skirt);
        g.fillEllipse (c.x - sr, c.y - sr, sr * 2, sr * 2);
        g.setColour (juce::Colours::black);
        g.drawEllipse (c.x - sr, c.y - sr, sr * 2, sr * 2, 1.2f);
        const float cr = sr * 0.78f;
        juce::ColourGradient cap (juce::Colour (0xff2c2c35), c.x, c.y - cr, juce::Colour (0xff121217), c.x, c.y + cr, false);
        g.setGradientFill (cap);
        g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2, 1.0f);
        pointer (cr * 0.15f, sr * 0.92f, std::max (2.0f, r * 0.11f), juce::Colour (0xffe8e2d4));
        if (style != KnobStyle::Pedal && s.isMouseOverOrDragging())
        {
            g.setColour (p.accent.withAlpha (0.5f));
            g.drawEllipse (c.x - sr - 2, c.y - sr - 2, sr * 2 + 4, sr * 2 + 4, 1.5f);
        }
        return;
    }
    if (style == KnobStyle::ChickenHead)
    {
        shadowEllipse (g, c, r * 0.62f, 0.55f);
        const float br = r * 0.62f;
        juce::ColourGradient base (juce::Colour (0xff34302c), c.x - br, c.y - br, juce::Colour (0xff0c0b0a), c.x + br, c.y + br, false);
        g.setGradientFill (base);
        g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);
        // The "chicken head": a tapered pointer body.
        juce::Path head;
        head.startNewSubPath (-r * 0.30f, r * 0.28f);
        head.quadraticTo (-r * 0.36f, -r * 0.2f, -r * 0.12f, -r * 0.98f);
        head.lineTo (r * 0.12f, -r * 0.98f);
        head.quadraticTo (r * 0.36f, -r * 0.2f, r * 0.30f, r * 0.28f);
        head.quadraticTo (0.0f, r * 0.5f, -r * 0.30f, r * 0.28f);
        head.closeSubPath();
        const auto t = juce::AffineTransform::rotation (angle).translated (c.x, c.y);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillPath (head, juce::AffineTransform::rotation (angle).translated (c.x + 2.0f, c.y + 3.0f));
        juce::ColourGradient body (juce::Colour (0xff2f2b28), c.x - r, c.y - r, juce::Colour (0xff080807), c.x + r, c.y + r, false);
        g.setGradientFill (body);
        g.fillPath (head, t);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.strokePath (head, juce::PathStrokeType (1.0f), t);
        pointer (r * 0.1f, r * 0.9f, std::max (1.8f, r * 0.07f), juce::Colour (0xfff0e6cc));
        if (s.isMouseOverOrDragging())
        {
            g.setColour (p.accent.withAlpha (0.45f));
            g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 1.5f);
        }
        return;
    }

    // Panel knobs: one look per skin.
    switch (skin)
    {
        case SkinId::ToxicLab:
        {
            const float arcR = r - 2.0f;
            juce::Path track, val;
            track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, start, end, true);
            val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, start, angle, true);
            g.setColour (p.knobTrack);
            g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (s.isEnabled() ? p.accent : p.dim);
            g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            const float kr = r * 0.72f;
            juce::ColourGradient grad (juce::Colour (0xff3a3a48), c.x - kr * 0.5f, c.y - kr, juce::Colour (0xff101016), c.x + kr * 0.4f, c.y + kr, false);
            g.setGradientFill (grad);
            g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
            pointer (kr * 0.2f, kr - 3.0f, 3.0f, p.text);
            break;
        }
        case SkinId::StudioNoir:
        {
            g.setColour (p.dim);
            for (int i = 0; i <= 10; ++i)
            {
                const float a = start + (end - start) * (float) i / 10.0f;
                const float rr = r - 1.5f;
                g.fillEllipse (c.x + std::sin (a) * rr - 1.3f, c.y - std::cos (a) * rr - 1.3f, 2.6f, 2.6f);
            }
            const float kr = r * 0.76f;
            shadowEllipse (g, c, kr, 0.5f);
            juce::ColourGradient grad (juce::Colour (0xff3b3631), c.x - kr * 0.6f, c.y - kr, juce::Colour (0xff0e0d0c), c.x + kr * 0.5f, c.y + kr, false);
            g.setGradientFill (grad);
            g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.drawEllipse (c.x - kr * 0.8f, c.y - kr * 0.8f, kr * 1.6f, kr * 1.6f, 1.0f);
            pointer (kr * 0.25f, kr - 2.0f, 3.2f, p.text);
            break;
        }
        case SkinId::Schematic:
        {
            const float arcR = r - 2.0f;
            juce::Path track, val;
            track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, start, end, true);
            val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, start, angle, true);
            g.setColour (p.knobTrack);
            g.strokePath (track, juce::PathStrokeType (1.5f));
            g.setColour (p.accent);
            g.strokePath (val, juce::PathStrokeType (2.0f));
            g.setColour (p.accent2);
            g.fillEllipse (c.x + std::sin (angle) * arcR - 4.0f, c.y - std::cos (angle) * arcR - 4.0f, 8.0f, 8.0f);
            if (s.getTextBoxPosition() == juce::Slider::NoTextBox)
            {
                g.setColour (p.text);
                g.setFont (skinFont (skin, FontRole::Mono, std::max (9.0f, r * 0.30f)));
                g.drawFittedText (s.getTextFromValue (s.getValue()), bounds.withSizeKeepingCentre (r * 1.5f, r).toNearestInt(), juce::Justification::centred, 1);
            }
            break;
        }
    }
}

void SkinLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle st, juce::Slider& s)
{
    const auto& p = pal();
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    if (st == juce::Slider::LinearVertical)
    {
        const auto groove = juce::Rectangle<float> (3.0f, r.getHeight()).withCentre (r.getCentre());
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.fillRoundedRectangle (groove, 1.5f);
        g.setColour (p.line);
        g.drawHorizontalLine ((int) r.getCentreY(), r.getX() + 1.0f, r.getRight() - 1.0f);
        const float capH = std::max (6.0f, r.getWidth() * 0.55f);
        auto cap = juce::Rectangle<float> (r.getWidth() * 0.95f, capH).withCentre ({ r.getCentreX(), pos });
        g.setColour (juce::Colour (0xff26262e));
        g.fillRoundedRectangle (cap, 2.0f);
        g.setColour (p.accent);
        g.fillRect (cap.withSizeKeepingCentre (cap.getWidth() - 3.0f, 1.6f));
        (void) s;
        return;
    }
    LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, 0, 0, st, s);
}

void SkinLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    const auto& p = pal();
    const auto r = b.getLocalBounds().toFloat();
    const float d = std::min (14.0f, r.getHeight() - 4.0f);
    const juce::Rectangle<float> led (r.getX() + 2, r.getCentreY() - d * 0.5f, d, d);
    const bool on = b.getToggleState();
    if (on) { g.setColour (p.accent.withAlpha (0.25f)); g.fillEllipse (led.expanded (3.0f)); }
    g.setColour (on ? p.accent : p.panel2.brighter (0.1f));
    g.fillEllipse (led);
    g.setColour (highlighted ? p.text : p.line);
    g.drawEllipse (led, 1.0f);
    g.setColour (p.text);
    g.setFont (skinFont (skin, FontRole::Body, 15.0f));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (d + 8), juce::Justification::centredLeft);
}

void SkinLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    const auto& p = pal();
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    const float rad = skin == SkinId::Schematic ? 2.0f : 6.0f;
    g.setColour (on ? p.accent : (down ? p.accent2 : (highlighted ? p.panel2.brighter (0.15f) : p.panel2)));
    g.fillRoundedRectangle (r, rad);
    g.setColour (on ? p.accent : p.line);
    g.drawRoundedRectangle (r, rad, 1.0f);
}

void SkinLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const auto& p = pal();
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (b.getToggleState() ? p.bg : p.text);
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (4, 2), juce::Justification::centred, 1);
}

void SkinLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto& p = pal();
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (p.panel2);
    g.fillRoundedRectangle (r, skin == SkinId::Schematic ? 2.0f : 6.0f);
    g.setColour (box.hasKeyboardFocus (false) ? p.accent : p.line);
    g.drawRoundedRectangle (r, skin == SkinId::Schematic ? 2.0f : 6.0f, 1.0f);
    juce::Path arrow;
    const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
    arrow.startNewSubPath (ax - 5, ay - 2.5f); arrow.lineTo (ax, ay + 2.5f); arrow.lineTo (ax + 5, ay - 2.5f);
    g.setColour (p.accent);
    g.strokePath (arrow, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void SkinLookAndFeel::drawBubble (juce::Graphics& g, juce::BubbleComponent&, const juce::Point<float>&, const juce::Rectangle<float>& body)
{
    const auto& p = pal();
    g.setColour (p.panel2.withAlpha (0.96f));
    g.fillRoundedRectangle (body, 5.0f);
    g.setColour (p.accent);
    g.drawRoundedRectangle (body, 5.0f, 1.2f);
}

void SkinLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    const auto& p = pal();
    g.setColour (p.panel2);
    g.fillRoundedRectangle (0, 0, (float) width, (float) height, 5.0f);
    g.setColour (p.line);
    g.drawRoundedRectangle (0.5f, 0.5f, (float) width - 1, (float) height - 1, 5.0f, 1.0f);
    g.setColour (p.text);
    g.setFont (skinFont (skin, FontRole::Body, 15.0f));
    g.drawFittedText (text, 8, 4, width - 16, height - 8, juce::Justification::centredLeft, 4);
}

// ============================================================================ ParamKnob
ParamKnob::ParamKnob (juce::AudioProcessorValueTreeState& s, const juce::String& id, KnobStyle st, bool textBox)
    : style (st), paramId (id)
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    setTextBoxStyle (textBox ? juce::Slider::TextBoxBelow : juce::Slider::NoTextBox, false, 80, 16);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setVelocityBasedMode (false);
    setMouseDragSensitivity (220); // ~2 cm of mouse travel for the full range: precise but quick
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, *this);
    if (auto* prm = s.getParameter (id))
    {
        setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
        setTooltip (prm->getName (64));
    }
    setPopupDisplayEnabled (true, true, nullptr, 900);
    setScrollWheelEnabled (true);
}

CaptionKnob::CaptionKnob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& cap, KnobStyle st)
    : knob (s, id, st, true), caption (cap)
{
    addAndMakeVisible (knob);
}

void CaptionKnob::resized() { knob.setBounds (getLocalBounds().withTrimmedTop (18)); }

void CaptionKnob::paint (juce::Graphics& g)
{
    const auto sk = skinOf (*this);
    g.setColour (palette (sk).dim);
    g.setFont (skinFont (sk, FontRole::Label, 14.0f));
    g.drawText (sk == SkinId::StudioNoir ? caption.toUpperCase() : caption, getLocalBounds().removeFromTop (18), juce::Justification::centred);
}

// ============================================================================ ParamToggle
ParamToggle::ParamToggle (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text, Look l)
    : juce::ToggleButton (text), look (l)
{
    att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, *this);
    if (auto* prm = s.getParameter (id)) setTooltip (prm->getName (64));
}

void ParamToggle::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto sk = skinOf (*this);
    const auto& p = palette (sk);
    const bool on = getToggleState();
    auto r = getLocalBounds().toFloat();
    if (look == Look::Led) { getLookAndFeel().drawToggleButton (g, *this, highlighted, down); return; }
    if (look == Look::Pill)
    {
        g.setColour (on ? p.accent : p.panel2);
        g.fillRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f);
        g.setColour (on ? p.accent : p.line);
        g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.0f);
        g.setColour (on ? p.bg : p.text);
        g.setFont (skinFont (sk, FontRole::Label, std::min (15.0f, r.getHeight() * 0.6f)));
        g.drawText (getButtonText(), r, juce::Justification::centred);
        return;
    }
    // Bat toggle: chrome bushing + lever up (on) / down (off), caption under it.
    auto cap = r.removeFromBottom (std::min (14.0f, r.getHeight() * 0.3f));
    const auto c = r.getCentre();
    const float s = std::min (r.getWidth(), r.getHeight()) * 0.5f;
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (c.x - s * 0.55f + 1.5f, c.y - s * 0.55f + 2.5f, s * 1.1f, s * 1.1f);
    juce::ColourGradient chrome (juce::Colour (0xffe6e6ea), c.x - s * 0.5f, c.y - s * 0.5f, juce::Colour (0xff55555c), c.x + s * 0.5f, c.y + s * 0.5f, false);
    g.setGradientFill (chrome);
    g.fillEllipse (c.x - s * 0.5f, c.y - s * 0.5f, s, s);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (c.x - s * 0.5f, c.y - s * 0.5f, s, s, 1.0f);
    const float len = s * 0.95f * (on ? -1.0f : 1.0f);
    juce::Path lever;
    lever.addRoundedRectangle (c.x - s * 0.13f, std::min (c.y, c.y + len), s * 0.26f, std::abs (len), s * 0.13f);
    juce::ColourGradient lg (juce::Colour (0xfff5f5f7), c.x - s * 0.13f, c.y, juce::Colour (0xff8a8a92), c.x + s * 0.13f, c.y, false);
    g.setGradientFill (lg);
    g.fillPath (lever);
    g.setColour (on ? p.accent : p.dim);
    g.setFont (skinFont (sk, FontRole::Label, cap.getHeight()));
    g.drawText (getButtonText(), cap, juce::Justification::centred);
    if (highlighted) { g.setColour (p.accent.withAlpha (0.4f)); g.drawEllipse (c.x - s * 0.6f, c.y - s * 0.6f, s * 1.2f, s * 1.2f, 1.0f); }
}

// ============================================================================ ChoiceButton
ChoiceButton::ChoiceButton (juce::AudioProcessorValueTreeState& s, const juce::String& id)
    : param (s.getParameter (id))
{
    att = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v) { index = (int) std::lround (v); repaint(); }, nullptr);
    att->sendInitialUpdate();
    setTooltip (param->getName (64));
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

ChoiceButton::~ChoiceButton() = default;

void ChoiceButton::paint (juce::Graphics& g)
{
    const auto sk = skinOf (*this);
    const auto& p = palette (sk);
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (isMouseOver() ? p.accent : p.line);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    auto choices = param->getAllValueStrings();
    const juce::String txt = labelFor ? labelFor (index) : choices[index];
    g.setColour (p.accent);
    g.setFont (skinFont (sk, FontRole::Label, std::min (15.0f, r.getHeight() * 0.68f)));
    g.drawFittedText (txt, r.reduced (3, 0).toNearestInt(), juce::Justification::centred, 1);
}

void ChoiceButton::mouseDown (const juce::MouseEvent&)
{
    juce::PopupMenu m;
    auto choices = param->getAllValueStrings();
    for (int i = 0; i < choices.size(); ++i) m.addItem (i + 1, labelFor ? labelFor (i) : choices[i], true, i == index);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe = juce::Component::SafePointer<ChoiceButton> (this)] (int r)
    {
        if (safe != nullptr && r > 0) safe->att->setValueAsCompleteGesture ((float) (r - 1));
    });
}

// ============================================================================ Meters
void LevelMeter::paint (juce::Graphics& g)
{
    const auto sk = skinOf (*this);
    const auto& p = palette (sk);
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r, 2.0f);
    auto bar = [&] (juce::Rectangle<float> b, float v)
    {
        const float db = juce::Decibels::gainToDecibels (v, -60.0f);
        const float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        auto fill = b.withTop (b.getBottom() - b.getHeight() * frac);
        g.setColour (db > -1.0f ? juce::Colour (0xffff3b30) : (db > -12.0f ? juce::Colour (0xffffd400) : p.accent));
        g.fillRect (fill);
    };
    auto left = r.reduced (2.0f);
    auto right = left.removeFromRight (left.getWidth() * 0.5f);
    bar (left.withTrimmedRight (1.0f), lv);
    bar (right.withTrimmedLeft (1.0f), rv);
}

void VuMeter::set (float level)
{
    // VU ballistics: ~300 ms integration (called at 30 Hz).
    const float db = juce::Decibels::gainToDecibels (level, -40.0f);
    const float target = juce::jlimit (0.0f, 1.0f, (db + 26.0f) / 29.0f); // -26..+3 VU around -18 dBFS = 0 VU
    needle += (target - needle) * 0.28f;
    repaint();
}

void VuMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (0xff2b2722));
    g.fillRoundedRectangle (r, 6.0f);
    auto face = r.reduced (4.0f);
    juce::ColourGradient warm (juce::Colour (0xfff6e9c7), face.getCentreX(), face.getY(), juce::Colour (0xffe2cc98), face.getCentreX(), face.getBottom(), false);
    g.setGradientFill (warm);
    g.fillRoundedRectangle (face, 4.0f);
    const juce::Point<float> pivot (face.getCentreX(), face.getBottom() + face.getHeight() * 0.25f);
    const float R = face.getHeight() * 0.95f;
    const float a0 = -0.85f, a1 = 0.85f;
    juce::Path scale;
    scale.addCentredArc (pivot.x, pivot.y, R * 0.82f, R * 0.82f, 0.0f, a0, a1, true);
    g.setColour (juce::Colour (0xff3a3128));
    g.strokePath (scale, juce::PathStrokeType (1.2f));
    juce::Path red;
    red.addCentredArc (pivot.x, pivot.y, R * 0.82f, R * 0.82f, 0.0f, a0 + (a1 - a0) * (26.0f / 29.0f), a1, true);
    g.setColour (juce::Colour (0xffb5321e));
    g.strokePath (red, juce::PathStrokeType (3.5f));
    g.setFont (skinFont (SkinId::StudioNoir, FontRole::Body, face.getHeight() * 0.16f));
    for (int db : { -20, -10, -7, -5, -3, 0, 3 })
    {
        const float a = a0 + (a1 - a0) * ((float) db + 26.0f) / 29.0f;
        const auto p1 = pivot + juce::Point<float> (std::sin (a), -std::cos (a)) * (R * 0.82f);
        const auto p2 = pivot + juce::Point<float> (std::sin (a), -std::cos (a)) * (R * 0.90f);
        g.setColour (db >= 0 ? juce::Colour (0xffb5321e) : juce::Colour (0xff3a3128));
        g.drawLine ({ p1, p2 }, 1.2f);
        const auto tp = pivot + juce::Point<float> (std::sin (a), -std::cos (a)) * (R * 0.99f);
        g.drawText (juce::String (std::abs (db)), juce::Rectangle<float> (24, 12).withCentre (tp), juce::Justification::centred);
    }
    g.setColour (juce::Colour (0xff3a3128));
    g.drawText ("VU  " + label, face.withTrimmedTop (face.getHeight() * 0.55f).withTrimmedBottom (2.0f), juce::Justification::centredBottom);
    g.saveState();
    g.reduceClipRegion (face.toNearestInt());
    const float a = a0 + (a1 - a0) * needle;
    g.setColour (juce::Colour (0xff1d1a17));
    g.drawLine ({ pivot, pivot + juce::Point<float> (std::sin (a), -std::cos (a)) * (R * 0.92f) }, 1.6f);
    g.restoreState();
}

void TunerDisplay::paint (juce::Graphics& g)
{
    const auto sk = skinOf (*this);
    const auto& p = palette (sk);
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (r, 6.0f);
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const bool valid = note >= 0;
    const bool inTune = valid && std::abs (c) < 3.0;
    g.setColour (valid ? (inTune ? p.accent : p.text) : p.dim);
    g.setFont (skinFont (sk, FontRole::Title, 22.0f));
    g.drawText (valid ? juce::String (names[note % 12]) : "-", r.removeFromLeft (46), juce::Justification::centred);
    auto scale = r.reduced (6.0f, 14.0f);
    g.setColour (p.line);
    g.fillRect (scale.withHeight (2.0f).withCentre (scale.getCentre()));
    for (int k = -4; k <= 4; ++k)
    {
        const float x = scale.getCentreX() + (float) k / 4.0f * scale.getWidth() * 0.5f;
        g.fillRect (juce::Rectangle<float> (1.0f, k == 0 ? scale.getHeight() : scale.getHeight() * 0.45f).withCentre ({ x, scale.getCentreY() }));
    }
    if (valid)
    {
        const float x = scale.getCentreX() + (float) juce::jlimit (-50.0, 50.0, c) / 50.0f * scale.getWidth() * 0.5f;
        g.setColour (inTune ? p.accent : p.accent2);
        g.fillRoundedRectangle (juce::Rectangle<float> (6.0f, scale.getHeight() + 6.0f).withCentre ({ x, scale.getCentreY() }), 2.0f);
    }
    g.setColour (p.dim);
    g.setFont (skinFont (sk, FontRole::Mono, 10.0f));
    g.drawText (valid ? (c >= 0 ? "+" : "") + juce::String ((int) std::lround (c)) + " cent" : "TUNER", getLocalBounds().toFloat().removeFromBottom (13.0f).withTrimmedLeft (46.0f), juce::Justification::centred);
}

void ScopeDisplay::set (const float* data, int n)
{
    pts.assign (data, data + n);
    repaint();
}

void ScopeDisplay::paint (juce::Graphics& g)
{
    const auto& p = palette (SkinId::Schematic);
    auto r = getLocalBounds().toFloat();
    g.setColour (p.panel);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (p.line.withAlpha (0.6f));
    for (int k = 1; k < 4; ++k) g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * k / 4.0f), r.getX(), r.getRight());
    for (int k = 1; k < 8; ++k) g.drawVerticalLine ((int) (r.getX() + r.getWidth() * k / 8.0f), r.getY(), r.getBottom());
    if (pts.size() < 2) return;
    // Trigger on a rising zero crossing so the waveform stands still.
    size_t start = 0;
    for (size_t i = 1; i < pts.size() / 2; ++i) if (pts[i - 1] < 0.0f && pts[i] >= 0.0f) { start = i; break; }
    const size_t n = std::min (pts.size() - start, pts.size() / 2);
    juce::Path wave;
    for (size_t i = 0; i < n; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / (float) (n - 1);
        const float y = r.getCentreY() - juce::jlimit (-1.0f, 1.0f, pts[start + i] * 2.0f) * r.getHeight() * 0.45f;
        if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
    }
    g.setColour (p.accent.withAlpha (0.25f));
    g.strokePath (wave, juce::PathStrokeType (4.0f));
    g.setColour (p.accent);
    g.strokePath (wave, juce::PathStrokeType (1.5f));
}
} // namespace ui
