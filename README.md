# Mesmer Spring (working name)

A spring reverb time-effects box (VST3 + Standalone). It has three sections: Cho-Fla, Delay
(Clean/BBD/Tape with Freeze) and a modelled spring tank with Drip, Boing, Dwell, Kick and a
modulated pre-delay. The delay can sync to the host tempo, Freeze can latch or hold, and user
presets can be saved. See `docs/DESIGN.md`, and `docs/shots/` for the five UI themes.

## Build

```
cmake -B build -DCMAKE_BUILD_TYPE=Release            # fetches JUCE 8.0.4
# or: cmake -B build -DJUCE_DIR=/path/to/JUCE
cmake --build build --config Release
ctest --test-dir build                                # engine tests
./build/mesmer_demo docs/demo.wav                     # render the audio demo
```

macOS builds target macOS 11+ universal (Intel + Apple Silicon). No AVX2 or other `-march`
flags are used, and the DSP is scalar C++. Windows builds use the static runtime. On Linux,
install the JUCE dependencies first (ALSA, X11, freetype and fontconfig dev packages).

Layout: `Source/engine` is plain C++ (spring tank, effects, presets), tested in `tests/`.
`Source/plugin` is the JUCE wrapper and UI. `tools/` holds developer tools: the demo renderer,
and `-DMESMER_BUILD_SNAPSHOT=ON` for the theme screenshots.
