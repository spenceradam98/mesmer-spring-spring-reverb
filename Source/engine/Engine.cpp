#include "Engine.h"

namespace ms
{
namespace
{
// transparent below 0.9, then a smooth knee that never exceeds 1.0
inline double softLimit (double x)
{
    const double ax = std::fabs (x);
    if (ax <= 0.9) return x;
    const double y = 0.9 + 0.1 * std::tanh ((ax - 0.9) / 0.1);
    return x < 0 ? -y : y;
}
inline double lerp (double a, double b, double t) { return a + (b - a) * t; }
} // namespace

void Engine::prepare (double sampleRate, int)
{
    fs = sampleRate;

    for (auto* s : { &sIn, &sOut, &sBypass, &sChoOn, &sDlyOn, &sRevOn, &sChoMix, &sDlyMix, &sRevAmt, &sDwell, &sFreeze })
        s->setTime (fs, 0.02);
    sDlyTime.setTime (fs, 0.12);   // delay time glides like a real BBD/tape clock change
    sChoTime.setTime (fs, 0.05);
    sPre.setTime (fs, 0.08);

    for (int c = 0; c < 2; ++c)
    {
        choLine[c].allocate ((int) (0.06 * fs) + 8);
        choDc[c].set (fs, 12.0);
        dlyLine[c].allocate ((int) (1.1 * fs) + 8);
        bbdPre[c].lowpass (fs, 3900.0, 0.707);
        bbdPost[c].lowpass (fs, 3300.0, 0.8);
        tapeLp[c].lowpass (fs, 5600.0, 0.6);
        tapeBump[c].peak (fs, 110.0, 1.0, 2.0);
        loopHp[c].highpass (fs, 45.0, 0.6);
    }
    tapeWow.set (fs, 0.9, 0x1A2B3Cu);
    tapeFlutter.set (fs, 9.0, 0x4D5E6Fu);
    choLfo.prepare (fs, 0x11u);
    revLfo.prepare (fs, 0x22u);

    preLine.allocate ((int) (0.30 * fs) + 8);
    dwellHp.highpass (fs, 110.0, 0.6);
    tank.prepare (fs);

    peakFall = std::exp (-1.0 / (0.3 * fs));
    levelA = std::exp (-1.0 / (0.08 * fs));
    reset();
}

void Engine::reset()
{
    const auto& p = params;
    sIn.reset (dbToGain (p.inputDb)); sOut.reset (dbToGain (p.outputDb)); sBypass.reset (p.bypass ? 1 : 0);
    sChoOn.reset (p.choOn ? 1 : 0); sDlyOn.reset (p.dlyOn ? 1 : 0); sRevOn.reset (p.revOn ? 1 : 0);
    sChoMix.reset (p.choMix); sDlyMix.reset (p.dlyMix); sRevAmt.reset (p.revAmount);
    sDlyTime.reset (p.dlyTimeMs); sChoTime.reset (p.choTimeMs); sPre.reset (p.revPreDelayMs);
    sDwell.reset (p.revDwell); sFreeze.reset (p.dlyFreeze ? 1 : 0);
    for (int c = 0; c < 2; ++c)
    {
        choLine[c].clear(); dlyLine[c].clear(); choDc[c].reset();
        bbdPre[c].reset(); bbdPost[c].reset(); tapeLp[c].reset(); tapeBump[c].reset(); loopHp[c].reset();
        choFb[c] = dlyFbSig[c] = 0;
    }
    choLfo.reset (0.0); revLfo.reset (0.25);
    preLine.clear(); dwellHp.reset();
    tank.reset();
    inPeak = revLevel = 0;
}

// ---------------------------------------------------------------- chorus / flanger
void Engine::processCho (double& l, double& r)
{
    const auto& p = params;
    const double on = sChoOn.process (p.choOn ? 1.0 : 0.0);
    const double mix = sChoMix.process (p.choMix);
    const double time = sChoTime.process (p.choTimeMs);
    choLfoNow = choLfo.process (p.choWave);
    if (on < 1e-4) { choLine[0].push (l); choLine[1].push (r); choFb[0] = choFb[1] = 0; return; }

    // As on the hardware, Depth narrows the influence of Time: at full depth the LFO sweeps the
    // whole 2.5..50 ms range on its own. Squared so small depths give flanger-sized sweeps.
    const double depth = clampd (p.choDepth, 0.0, 1.0), dd = depth * depth;
    const double centre = 2.5 + (clampd (time, 2.5, 50.0) - 2.5) * (1.0 - dd);
    double in[2] = { l, r };
    for (int c = 0; c < 2; ++c)
    {
        const double lfo = c == 0 ? choLfoNow : -choLfoNow;     // stereo: the channels move in reverse
        const double ms = clampd (centre + dd * 47.5 * (0.5 + 0.5 * lfo) * (centre < 26 ? 1.0 : -1.0), 1.0, 52.0);
        const double wet = choLine[c].tapFrac (ms * 0.001 * fs);
        choLine[c].push (in[c] + softLimit (choDc[c].process (wet) * p.choFeedback));
        const double fx = in[c] * (1.0 - mix) + wet * mix;
        in[c] = lerp (in[c], fx, on);
    }
    l = in[0]; r = in[1];
}

// ---------------------------------------------------------------- delay
void Engine::processDelay (double& l, double& r)
{
    const auto& p = params;
    const double on = sDlyOn.process (p.dlyOn ? 1.0 : 0.0);
    const double mix = sDlyMix.process (p.dlyMix);
    const double frz = sFreeze.process (p.dlyFreeze ? 1.0 : 0.0);
    double timeMs = sDlyTime.process (clampd (p.dlyTimeMs, 40.0, 1000.0));

    if (p.dlyMode == DelayMode::Tape)
        timeMs *= 1.0 + (1.0 - frz) * (0.0022 * tapeWow.process() + 0.0007 * tapeFlutter.process());

    // when the section is off, the line keeps playing out its repeats but gets no new input,
    // so switching it off leaves a natural tail (like a hardware trails mode)
    const double inGain = (1.0 - frz) * on;
    const double fb = lerp (clampd (p.dlyFeedback, 0.0, 1.0) * 1.03, 1.0, frz);

    const double d = timeMs * 0.001 * fs;
    const double y[2] = { dlyLine[0].tapFrac (d), dlyLine[1].tapFrac (d) };

    double loop[2];
    for (int c = 0; c < 2; ++c)
    {
        double v = y[c];
        switch (p.dlyMode)
        {
            case DelayMode::Clean: v = softLimit (v); break;
            case DelayMode::Bbd:
                v = bbdPost[c].process (v);
                v = std::tanh (v * 1.25) / 1.25 + clockNoise.bi() * 2.0e-5;
                break;
            case DelayMode::Tape:
                v = tapeBump[c].process (tapeLp[c].process (v));
                v = std::tanh (v * 1.6) / 1.6;
                break;
        }
        v = loopHp[c].process (v);
        loop[c] = lerp (v, y[c], frz);   // frozen: hold the loop exactly, no further losses
    }

    double inL = l * inGain, inR = r * inGain;
    if (p.dlyMode == DelayMode::Bbd) { inL = bbdPre[0].process (inL); inR = bbdPre[1].process (inR); }

    if (p.dlyPingPong)
    {
        dlyLine[0].push (softLimit (0.5 * (inL + inR) + fb * loop[1]));
        dlyLine[1].push (softLimit (fb * loop[0]));
    }
    else
    {
        dlyLine[0].push (softLimit (inL + fb * loop[0]));
        dlyLine[1].push (softLimit (inR + fb * loop[1]));
    }

    // the wet output stays audible while repeats ring out after the section is switched off
    // (switching Delay off while Freeze is held latches the frozen loop, as on the hardware)
    l += y[0] * mix;
    r += y[1] * mix;
}

// ---------------------------------------------------------------- spring reverb
void Engine::processReverb (double& l, double& r)
{
    const auto& p = params;
    const double on = sRevOn.process (p.revOn ? 1.0 : 0.0);
    const double amt = sRevAmt.process (p.revAmount);
    const double pre = sPre.process (clampd (p.revPreDelayMs, 0.0, 250.0));
    const double dwell = sDwell.process (p.revDwell);
    revLfoNow = revLfo.process (p.revWave);

    // the tank keeps ringing out when switched off; it just stops being fed
    const double mono = 0.5 * (l + r) * on;
    preLine.push (mono);
    const double depth = clampd (p.revDepth, 0.0, 1.0);
    const double preSamples = (pre + depth * 9.0 * (0.5 + 0.5 * revLfoNow)) * 0.001 * fs;
    double x = preLine.tapFrac (std::max (0.0, preSamples));

    // Dwell: the tube driver into the tank. Low = clean and quiet, high = loud and crunchy.
    // Level-compensated so the knob mostly changes the drive, not the loudness.
    const double g = dbToGain (-6.0 + 30.0 * dwell);
    x = std::tanh (dwellHp.process (x) * g) / std::pow (g, 0.85);

    double wl = 0, wr = 0;
    tank.process (x, revLfoNow * depth, p.revWidth, wl, wr);

    const double dryG = std::min (1.0, 2.0 * (1.0 - amt));
    const double wetG = std::min (1.0, 2.0 * amt) * 1.5;
    const double outL = l * dryG + wl * wetG, outR = r * dryG + wr * wetG;
    // dry stays untouched when the section is off; wet tails remain
    l = lerp (l + wl * wetG, outL, on);
    r = lerp (r + wr * wetG, outR, on);
    const double lev = std::fabs (wl) + std::fabs (wr);
    revLevel = lev + levelA * (revLevel - lev);
}

// ---------------------------------------------------------------- block
void Engine::process (float* left, float* right, int n)
{
    const auto& p = params;
    if (const float k = kickRequest.exchange (0.0f); k > 0.0f) tank.kick (k);

    tank.setShape (p.tank, p.revDrip, p.revBoing, p.revTone, p.revDecayS);
    choLfo.setRate (fs, p.choRateHz);
    revLfo.setRate (fs, p.revRateHz);

    const bool stereo = left != right;
    const double inT = dbToGain (p.inputDb), outT = dbToGain (p.outputDb), byT = p.bypass ? 1.0 : 0.0;

    for (int i = 0; i < n; ++i)
    {
        const double dryL = left[i], dryR = stereo ? right[i] : left[i];
        const double gi = sIn.process (inT);
        double l = dryL * gi, r = dryR * gi;
        const double pk = std::max (std::fabs (l), std::fabs (r));
        inPeak = pk > inPeak ? pk : inPeak * peakFall;

        switch (p.routing)
        {
            case Routing::ChoDlyRev: processCho (l, r); processDelay (l, r); processReverb (l, r); break;
            case Routing::ChoRevDly: processCho (l, r); processReverb (l, r); processDelay (l, r); break;
            case Routing::RevChoDly: processReverb (l, r); processCho (l, r); processDelay (l, r); break;
        }

        const double go = sOut.process (outT);
        l *= go; r *= go;
        const double b = sBypass.process (byT);
        l = lerp (l, dryL, b);
        r = lerp (r, dryR, b);

        if (stereo) { left[i] = (float) l; right[i] = (float) r; }
        else left[i] = (float) (0.5 * (l + r));
    }

    inPeakOut.store ((float) inPeak, std::memory_order_relaxed);
    revLevelOut.store ((float) revLevel, std::memory_order_relaxed);
    choLfoOut.store ((float) choLfoNow, std::memory_order_relaxed);
    revLfoOut.store ((float) revLfoNow, std::memory_order_relaxed);
}
} // namespace ms
