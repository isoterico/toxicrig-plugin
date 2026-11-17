#include "PluginEditor.h"
#include "WebEditor.h"

namespace ui
{
namespace
{
class GearButton : public juce::Button
{
public:
    GearButton() : juce::Button ("Impostazioni") { setTooltip ("Impostazioni: stile grafico, dimensione finestra..."); }
    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        const auto& p = palette (skinOf (*this));
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (down ? p.accent2 : (over ? p.panel2.brighter (0.2f) : p.panel2));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (p.line);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
        const auto c = r.getCentre();
        const float R = r.getHeight() * 0.32f;
        juce::Path gearPath;
        for (int i = 0; i < 16; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / 16.0f;
            const float rr = (i % 2 == 0) ? R : R * 0.74f;
            const auto pt = c + juce::Point<float> (std::cos (a), std::sin (a)) * rr;
            if (i == 0) gearPath.startNewSubPath (pt); else gearPath.lineTo (pt);
        }
        gearPath.closeSubPath();
        g.setColour (over ? p.accent : p.text);
        g.fillPath (gearPath);
        g.setColour (down ? p.accent2 : p.panel2);
        g.fillEllipse (c.x - R * 0.36f, c.y - R * 0.36f, R * 0.72f, R * 0.72f);
    }
};
} // namespace

// ============================================================================ Header
Header::Header (ToxicRigProcessor& p) : proc (p)
{
    addAndMakeVisible (presetBox);
    refreshPresets();
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
                const auto f = fc.getResult();
                if (f != juce::File()) proc.saveUserPreset (f.withFileExtension ("trpreset"));
                refreshPresets();
            });
        }
        else if (id == 9001) ToxicRigProcessor::userPresetFolder().startAsProcess();
    };
    prevBtn.onClick = [this] { const int n = (int) rig::factoryPresets().size(); presetBox.setSelectedId (((std::max (1, presetBox.getSelectedId()) - 2 + n) % n) + 1); };
    nextBtn.onClick = [this] { const int n = (int) rig::factoryPresets().size(); presetBox.setSelectedId ((std::max (0, presetBox.getSelectedId()) % n) + 1); };
    saveBtn.onClick = [this] { presetBox.setSelectedId (9000); };
    aBtn.onClick = [this] { proc.recallAB (0); };
    bBtn.onClick = [this] { proc.recallAB (1); };
    prevBtn.setTooltip ("Preset precedente");
    nextBtn.setTooltip ("Preset successivo");
    saveBtn.setTooltip ("Salva il suono attuale come preset");
    aBtn.setTooltip ("Confronto A/B: versione A del suono");
    bBtn.setTooltip ("Confronto A/B: versione B (la prima volta parte come copia di A)");
    for (auto* b : { &prevBtn, &nextBtn, &saveBtn, &aBtn, &bBtn }) addAndMakeVisible (b);
    osBox = std::make_unique<ChoiceButton> (p.apvts, rig::ids::oversampling);
    osBox->setTooltip ("Oversampling: piu' alto = meno aliasing sulle note acute, piu' CPU. 4x e' l'equilibrio giusto.");
    addAndMakeVisible (*osBox);
    gear = std::make_unique<GearButton>();
    gear->onClick = [this] { showSettings(); };
    addAndMakeVisible (*gear);
    inKnob = std::make_unique<ParamKnob> (p.apvts, rig::ids::inGain, KnobStyle::Panel);
    outKnob = std::make_unique<ParamKnob> (p.apvts, rig::ids::outVol, KnobStyle::Panel);
    inKnob->setTooltip ("Input: regola finche' il meter arriva a circa meta' quando suoni forte");
    outKnob->setTooltip ("Volume d'uscita");
    for (juce::Component* c : std::initializer_list<juce::Component*> { inKnob.get(), outKnob.get(), &inMeter, &outMeter, &vuIn, &vuOut, &tuner })
        addAndMakeVisible (c);
}

void Header::refreshPresets()
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
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}

void Header::showSettings()
{
    const int cur = std::max (0, proc.getSkin());
    juce::PopupMenu skins, sizes, m;
    for (int i = 0; i < skinNames.size(); ++i) skins.addItem (100 + i, skinNames[i], true, i == cur);
    const float scales[] = { 0.6f, 0.75f, 0.85f, 1.0f, 1.25f, 1.5f, 2.0f };
    for (int i = 0; i < 7; ++i) sizes.addItem (200 + i, juce::String (juce::roundToInt (scales[i] * 100)) + "%");
    m.addSectionHeader ("TOXICRIG " JucePlugin_VersionString);
    m.addSubMenu ("Stile grafico", skins);
    m.addSubMenu ("Dimensione finestra", sizes);
    m.addItem (300, "Rimetti i pedali nell'ordine iniziale");
    const auto stage = rig::stageStatus();
    m.addItem (401, "Palco 3D: " + stage, false, false);
    if (stage == "pronto" || stage.startsWith ("disattivato"))
        m.addItem (400, "Passa al palco 3D");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (gear.get()), [this, scales] (int r)
    {
        if (r >= 100 && r < 103 && onSkin) onSkin (r - 100);
        else if (r >= 200 && r < 207 && onScale) onScale (scales[r - 200]);
        else if (r == 300) proc.setRouting (rig::Routing::defaults());
        else if (r == 400)
        {
            settings().setValue ("classicUi", false);
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Palco 3D",
                "Chiudi e riapri la finestra di ToxicRig: si aprira' il palco 3D.");
        }
    });
}

void Header::updateMeters (float in, float outL, float outR)
{
    inMeter.set (in, in);
    outMeter.set (outL, outR);
    vuIn.set (in);
    vuOut.set (std::max (outL, outR));
    aBtn.setToggleState (proc.currentAB == 0, juce::dontSendNotification);
    bBtn.setToggleState (proc.currentAB == 1, juce::dontSendNotification);
}

void Header::paint (juce::Graphics& g)
{
    const auto sk = skinOf (*this);
    const auto& p = palette (sk);
    auto r = getLocalBounds().toFloat();
    if (sk == SkinId::StudioNoir)
    {
        juce::ColourGradient wood (juce::Colour (0xff2b2824), 0, 0, juce::Colour (0xff171513), 0, r.getHeight(), false);
        g.setGradientFill (wood);
    }
    else g.setColour (sk == SkinId::ToxicLab ? juce::Colour (0xf20a0a10) : p.panel);
    g.fillRect (r);
    g.setColour (sk == SkinId::ToxicLab ? p.accent2.withAlpha (0.4f) : p.line);
    g.drawHorizontalLine (getHeight() - 1, 0, r.getWidth());
    switch (sk)
    {
        case SkinId::ToxicLab:
            g.setColour (p.accent);
            g.setFont (skinFont (sk, FontRole::Title, 25.0f));
            g.drawText ("TOXICRIG", 16, 8, 180, 30, juce::Justification::centredLeft);
            g.setColour (p.accent2.brighter (0.3f));
            g.setFont (skinFont (sk, FontRole::Label, 13.0f));
            g.drawText ("BY TOXICZONE", 17, 38, 180, 16, juce::Justification::centredLeft);
            break;
        case SkinId::StudioNoir:
            g.setColour (p.text);
            g.setFont (skinFont (sk, FontRole::Serif, 34.0f));
            g.drawText ("ToxicRig", 16, 4, 180, 38, juce::Justification::centredLeft);
            g.setColour (p.dim);
            g.setFont (skinFont (sk, FontRole::Body, 13.0f));
            g.drawText ("TUBE ENGINE  N. 001", 18, 40, 180, 16, juce::Justification::centredLeft);
            break;
        case SkinId::Schematic:
            g.setColour (p.text);
            g.setFont (skinFont (sk, FontRole::Title, 24.0f));
            g.drawText ("TOXIC/RIG", 16, 8, 180, 30, juce::Justification::centredLeft);
            g.setColour (p.dim);
            g.setFont (skinFont (sk, FontRole::Mono, 11.0f));
            g.drawText ("rev " JucePlugin_VersionString " - schematic", 17, 38, 180, 16, juce::Justification::centredLeft);
            break;
    }
    g.setColour (p.dim);
    g.setFont (skinFont (sk, FontRole::Label, 11.5f));
    g.drawText ("OVERSAMPLING", 650, 4, 86, 14, juce::Justification::centred);
    g.drawText ("INPUT", inKnob->getBounds().withY (1).withHeight (13), juce::Justification::centred);
    g.drawText ("OUTPUT", outKnob->getBounds().withY (1).withHeight (13), juce::Justification::centred);
}

void Header::resized()
{
    const auto sk = skinOf (*this);
    presetBox.setBounds (200, 16, 220, 32);
    prevBtn.setBounds (426, 16, 32, 32);
    nextBtn.setBounds (462, 16, 32, 32);
    saveBtn.setBounds (500, 16, 66, 32);
    aBtn.setBounds (578, 16, 32, 32);
    bBtn.setBounds (614, 16, 32, 32);
    osBox->setBounds (656, 20, 74, 30);
    gear->setBounds (740, 16, 34, 32);
    const bool noir = sk == SkinId::StudioNoir;
    inMeter.setVisible (! noir); outMeter.setVisible (! noir);
    vuIn.setVisible (noir); vuOut.setVisible (noir);
    if (noir)
    {
        inKnob->setBounds (784, 15, 46, 46);
        vuIn.setBounds (836, 4, 92, 56);
        tuner.setBounds (934, 8, 186, 50);
        outKnob->setBounds (1124, 15, 46, 46);
        vuOut.setBounds (1176, 4, 96, 56);
    }
    else
    {
        inKnob->setBounds (790, 15, 46, 46);
        inMeter.setBounds (844, 10, 12, 46);
        tuner.setBounds (870, 8, 300, 50);
        outKnob->setBounds (1186, 15, 46, 46);
        outMeter.setBounds (1240, 10, 12, 46);
    }
}

// ============================================================================ Root
Root::Root (ToxicRigProcessor& p) : proc (p), header (p)
{
    addAndMakeVisible (header);
    header.onSkin = [this] (int s) { setSkin ((SkinId) s); };
    header.onScale = [this] (float s) { if (onScale) onScale (s); };
    setSkin ((SkinId) (proc.getSkin() >= 0 ? proc.getSkin() : juce::jlimit (0, 2, settings().getIntValue ("skin", 0))));
    startTimerHz (30);
}

Root::~Root()
{
    view.reset();
    setLookAndFeel (nullptr);
}

void Root::setSkin (SkinId s)
{
    skin = s;
    proc.setSkin ((int) s);
    settings().setValue ("skin", (int) s);
    auto newLnf = std::make_unique<SkinLookAndFeel> (s);
    setLookAndFeel (newLnf.get());
    view.reset();
    lnf = std::move (newLnf);
    if (s == SkinId::ToxicLab) view = std::make_unique<StageView> (proc);
    else view = std::make_unique<ClassicView> (proc);
    addAndMakeVisible (*view);
    resized();
    header.resized();
    repaint();
}

void Root::paint (juce::Graphics& g)
{
    const auto& p = palette (skin);
    g.fillAll (p.bg);
    if (skin == SkinId::Schematic)
    {
        g.setColour (p.accent.withAlpha (0.05f));
        for (int x = 0; x < W; x += 32) g.drawVerticalLine (x, 64.0f, (float) H);
        for (int y = 64; y < H; y += 32) g.drawHorizontalLine (y, 0.0f, (float) W);
    }
    else if (skin == SkinId::StudioNoir)
    {
        juce::ColourGradient glow (juce::Colour (0xff2b2824), W * 0.5f, 64.0f, p.bg, W * 0.5f, (float) H, true);
        g.setGradientFill (glow);
        g.fillRect (0, 64, W, H - 64);
    }
}

void Root::resized()
{
    header.setBounds (0, 0, W, 64);
    if (view) view->setBounds (0, 64, W, H - 64);
}

void Root::timerCallback()
{
    auto& e = proc.engine;
    inLevel = std::max (inLevel * 0.85f, e.inPeak.exchange (0.0f));
    outL = std::max (outL * 0.85f, e.outPeakL.exchange (0.0f));
    outR = std::max (outR * 0.85f, e.outPeakR.exchange (0.0f));
    header.updateMeters (inLevel, outL, outR);

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
        const size_t Wn = tunerWindow.size();
        if (fresh.size() >= Wn) std::copy (fresh.end() - (long) Wn, fresh.end(), tunerWindow.begin());
        else if (! fresh.empty())
        {
            std::move (tunerWindow.begin() + (long) fresh.size(), tunerWindow.end(), tunerWindow.begin());
            std::copy (fresh.begin(), fresh.end(), tunerWindow.end() - (long) fresh.size());
        }
    }
    if (++tick % 3 == 0)
    {
        const auto ni = tz::hzToNote (detector.detect (tunerWindow.data(), (int) tunerWindow.size()));
        header.setTuner (ni.midi, ni.cents);
    }
}
} // namespace ui

// ============================================================================ Editor
ToxicRigEditor::ToxicRigEditor (ToxicRigProcessor& p) : AudioProcessorEditor (p), root (p)
{
    addAndMakeVisible (root);
    root.onScale = [this] (float s) { setScale (s); };
    setResizable (true, true);
    setResizeLimits ((int) (ui::Root::W * 0.5), (int) (ui::Root::H * 0.5), ui::Root::W * 2, ui::Root::H * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) ui::Root::W / ui::Root::H);
    const float s = (float) ui::settings().getDoubleValue ("scale", 0.85);
    setSize ((int) (ui::Root::W * s), (int) (ui::Root::H * s));
}

void ToxicRigEditor::setScale (float s)
{
    setSize ((int) (ui::Root::W * s), (int) (ui::Root::H * s));
}

void ToxicRigEditor::resized()
{
    const float scale = (float) getWidth() / (float) ui::Root::W;
    root.setBounds (0, 0, ui::Root::W, ui::Root::H);
    root.setTransform (juce::AffineTransform::scale (scale));
    ui::settings().setValue ("scale", (double) scale);
}
