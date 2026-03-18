/*
  ==============================================================================

    PitchModule.cpp
    Created: 18 Mar 2026 12:07:55am
    Author:  franc

  ==============================================================================
*/

#include "PitchModule.h"

PitchModule::PitchModule(juce::AudioProcessorValueTreeState& apvts)
{
    auto setupKnob = [this](juce::Slider& k) {
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(k);
        };

    setupKnob(transKnob);
    setupKnob(fineKnob);
    setupKnob(scaleKnob);

    transAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "PITCH_TRANS", transKnob);
    fineAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "PITCH_FINE", fineKnob);
    scaleAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "PITCH_SCALE", scaleKnob);
}

PitchModule::~PitchModule() {}

void PitchModule::resized()
{
    auto area = getLocalBounds().reduced(10);
    int widthUnit = area.getWidth() / 3;

    transKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    fineKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    scaleKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
}