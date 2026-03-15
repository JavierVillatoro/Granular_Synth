/*
  ==============================================================================

    Spraymodule.cpp
    Created: 16 Mar 2026 12:13:23am
    Author:  franc

  ==============================================================================
*/

#include "SprayModule.h"

SprayModule::SprayModule(juce::AudioProcessorValueTreeState& apvts)
{
    // Configuramos el knob de Spray Position
    sprayPosKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    sprayPosKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0); // Sin números
    addAndMakeVisible(sprayPosKnob);

    // Lo conectamos al parámetro "SPRAY_POS" que creamos en el Processor
    sprayPosAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPRAY_POS", sprayPosKnob);
}

void SprayModule::resized()
{
    auto area = getLocalBounds().reduced(10);
    auto widthUnit = area.getWidth() / 3; // Dividimos el pastel en 3

    // El primer tercio es para el Spray de Posición
    sprayPosKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));

    // Los otros dos tercios quedan vacíos por ahora (para Pitch y Pan en el futuro)
}
