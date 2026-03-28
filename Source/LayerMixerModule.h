/*
  ==============================================================================

    LayerMixerModule.h
    Created: 28 Mar 2026 5:44:39pm
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class LayerMixerModule : public juce::Component
{
public:
    // Le pasamos el APVTS y un "prefijo" (ej: "L1_") para que este módulo sirva para la capa 1, 2, 3...
    LayerMixerModule(juce::AudioProcessorValueTreeState& apvts, juce::String layerPrefix);
    ~LayerMixerModule() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String prefix;

    // --- LOS 4 KNOBS DE EQ ---
    juce::Slider eqLow, eqMidLow, eqMidHigh, eqHigh;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachLow, attachMidLow, attachMidHigh, attachHigh;

    // --- EL FADER PROFESIONAL ---
    juce::Slider volumeFader;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachVol;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayerMixerModule)
};
