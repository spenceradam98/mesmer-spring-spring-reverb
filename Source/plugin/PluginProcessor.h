#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Engine.h"
#include "Presets.h"

class MesmerProcessor : public juce::AudioProcessor
{
public:
    MesmerProcessor();
    ~MesmerProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override;

    // UI hooks
    void kickTank() { engine.kickTank (1.0f); }
    const ms::Engine& getEngine() const { return engine; }
    int getThemeIndex() const { return themeIndex; }
    void setThemeIndex (int t) { themeIndex = t; }

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    ms::Params readParams() const;

    ms::Engine engine;
    int currentProgram = 0;
    int themeIndex = 0;
    bool lastKick = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MesmerProcessor)
};
