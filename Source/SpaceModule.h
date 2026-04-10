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
    void paint(juce::Graphics& g) override;

    // NUEVO: Función para cambiar la capa en tiempo real
    void setLayer(int layerIndex);

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String layerPrefix;

    // NUEVO: Knobs para las 3 voces/monjes (V1, V2, V3) en el Pad
    juce::Slider v1LevelKnob, v2LevelKnob, v3LevelKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> v1LevelAttach, v2LevelAttach, v3LevelAttach;

    // Knobs principales del módulo Space
    juce::Slider sizeKnob, fbackKnob, mixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sizeAttach, fbackAttach, mixAttach;

    // NUEVO: Labels para los nombres (debajo de los knobs)
    juce::Label sizeLabel, fbackLabel, mixLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpaceModule)
};
