/*
  ==============================================================================

    FilterModule.h
    Created: 18 Mar 2026 9:10:29pm
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class FilterModule : public juce::Component
{
public:
    FilterModule(juce::AudioProcessorValueTreeState& apvts);
    ~FilterModule() override;

    void resized() override;

private:
    // Los 3 knobs del filtro analógico
    juce::Slider lpfKnob;
    juce::Slider resKnob;
    juce::Slider hpfKnob;

    // Los cables para conectarlos al cerebro
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lpfAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> resAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hpfAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterModule)
};
