/*
  ==============================================================================

    ScanModule.cpp
    Created: 15 Mar 2026 7:33:44pm
    Author:  franc

  ==============================================================================
*/

#include "ScanModule.h"

// 1. CONSTRUCTOR: Configuramos el knob y lo conectamos al cerebro
ScanModule::ScanModule(juce::AudioProcessorValueTreeState& apvts)
{
    positionKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    //positionKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    positionKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(positionKnob);

    // Conectamos el cable
    positionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "POSITION", positionKnob);

    scanSpeedKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    //scanSpeedKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    scanSpeedKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(scanSpeedKnob);

    scanSpeedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SCAN_SPEED", scanSpeedKnob);
}

ScanModule::~ScanModule() {}

// 2. DIBUJO DE FONDO: Le ponemos un borde sutil para ver dónde está nuestro módulo
void ScanModule::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRect(getLocalBounds(), 1); // Dibuja un cuadrado alrededor del módulo
}

// 3. COLOCACIÓN: Ponemos el knob en el centro de este módulo
void ScanModule::resized()
{
    // 1. Tomamos el área total con un margen de 10 píxeles
    auto area = getLocalBounds().reduced(10);

    // 2. Calculamos cuánto mide exactamente un tercio del ancho total
    auto widthUnit = area.getWidth() / 3;

    // 3. Cortamos el primer tercio desde la izquierda para POSITION
    positionKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));

    // 4. Cortamos el segundo tercio (el central) para SCAN SPEED
    scanSpeedKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));

    // 5. El área que queda en la variable 'area' es el tercio de la derecha.
    // Como no llamamos a setBounds para ningún knob con lo que queda,
    // ese espacio se queda vacío y reservado para el futuro knob de "Direction".
}
