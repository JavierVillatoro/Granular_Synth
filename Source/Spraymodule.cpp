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
    // Esta pequeña función interna nos ayuda a no repetir código para cada botón
    auto setupKnob = [this](juce::Slider& k) {
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(k);
        };

    // Configuramos los 3 botones
    setupKnob(posSprayKnob);
    setupKnob(pitchSprayKnob);
    setupKnob(panSprayKnob);

    // Conectamos cada botón a su parámetro correspondiente en el procesador
    posSprayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPRAY_POS", posSprayKnob);
    pitchSprayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPRAY_PITCH", pitchSprayKnob);
    panSprayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPRAY_PAN", panSprayKnob);
}

void SprayModule::resized()
{
    // Quitamos 10 píxeles de margen para que los botones no peguen con el borde
    auto area = getLocalBounds().reduced(10);

    // Dividimos el ancho total en 3 columnas iguales
    int widthUnit = area.getWidth() / 3;

    // Repartimos los botones de izquierda a derecha
    posSprayKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));   // Columna 1: Position
    pitchSprayKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5)); // Columna 2: Pitch
    panSprayKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));   // Columna 3: Pan
}
