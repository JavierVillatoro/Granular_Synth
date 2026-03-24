/*
  ==============================================================================

    MasterModule.h
    Created: 23 Mar 2026 10:44:40pm
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h" // Necesario para acceder a las variables del procesador

class MasterModule : public juce::Component, public juce::Timer
{
public:
    MasterModule(juce::AudioProcessorValueTreeState& apvts, Granular_SynthAudioProcessor& p);
    ~MasterModule() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override; // El reloj para animar las barras

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    Granular_SynthAudioProcessor& audioProcessor;

    // --- Controles Visuales ---
    juce::Slider volKnob;
    juce::Slider limitKnob;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> limitAttach;

    // --- Variables para la animación suave del Vúmetro ---
    float currentLevelL = -60.0f;
    float currentLevelR = -60.0f;

    juce::Rectangle<int> meterArea; // La caja donde dibujaremos las barras

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterModule)
};
