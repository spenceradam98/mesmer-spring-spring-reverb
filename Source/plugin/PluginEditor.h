#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Themes.h"

class MesmerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MesmerLookAndFeel() { setTheme (themes()[0]); }
    void setTheme (const Theme&);
    const Theme& theme() const { return *t; }

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::FontOptions (13.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return juce::FontOptions (13.0f, juce::Font::bold); }
    juce::Label* createSliderTextBox (juce::Slider&) override;

private:
    const Theme* t = nullptr;
};

class MesmerEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MesmerEditor (MesmerProcessor&);
    ~MesmerEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Knob
    {
        juce::Slider slider;
        juce::String label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    struct Toggle
    {
        juce::ToggleButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attach;
    };
    struct Choice
    {
        juce::ComboBox box;
        juce::String label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attach;
    };

    Knob& knob (const char* id, const juce::String& label, int section, bool bipolar = false);
    Toggle& toggle (const char* id, const juce::String& text, int section, const char* style);
    Choice& choice (const char* id, const juce::String& label);
    void placeKnob (Knob&, int x, int y, int size);
    void applyTheme (int index);
    void refreshPresetBox();
    void drawPanel (juce::Graphics&, juce::Rectangle<int>, const juce::String& title, int section);
    void drawTankWindow (juce::Graphics&, juce::Rectangle<float>);
    void drawSpiral (juce::Graphics&, juce::Point<float> centre, float radius, float angle, juce::Colour a, juce::Colour b);

    MesmerProcessor& proc;
    MesmerLookAndFeel lnf;

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Toggle>> toggles;
    std::vector<std::unique_ptr<Choice>> choices;

    Knob *choTime, *choDepth, *choRate, *choFb, *choMix;
    Knob *dlyTime, *dlyFb, *dlyMix;
    Knob *revPre, *revDepth, *revRate, *revAmount, *revDecay, *revDwell, *revDrip, *revBoing, *revTone, *revWidth;
    Knob *input, *output;
    Toggle *choOn, *dlyOn, *revOn, *freeze, *pingPong, *bypass;
    Choice *choWave, *dlyMode, *revWave, *revTank, *routing;

    juce::ComboBox presetBox, themeBox;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" }, kick { "KICK" };

    juce::Rectangle<int> choPanel, dlyPanel, revPanel, tankWindow, footer;
    float spiralAngle = 0, tankAngle = 0, revLevel = 0, kickShake = 0, inPeak = 0;
    double phase = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MesmerEditor)
};
