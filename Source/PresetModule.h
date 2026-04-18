/*
  ==============================================================================

    PresetModule.h
    Created: 18 Apr 2026 1:10:21am
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class PresetModule : public juce::Component
{
public:
    PresetModule(Granular_SynthAudioProcessor& p);
    ~PresetModule() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Función para que el Editor le avise cuando el usuario cambia de capa
    void setLayer(int layer);

private:
    Granular_SynthAudioProcessor& audioProcessor;

    juce::TextButton presetButtons[16];
    juce::TextButton saveButton{ "SAVE" };
    juce::TextButton loadButton{ "LOAD" };
    juce::TextButton initButton{ "INIT" };

    int currentPresetIndex = -1;
    int loadedPresetIndex = -1;
    int activeLayer = 1;

    void updatePresetButtonColors();
    bool presetHasData[16] = { false };

    void mouseDown(const juce::MouseEvent& event) override;

    int lastClickedIndex = -1;
    juce::uint32 lastClickTime = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetModule)
};
