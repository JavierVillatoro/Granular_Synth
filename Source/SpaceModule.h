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
    SpaceModule(juce::AudioProcessorValueTreeState& apvts);
    ~SpaceModule() override;
    void resized() override;

private:
    juce::Slider sizeKnob, fbackKnob, mixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sizeAttach, fbackAttach, mixAttach;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpaceModule)
};
