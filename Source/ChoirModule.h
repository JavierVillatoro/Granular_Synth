/*
  ==============================================================================

    ChoirModule.h
    Created: 11 Apr 2026 10:10:19pm
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class ChoirModule : public juce::Component
{
public:
    ChoirModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix);
    ~ChoirModule() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void setLayer(int layerIndex);

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String layerPrefix;

    // HALO
    juce::Slider haloPitch, haloShimmer, haloColor, haloMix;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachHPitch, attachHShimmer, attachHColor, attachHMix;

    // ENSEMBLE
    juce::Slider ensRate, ensDepth, ensWidth, ensMix;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachERate, attachEDepth, attachEWidth, attachEMix;

    // Colores dinámicos
    juce::Colour currentLayerColor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChoirModule)
};
