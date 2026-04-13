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
    LayerMixerModule(juce::AudioProcessorValueTreeState& apvts, juce::String layerPrefix);
    ~LayerMixerModule() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Cambia colores y parámetros al momento
    void setLayer(int layerIndex);

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String prefix;
    int currentLayer = 1;

    // 4 knobs de eq
    juce::Slider eqLow, eqMidLow, eqMidHigh, eqHigh;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachLow, attachMidLow, attachMidHigh, attachHigh;

    // Fadder
    juce::Slider volumeFader;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachVol;

    // --- NUEVO: Traje del Fader ---
    std::unique_ptr<juce::LookAndFeel> faderStyle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayerMixerModule)
};
