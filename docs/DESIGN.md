# Mesmer Spring: design plan (v0.1)

**Source idea:** "Dredbox hypnosis / spring reverb" from Adam's list. I read this as the
Dreadbox Hypnosis, a desktop time-effects box with three sections: Cho-Fla, Delay and a real
3-spring tank. Mesmer Spring keeps that layout and puts the spring at the centre. It is an
original design. It uses no Dreadbox names, artwork or circuits.

## What it mirrors from the hardware

| Hardware section | Hardware controls | Mesmer Spring |
|---|---|---|
| Cho-Fla | Time (2.5–50 ms), Depth, Rate (10 Hz–15 s), Feedback, LFO wave Sine/Tri/Random, On | The same controls, plus a Mix knob. Feedback is bipolar for negative flanging. Depth 0 gives a static comb. Higher Depth narrows Time's range, as on the hardware. The two channels move in opposite directions. |
| Delay | Time (40–450 ms), Mix, Feedback (max = infinite), Freeze, mode Clean/BBD/Tape, ping-pong in stereo | The same controls. Time is extended to 1000 ms and glides when changed. Ping-Pong is a switch. Freeze holds the loop with no loss. Turning Delay off while Freeze is on keeps the loop latched, as on the hardware. |
| Spring reverb | Pre-Delay, Depth (pre-delay modulation), Rate, Amount (dry/wet), LFO wave Sine/Square/Random, On, mono tank | The same controls, plus the tank itself: **Decay, Dwell** (drive into the tank), **Drip** (dispersion), **Boing** (pickup resonance), **Tone**, **Width** (0 = mono, like the hardware; 1 = wide twin tank), **Tank** size Short/Medium/Long, and a **Kick** button that shakes the tank. |
| Global | Input level with peak lamp, Output, Bypass | The same, plus an **Order** switch: Cho>Dly>Rev, Cho>Rev>Dly or Rev>Cho>Dly. |

## Spring model

Each tank has 6 springs, 3 for each side. Every spring is a feedback loop made of stretched
allpass sections, which give the falling chirp (the "drip"), followed by a transit delay and a
loss low-pass. This follows Välimäki, Parker & Abel (2010). Drip sets the allpass coefficient
and the number of sections, from 18 to 60. Before the tank, a tanh "Dwell" driver stands in
for the tube that drives a real tank. After it, a pickup resonance peak gives the "boing". The
LFO modulates the pre-delay, as on the hardware, and also moves the spring transit time a
little, which gives the swirl. Kick feeds a noise burst and a low thump into the tank.

## Scope of v0.1

- JUCE 8.0.4, CMake, VST3 and Standalone. Plain scalar C++17 with no `-march` flags, so it runs on macOS 11 Intel (no AVX2 needed). macOS builds are universal. Windows builds use the static CRT.
- 32 factory presets in 5 categories, and 5 UI themes: Trance, Cream Tolex, Midnight Chrome, Mint Surf and Oxblood.
- Engine tests at 44.1, 48 and 96 kHz cover decay, dispersion, bypass, delay timing, freeze in all 3 modes, Kick, width, and every preset staying bounded. The worst case uses about 6% of one core.

## Open questions / next steps

1. Freeze is a latching switch here. On the hardware it only holds while pressed. A momentary option could be added.
2. Saving user presets. For now the host saves the plugin state.
3. Tempo sync for delay time and LFO rates (the hardware has none).
4. Installers, signing and a GitHub repo, when Adam asks.
