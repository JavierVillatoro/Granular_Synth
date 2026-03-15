/*
  ==============================================================================

    EngineModule.cpp
    Created: 15 Mar 2026 7:58:18pm
    Author:  franc

  ==============================================================================
*/

#include "EngineModule.h"

EngineModule::EngineModule(juce::AudioProcessorValueTreeState& apvts)
{
    grainSizeKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    grainSizeKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(grainSizeKnob);

    // Conectamos el cable del knob al parámetro "GRAIN_SIZE"
    grainSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "GRAIN_SIZE", grainSizeKnob);
}

EngineModule::~EngineModule() {}

void EngineModule::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRect(getLocalBounds(), 1);
}

void EngineModule::resized()
{
    grainSizeKnob.setBounds(getLocalBounds().reduced(20));
}
