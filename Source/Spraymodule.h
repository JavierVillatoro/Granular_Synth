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
    // Usaremos los nombres exactos que pide tu compilador
    juce::Slider posSprayKnob;
    juce::Slider panSprayKnob;
    juce::Slider pitchSprayKnob;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> posSprayAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchSprayAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panSprayAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SprayModule)
};
