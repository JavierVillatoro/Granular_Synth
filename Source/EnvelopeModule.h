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

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;

    // Mouse memory
    int activeNode = -1; // 0=AmpA, 1=AmpD/S, 2=AmpR,  3=Env2A, 4=Env2D/S, 5=Env2R
    float startParamX = 0.0f;
    float startParamY = 0.0f;

    // Funcin interna para dibujar cada envolvente de forma elegante
    void drawEnvelope(juce::Graphics& g, juce::Rectangle<int> bounds, juce::String name,
        float a, float d, float s, float r, juce::Colour envColor);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeModule)
};
