/*
  ==============================================================================

    LayerControlsModule.h
    Created: 26 Mar 2026 2:14:20am
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class LayerControlsModule : public juce::Component
{
public:
    // Le pasamos el APVTS y un prefijo (ej: "L1_")
    LayerControlsModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix);
    ~LayerControlsModule() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramPrefix; // Aquí guardaremos "L1_" o "L2_"

    juce::TextButton playButton;
    juce::TextButton midiButton;
    juce::TextButton holdButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> midiAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> holdAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayerControlsModule)
};
