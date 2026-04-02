/*
  ==============================================================================

    EngineModule.cpp
    Created: 15 Mar 2026 7:58:18pm
    Author:  franc

  ==============================================================================
*/

#include "EngineModule.h"

// AÑADIDO: Recibimos y guardamos el apvts y el prefijo
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

    // Arrancamos por defecto simulando que estamos en la Capa 1
    setLayer(1);
}

void EngineModule::setLayer(int layerIndex)
{
    // 1. Cambiamos la identidad
    layerPrefix = (layerIndex == 1) ? "L1_" : "L2_";

    // 2. Cambiamos el color visual
    // Capa 1 = Cyan (Barra y Punto)
    // Capa 2 = Magenta (Barra) y Rosa (Punto)
    juce::Colour layerColor = (layerIndex == 1) ? juce::Colours::cyan : juce::Colours::magenta;
    juce::Colour dotColor = (layerIndex == 1) ? juce::Colours::white : juce::Colours::pink;

    // Actualizamos la barra
    sizeKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);
    densityKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);
    shapeKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);

    // Actualizamos el "puntito"
    sizeKnob.setColour(juce::Slider::thumbColourId, dotColor);
    densityKnob.setColour(juce::Slider::thumbColourId, dotColor);
    shapeKnob.setColour(juce::Slider::thumbColourId, dotColor);

    // 3. ¡EL TRUCO MAGISTRAL! Desenchufamos los cables viejos y ponemos los nuevos
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
