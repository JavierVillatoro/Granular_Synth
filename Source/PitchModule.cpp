/*
  ==============================================================================

    PitchModule.cpp
    Created: 18 Mar 2026 12:07:55am
    Author:  franc

  ==============================================================================
*/

#include "PitchModule.h"

PitchModule::PitchModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : apvtsRef(apvts), layerPrefix(prefix)
{
    auto setupKnob = [this](juce::Slider& k) {
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(k);
        };

    setupKnob(transKnob);
    setupKnob(fineKnob);
    setupKnob(scaleKnob);

    setLayer(1);
}

PitchModule::~PitchModule() {}

void PitchModule::setLayer(int layerIndex)
{
    layerPrefix = (layerIndex == 1) ? "L1_" : "L2_";

    juce::Colour layerColor = (layerIndex == 1) ? juce::Colours::cyan : juce::Colours::magenta;
    juce::Colour dotColor = (layerIndex == 1) ? juce::Colours::white : juce::Colours::pink;

    transKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);
    fineKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);
    scaleKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);

    transKnob.setColour(juce::Slider::thumbColourId, dotColor);
    fineKnob.setColour(juce::Slider::thumbColourId, dotColor);
    scaleKnob.setColour(juce::Slider::thumbColourId, dotColor);

    transAttach.reset();
    fineAttach.reset();
    scaleAttach.reset();

    transAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "PITCH_TRANS", transKnob);
    fineAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "PITCH_FINE", fineKnob);
    scaleAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "PITCH_SCALE", scaleKnob);
}

void PitchModule::resized()
{
    auto area = getLocalBounds().reduced(10);
    int widthUnit = area.getWidth() / 3;

    transKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    fineKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    scaleKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
}