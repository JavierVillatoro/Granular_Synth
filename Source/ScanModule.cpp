/*
  ==============================================================================

    ScanModule.cpp
    Created: 15 Mar 2026 7:33:44pm
    Author:  franc

  ==============================================================================
*/

#include "ScanModule.h"

// AÑADIDO: Recibimos y guardamos el prefijo
ScanModule::ScanModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : layerPrefix(prefix)
{
    auto setupKnob = [this](juce::Slider& k) {
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(k);
        };

    setupKnob(posKnob);
    setupKnob(speedKnob);
    setupKnob(dirKnob);

    // USAMOS EL PREFIJO
    posAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, layerPrefix + "POSITION", posKnob);
    speedAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, layerPrefix + "SCAN_SPEED", speedKnob);
    dirAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, layerPrefix + "SCAN_MODE", dirKnob);
}

ScanModule::~ScanModule() {}

void ScanModule::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRect(getLocalBounds(), 1);
}

void ScanModule::resized()
{
    auto area = getLocalBounds().reduced(10);
    int widthUnit = area.getWidth() / 3;

    posKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    speedKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    dirKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
}
