/*
  ==============================================================================

    DistModule.cpp
    Created: 24 Mar 2026 1:50:24am
    Author:  franc

  ==============================================================================
*/

#include "DistModule.h"

DistModule::DistModule(juce::AudioProcessorValueTreeState& apvts) : apvtsRef(apvts)
{
    // Función rápida para configurar nuestros Knobs Cyan
    auto setupKnob = [this](juce::Slider& slider, const juce::String& paramID, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

        // Estética Cyan para indicar que afecta a la Capa 1
        slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan.withAlpha(0.7f));
        slider.setColour(juce::Slider::thumbColourId, juce::Colours::cyan);

        addAndMakeVisible(slider);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, paramID, slider);
        };

    setupKnob(driveKnob, "DIST_DRIVE", driveAttach);
    setupKnob(mixKnob, "DIST_MIX", mixAttach);

    // ==========================================
    // CONFIGURACIÓN DEL MENÚ DESPLEGABLE (TIPO)
    // ==========================================
    typeCombo.addItemList({ "Soft Clip", "Hard Clip", "Foldback", "Bitcrush" }, 1);
    typeCombo.setJustificationType(juce::Justification::centred);

    // Lo hacemos invisible/elegante: sin fondo ni bordes, solo el texto y la flechita cyan
    typeCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    typeCombo.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    typeCombo.setColour(juce::ComboBox::textColourId, juce::Colours::cyan.withAlpha(0.8f));
    typeCombo.setColour(juce::ComboBox::arrowColourId, juce::Colours::cyan.withAlpha(0.8f));

    addAndMakeVisible(typeCombo);
    typeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvtsRef, "DIST_TYPE", typeCombo);
}

DistModule::~DistModule() {}

void DistModule::paint(juce::Graphics& g)
{
    // NUEVO: Fondo sutil Cyan para enmarcar el módulo
    //g.setColour(juce::Colours::cyan.withAlpha(0.05f));
    //g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
    // Dibujamos las etiquetas encima de los Knobs
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(11.0f, juce::Font::bold));

    g.drawText("DRIVE", driveKnob.getX(), driveKnob.getY() - 15, driveKnob.getWidth(), 15, juce::Justification::centred);
    g.drawText("MIX", mixKnob.getX(), mixKnob.getY() - 15, mixKnob.getWidth(), 15, juce::Justification::centred);
}

void DistModule::resized()
{
    // Reducimos el margen general de 5 a solo 2 píxeles
    auto area = getLocalBounds().reduced(2);

    // El menú desplegable abajo a la derecha
    auto bottomArea = area.removeFromBottom(20);
    typeCombo.setBounds(bottomArea.removeFromRight(85));

    // Hueco arriba para los textos
    area.removeFromTop(15);

    int knobWidth = area.getWidth() / 2;

    // Le quitamos el ".reduced(5)" para que los knobs usen el 100% del ancho disponible
    driveKnob.setBounds(area.removeFromLeft(knobWidth));
    mixKnob.setBounds(area);
}
