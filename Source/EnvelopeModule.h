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
    EnvelopeModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix);
    ~EnvelopeModule() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    // NUEVO: Funciara cambiar de capa visual y lgicamente
    void setLayer(int layerIndex);

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String layerPrefix;
    int currentLayer = 1; // Para el color

    int activeNode = -1;
    float startParamX = 0.0f;
    float startParamY = 0.0f;

    void drawEnvelope(juce::Graphics& g, juce::Rectangle<int> bounds, juce::String name,
        float a, float d, float s, float r, juce::Colour envColor);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeModule)
};
