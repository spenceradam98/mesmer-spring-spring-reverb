// Renders a short demo: a plucked-string phrase and a drum hit through several factory presets.
// Usage: mesmer_demo out.wav   (writes 16-bit stereo 48 kHz)
#include "Engine.h"
#include "Presets.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

using namespace ms;

static void pluck (std::vector<float>& buf, double fs, size_t at, double hz, double amp)
{
    // Karplus-Strong string
    const int n = (int) (fs / hz);
    std::vector<double> line ((size_t) n);
    Rng rng ((uint32_t) (at + 1));
    for (auto& v : line) v = rng.bi();
    double last = 0;
    for (size_t i = 0; i < (size_t) (fs * 1.6) && at + i < buf.size(); ++i)
    {
        const size_t k = i % (size_t) n;
        const double y = 0.5 * (line[k] + last) * 0.996;
        last = line[k]; line[k] = y;
        buf[at + i] += (float) (y * amp);
    }
}

int main (int argc, char** argv)
{
    const double fs = 48000.0;
    const char* names[] = { "Surf Drip", "Hypnotic Swirl", "Tape Echo Spring", "Jet Flange Tank", "Broken Amp" };
    std::vector<float> outL, outR;
    for (const char* nm : names)
    {
        Params p;
        for (auto& pr : factoryPresets()) if (std::string (pr.name) == nm) p = pr.p;
        Engine e; e.setParams (p); e.prepare (fs, 256);
        const size_t len = (size_t) (fs * 6.0);
        std::vector<float> l (len, 0.0f);
        const double notes[] = { 196.0, 246.9, 293.7, 392.0, 329.6, 293.7 };
        for (int i = 0; i < 6; ++i) pluck (l, fs, (size_t) (fs * (0.1 + 0.32 * i)), notes[i], 0.35);
        // a snare-ish hit to show the "crash"
        Rng rng (3);
        for (size_t i = 0; i < (size_t) (0.15 * fs); ++i) l[(size_t) (fs * 2.6) + i] += (float) (0.5 * rng.bi() * std::exp (-(double) i / (0.03 * fs)));
        std::vector<float> r = l;
        for (size_t i = 0; i < len; i += 256) e.process (l.data() + i, r.data() + i, (int) std::min<size_t> (256, len - i));
        outL.insert (outL.end(), l.begin(), l.end());
        outR.insert (outR.end(), r.begin(), r.end());
    }
    std::ofstream f (argc > 1 ? argv[1] : "demo.wav", std::ios::binary);
    const uint32_t frames = (uint32_t) outL.size(), dataBytes = frames * 4, rate = 48000, byteRate = rate * 4;
    const uint16_t fmt = 1, ch = 2, align = 4, bits = 16; const uint32_t fmtLen = 16, riffLen = 36 + dataBytes;
    f.write ("RIFF", 4); f.write ((const char*) &riffLen, 4); f.write ("WAVEfmt ", 8); f.write ((const char*) &fmtLen, 4);
    f.write ((const char*) &fmt, 2); f.write ((const char*) &ch, 2); f.write ((const char*) &rate, 4); f.write ((const char*) &byteRate, 4);
    f.write ((const char*) &align, 2); f.write ((const char*) &bits, 2); f.write ("data", 4); f.write ((const char*) &dataBytes, 4);
    for (uint32_t i = 0; i < frames; ++i)
        for (float v : { outL[i], outR[i] }) { const int16_t s = (int16_t) std::lround (std::max (-1.0f, std::min (1.0f, v * 0.8f)) * 32767.0f); f.write ((const char*) &s, 2); }
    std::printf ("wrote %u frames\n", frames);
    return 0;
}
