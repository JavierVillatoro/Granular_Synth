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
    juce::Colour layerColor;
    juce::Colour dotColor;

    if (layerIndex == 1) {
        layerPrefix = "L1_";
        layerColor = juce::Colours::cyan;
        dotColor = juce::Colours::white;
    }
    else if (layerIndex == 2) {
        layerPrefix = "L2_";
        layerColor = juce::Colours::magenta;
        dotColor = juce::Colours::pink;
    }
    else if (layerIndex == 3) {
        layerPrefix = "L3_";
        layerColor = juce::Colours::orange;
        dotColor = juce::Colours::whitesmoke;
    }
    else if (layerIndex == 4) {
        layerPrefix = "L4_";
        layerColor = juce::Colours::lime;
        dotColor = juce::Colours::lightgrey.withAlpha(0.9f);
    }

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