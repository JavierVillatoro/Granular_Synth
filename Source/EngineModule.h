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
    // AADIDO: El parmetro juce::String prefix
    //EngineModule(juce::AudioProcessorValueTreeState& apvtsuce::String prefix);
    EngineModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix);
    void resized() override;

private:
    juce::String layerPrefix; // <-- Guarda la identidad de la capa

 
    juce::Slider sizeKnob;
    juce::Slider densityKnob;
    juce::Slider shapeKnob;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sizeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> densityAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> shapeAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineModule)
};
