/*
  ==============================================================================

    FxFormantModule.h
    Created: 10 Apr 2026 12:18:11pm
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class XYPad : public juce::Component
{
public:
    XYPad(juce::AudioProcessorValueTreeState& apvts, juce::String paramX, juce::String paramY);
    ~XYPad() override;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    void updateParameters(juce::String newParamX, juce::String newParamY);
    void setColors(juce::Colour newBaseColor);

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String currentParamX;
    juce::String currentParamY;
    juce::Colour baseColor = juce::Colours::cyan;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYPad)
};

class FxFormantModule : public juce::Component, public juce::Timer
{
public:
    // Aquí está el int vId que le faltaba al compilador
    FxFormantModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix, int vId);
    ~FxFormantModule() override;

    void setLayer(int layerIndex);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String layerPrefix;
    int currentLayer = 1;

    // Aquí está la variable que decía que "no estaba definida"
    int voiceId;

    juce::Colour currentBaseColor = juce::Colours::cyan;

    XYPad xyPad;
    juce::Slider mixSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxFormantModule)
};
