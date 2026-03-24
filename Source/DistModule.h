/*
  ==============================================================================

    DistModule.h
    Created: 24 Mar 2026 1:50:24am
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class DistModule : public juce::Component
{
public:
    DistModule(juce::AudioProcessorValueTreeState& apvts);
    ~DistModule() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;

    // --- Controles Visuales ---
    juce::Slider driveKnob;
    juce::Slider mixKnob;
    juce::ComboBox typeCombo;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistModule)
};
