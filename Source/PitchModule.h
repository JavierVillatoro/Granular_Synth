/*
  ==============================================================================

    PitchModule.h
    Created: 18 Mar 2026 12:07:55am
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class PitchModule : public juce::Component
{
public:
    // AADIDO: Prefijo de capa
    //PitchModule(juce::AudioProcessorValueTreeState& apvts juce::String prefix);
    PitchModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix);
    ~PitchModule() override;
    void resized() override;

private:
    juce::String layerPrefix; // <-- Identidad


    juce::Slider transKnob;
    juce::Slider fineKnob;
    juce::Slider scaleKnob;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> transAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fineAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> scaleAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchModule)
};
