/*
  ==============================================================================

    BpmModule.h
    Created: 25 Mar 2026 12:13:37am
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class BpmModule : public juce::Component
{
public:
    BpmModule(juce::AudioProcessorValueTreeState& apvts);
    ~BpmModule() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;

    juce::Slider bpmKnob;
    juce::TextButton syncButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bpmAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BpmModule)
};
