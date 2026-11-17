// Engine tests: run with ctest or directly. Exit code 0 = all passed.
#include "Engine.h"
#include "Presets.h"
#include <cstdio>
#include <cmath>
#include <memory>
#include <vector>

using namespace ms;

static int failures = 0;
#define CHECK(cond, ...) do { if (! (cond)) { ++failures; std::printf ("FAIL %s:%d: ", __FILE__, __LINE__); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

struct Stereo { std::vector<float> l, r; };

static Stereo run (Engine& e, Stereo s, int block = 256)
{
    for (size_t i = 0; i < s.l.size(); i += (size_t) block)
    {
        const int n = (int) std::min<size_t> ((size_t) block, s.l.size() - i);
        e.process (s.l.data() + i, s.r.data() + i, n);
    }
    return s;
}

static Stereo impulse (double fs, double seconds, double at = 0.05)
{
    Stereo s { std::vector<float> ((size_t) (seconds * fs), 0.0f), {} };
    for (int i = 0; i < 24; ++i) s.l[(size_t) (at * fs) + (size_t) i] = 0.5f * (float) std::sin (kPi * i / 24.0);
    s.r = s.l;
    return s;
}

static double rms (const std::vector<float>& v, double fs, double from, double to)
{
    const size_t a = (size_t) (from * fs), b = std::min (v.size(), (size_t) (to * fs));
    double acc = 0; for (size_t i = a; i < b; ++i) acc += (double) v[i] * v[i];
    return b > a ? std::sqrt (acc / (double) (b - a)) : 0.0;
}

static bool finite (const Stereo& s, float limit = 4.0f)
{
    for (size_t i = 0; i < s.l.size(); ++i)
        if (! std::isfinite (s.l[i]) || ! std::isfinite (s.r[i]) || std::fabs (s.l[i]) > limit || std::fabs (s.r[i]) > limit) return false;
    return true;
}

static std::unique_ptr<Engine> make (const Params& p, double fs)
{
    auto e = std::make_unique<Engine>();
    e->setParams (p); e->prepare (fs, 512);
    return e;
}

static Params wetSpring()
{
    Params p; p.revOn = true; p.revAmount = 1.0f; p.choOn = false; p.dlyOn = false; p.revDepth = 0.0f; p.revPreDelayMs = 0.0f;
    return p;
}

int main()
{
    for (double fs : { 44100.0, 48000.0, 96000.0 })
    {
        // 1. spring impulse response: there is a tail, and it decays
        {
            auto p = wetSpring(); p.revDecayS = 2.0f;
            auto e = make (p, fs);
            auto out = run (*e, impulse (fs, 5.0));
            const double early = rms (out.l, fs, 0.06, 0.6), late = rms (out.l, fs, 4.0, 5.0);
            CHECK (finite (out), "spring output not finite at %g", fs);
            CHECK (early > 1e-3, "spring tail too quiet at %g: %g", fs, early);
            CHECK (late < early * 0.05, "spring does not decay at %g: early %g late %g", fs, early, late);
        }
        // 2. longer decay setting rings longer
        {
            auto p = wetSpring(); p.revDecayS = 1.0f;
            auto a = run (*make (p, fs), impulse (fs, 3.0));
            p.revDecayS = 6.0f;
            auto b = run (*make (p, fs), impulse (fs, 3.0));
            CHECK (rms (b.l, fs, 1.5, 2.5) > 4.0 * rms (a.l, fs, 1.5, 2.5), "decay control has no effect at %g", fs);
        }
        // 3. dispersion: the tank's first arrival is a falling chirp, so high frequencies arrive
        //    before low ones. Compare the arrival of energy above/below 1 kHz with a crude split.
        {
            auto p = wetSpring(); p.revDrip = 1.0f; p.revDecayS = 0.6f; p.revBoing = 0.0f;
            auto out = run (*make (p, fs), impulse (fs, 0.4, 0.0));
            // one-pole split
            std::vector<double> lo (out.l.size()), hi (out.l.size());
            const double a = std::exp (-2.0 * kPi * 700.0 / fs);
            double z = 0;
            for (size_t i = 0; i < out.l.size(); ++i) { z = out.l[i] + a * (z - out.l[i]); lo[i] = z; hi[i] = out.l[i] - z; }
            auto centroid = [&] (const std::vector<double>& v)
            {
                double num = 0, den = 0;
                for (size_t i = 0; i < (size_t) (0.12 * fs); ++i) { num += (double) i * v[i] * v[i]; den += v[i] * v[i]; }
                return den > 0 ? num / den / fs : 0.0;
            };
            CHECK (centroid (hi) < centroid (lo), "no dispersion at %g: hi %g lo %g", fs, centroid (hi), centroid (lo));
        }
        // 4. bypass returns the dry input exactly
        {
            Params p; p.bypass = true; p.choOn = p.dlyOn = true;
            auto e = make (p, fs);
            Stereo in { std::vector<float> (4096), std::vector<float> (4096) };
            for (size_t i = 0; i < in.l.size(); ++i) { in.l[i] = (float) std::sin (0.01 * i); in.r[i] = (float) std::cos (0.013 * i); }
            auto out = run (*e, in);
            double err = 0; for (size_t i = 0; i < in.l.size(); ++i) err = std::max (err, (double) std::fabs (out.l[i] - in.l[i]) + std::fabs (out.r[i] - in.r[i]));
            CHECK (err < 1e-5, "bypass not transparent at %g: %g", fs, err);
        }
        // 5. delay: the first repeat lands at the set time
        {
            Params p; p.revOn = false; p.dlyOn = true; p.dlyMode = DelayMode::Clean; p.dlyTimeMs = 250.0f; p.dlyFeedback = 0.0f;
            p.dlyMix = 1.0f; p.dlyPingPong = false;
            auto out = run (*make (p, fs), impulse (fs, 0.6, 0.05));
            size_t best = 0; float bv = 0;
            for (size_t i = (size_t) (0.1 * fs); i < out.l.size(); ++i) if (std::fabs (out.l[i]) > bv) { bv = std::fabs (out.l[i]); best = i; }
            const double ms = (best / fs - 0.05) * 1000.0;
            CHECK (std::fabs (ms - 250.0) < 2.0, "delay repeat at %g ms, expected 250 (fs %g)", ms, fs);
        }
        // 6. freeze holds the loop with no decay and ignores new input
        for (auto mode : { DelayMode::Clean, DelayMode::Bbd, DelayMode::Tape })
        {
            Params p; p.revOn = false; p.dlyOn = true; p.dlyMode = mode; p.dlyTimeMs = 200.0f; p.dlyFeedback = 0.5f; p.dlyMix = 1.0f;
            auto e = make (p, fs);
            Stereo noise { std::vector<float> ((size_t) (0.3 * fs)), {} };
            Rng rng (7); for (auto& v : noise.l) v = (float) (0.3 * rng.bi());
            noise.r = noise.l;
            run (*e, noise);
            p.dlyFreeze = true; e->setParams (p);
            auto held = run (*e, Stereo { std::vector<float> ((size_t) (4.0 * fs), 0.0f), std::vector<float> ((size_t) (4.0 * fs), 0.0f) });
            const double a = rms (held.l, fs, 0.5, 1.0), b = rms (held.l, fs, 3.5, 4.0);
            CHECK (a > 1e-3 && b > a * 0.7, "freeze does not hold (mode %d, fs %g): %g -> %g", (int) mode, fs, a, b);
            CHECK (finite (held, 1.5f), "freeze output not bounded (mode %d)", (int) mode);
        }
        // 7. kick makes sound from silence
        {
            auto p = wetSpring();
            auto e = make (p, fs);
            e->kickTank (1.0f);
            auto out = run (*e, Stereo { std::vector<float> ((size_t) fs), std::vector<float> ((size_t) fs) });
            CHECK (rms (out.l, fs, 0.0, 0.5) > 1e-3, "kick is silent at %g", fs);
            CHECK (finite (out), "kick output not finite");
        }
        // 8. width: 0 = identical channels from mono input, 1 = decorrelated
        {
            auto p = wetSpring(); p.revWidth = 0.0f;
            auto m = run (*make (p, fs), impulse (fs, 1.0));
            double d = 0; for (size_t i = 0; i < m.l.size(); ++i) d = std::max (d, (double) std::fabs (m.l[i] - m.r[i]));
            CHECK (d < 1e-6, "width 0 should be mono, diff %g", d);
            p.revWidth = 1.0f;
            auto w = run (*make (p, fs), impulse (fs, 1.0));
            d = 0; for (size_t i = 0; i < w.l.size(); ++i) d = std::max (d, (double) std::fabs (w.l[i] - w.r[i]));
            CHECK (d > 1e-3, "width 1 should be stereo, diff %g", d);
        }
    }

    // 9. every preset, worst-case loud input, stays bounded and finite; odd block sizes too
    {
        const double fs = 48000.0;
        for (const auto& pr : factoryPresets())
        {
            auto e = make (pr.p, fs);
            Stereo s { std::vector<float> ((size_t) (3.0 * fs)), {} };
            Rng rng (99);
            for (size_t i = 0; i < s.l.size(); ++i) s.l[i] = i < (size_t) (1.5 * fs) ? (float) (0.9 * rng.bi()) : 0.0f;
            s.r = s.l;
            auto out = run (*e, s, 37);
            float mx = 0; for (size_t i = 0; i < out.l.size(); ++i) mx = std::max (mx, std::max (std::fabs (out.l[i]), std::fabs (out.r[i])));
            CHECK (finite (out, 3.0f), "preset '%s' output not finite/bounded (peak %.2f)", pr.name, mx);
        }
        CHECK (factoryPresets().size() >= 30, "expected at least 30 presets, have %d", (int) factoryPresets().size());
    }

    // 11. tempo sync: note values follow the host tempo and always fit the delay line
    {
        Params p; p.dlySync = true; p.bpm = 120.0;
        p.dlyDivision = 8;  CHECK (std::fabs (delayTimeMs (p) - 500.0) < 1e-9, "1/4 at 120 bpm should be 500 ms, got %g", delayTimeMs (p));
        p.dlyDivision = 6;  CHECK (std::fabs (delayTimeMs (p) - 375.0) < 1e-9, "1/8D at 120 bpm should be 375 ms, got %g", delayTimeMs (p));
        p.bpm = 40.0; p.dlyDivision = 10;
        CHECK (std::fabs (delayTimeMs (p) - 750.0) < 1e-9, "1/2 at 40 bpm should halve to 750 ms, got %g", delayTimeMs (p));
        p.dlySync = false; p.dlyTimeMs = 2000.0f;
        CHECK (std::fabs (delayTimeMs (p) - 1000.0) < 1e-9, "free time should clamp to 1000 ms");
    }

    // 10. mono processing (same pointer for both channels) works
    {
        auto e = make (wetSpring(), 48000.0);
        auto s = impulse (48000.0, 1.0);
        for (size_t i = 0; i < s.l.size(); i += 128) e->process (s.l.data() + i, s.l.data() + i, (int) std::min<size_t> (128, s.l.size() - i));
        CHECK (rms (s.l, 48000.0, 0.06, 0.8) > 1e-3, "mono processing silent");
    }

    if (failures == 0) std::printf ("All engine tests passed.\n");
    return failures == 0 ? 0 : 1;
}
