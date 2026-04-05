/*
  ==============================================================================

    ScanModule.h
    Created: 15 Mar 2026 7:33:44pm
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class ScanModule : public juce::Component
{
public:
    ScanModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix);
    ~ScanModule() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // NUEVA FUNCIÓN PARA CAMBIAR DE CAPA
    void setLayer(int layerIndex);

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String layerPrefix;

    juce::Slider posKnob;
    juce::Slider speedKnob;
    juce::Slider dirKnob;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> posAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> speedAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dirAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScanModule)
};
