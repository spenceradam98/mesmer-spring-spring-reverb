#include "PluginEditor.h"
#include "ParamIds.h"

using namespace juce;

namespace
{
constexpr int W = 1080, H = 612;
constexpr float kStart = MathConstants<float>::pi * -0.78f, kEnd = MathConstants<float>::pi * 0.78f;
}

// ============================================================================ LookAndFeel
static Colour fieldInk (const Theme& th)
{
    // text on dark "window" fields must stay readable in light themes too
    return th.window.getPerceivedBrightness() < 0.5f ? (th.ink.getPerceivedBrightness() > 0.5f ? th.ink : Colour (0xfff1ece0)) : th.ink;
}

void MesmerLookAndFeel::setTheme (const Theme& th)
{
    t = &th;
    const Colour fi = fieldInk (th);
    setColour (ComboBox::backgroundColourId, th.window);
    setColour (ComboBox::textColourId, fi);
    setColour (ComboBox::outlineColourId, th.panelEdge);
    setColour (ComboBox::arrowColourId, th.inkDim);
    setColour (PopupMenu::backgroundColourId, th.panel);
    setColour (PopupMenu::textColourId, th.ink);
    setColour (PopupMenu::headerTextColourId, th.inkDim);
    setColour (PopupMenu::highlightedBackgroundColourId, th.accent[2].withAlpha (0.35f));
    setColour (PopupMenu::highlightedTextColourId, th.ink);
    setColour (TextButton::textColourOffId, fi);
    setColour (TextButton::textColourOnId, th.window);
    setColour (Label::outlineColourId, Colours::transparentBlack);
    setColour (Label::backgroundColourId, Colours::transparentBlack);
    setColour (Slider::textBoxTextColourId, th.inkDim);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Slider::textBoxHighlightColourId, th.accent[2].withAlpha (0.4f));
    setColour (Label::textColourId, th.inkDim);
    setColour (Label::textWhenEditingColourId, th.ink);
    setColour (TextEditor::textColourId, th.ink);
    setColour (TextEditor::outlineColourId, Colours::transparentBlack);
    setColour (TextEditor::focusedOutlineColourId, th.accent[2]);
    setColour (TextEditor::backgroundColourId, th.window);
    setColour (TextEditor::highlightColourId, th.accent[2].withAlpha (0.4f));
    setColour (CaretComponent::caretColourId, th.ink);
}

Label* MesmerLookAndFeel::createSliderTextBox (Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (FontOptions (11.5f));
    l->setJustificationType (Justification::centred);
    return l;
}

void MesmerLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float start, float end, Slider& s)
{
    const auto& th = *t;
    const int section = s.getProperties()["section"];
    const bool bipolar = s.getProperties()["bipolar"];
    const Colour acc = th.accent[jlimit (0, 2, section)];

    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float d = jmin (b.getWidth(), b.getHeight()) - 4.0f;
    const auto c = b.getCentre();
    const float rArc = d * 0.5f - 2.0f;
    const float angle = start + pos * (end - start);

    // arc track and value arc
    Path track;
    track.addCentredArc (c.x, c.y, rArc, rArc, 0.0f, start, end, true);
    g.setColour (th.track);
    g.strokePath (track, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));
    Path val;
    const float from = bipolar ? (start + end) * 0.5f : start;
    val.addCentredArc (c.x, c.y, rArc, rArc, 0.0f, jmin (from, angle), jmax (from, angle), true);
    g.setColour (acc);
    g.strokePath (val, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));

    // body
    const float rb = rArc - 7.0f;
    auto body = Rectangle<float> (rb * 2.0f, rb * 2.0f).withCentre (c);
    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillEllipse (body.translated (0.0f, 2.5f));
    g.setGradientFill (ColourGradient (th.knobBody.brighter (0.25f), body.getX(), body.getY(), th.knobBody.darker (0.35f), body.getRight(), body.getBottom(), false));
    g.fillEllipse (body);
    auto cap = body.reduced (rb * 0.22f);
    g.setGradientFill (ColourGradient (th.knobCap.brighter (0.2f), cap.getX(), cap.getY(), th.knobCap.darker (0.3f), cap.getRight(), cap.getBottom(), false));
    g.fillEllipse (cap);

    // pointer
    g.setColour (th.knobPointer);
    g.drawLine (Line<float> (c.getPointOnCircumference (rb * 0.25f, angle), c.getPointOnCircumference (rb * 0.92f, angle)), 2.6f);
}

void MesmerLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool highlighted, bool)
{
    const auto& th = *t;
    const int section = b.getProperties()["section"];
    const Colour acc = th.accent[jlimit (0, 2, section)];
    const bool on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat();
    const String style = b.getProperties()["style"].toString();

    if (style == "big")   // Freeze / Bypass: a lit pad
    {
        auto pad = r.reduced (2.0f);
        g.setColour (on ? acc : th.window);
        g.fillRoundedRectangle (pad, 6.0f);
        g.setColour (on ? acc.brighter (0.4f) : (highlighted ? acc.withAlpha (0.7f) : th.panelEdge));
        g.drawRoundedRectangle (pad, 6.0f, 1.5f);
        g.setColour (on ? th.window : fieldInk (th));
        g.setFont (FontOptions (13.0f, Font::bold));
        g.drawText (b.getButtonText(), pad, Justification::centred);
        return;
    }

    // LED + label
    auto led = Rectangle<float> (12.0f, 12.0f).withCentre ({ r.getX() + 9.0f, r.getCentreY() });
    if (on)
    {
        g.setColour (acc.withAlpha (0.35f));
        g.fillEllipse (led.expanded (4.0f));
    }
    g.setColour (on ? acc : th.ledOff);
    g.fillEllipse (led);
    g.setColour (Colours::black.withAlpha (0.4f));
    g.drawEllipse (led, 1.0f);
    g.setColour (highlighted ? th.ink : th.inkDim);
    g.setFont (FontOptions (12.0f, Font::bold));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (20.0f), Justification::centredLeft);
}

void MesmerLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool highlighted, bool down)
{
    const auto& th = *t;
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool hot = b.getProperties()["hot"];
    g.setColour (down ? th.accent[2] : th.window);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (hot || highlighted ? th.accent[2] : th.panelEdge);
    g.drawRoundedRectangle (r, 5.0f, hot ? 1.8f : 1.0f);
}

// ============================================================================ Editor
MesmerEditor::MesmerEditor (MesmerProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    lnf.setTheme (themes()[(size_t) jlimit (0, (int) themes().size() - 1, proc.getThemeIndex())]);

    // chorus / flanger
    choTime = &knob (ids::choTime, "TIME", 0);
    choDepth = &knob (ids::choDepth, "DEPTH", 0);
    choRate = &knob (ids::choRate, "RATE", 0);
    choFb = &knob (ids::choFeedback, "FEEDBACK", 0, true);
    choMix = &knob (ids::choMix, "MIX", 0);
    choOn = &toggle (ids::choOn, "ON", 0, "led");
    choWave = &choice (ids::choWave, "LFO");

    // delay
    dlyTime = &knob (ids::dlyTime, "TIME", 1);
    dlyFb = &knob (ids::dlyFeedback, "FEEDBACK", 1);
    dlyMix = &knob (ids::dlyMix, "MIX", 1);
    dlyOn = &toggle (ids::dlyOn, "ON", 1, "led");
    pingPong = &toggle (ids::dlyPingPong, "PING-PONG", 1, "led");
    dlySync = &toggle (ids::dlySync, "SYNC", 1, "led");
    dlyDivision = &choice (ids::dlyDivision, "NOTE");
    freezeHold = &toggle (ids::freezeHold, "HOLD TO FREEZE", 1, "led");
    freezeHold->button.setTooltip ("On: Freeze holds only while pressed. Off: click to latch.");

    // Freeze: latching, or momentary while held (Hold mode, as on the hardware)
    freeze.getProperties().set ("section", 1);
    freeze.getProperties().set ("style", "big");
    freeze.setClickingTogglesState (false);
    freeze.onStateChange = [this]
    {
        const bool down = freeze.isDown();
        if (down == freezeWasDown) return;
        freezeWasDown = down;
        const bool hold = proc.apvts.getRawParameterValue (ids::freezeHold)->load() > 0.5f;
        if (hold) setFreeze (down);
        else if (down) setFreeze (proc.apvts.getRawParameterValue (ids::dlyFreeze)->load() < 0.5f);
    };
    addAndMakeVisible (freeze);
    dlyMode = &choice (ids::dlyMode, "MODE");

    // spring
    revPre = &knob (ids::revPre, "PRE-DELAY", 2);
    revDepth = &knob (ids::revDepth, "DEPTH", 2);
    revRate = &knob (ids::revRate, "RATE", 2);
    revAmount = &knob (ids::revAmount, "AMOUNT", 2);
    revDecay = &knob (ids::revDecay, "DECAY", 2);
    revDwell = &knob (ids::revDwell, "DWELL", 2);
    revDrip = &knob (ids::revDrip, "DRIP", 2);
    revBoing = &knob (ids::revBoing, "BOING", 2);
    revTone = &knob (ids::revTone, "TONE", 2, true);
    revWidth = &knob (ids::revWidth, "WIDTH", 2);
    revOn = &toggle (ids::revOn, "ON", 2, "led");
    revWave = &choice (ids::revWave, "LFO");
    revTank = &choice (ids::revTank, "TANK");

    // global
    input = &knob (ids::input, "INPUT", 2, true);
    output = &knob (ids::output, "OUTPUT", 2, true);
    bypass = &toggle (ids::bypass, "BYPASS", 2, "big");
    routing = &choice (ids::routing, "ORDER");

    // kick: momentary, drives the automatable Kick parameter
    kick.getProperties().set ("hot", true);
    kick.setTooltip ("Shake the spring tank");
    kick.onStateChange = [this]
    {
        if (auto* prm = proc.apvts.getParameter (ids::kick))
        {
            const float v = kick.isDown() ? 1.0f : 0.0f;
            if (std::abs (prm->getValue() - v) > 0.5f) prm->setValueNotifyingHost (v);
        }
    };
    addAndMakeVisible (kick);

    // presets: factory list, then the user's saved presets (ids from 1000)
    addAndMakeVisible (presetBox);
    refreshPresetBox();
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= 1000)
        {
            if (isPositiveAndBelow (id - 1000, userFiles.size())) proc.loadUserPreset (userFiles[id - 1000]);
        }
        else if (id > 0) proc.setCurrentProgram (id - 1);
    };
    addAndMakeVisible (savePreset);
    savePreset.setTooltip ("Save the current sound as a user preset");
    savePreset.onClick = [this]
    {
        auto folder = MesmerProcessor::getUserPresetFolder();
        folder.createDirectory();
        chooser = std::make_unique<FileChooser> ("Save preset", folder.getChildFile (proc.getPresetName() + ".mspreset"), "*.mspreset");
        chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
                              [this] (const FileChooser& fc)
                              {
                                  auto f = fc.getResult();
                                  if (f == File()) return;
                                  proc.saveUserPreset (f.withFileExtension ("mspreset"));
                                  refreshPresetBox();
                              });
    };
    for (auto* b : { &prevPreset, &nextPreset })
    {
        addAndMakeVisible (*b);
        b->onClick = [this, b]
        {
            const int n = proc.getNumPrograms();
            const int cur = jmax (0, proc.getCurrentProgram());
            const int next = (cur + (b == &nextPreset ? 1 : n - 1)) % n;
            proc.setCurrentProgram (next);
            presetBox.setSelectedId (next + 1, dontSendNotification);
        };
    }

    // themes
    addAndMakeVisible (themeBox);
    for (int i = 0; i < (int) themes().size(); ++i) themeBox.addItem (themes()[(size_t) i].name, i + 1);
    themeBox.setSelectedId (proc.getThemeIndex() + 1, dontSendNotification);
    themeBox.onChange = [this] { applyTheme (themeBox.getSelectedId() - 1); };

    applyTheme (proc.getThemeIndex());
    setSize (W, H);
    startTimerHz (30);
}

void MesmerEditor::setFreeze (bool on)
{
    if (auto* p = proc.apvts.getParameter (ids::dlyFreeze))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (on ? 1.0f : 0.0f);
        p->endChangeGesture();
    }
    freeze.setToggleState (on, dontSendNotification);
}

MesmerEditor::~MesmerEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

MesmerEditor::Knob& MesmerEditor::knob (const char* id, const String& label, int section, bool bipolar)
{
    auto k = std::make_unique<Knob>();
    k->label = label;
    k->slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k->slider.setRotaryParameters (kStart + MathConstants<float>::twoPi, kEnd + MathConstants<float>::twoPi, true);
    k->slider.setTextBoxStyle (Slider::TextBoxBelow, false, 84, 16);
    k->slider.getProperties().set ("section", section);
    k->slider.getProperties().set ("bipolar", bipolar);
    k->slider.setPopupDisplayEnabled (false, false, nullptr);
    addAndMakeVisible (k->slider);
    k->attach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k->slider);
    knobs.push_back (std::move (k));
    return *knobs.back();
}

MesmerEditor::Toggle& MesmerEditor::toggle (const char* id, const String& text, int section, const char* style)
{
    auto t = std::make_unique<Toggle>();
    t->button.setButtonText (text);
    t->button.getProperties().set ("section", section);
    t->button.getProperties().set ("style", style);
    addAndMakeVisible (t->button);
    t->attach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, t->button);
    toggles.push_back (std::move (t));
    return *toggles.back();
}

MesmerEditor::Choice& MesmerEditor::choice (const char* id, const String& label)
{
    auto c = std::make_unique<Choice>();
    c->label = label;
    if (auto* prm = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter (id)))
        c->box.addItemList (prm->choices, 1);
    addAndMakeVisible (c->box);
    c->attach = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, c->box);
    choices.push_back (std::move (c));
    return *choices.back();
}

void MesmerEditor::refreshPresetBox()
{
    presetBox.clear (dontSendNotification);
    const auto& ps = ms::factoryPresets();
    String lastCat;
    auto* menu = presetBox.getRootMenu();
    for (int i = 0; i < (int) ps.size(); ++i)
    {
        const String cat (ps[(size_t) i].category);
        if (cat != lastCat) { menu->addSectionHeader (cat); lastCat = cat; }
        menu->addItem (i + 1, ps[(size_t) i].name);
    }
    userFiles = MesmerProcessor::getUserPresets();
    if (! userFiles.isEmpty())
    {
        menu->addSectionHeader ("User");
        for (int i = 0; i < userFiles.size(); ++i) menu->addItem (1000 + i, userFiles[i].getFileNameWithoutExtension());
    }
    if (proc.getCurrentProgram() >= 0) presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    else presetBox.setText (proc.getPresetName(), dontSendNotification);
}

void MesmerEditor::applyTheme (int index)
{
    index = jlimit (0, (int) themes().size() - 1, index);
    proc.setThemeIndex (index);
    lnf.setTheme (themes()[(size_t) index]);
    for (auto& k : knobs)
    {
        // value boxes are Labels created by the Slider: give them the theme colours explicitly
        k->slider.setColour (Slider::textBoxTextColourId, lnf.theme().inkDim);
        k->slider.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
        k->slider.setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    }
    sendLookAndFeelChange();
    repaint();
}

void MesmerEditor::placeKnob (Knob& k, int x, int y, int size)
{
    // label is painted above; the slider holds the knob and its value box
    k.slider.setBounds (x, y + 14, size, size + 16);
}

// ---------------------------------------------------------------- layout
void MesmerEditor::resized()
{
    // header
    prevPreset.setBounds (330, 18, 30, 28);
    presetBox.setBounds (364, 18, 260, 28);
    nextPreset.setBounds (628, 18, 30, 28);
    savePreset.setBounds (664, 18, 56, 28);
    themeBox.setBounds (W - 300, 18, 150, 28);
    bypass->button.setBounds (W - 136, 16, 116, 32);

    choPanel = { 16, 72, 236, 462 };
    dlyPanel = { 264, 72, 236, 462 };
    revPanel = { 512, 72, 552, 462 };
    footer = { 16, 544, W - 32, 56 };

    // chorus / flanger
    {
        auto p = choPanel;
        choOn->button.setBounds (p.getRight() - 64, p.getY() + 8, 56, 22);
        const int k = 84, x0 = p.getX() + 20, x1 = p.getX() + 132, y0 = p.getY() + 44;
        placeKnob (*choTime, x0, y0, k);
        placeKnob (*choDepth, x1, y0, k);
        placeKnob (*choRate, x0, y0 + 128, k);
        placeKnob (*choFb, x1, y0 + 128, k);
        placeKnob (*choMix, x0, y0 + 256, k);
        choWave->box.setBounds (x1 - 4, y0 + 256 + 40, 96, 26);
    }
    // delay
    {
        auto p = dlyPanel;
        dlyOn->button.setBounds (p.getRight() - 64, p.getY() + 8, 56, 22);
        const int k = 84, x0 = p.getX() + 20, x1 = p.getX() + 132, y0 = p.getY() + 44;
        placeKnob (*dlyTime, x0, y0, k);
        placeKnob (*dlyFb, x1, y0, k);
        placeKnob (*dlyMix, x0, y0 + 128, k);
        dlyMode->box.setBounds (x1 - 4, y0 + 128 + 40, 96, 26);
        pingPong->button.setBounds (x1 - 4, y0 + 128 + 76, 110, 22);
        dlySync->button.setBounds (x0 + 6, y0 + 262, 80, 24);
        dlyDivision->box.setBounds (x1 - 4, y0 + 262, 96, 24);
        freeze.setBounds (p.getX() + 20, y0 + 300, p.getWidth() - 40, 70);
        freezeHold->button.setBounds (p.getX() + 24, y0 + 378, p.getWidth() - 40, 22);
    }
    // spring
    {
        auto p = revPanel;
        revOn->button.setBounds (p.getRight() - 64, p.getY() + 8, 56, 22);
        const int k = 84, gap = 132, x0 = p.getX() + 24, y0 = p.getY() + 40;
        placeKnob (*revPre, x0, y0, k);
        placeKnob (*revDepth, x0 + gap, y0, k);
        placeKnob (*revRate, x0 + gap * 2, y0, k);
        placeKnob (*revAmount, x0 + gap * 3, y0, k);

        tankWindow = { p.getX() + 16, y0 + 128, p.getWidth() - 140, 110 };
        revWave->box.setBounds (tankWindow.getRight() + 12, tankWindow.getY() + 4, 96, 24);
        revTank->box.setBounds (tankWindow.getRight() + 12, tankWindow.getY() + 44, 96, 24);
        kick.setBounds (tankWindow.getRight() + 12, tankWindow.getBottom() - 30, 96, 30);

        const int k2 = 70, gap2 = 88, y1 = tankWindow.getBottom() + 14;
        Knob* row[] = { revDecay, revDwell, revDrip, revBoing, revTone, revWidth };
        for (int i = 0; i < 6; ++i) placeKnob (*row[i], p.getX() + 12 + i * gap2, y1, k2);
    }
    // footer
    {
        const int y = footer.getY() - 8;
        input->slider.setBounds (footer.getX() + 72, y, 56, 64);
        output->slider.setBounds (footer.getRight() - 72 - 56, y, 56, 64);
        input->slider.setTextBoxStyle (Slider::TextBoxRight, false, 64, 16);
        output->slider.setTextBoxStyle (Slider::TextBoxRight, false, 64, 16);
        input->slider.setBounds (footer.getX() + 64, footer.getY() + 4, 120, 48);
        output->slider.setBounds (footer.getRight() - 132, footer.getY() + 4, 120, 48);
        routing->box.setBounds (footer.getCentreX() - 80, footer.getY() + 14, 160, 26);
    }
}

// ---------------------------------------------------------------- painting
void MesmerEditor::drawSpiral (Graphics& g, Point<float> c, float radius, float angle, Colour a, Colour b)
{
    // two interleaved Archimedean arms: the "hypnotic" eye
    for (int arm = 0; arm < 2; ++arm)
    {
        Path sp;
        const int steps = 140;
        for (int i = 0; i <= steps; ++i)
        {
            const float u = (float) i / steps;
            const float th = angle + arm * MathConstants<float>::pi + u * MathConstants<float>::twoPi * 3.0f;
            const auto pt = c.getPointOnCircumference (radius * u, th);
            if (i == 0) sp.startNewSubPath (pt); else sp.lineTo (pt);
        }
        g.setColour (arm == 0 ? a : b);
        g.strokePath (sp, PathStrokeType (radius * 0.09f, PathStrokeType::curved, PathStrokeType::rounded));
    }
}

void MesmerEditor::drawPanel (Graphics& g, Rectangle<int> r, const String& title, int section)
{
    const auto& th = lnf.theme();
    auto rf = r.toFloat();
    g.setColour (Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (rf.translated (0.0f, 3.0f), 10.0f);
    g.setColour (th.panel);
    g.fillRoundedRectangle (rf, 10.0f);
    g.setColour (th.panelEdge);
    g.drawRoundedRectangle (rf.reduced (0.5f), 10.0f, 1.0f);
    g.setColour (th.accent[section]);
    g.fillRoundedRectangle (rf.getX() + 14.0f, rf.getY() + 14.0f, 4.0f, 14.0f, 2.0f);
    g.setColour (th.ink);
    g.setFont (FontOptions (14.0f, Font::bold));
    g.drawText (title, r.getX() + 26, r.getY() + 10, 200, 22, Justification::centredLeft);
}

void MesmerEditor::drawTankWindow (Graphics& g, Rectangle<float> r)
{
    const auto& th = lnf.theme();
    g.setColour (th.window);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (th.panelEdge);
    g.drawRoundedRectangle (r, 8.0f, 1.0f);

    auto inner = r.reduced (14.0f, 12.0f);
    const float eyeR = inner.getHeight() * 0.5f;
    auto eye = inner.removeFromRight (eyeR * 2.0f);
    inner.removeFromRight (10.0f);

    // transducers at both ends
    g.setColour (th.inkDim.withAlpha (0.5f));
    g.fillRoundedRectangle (inner.getX() - 6.0f, inner.getY(), 8.0f, inner.getHeight(), 2.0f);
    g.fillRoundedRectangle (inner.getRight() - 2.0f, inner.getY(), 8.0f, inner.getHeight(), 2.0f);

    // three springs; their wobble follows the wet level, and a kick shakes them hard
    const float level = jlimit (0.0f, 1.0f, revLevel * 3.0f + kickShake);
    for (int s = 0; s < 3; ++s)
    {
        const float yc = inner.getY() + inner.getHeight() * (0.2f + 0.3f * s);
        Path coil;
        const int turns = 34 + 4 * s;
        const int steps = turns * 10;
        for (int i = 0; i <= steps; ++i)
        {
            const float u = (float) i / steps;
            const float x = inner.getX() + u * inner.getWidth();
            const float wave = std::sin ((float) phase * (3.0f + s) - u * 9.0f) * level * 7.0f * std::sin (u * MathConstants<float>::pi);
            const float y = yc + std::sin (u * turns * MathConstants<float>::twoPi) * 5.0f + wave;
            if (i == 0) coil.startNewSubPath (x, y); else coil.lineTo (x, y);
        }
        g.setColour (th.coil.withAlpha (0.55f + 0.45f * level));
        g.strokePath (coil, PathStrokeType (1.3f));
    }

    // the pickup end shows a slowly turning spiral driven by the reverb LFO
    drawSpiral (g, eye.getCentre(), eyeR * 0.95f, tankAngle, th.accent[2], th.accent[0].withAlpha (0.8f));
}

void MesmerEditor::paint (Graphics& g)
{
    const auto& th = lnf.theme();
    g.setGradientFill (ColourGradient (th.bgTop, 0.0f, 0.0f, th.bgBottom, 0.0f, (float) H, false));
    g.fillAll();

    // logo
    if (th.spiral)
    {
        Graphics::ScopedSaveState s (g);
        g.setOpacity (0.35f);
        drawSpiral (g, { 44.0f, 32.0f }, 24.0f, spiralAngle, th.accent[2], th.accent[0]);
    }
    else
    {
        g.setColour (th.accent[2]);
        g.fillEllipse (26.0f, 14.0f, 36.0f, 36.0f);
        g.setColour (th.bgTop);
        drawSpiral (g, { 44.0f, 32.0f }, 15.0f, spiralAngle, th.bgTop, th.bgTop);
    }
    g.setColour (th.ink);
    g.setFont (FontOptions (26.0f, Font::bold));
    g.drawText ("MESMER", 76, 10, 200, 30, Justification::centredLeft);
    g.setColour (th.inkDim);
    g.setFont (FontOptions (11.0f, Font::bold));
    g.drawText ("SPRING  .  TIME EFFECTS", 78, 38, 220, 14, Justification::centredLeft);
    g.drawText ("THEME", W - 352, 18, 48, 28, Justification::centredRight);

    drawPanel (g, choPanel, "CHO-FLA", 0);
    drawPanel (g, dlyPanel, "DELAY", 1);
    drawPanel (g, revPanel, "SPRING REVERB", 2);
    drawTankWindow (g, tankWindow.toFloat());

    // knob labels
    g.setFont (FontOptions (11.5f, Font::bold));
    for (auto& k : knobs)
    {
        if (k.get() == input || k.get() == output) continue;
        auto b = k->slider.getBounds();
        g.setColour (th.ink);
        g.drawText (k->label, b.getX() - 10, b.getY() - 15, b.getWidth() + 20, 14, Justification::centred);
    }
    for (auto& c : choices)
    {
        if (c.get() == routing) continue;
        auto b = c->box.getBounds();
        g.setColour (th.inkDim);
        g.setFont (FontOptions (10.5f, Font::bold));
        g.drawText (c->label, b.getX(), b.getY() - 14, b.getWidth(), 13, Justification::centredLeft);
    }

    // footer
    g.setColour (th.panel.withAlpha (0.7f));
    g.fillRoundedRectangle (footer.toFloat(), 10.0f);
    g.setColour (th.ink);
    g.setFont (FontOptions (12.0f, Font::bold));
    g.drawText ("INPUT", footer.getX() + 12, footer.getY(), 50, footer.getHeight(), Justification::centredLeft);
    g.drawText ("OUTPUT", footer.getRight() - 200, footer.getY(), 64, footer.getHeight(), Justification::centredRight);
    g.drawText ("ORDER", footer.getCentreX() - 150, footer.getY(), 62, footer.getHeight(), Justification::centredLeft);

    // input peak lamp: set Input so it only blinks on the loudest hits
    auto lamp = Rectangle<float> (12.0f, 12.0f).withCentre ({ (float) footer.getX() + 200.0f, (float) footer.getCentreY() });
    const Colour lampCol = inPeak > 0.95f ? Colour (0xffff3b30) : (inPeak > 0.05f ? Colour (0xff3ad46b).withAlpha (0.4f + 0.6f * jmin (1.0f, inPeak)) : th.ledOff);
    g.setColour (lampCol);
    g.fillEllipse (lamp);
    g.setColour (th.inkDim);
    g.setFont (FontOptions (10.5f, Font::bold));
    g.drawText ("PEAK", (int) lamp.getRight() + 6, footer.getY(), 40, footer.getHeight(), Justification::centredLeft);
}

void MesmerEditor::timerCallback()
{
    const auto& e = proc.getEngine();
    const auto& prm = proc.apvts;
    const float rate = prm.getRawParameterValue (ids::revRate)->load();
    phase += 0.25;
    spiralAngle += 0.02f;
    tankAngle += (float) (MathConstants<double>::twoPi * rate / 30.0) * 0.35f + 0.01f;
    revLevel = revLevel * 0.6f + e.getReverbLevel() * 0.4f;
    inPeak = e.getInputPeak();
    kickShake = kick.isDown() ? 1.0f : kickShake * 0.9f;

    if (proc.getCurrentProgram() >= 0)
    {
        if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
            presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    }
    else if (presetBox.getText() != proc.getPresetName())
        presetBox.setText (proc.getPresetName(), dontSendNotification);

    // Freeze lamp follows the parameter (host automation included); synced time dims the Time knob
    const bool frozen = prm.getRawParameterValue (ids::dlyFreeze)->load() > 0.5f;
    if (freeze.getToggleState() != frozen) freeze.setToggleState (frozen, dontSendNotification);
    const bool synced = prm.getRawParameterValue (ids::dlySync)->load() > 0.5f;
    dlyTime->slider.setAlpha (synced ? 0.35f : 1.0f);
    dlyDivision->box.setAlpha (synced ? 1.0f : 0.45f);

    repaint (tankWindow);
    repaint (footer);
    repaint (0, 0, 80, 64);
}
