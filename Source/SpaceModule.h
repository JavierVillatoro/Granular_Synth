/*
  ==============================================================================

    SpaceModule.h
    Created: 19 Mar 2026 3:04:12am
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class SpaceModule : public juce::Component
{
public:
    SpaceModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix);
    ~SpaceModule() override;
    void resized() override;

    // NUEVO: Función para cambiar la capa en tiempo real
    void setLayer(int layerIndex);

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String layerPrefix;

    juce::Slider sizeKnob, fbackKnob, mixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sizeAttach, fbackAttach, mixAttach;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpaceModule)
};
