/*
  ==============================================================================

    Spraymodule.h
    Created: 16 Mar 2026 12:13:23am
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class SprayModule : public juce::Component
{
public:
    SprayModule(juce::AudioProcessorValueTreeState& apvts);
    void resized() override;

private:
    juce::Slider sprayPosKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sprayPosAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SprayModule)
};
