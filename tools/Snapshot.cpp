// Renders the editor in every theme to PNG files (used for docs and visual checks).
// Usage: mesmer_snapshot <outdir> [presetIndex]
#include "../Source/plugin/PluginProcessor.h"
#include "../Source/plugin/PluginEditor.h"

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
    return 0;
}
