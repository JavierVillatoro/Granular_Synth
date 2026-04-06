/*
  ==============================================================================

    ScanModule.cpp
    Created: 15 Mar 2026 7:33:44pm
    Author:  franc

  ==============================================================================
*/

#include "ScanModule.h"

ScanModule::ScanModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : apvtsRef(apvts), layerPrefix(prefix)
{
    auto setupKnob = [this](juce::Slider& k) {
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(k);
        };

    setupKnob(posKnob);
    setupKnob(speedKnob);
    setupKnob(dirKnob);

    setLayer(1); // Arrancamos en Capa 1
}

ScanModule::~ScanModule() {}

void ScanModule::setLayer(int layerIndex)
{
    layerPrefix = (layerIndex == 1) ? "L1_" : "L2_";

    juce::Colour layerColor = (layerIndex == 1) ? juce::Colours::cyan : juce::Colours::magenta;
    juce::Colour dotColor = (layerIndex == 1) ? juce::Colours::white : juce::Colours::pink;

    posKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);
    speedKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);
    dirKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);

    posKnob.setColour(juce::Slider::thumbColourId, dotColor);
    speedKnob.setColour(juce::Slider::thumbColourId, dotColor);
    dirKnob.setColour(juce::Slider::thumbColourId, dotColor);

    posAttach.reset();
    speedAttach.reset();
    dirAttach.reset();

    posAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "POSITION", posKnob);
    speedAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "SCAN_SPEED", speedKnob);
    dirAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "SCAN_MODE", dirKnob);
}

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
