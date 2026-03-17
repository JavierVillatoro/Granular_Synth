/*
  ==============================================================================

    ScanModule.cpp
    Created: 15 Mar 2026 7:33:44pm
    Author:  franc

  ==============================================================================
*/

#include "ScanModule.h"

// 1. CONSTRUCTOR: Configuramos los knobs y los conectamos al cerebro
ScanModule::ScanModule(juce::AudioProcessorValueTreeState& apvts)
{
    // Función rápida para configurar el estilo visual de los knobs y no repetir código
    auto setupKnob = [this](juce::Slider& k) {
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(k);
        };

    setupKnob(posKnob);
    setupKnob(speedKnob);
    setupKnob(dirKnob);

    // Conectamos a los parámetros del Processor usando los nombres correctos
    posAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "POSITION", posKnob);
    speedAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SCAN_SPEED", speedKnob);
    dirAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SCAN_MODE", dirKnob);
}

// 2. DESTRUCTOR (Se queda como lo tenías)
ScanModule::~ScanModule() {}

// 3. DIBUJO DE FONDO: Le ponemos un borde sutil para ver dónde está nuestro módulo
void ScanModule::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRect(getLocalBounds(), 1); // Dibuja un cuadrado alrededor del módulo
}

// 4. POSICIONAMIENTO DE LOS BOTONES
void ScanModule::resized()
{
    // Margen global de 10 píxeles para que los knobs respiren
    auto area = getLocalBounds().reduced(10);

    // Dividimos el ancho total en 3 columnas iguales
    int widthUnit = area.getWidth() / 3;

    // Repartimos los 3 knobs de izquierda a derecha.
    posKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));   // Columna 1: Position
    speedKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5)); // Columna 2: Speed
    dirKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));   // Columna 3: Direction (Modos)
}
