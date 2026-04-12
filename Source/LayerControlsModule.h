/*
  ==============================================================================

    LayerControlsModule.h

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class LayerControlsModule : public juce::Component
{
public:
    LayerControlsModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix);
    ~LayerControlsModule() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramPrefix;

    juce::TextButton playButton;
    juce::TextButton midiButton;
    juce::TextButton holdButton;
    //juce::TextButton muteButton;

    juce::Slider panSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAttach;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> midiAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> holdAttach;
    //std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayerControlsModule)
};
