/*
  ==============================================================================

    EnvelopeModule.h
    Created: 18 Mar 2026 10:57:07pm
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class EnvelopeModule : public juce::Component, public juce::Timer
{
public:
    EnvelopeModule(juce::AudioProcessorValueTreeState& apvts);
    ~EnvelopeModule() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;

    // Función interna para dibujar cada envolvente de forma elegante
    void drawEnvelope(juce::Graphics& g, juce::Rectangle<int> bounds, juce::String name,
        float a, float d, float s, float r);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeModule)
};
