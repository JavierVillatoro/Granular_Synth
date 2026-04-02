/*
  ==============================================================================

    EngineModule.h
    Created: 15 Mar 2026 7:58:18pm
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class EngineModule : public juce::Component
{
public:
    EngineModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix);
    void resized() override;

    // NUEVO: La funcin que llama el Editor al hacer clic
    void setLayer(int layerIndex);

private:
    juce::AudioProcessorValueTreeState& apvtsRef; // Guardamos el APVTS para reconectar
    juce::String layerPrefix; // <-- Guarda la identidad de la capa

    juce::Slider sizeKnob;
    juce::Slider densityKnob;
    juce::Slider shapeKnob;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sizeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> densityAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> shapeAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineModule)
};
