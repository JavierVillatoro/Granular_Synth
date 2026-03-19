/*
  ==============================================================================

    SpaceModule.cpp
    Created: 19 Mar 2026 3:04:12am
    Author:  franc

  ==============================================================================
*/

#include "SpaceModule.h"

SpaceModule::SpaceModule(juce::AudioProcessorValueTreeState& apvts)
{
    auto setupKnob = [this](juce::Slider& k) {
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(k);
        };

    setupKnob(sizeKnob); setupKnob(fbackKnob); setupKnob(mixKnob);

    sizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPACE_SIZE", sizeKnob);
    fbackAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPACE_FBACK", fbackKnob);
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPACE_MIX", mixKnob);
}

SpaceModule::~SpaceModule() {}

void SpaceModule::resized()
{
    auto area = getLocalBounds().reduced(10);
    int widthUnit = area.getWidth() / 3;
    sizeKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    fbackKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    mixKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
}
