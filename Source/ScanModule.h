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
    // --- LOS 3 BOTONES ---
    juce::Slider posKnob;
    juce::Slider speedKnob;
    juce::Slider dirKnob;

    // --- LOS 3 CABLES --- (Nombres corregidos para que coincidan con tu .cpp)
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> posAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> speedAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dirAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScanModule)
};
