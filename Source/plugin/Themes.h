// UI colour themes. Each theme is a full palette; the editor and LookAndFeel read only from it.
#pragma once

#include <juce_graphics/juce_graphics.h>
#include <array>

struct Theme
{
    const char* name;
    juce::Colour bgTop, bgBottom;      // window gradient
    juce::Colour panel, panelEdge;     // section panels
    juce::Colour ink, inkDim;          // text
    juce::Colour accent[3];            // per section: cho-fla, delay, spring
    juce::Colour knobBody, knobCap, knobPointer;
    juce::Colour track;                // knob arc background
    juce::Colour window;               // tank window background
    juce::Colour coil;                 // spring coils in the tank window
    juce::Colour ledOff;
    bool spiral;                       // draw the rotating spiral behind the logo
};

inline const std::array<Theme, 5>& themes()
{
    using C = juce::Colour;
    static const std::array<Theme, 5> t { {
        { "Trance",
          C (0xff1b1030), C (0xff07040f), C (0xff241640), C (0xff4a3378),
          C (0xfff1e9ff), C (0xffa898c8), { C (0xff4de1ff), C (0xffff9f43), C (0xffff4fd8) },
          C (0xff2c1f4a), C (0xff120b22), C (0xfff1e9ff), C (0xff3a2a5e), C (0xff0b0618), C (0xffc9b8ff), C (0xff3b2d55), true },
        { "Cream Tolex",
          C (0xffe9dfc8), C (0xffcfc2a3), C (0xfff4ecd9), C (0xffa8987a),
          C (0xff2b2118), C (0xff6e604c), { C (0xff2f7d74), C (0xffd2691e), C (0xffb3282d) },
          C (0xff1f1a15), C (0xff3a322a), C (0xfff4ecd9), C (0xffcdbf9f), C (0xff2b2118), C (0xffd8cfbc), C (0xffb5a789), false },
        { "Midnight Chrome",
          C (0xff1c1f24), C (0xff08090b), C (0xff23272d), C (0xff4a5058),
          C (0xffe6e8eb), C (0xff8d939b), { C (0xff7fd1ff), C (0xffffb347), C (0xffffd36b) },
          C (0xffb9bec5), C (0xff5c6168), C (0xff111316), C (0xff34393f), C (0xff050607), C (0xffd9dde2), C (0xff3a3f45), false },
        { "Mint Surf",
          C (0xff9fd8c4), C (0xff6fb7a1), C (0xffd8f1e6), C (0xff5c9c88),
          C (0xff12352c), C (0xff3c6a5c), { C (0xff1f6fb2), C (0xffe0703a), C (0xffd8405a) },
          C (0xfff7f4ea), C (0xffdcd6c4), C (0xff12352c), C (0xffa9d6c5), C (0xff12352c), C (0xffd8f1e6), C (0xff8fbfae), false },
        { "Oxblood",
          C (0xff3a0f14), C (0xff150407), C (0xff4a141b), C (0xff7a2b33),
          C (0xfff6e3c4), C (0xffc79e88), { C (0xffe8c26a), C (0xfff08a4b), C (0xffffd98a) },
          C (0xff1a0a0c), C (0xff2e1115), C (0xffe8c26a), C (0xff5c1e25), C (0xff0f0305), C (0xffe8c26a), C (0xff5c1e25), true },
    } };
    return t;
}
