/*
  ==============================================================================

    EngineModule.cpp
    Created: 15 Mar 2026 7:58:18pm
    Author:  franc

  ==============================================================================
*/

#include "EngineModule.h"

// AÑADIDO: Recibimos y guardamos el prefijo
EngineModule::EngineModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : layerPrefix(prefix)
{
    // --- SIZE (Izquierda) ---
    sizeKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    sizeKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(sizeKnob);
    // USAMOS EL PREFIJO
    sizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, layerPrefix + "GRAIN_SIZE", sizeKnob);

    // --- DENSITY (Centro) ---
    densityKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    densityKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(densityKnob);
    // USAMOS EL PREFIJO
    densityAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, layerPrefix + "DENSITY", densityKnob);

    // --- SHAPE (Derecha) ---
    shapeKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    shapeKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(shapeKnob);
    // USAMOS EL PREFIJO
    shapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, layerPrefix + "SHAPE", shapeKnob);
}

void EngineModule::resized()
{
    auto area = getLocalBounds().reduced(10);
    int widthUnit = area.getWidth() / 3;

    sizeKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    densityKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    shapeKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
}
