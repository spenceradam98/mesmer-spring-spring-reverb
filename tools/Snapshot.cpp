// Renders the editor in every theme to PNG files (used for docs and visual checks).
// Usage: mesmer_snapshot <outdir> [presetIndex]
#include "../Source/plugin/PluginProcessor.h"
#include "../Source/plugin/PluginEditor.h"
#include <cstdio>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File outDir (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    outDir.createDirectory();
    MesmerProcessor proc;
    proc.setCurrentProgram (argc > 2 ? juce::String (argv[2]).getIntValue() : 9);
    proc.prepareToPlay (48000.0, 512);
    for (int t = 0; t < (int) themes().size(); ++t)
    {
        proc.setThemeIndex (t);
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditorIfNeeded());
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        auto f = outDir.getChildFile ("theme-" + juce::String (t) + "-" + juce::String (themes()[(size_t) t].name).replaceCharacter (' ', '-').toLowerCase() + ".png");
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat().writeImageToStream (img, os);
    }

    // user preset round trip: save, change a parameter, load, and check it came back
    {
        auto f = juce::File::createTempFile (".mspreset");
        auto* drip = proc.apvts.getParameter ("revDrip");
        drip->setValueNotifyingHost (0.25f);
        const bool saved = proc.saveUserPreset (f);
        drip->setValueNotifyingHost (0.9f);
        const bool loaded = proc.loadUserPreset (f);
        const bool ok = saved && loaded && std::abs (drip->getValue() - 0.25f) < 1e-4f;
        std::printf ("user preset round trip: %s\n", ok ? "ok" : "FAILED");
        f.deleteFile();
        if (! ok) return 1;
    }
    return 0;
}
