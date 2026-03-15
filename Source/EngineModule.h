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
    EngineModule(juce::AudioProcessorValueTreeState& apvts);
    ~EngineModule() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Slider grainSizeKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainSizeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineModule)
};
