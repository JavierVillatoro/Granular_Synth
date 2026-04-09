/*
  ==============================================================================

    EngineModule.cpp
    Created: 15 Mar 2026 7:58:18pm
    Author:  franc

  ==============================================================================
*/

#include "EngineModule.h"

EngineModule::EngineModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : apvtsRef(apvts), layerPrefix(prefix)
{
    auto setupKnob = [this](juce::Slider& slider) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(slider);
        };

    setupKnob(sizeKnob);
    setupKnob(densityKnob);
    setupKnob(shapeKnob);

    setLayer(1);
}

void EngineModule::setLayer(int layerIndex)
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

    sizeKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);
    densityKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);
    shapeKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);

    sizeKnob.setColour(juce::Slider::thumbColourId, dotColor);
    densityKnob.setColour(juce::Slider::thumbColourId, dotColor);
    shapeKnob.setColour(juce::Slider::thumbColourId, dotColor);

    sizeAttach.reset();
    densityAttach.reset();
    shapeAttach.reset();

    sizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "GRAIN_SIZE", sizeKnob);
    densityAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "DENSITY", densityKnob);
    shapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "SHAPE", shapeKnob);
}

void EngineModule::resized()
{
    auto area = getLocalBounds().reduced(10);
    int widthUnit = area.getWidth() / 3;

    sizeKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    densityKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    shapeKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
}
