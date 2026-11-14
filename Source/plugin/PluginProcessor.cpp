#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParamIds.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout MesmerProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    const ms::Params d;   // defaults come from the engine

    auto pct = [] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; };
    auto bipct = [] (float v, int) { return (v > 0.005f ? "+" : "") + String (roundToInt (v * 100.0f)) + " %"; };
    auto msTxt = [] (float v, int) { return v < 10.0f ? String (v, 1) + " ms" : String (roundToInt (v)) + " ms"; };
    auto hzTxt = [] (float v, int) { return v < 1.0f ? String (1.0f / v, 1) + " s" : String (v, 2) + " Hz"; };
    auto dbTxt = [] (float v, int) { return (v > 0.05f ? "+" : "") + String (v, 1) + " dB"; };
    auto sTxt = [] (float v, int) { return String (v, 1) + " s"; };

    auto skewed = [] (float lo, float hi, float centre) { NormalisableRange<float> r (lo, hi); r.setSkewForCentre (centre); return r; };
    auto add = [&] (const char* id, const char* name, NormalisableRange<float> r, float def, std::function<String (float, int)> txt)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def,
                                                            AudioParameterFloatAttributes().withStringFromValueFunction (std::move (txt))));
    };
    auto unit = [&] (const char* id, const char* name, float def) { add (id, name, { 0.0f, 1.0f }, def, pct); };
    auto toggle = [&] (const char* id, const char* name, bool def) { p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def)); };
    auto choice = [&] (const char* id, const char* name, StringArray items, int def) { p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def)); };

    // global
    add (ids::input, "Input", { -24.0f, 24.0f }, d.inputDb, dbTxt);
    add (ids::output, "Output", { -24.0f, 24.0f }, d.outputDb, dbTxt);
    toggle (ids::bypass, "Bypass", false);
    choice (ids::routing, "Routing", { "Cho > Dly > Rev", "Cho > Rev > Dly", "Rev > Cho > Dly" }, 0);

    // chorus / flanger
    toggle (ids::choOn, "Cho-Fla On", d.choOn);
    add (ids::choTime, "Cho-Fla Time", skewed (2.5f, 50.0f, 12.0f), d.choTimeMs, msTxt);
    unit (ids::choDepth, "Cho-Fla Depth", d.choDepth);
    add (ids::choRate, "Cho-Fla Rate", skewed (0.066f, 10.0f, 1.0f), d.choRateHz, hzTxt);
    add (ids::choFeedback, "Cho-Fla Feedback", { -0.95f, 0.95f }, d.choFeedback, bipct);
    unit (ids::choMix, "Cho-Fla Mix", d.choMix);
    choice (ids::choWave, "Cho-Fla LFO Wave", { "Sine", "Triangle", "Random" }, 0);

    // delay
    toggle (ids::dlyOn, "Delay On", d.dlyOn);
    add (ids::dlyTime, "Delay Time", skewed (40.0f, 1000.0f, 300.0f), d.dlyTimeMs, msTxt);
    unit (ids::dlyFeedback, "Delay Feedback", d.dlyFeedback);
    unit (ids::dlyMix, "Delay Mix", d.dlyMix);
    choice (ids::dlyMode, "Delay Mode", { "Clean", "BBD", "Tape" }, (int) d.dlyMode);
    toggle (ids::dlyFreeze, "Delay Freeze", false);
    toggle (ids::dlyPingPong, "Delay Ping-Pong", d.dlyPingPong);

    // spring reverb
    toggle (ids::revOn, "Spring On", d.revOn);
    add (ids::revPre, "Spring Pre-Delay", skewed (0.0f, 250.0f, 40.0f), d.revPreDelayMs, msTxt);
    unit (ids::revDepth, "Spring Mod Depth", d.revDepth);
    add (ids::revRate, "Spring Mod Rate", skewed (0.066f, 10.0f, 1.0f), d.revRateHz, hzTxt);
    choice (ids::revWave, "Spring LFO Wave", { "Sine", "Square", "Random" }, 0);
    unit (ids::revAmount, "Spring Amount", d.revAmount);
    add (ids::revDecay, "Spring Decay", skewed (0.4f, 12.0f, 3.0f), d.revDecayS, sTxt);
    unit (ids::revDwell, "Spring Dwell", d.revDwell);
    unit (ids::revDrip, "Spring Drip", d.revDrip);
    unit (ids::revBoing, "Spring Boing", d.revBoing);
    add (ids::revTone, "Spring Tone", { -1.0f, 1.0f }, d.revTone, bipct);
    unit (ids::revWidth, "Spring Width", d.revWidth);
    choice (ids::revTank, "Spring Tank", { "Short", "Medium", "Long" }, (int) d.tank);
    toggle (ids::kick, "Kick Tank", false);

    return { p.begin(), p.end() };
}

MesmerProcessor::MesmerProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MesmerSpring", createLayout())
{
}

bool MesmerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet(), in = layouts.getMainInputChannelSet();
    const bool okOut = out == AudioChannelSet::mono() || out == AudioChannelSet::stereo();
    const bool okIn = in == AudioChannelSet::mono() || in == AudioChannelSet::stereo();
    return okOut && okIn;
}

AudioProcessorParameter* MesmerProcessor::getBypassParameter() const { return apvts.getParameter (ids::bypass); }

void MesmerProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.setParams (readParams());
    engine.prepare (sampleRate, samplesPerBlock);
}

ms::Params MesmerProcessor::readParams() const
{
    auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    auto on = [&] (const char* id) { return v (id) > 0.5f; };
    auto idx = [&] (const char* id) { return (int) std::lround (v (id)); };
    ms::Params p;
    p.inputDb = v (ids::input);
    p.outputDb = v (ids::output);
    p.bypass = on (ids::bypass);
    p.routing = (ms::Routing) idx (ids::routing);

    p.choOn = on (ids::choOn);
    p.choTimeMs = v (ids::choTime);
    p.choDepth = v (ids::choDepth);
    p.choRateHz = v (ids::choRate);
    p.choFeedback = v (ids::choFeedback);
    p.choMix = v (ids::choMix);
    static const ms::LfoShape choWaves[] = { ms::LfoShape::Sine, ms::LfoShape::Triangle, ms::LfoShape::Random };
    p.choWave = choWaves[jlimit (0, 2, idx (ids::choWave))];

    p.dlyOn = on (ids::dlyOn);
    p.dlyTimeMs = v (ids::dlyTime);
    p.dlyFeedback = v (ids::dlyFeedback);
    p.dlyMix = v (ids::dlyMix);
    p.dlyMode = (ms::DelayMode) jlimit (0, 2, idx (ids::dlyMode));
    p.dlyFreeze = on (ids::dlyFreeze);
    p.dlyPingPong = on (ids::dlyPingPong);

    p.revOn = on (ids::revOn);
    p.revPreDelayMs = v (ids::revPre);
    p.revDepth = v (ids::revDepth);
    p.revRateHz = v (ids::revRate);
    static const ms::LfoShape revWaves[] = { ms::LfoShape::Sine, ms::LfoShape::Square, ms::LfoShape::Random };
    p.revWave = revWaves[jlimit (0, 2, idx (ids::revWave))];
    p.revAmount = v (ids::revAmount);
    p.revDecayS = v (ids::revDecay);
    p.revDwell = v (ids::revDwell);
    p.revDrip = v (ids::revDrip);
    p.revBoing = v (ids::revBoing);
    p.revTone = v (ids::revTone);
    p.revWidth = v (ids::revWidth);
    p.tank = (ms::TankType) jlimit (0, 2, idx (ids::revTank));
    return p;
}

void MesmerProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels(), outCh = getTotalNumOutputChannels();
    for (int c = inCh; c < outCh; ++c) buffer.clear (c, 0, n);
    if (inCh == 1 && outCh == 2) buffer.copyFrom (1, 0, buffer, 0, 0, n);   // mono in, stereo out

    // the Kick parameter fires on its rising edge, so it can be automated
    const bool k = apvts.getRawParameterValue (ids::kick)->load() > 0.5f;
    if (k && ! lastKick) engine.kickTank (1.0f);
    lastKick = k;

    engine.setParams (readParams());
    float* l = buffer.getWritePointer (0);
    float* r = outCh > 1 ? buffer.getWritePointer (1) : l;
    engine.process (l, r, n);
}

int MesmerProcessor::getNumPrograms() { return (int) ms::factoryPresets().size(); }

const String MesmerProcessor::getProgramName (int index)
{
    const auto& ps = ms::factoryPresets();
    return isPositiveAndBelow (index, (int) ps.size()) ? String (ps[(size_t) index].name) : String();
}

void MesmerProcessor::setCurrentProgram (int index)
{
    const auto& ps = ms::factoryPresets();
    if (! isPositiveAndBelow (index, (int) ps.size())) return;
    currentProgram = index;
    const auto& p = ps[(size_t) index].p;
    auto set = [this] (const char* id, float value)
    {
        if (auto* prm = apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (value));
    };
    auto waveIndex = [] (ms::LfoShape s, bool rev)
    {
        if (s == ms::LfoShape::Random) return 2.0f;
        if (rev) return s == ms::LfoShape::Square ? 1.0f : 0.0f;
        return s == ms::LfoShape::Triangle ? 1.0f : 0.0f;
    };
    // a preset sets the sound; input/output gain, bypass and freeze stay where the player left them
    set (ids::routing, (float) p.routing);
    set (ids::choOn, p.choOn); set (ids::choTime, p.choTimeMs); set (ids::choDepth, p.choDepth);
    set (ids::choRate, p.choRateHz); set (ids::choFeedback, p.choFeedback); set (ids::choMix, p.choMix);
    set (ids::choWave, waveIndex (p.choWave, false));
    set (ids::dlyOn, p.dlyOn); set (ids::dlyTime, p.dlyTimeMs); set (ids::dlyFeedback, p.dlyFeedback);
    set (ids::dlyMix, p.dlyMix); set (ids::dlyMode, (float) p.dlyMode); set (ids::dlyPingPong, p.dlyPingPong);
    set (ids::revOn, p.revOn); set (ids::revPre, p.revPreDelayMs); set (ids::revDepth, p.revDepth);
    set (ids::revRate, p.revRateHz); set (ids::revWave, waveIndex (p.revWave, true)); set (ids::revAmount, p.revAmount);
    set (ids::revDecay, p.revDecayS); set (ids::revDwell, p.revDwell); set (ids::revDrip, p.revDrip);
    set (ids::revBoing, p.revBoing); set (ids::revTone, p.revTone); set (ids::revWidth, p.revWidth);
    set (ids::revTank, (float) p.tank);
}

void MesmerProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("theme", themeIndex, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void MesmerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = ValueTree::fromXml (*xml);
            currentProgram = tree.getProperty ("program", 0);
            themeIndex = tree.getProperty ("theme", 0);
            apvts.replaceState (tree);
        }
}

AudioProcessorEditor* MesmerProcessor::createEditor() { return new MesmerEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MesmerProcessor(); }
