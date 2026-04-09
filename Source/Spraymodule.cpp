/*
  ==============================================================================

    Spraymodule.cpp
    Created: 16 Mar 2026 12:13:23am
    Author:  franc

  ==============================================================================
*/

#include "SprayModule.h"

SprayModule::SprayModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : apvtsRef(apvts), layerPrefix(prefix)
{
    auto setupKnob = [this](juce::Slider& k) {
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(k);
        };

    setupKnob(posSprayKnob);
    setupKnob(pitchSprayKnob);
    setupKnob(panSprayKnob);

    setLayer(1);
}

void SprayModule::setLayer(int layerIndex)
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

    posSprayKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);
    pitchSprayKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);
    panSprayKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);

    posSprayKnob.setColour(juce::Slider::thumbColourId, dotColor);
    pitchSprayKnob.setColour(juce::Slider::thumbColourId, dotColor);
    panSprayKnob.setColour(juce::Slider::thumbColourId, dotColor);

    posSprayAttach.reset();
    pitchSprayAttach.reset();
    panSprayAttach.reset();

    posSprayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "SPRAY_POS", posSprayKnob);
    pitchSprayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "SPRAY_PITCH", pitchSprayKnob);
    panSprayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "SPRAY_PAN", panSprayKnob);
}

void SprayModule::resized()
{
    auto area = getLocalBounds().reduced(10);
    int widthUnit = area.getWidth() / 3;

    posSprayKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    pitchSprayKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    panSprayKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
}
