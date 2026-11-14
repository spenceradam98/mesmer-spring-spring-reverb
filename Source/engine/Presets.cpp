#include "Presets.h"

namespace ms
{
namespace
{
// Builders keep the table readable: start from a dry spring and change what matters.
Params spring (float amount, float decay, float drip, float boing, float dwell, float tone, TankType t = TankType::Medium)
{
    Params p;
    p.revOn = true; p.revAmount = amount; p.revDecayS = decay; p.revDrip = drip; p.revBoing = boing;
    p.revDwell = dwell; p.revTone = tone; p.tank = t;
    p.revDepth = 0.0f; p.revPreDelayMs = 0.0f;
    return p;
}
Params& mod (Params& p, float depth, float rate, LfoShape w = LfoShape::Sine, float pre = 0.0f)
{
    p.revDepth = depth; p.revRateHz = rate; p.revWave = w; p.revPreDelayMs = pre; return p;
}
Params& cho (Params& p, float timeMs, float depth, float rate, float fb, float mix, LfoShape w = LfoShape::Sine)
{
    p.choOn = true; p.choTimeMs = timeMs; p.choDepth = depth; p.choRateHz = rate; p.choFeedback = fb; p.choMix = mix; p.choWave = w;
    return p;
}
Params& dly (Params& p, float timeMs, float fb, float mix, DelayMode m, bool pingPong = true)
{
    p.dlyOn = true; p.dlyTimeMs = timeMs; p.dlyFeedback = fb; p.dlyMix = mix; p.dlyMode = m; p.dlyPingPong = pingPong;
    return p;
}

std::vector<Preset> build()
{
    std::vector<Preset> v;
    auto add = [&v] (const char* name, const char* cat, Params p) { v.push_back ({ name, cat, p }); };

    // ---------------- Classic spring
    add ("Init Spring", "Classic", spring (0.35f, 2.8f, 0.55f, 0.4f, 0.45f, 0.0f));
    add ("Surf Drip", "Classic", spring (0.55f, 3.2f, 0.9f, 0.75f, 0.7f, 0.25f, TankType::Long));
    add ("Amp Tank", "Classic", spring (0.28f, 2.2f, 0.5f, 0.35f, 0.4f, -0.1f));
    add ("Short Box Tank", "Classic", spring (0.32f, 1.6f, 0.45f, 0.3f, 0.45f, 0.1f, TankType::Short));
    add ("Long Six Spring", "Classic", [] { auto p = spring (0.45f, 4.5f, 0.7f, 0.5f, 0.5f, 0.0f, TankType::Long); p.revWidth = 0.6f; return p; }());
    add ("Dub Splash", "Classic", [] { auto p = spring (0.6f, 3.8f, 0.8f, 0.9f, 0.85f, -0.2f, TankType::Long); return dly (p, 375.0f, 0.55f, 0.35f, DelayMode::Tape, false); }());
    add ("Dark Studio Plate-ish", "Classic", [] { auto p = spring (0.4f, 3.0f, 0.3f, 0.15f, 0.35f, -0.6f); p.revWidth = 1.0f; return mod (p, 0.08f, 0.3f, LfoShape::Sine, 18.0f); }());
    add ("Bright Twang", "Classic", spring (0.38f, 2.0f, 0.65f, 0.6f, 0.55f, 0.7f, TankType::Short));
    add ("Crunchy Dwell", "Classic", spring (0.5f, 2.6f, 0.6f, 0.6f, 1.0f, 0.2f));

    // ---------------- Hypnotic (the modulated, swirling side)
    add ("Hypnotic Swirl", "Hypnotic", [] { auto p = spring (0.5f, 5.0f, 0.6f, 0.4f, 0.5f, 0.0f, TankType::Long); p.revWidth = 0.8f; return mod (p, 0.55f, 0.35f, LfoShape::Sine, 30.0f); }());
    add ("Pendulum", "Hypnotic", [] { auto p = spring (0.45f, 4.0f, 0.5f, 0.4f, 0.45f, -0.1f); p.revWidth = 0.5f; mod (p, 0.4f, 0.15f, LfoShape::Sine, 60.0f); return cho (p, 18.0f, 0.35f, 0.12f, 0.0f, 0.45f); }());
    add ("Seasick Tank", "Hypnotic", [] { auto p = spring (0.5f, 4.5f, 0.7f, 0.5f, 0.5f, -0.2f, TankType::Long); return mod (p, 0.9f, 0.9f, LfoShape::Random, 25.0f); }());
    add ("Square Wobble", "Hypnotic", [] { auto p = spring (0.45f, 3.5f, 0.6f, 0.5f, 0.5f, 0.1f); return mod (p, 0.5f, 3.0f, LfoShape::Square, 40.0f); }());
    add ("Trance State", "Hypnotic", [] { auto p = spring (0.6f, 8.0f, 0.55f, 0.3f, 0.4f, -0.3f, TankType::Long); p.revWidth = 1.0f; mod (p, 0.35f, 0.08f, LfoShape::Sine, 80.0f); return dly (p, 600.0f, 0.6f, 0.3f, DelayMode::Bbd); }());
    add ("Spiral Staircase", "Hypnotic", [] { auto p = spring (0.5f, 6.0f, 0.75f, 0.45f, 0.5f, 0.0f); p.revWidth = 0.7f; mod (p, 0.3f, 0.25f); return dly (p, 220.0f, 0.7f, 0.35f, DelayMode::Clean); }());

    // ---------------- Chorus / flanger + spring
    add ("Chorus Drip", "Modulation", [] { auto p = spring (0.4f, 3.0f, 0.75f, 0.5f, 0.5f, 0.1f); return cho (p, 20.0f, 0.3f, 0.6f, 0.1f, 0.5f); }());
    add ("Jet Flange Tank", "Modulation", [] { auto p = spring (0.35f, 2.8f, 0.6f, 0.5f, 0.5f, 0.2f); return cho (p, 3.5f, 0.45f, 0.15f, 0.75f, 0.5f, LfoShape::Triangle); }());
    add ("Static Comb Spring", "Modulation", [] { auto p = spring (0.4f, 3.0f, 0.6f, 0.6f, 0.5f, 0.0f); return cho (p, 6.0f, 0.0f, 0.5f, 0.6f, 0.5f); }());
    add ("Random Ensemble", "Modulation", [] { auto p = spring (0.35f, 3.4f, 0.5f, 0.3f, 0.4f, -0.1f); p.revWidth = 0.6f; return cho (p, 26.0f, 0.4f, 1.2f, 0.0f, 0.5f, LfoShape::Random); }());
    add ("Negative Flange", "Modulation", [] { auto p = spring (0.3f, 2.5f, 0.6f, 0.5f, 0.5f, 0.0f); return cho (p, 2.8f, 0.5f, 0.25f, -0.8f, 0.5f, LfoShape::Triangle); }());
    add ("Rotor Wash", "Modulation", [] { auto p = spring (0.45f, 3.5f, 0.55f, 0.4f, 0.45f, 0.0f); p.revWidth = 1.0f; return cho (p, 9.0f, 0.35f, 5.5f, 0.15f, 0.5f); }());

    // ---------------- Delay + spring
    add ("Tape Echo Spring", "Delay", [] { auto p = spring (0.4f, 3.0f, 0.7f, 0.6f, 0.6f, -0.1f); return dly (p, 280.0f, 0.5f, 0.4f, DelayMode::Tape, false); }());
    add ("BBD Slapback", "Delay", [] { auto p = spring (0.3f, 2.0f, 0.6f, 0.5f, 0.5f, 0.0f, TankType::Short); return dly (p, 95.0f, 0.15f, 0.45f, DelayMode::Bbd, false); }());
    add ("Ping Pong Drip", "Delay", [] { auto p = spring (0.4f, 3.2f, 0.85f, 0.6f, 0.6f, 0.2f); return dly (p, 340.0f, 0.5f, 0.35f, DelayMode::Clean); }());
    add ("Runaway Tape", "Delay", [] { auto p = spring (0.45f, 4.0f, 0.7f, 0.7f, 0.7f, -0.2f, TankType::Long); return dly (p, 410.0f, 0.95f, 0.45f, DelayMode::Tape, false); }());
    add ("Dark BBD Wash", "Delay", [] { auto p = spring (0.55f, 6.0f, 0.5f, 0.3f, 0.45f, -0.5f, TankType::Long); p.revWidth = 0.8f; return dly (p, 520.0f, 0.65f, 0.35f, DelayMode::Bbd); }());

    // ---------------- Sound design
    add ("Infinite Shimmer Tank", "Sound Design", [] { auto p = spring (0.65f, 12.0f, 0.6f, 0.4f, 0.5f, 0.2f, TankType::Long); p.revWidth = 1.0f; mod (p, 0.3f, 0.2f); return cho (p, 14.0f, 0.3f, 0.3f, 0.3f, 0.45f); }());
    add ("Broken Amp", "Sound Design", [] { auto p = spring (0.6f, 2.4f, 0.95f, 1.0f, 1.0f, 0.5f, TankType::Short); return mod (p, 0.2f, 6.0f, LfoShape::Square); }());
    add ("Metal Box", "Sound Design", [] { auto p = spring (0.55f, 1.4f, 1.0f, 0.9f, 0.7f, 0.8f, TankType::Short); return cho (p, 2.5f, 0.0f, 0.5f, 0.85f, 0.5f); }());
    add ("Frozen Loop Bed", "Sound Design", [] { auto p = spring (0.5f, 7.0f, 0.5f, 0.3f, 0.4f, -0.3f, TankType::Long); p.revWidth = 1.0f; return dly (p, 900.0f, 0.85f, 0.4f, DelayMode::Bbd); }());
    add ("Full Wet Spring", "Sound Design", spring (1.0f, 3.5f, 0.7f, 0.5f, 0.55f, 0.0f));
    add ("Underwater Tank", "Sound Design", [] { auto p = spring (0.7f, 5.0f, 0.6f, 0.3f, 0.45f, -1.0f, TankType::Long); p.revWidth = 0.6f; mod (p, 0.7f, 0.45f, LfoShape::Random, 20.0f); return cho (p, 30.0f, 0.5f, 0.3f, 0.2f, 0.5f, LfoShape::Random); }());

    return v;
}
} // namespace

const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> presets = build();
    return presets;
}
} // namespace ms
