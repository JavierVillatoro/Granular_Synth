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
    // El constructor necesita el cerebro (apvts) para conectar los cables
    ScanModule(juce::AudioProcessorValueTreeState& apvts);
    ~ScanModule() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // ¡Nos traemos el knob de Posición aquí!
    juce::Slider positionKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> positionAttachment;

    juce::Slider scanSpeedKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> scanSpeedAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScanModule)
};
