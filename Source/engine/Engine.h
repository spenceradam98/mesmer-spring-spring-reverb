// Mesmer Spring engine: a spring-reverb-centred time-effects box. Plain C++17, no JUCE
// dependency, unit-testable.
//
// The panel follows the three-section layout of desktop "time effects" boxes in the spirit of
// the Dreadbox Hypnosis: a modulated short delay (chorus / flanger), a delay with Clean, BBD
// and Tape voices plus Freeze, and a spring reverb whose pre-delay is modulated by its own LFO.
// On top of that the spring section exposes the tank itself (decay, drip, boing, dwell, tone,
// tank size, width) and a Kick button that shakes it.
//
// Signal flow (order selectable):
//
//   in L/R -> Input gain -> [Cho-Fla] -> [Delay] -> [Spring] -> Output gain -> out L/R
//   Spring:  mono sum -> pre-delay (LFO modulated) -> Dwell drive -> tank (6 springs) -> Amount mix
//
// Every section has an on/off switch; Bypass crossfades the whole box to dry.
#pragma once

#include "Dsp.h"
#include "SpringTank.h"
#include <atomic>

namespace ms
{
enum class DelayMode { Clean = 0, Bbd, Tape };
enum class Routing { ChoDlyRev = 0, ChoRevDly, RevChoDly };

// Tempo-sync note values, in quarter-note beats.
struct Division { const char* name; double beats; };
inline constexpr Division kDivisions[] = {
    { "1/32", 0.125 }, { "1/16T", 1.0 / 6.0 }, { "1/16", 0.25 }, { "1/16D", 0.375 }, { "1/8T", 1.0 / 3.0 },
    { "1/8", 0.5 }, { "1/8D", 0.75 }, { "1/4T", 2.0 / 3.0 }, { "1/4", 1.0 }, { "1/4D", 1.5 }, { "1/2", 2.0 },
};
inline constexpr int kNumDivisions = (int) (sizeof (kDivisions) / sizeof (kDivisions[0]));


struct Params
{
    // ---- global ----
    float inputDb = 0.0f;           // -24..+24
    float outputDb = 0.0f;          // -24..+24
    bool bypass = false;
    Routing routing = Routing::ChoDlyRev;

    // ---- chorus / flanger ----
    bool choOn = false;
    float choTimeMs = 12.0f;        // 2.5..50 ms (short = flanger, long = chorus)
    float choDepth = 0.3f;          // 0..1 LFO sweep; 0 = static comb
    float choRateHz = 0.5f;         // 0.066..10 Hz
    float choFeedback = 0.0f;       // -0.95..0.95
    float choMix = 0.5f;            // 0..1
    LfoShape choWave = LfoShape::Sine;   // Sine, Triangle, Random

    // ---- delay ----
    bool dlyOn = false;
    float dlyTimeMs = 320.0f;       // 40..1000 ms
    float dlyFeedback = 0.35f;      // 0..1 (1 = repeats forever)
    float dlyMix = 0.3f;            // 0..1
    DelayMode dlyMode = DelayMode::Bbd;
    bool dlyFreeze = false;
    bool dlyPingPong = true;
    bool dlySync = false;           // lock the delay time to the host tempo
    int dlyDivision = 6;            // index into kDivisions (default 1/8 dotted)
    double bpm = 120.0;             // host tempo (set by the plugin each block)

    // ---- spring reverb ----
    bool revOn = true;
    float revPreDelayMs = 10.0f;    // 0..250 ms
    float revDepth = 0.15f;         // 0..1 pre-delay (and spring) modulation
    float revRateHz = 0.4f;         // 0.066..10 Hz
    LfoShape revWave = LfoShape::Sine;   // Sine, Square, Random
    float revAmount = 0.35f;        // 0..1 dry/wet
    float revDecayS = 2.8f;         // 0.4..12 s
    float revDwell = 0.45f;         // 0..1 drive into the tank
    float revDrip = 0.55f;          // 0..1 dispersion
    float revBoing = 0.4f;          // 0..1 pickup resonance
    float revTone = 0.0f;           // -1..1
    float revWidth = 0.0f;          // 0 = mono tank (as hardware) .. 1 = wide twin tank
    TankType tank = TankType::Medium;
};

// Delay time in ms for the current settings (synced or free), clamped to the delay's range.
double delayTimeMs (const Params& p);

class Engine
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void setParams (const Params& p) { params = p; }
    const Params& getParams() const { return params; }

    // Stereo in place. For a mono host pass the same pointer for both channels.
    void process (float* left, float* right, int numSamples);

    // shake the spring tank (thread-safe trigger, applied at the next block)
    void kickTank (float strength = 1.0f) { kickRequest.store (strength); }

    // ---- UI state (written by the audio thread) ----
    float getInputPeak() const { return inPeakOut.load (std::memory_order_relaxed); }
    float getReverbLevel() const { return revLevelOut.load (std::memory_order_relaxed); }
    float getChoLfo() const { return choLfoOut.load (std::memory_order_relaxed); }
    float getRevLfo() const { return revLfoOut.load (std::memory_order_relaxed); }

private:
    void processCho (double& l, double& r);
    void processDelay (double& l, double& r);
    void processReverb (double& l, double& r);

    Params params;
    double fs = 48000;

    // smoothed gains
    OnePoleLP sIn, sOut, sBypass, sChoOn, sDlyOn, sRevOn, sChoMix, sDlyMix, sRevAmt, sDlyTime, sChoTime, sPre, sDwell, sFreeze;

    // chorus / flanger
    DelayLine choLine[2];
    Lfo choLfo;
    double choFb[2] = { 0, 0 };
    DcBlocker choDc[2];

    // delay
    DelayLine dlyLine[2];
    double dlyFbSig[2] = { 0, 0 };
    Biquad bbdPre[2], bbdPost[2], tapeLp[2], tapeBump[2], loopHp[2];
    Drift tapeWow, tapeFlutter;
    Rng clockNoise { 0xBEEF01u };
    DelayMode lastMode = DelayMode::Clean;

    // reverb
    DelayLine preLine;
    Lfo revLfo;
    SpringTank tank;
    Biquad dwellHp;

    std::atomic<float> kickRequest { 0.0f };
    std::atomic<float> inPeakOut { 0 }, revLevelOut { 0 }, choLfoOut { 0 }, revLfoOut { 0 };
    double inPeak = 0, revLevel = 0, peakFall = 0.999, levelA = 0.999;
    double choLfoNow = 0, revLfoNow = 0;
};
} // namespace ms
