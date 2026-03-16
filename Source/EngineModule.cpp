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
    // --- SIZE (Izquierda) ---
    sizeKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    sizeKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(sizeKnob);
    sizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "GRAIN_SIZE", sizeKnob);

    // --- DENSITY (Centro) ---
    densityKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    densityKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(densityKnob);
    densityAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "DENSITY", densityKnob);

    // --- SHAPE (Derecha) ---
    shapeKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    shapeKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(shapeKnob);
    shapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SHAPE", shapeKnob);
}

void EngineModule::resized()
{
    auto area = getLocalBounds().reduced(10);
    int widthUnit = area.getWidth() / 3; // Dividimos la caja en 3 trozos iguales

    // Colocamos cada knob de izquierda a derecha. 
    // removeFromLeft va cortando el rectángulo como si fuera una barra de pan.
    sizeKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    densityKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    shapeKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
}
