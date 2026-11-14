// Spring tank model.
//
// A real tank is a pair of dispersive transmission lines: a click put in at the driver
// transducer arrives at the pickup as a falling "chirp" (high frequencies travel faster
// than low ones), and the wave keeps bouncing between the two ends. That chirp, repeated
// every transit, is the "drip" of surf guitar; the low, ringing "boing" comes from the
// pickup resonance and from the longest springs.
//
// Each spring here is a feedback loop (after Valimaki, Parker & Abel, "Parametric spring
// reverberation effect", JAES 2010):
//
//   in --(+)--> M stretched allpasses  H(z) = (a + z^-K) / (1 + a z^-K)  --> transit delay --+--> out
//         ^      (dispersion; K is set from the spring's cut-off fc = fs / 2K)                 |
//         +---------------- g * loss low-pass  <---------------------------------------------+
//
// "Drip" sets the allpass coefficient a and the number of sections M (more dispersion =
// longer chirps). The tank holds two sets of three springs of slightly different lengths:
// set A feeds the left output and set B the right, so the Width control can go from the
// mono tank of the hardware (A+B to both sides) to a wide twin tank.
//
// Plain C++17, scalar code only.
#pragma once

#include "Dsp.h"
#include <array>

namespace ms
{
enum class TankType { Short = 0, Medium, Long };

class SpringTank
{
public:
    static constexpr int kSprings = 6;          // 3 per side
    static constexpr int kMaxSections = 60;

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        for (int s = 0; s < kSprings; ++s)
        {
            auto& sp = springs[(size_t) s];
            // per-spring cut-off fc (the frequency above which the chirps stop) sets the stretch K
            const double fc = 4300.0 + 260.0 * (s % 3) + (s >= 3 ? 130.0 : 0.0);
            sp.K = std::max (1, (int) std::lround (fs / (2.0 * fc)));
            for (auto& ap : sp.ap) ap.allocate (sp.K);
            sp.delay.allocate ((int) (0.090 * fs) + 64);  // longest transit (Long tank, mod headroom)
            sp.loss.set (fs, fc * 0.9);
            sp.fb = 0;
        }
        inHp.highpass (fs, 90.0, 0.6);
        inLp.lowpass (fs, 5200.0, 0.6);
        for (int c = 0; c < 2; ++c)
        {
            outHp[c].highpass (fs, 120.0, 0.7);
            outLp[c].lowpass (fs, 5000.0, 0.7);
            dc[c].set (fs, 10.0);
        }
        kickRng = Rng (0x5EED5EEDu);
        kickLp.set (fs, 1800.0);
        configured = false;
        kDecay = std::exp (-1.0 / (0.012 * fs));
        tDecay = std::exp (-1.0 / (0.045 * fs));
        setShape (TankType::Medium, 0.5, 0.5, 0.0, 3.0);
        reset();
    }

    void reset()
    {
        for (auto& sp : springs)
        {
            sp.delay.clear();
            for (auto& ap : sp.ap) ap.clear();
            sp.loss.reset();
            sp.fb = 0;
        }
        inHp.reset(); inLp.reset();
        for (int c = 0; c < 2; ++c) { outHp[c].reset(); outLp[c].reset(); peak[c].reset(); lowSh[c].reset(); highSh[c].reset(); dc[c].reset(); }
        kickEnv = 0; thumpEnv = 0; thumpPhase = 0;
        kickLp.reset();
    }

    // tank: length family. drip 0..1 dispersion. boing 0..1 pickup resonance.
    // tone -1..1 (dark..bright). rt60 in seconds. Cheap to call every block: work is done only on change.
    void setShape (TankType tank, double drip, double boing, double tone, double rt60)
    {
        auto same = [] (double a, double b) { return std::fabs (a - b) < 1e-9; };
        if (configured && tank == lastTank && same (drip, lastDrip) && same (boing, lastBoing) && same (tone, lastTone) && same (rt60, lastRt60))
            return;
        configured = true;
        lastTank = tank; lastDrip = drip; lastBoing = boing; lastTone = tone; lastRt60 = rt60;

        static const double baseMs[kSprings] = { 31.0, 36.5, 41.9, 33.2, 38.7, 44.6 };
        const double scale = tank == TankType::Short ? 0.62 : (tank == TankType::Medium ? 1.0 : 1.55);
        const double a = 0.42 + 0.36 * clampd (drip, 0.0, 1.0);
        const int sections = 18 + (int) std::lround (42.0 * clampd (drip, 0.0, 1.0));

        for (int s = 0; s < kSprings; ++s)
        {
            auto& sp = springs[(size_t) s];
            sp.a = (s % 2 ? -0.015 : 0.015) + a;          // springs are never identical
            if (sections > sp.sections)                     // newly enabled sections start silent
                for (int k = sp.sections; k < sections; ++k) sp.ap[(size_t) k].clear();
            sp.sections = sections;
            // allpass group delay at DC: M * K * (1 - a) / (1 + a); the bulk delay makes up the rest
            const double apDelay = sections * sp.K * (1.0 - sp.a) / (1.0 + sp.a);
            const double total = baseMs[s] * scale * 0.001 * fs;
            sp.len = std::max (16.0, total - apDelay);
            const double loopSec = total / fs;
            sp.g = std::pow (10.0, -3.0 * loopSec / std::max (0.2, rt60));
            // darker tone loses more highs per round trip
            sp.loss.set (fs, 2200.0 + 2600.0 * (0.5 + 0.5 * tone) - 120.0 * (s % 3));
        }

        for (int c = 0; c < 2; ++c)
        {
            peak[c].peak (fs, 1650.0, 1.4, 1.0 + 7.0 * boing);       // pickup resonance: the "boing" colour
            lowSh[c].lowShelf (fs, 220.0, 2.0 + 4.0 * boing - 3.0 * std::max (0.0, tone));
            highSh[c].highShelf (fs, 3000.0, 6.0 * tone);
            outLp[c].lowpass (fs, 3600.0 + 2400.0 * (0.5 + 0.5 * tone), 0.7);
        }
    }

    // "Kick" the tank (what happens when the hardware gets bumped). strength 0..1.
    void kick (double strength)
    {
        kickEnv = std::max (kickEnv, 0.9 * strength);
        thumpEnv = std::max (thumpEnv, 1.2 * strength);
        thumpPhase = 0;
    }

    // x: mono drive signal (already through the dwell stage). lenMod: -1..1 slow modulation of
    // the transit time ("swirl"). width 0 = mono tank, 1 = A left / B right.
    inline void process (double x, double lenMod, double width, double& outL, double& outR)
    {
        double in = inLp.process (inHp.process (x));

        if (kickEnv > 1e-5 || thumpEnv > 1e-5)
        {
            in += kickLp.process (kickRng.bi()) * kickEnv * 2.5;
            thumpPhase += 2.0 * kPi * 55.0 / fs;
            in += std::sin (thumpPhase) * thumpEnv * 0.8;
            kickEnv *= kDecay;
            thumpEnv *= tDecay;
        }

        double side[2] = { 0, 0 };
        for (int s = 0; s < kSprings; ++s)
        {
            auto& sp = springs[(size_t) s];
            double v = (s % 3 == 1 ? -in : in) + sp.fb;
            for (int k = 0; k < sp.sections; ++k) v = sp.ap[(size_t) k].process (v, sp.a);
            sp.delay.push (v);
            const double d = sp.len * (1.0 + 0.004 * lenMod * (s % 2 ? -1.0 : 1.0));
            const double y = sp.delay.tapFrac (std::max (1.0, d));
            double f = sp.loss.process (y) * sp.g;
            f = f > 1.5 || f < -1.5 ? std::tanh (f / 1.5) * 1.5 : f;   // safety: a real spring can't run away
            sp.fb = f;
            side[s / 3] += y;
        }

        const double mid = 0.5 * (side[0] + side[1]);
        const double w = clampd (width, 0.0, 1.0);
        double o[2] = { mid + w * (side[0] - mid), mid + w * (side[1] - mid) };
        for (int c = 0; c < 2; ++c)
        {
            double v = outHp[c].process (o[c]);
            v = outLp[c].process (v);
            v = peak[c].process (v);
            v = lowSh[c].process (v);
            v = highSh[c].process (v);
            o[c] = dc[c].process (v) * 0.42;
        }
        outL = o[0]; outR = o[1];
    }

private:
    struct StretchedAllpass
    {
        // y[n] = a x[n] + x[n-K] - a y[n-K]
        std::vector<double> xb, yb; int w = 0, n = 2;
        void allocate (int K) { n = K; xb.assign ((size_t) n, 0.0); yb.assign ((size_t) n, 0.0); w = 0; }
        void clear() { std::fill (xb.begin(), xb.end(), 0.0); std::fill (yb.begin(), yb.end(), 0.0); w = 0; }
        inline double process (double x, double a)
        {
            // buffer of length K: the slot about to be overwritten holds x[n-K], y[n-K]
            const double y = a * x + xb[(size_t) w] - a * yb[(size_t) w];
            xb[(size_t) w] = x; yb[(size_t) w] = y;
            if (++w >= n) w = 0;
            return y;
        }
    };

    struct Spring
    {
        std::array<StretchedAllpass, kMaxSections> ap;
        DelayLine delay;
        OnePoleLP loss;
        int K = 5, sections = 30;
        double len = 1000, a = 0.6, g = 0.8, fb = 0;
    };


    double fs = 48000;
    std::array<Spring, kSprings> springs;
    Biquad inHp, inLp, outHp[2], outLp[2], peak[2], lowSh[2], highSh[2];
    DcBlocker dc[2];

    Rng kickRng;
    OnePoleLP kickLp;
    double kickEnv = 0, thumpEnv = 0, thumpPhase = 0, kDecay = 0.99, tDecay = 0.99;

    bool configured = false;
    TankType lastTank = TankType::Medium;
    double lastDrip = -1, lastBoing = -1, lastTone = -9, lastRt60 = -1;
};
} // namespace ms
