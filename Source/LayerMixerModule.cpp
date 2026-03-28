/*
  ==============================================================================

    LayerMixerModule.cpp
    Created: 28 Mar 2026 5:44:39pm
    Author:  franc

  ==============================================================================
*/

#include <JuceHeader.h>
#include "LayerMixerModule.h"

LayerMixerModule::LayerMixerModule(juce::AudioProcessorValueTreeState& apvts, juce::String layerPrefix)
    : apvtsRef(apvts), prefix(layerPrefix)
{
    // 1. CONFIGURACIÓN DE LOS KNOBS DE EQ
    auto setupEqKnob = [this](juce::Slider& slider) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan.withAlpha(0.8f));
        slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        addAndMakeVisible(slider);
        };

    setupEqKnob(eqLow);
    setupEqKnob(eqMidLow);
    setupEqKnob(eqMidHigh);
    setupEqKnob(eqHigh);

    // 2. CONFIGURACIÓN DEL FADER DE VOLUMEN (Con texto "dB")
    volumeFader.setSliderStyle(juce::Slider::LinearVertical);
    volumeFader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 45, 16);
    volumeFader.setTextValueSuffix(" dB"); // ¡Magia! Ahora el número lleva el apellido dB
    volumeFader.setColour(juce::Slider::trackColourId, juce::Colours::cyan.withAlpha(0.6f));
    volumeFader.setColour(juce::Slider::backgroundColourId, juce::Colours::black.withAlpha(0.5f));
    volumeFader.setColour(juce::Slider::thumbColourId, juce::Colours::cyan);
    volumeFader.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(volumeFader);

    // 3. CONECTAMOS TODO AL APVTS
    attachLow = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, prefix + "EQ_LOW", eqLow);
    attachMidLow = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, prefix + "EQ_MID_LOW", eqMidLow);
    attachMidHigh = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, prefix + "EQ_MID_HIGH", eqMidHigh);
    attachHigh = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, prefix + "EQ_HIGH", eqHigh);
    attachVol = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, prefix + "MIX_VOL", volumeFader);
}

LayerMixerModule::~LayerMixerModule() {}

void LayerMixerModule::paint(juce::Graphics& g)
{
    // Fondo profesional oscuro y borde Cyan
    g.fillAll(juce::Colour(0xff121212));
    g.setColour(juce::Colours::cyan.withAlpha(0.3f));
    g.drawRect(getLocalBounds(), 1);

    // --- LA LÍNEA DEL 0dB EXACTA E INFALIBLE ---
    // Le preguntamos al fader en qué píxel exacto cae el valor 0.0
    float zeroY = volumeFader.getY() + volumeFader.getPositionOfValue(0.0);

    // Dibujamos la línea Cyan justísima
    g.setColour(juce::Colours::cyan.withAlpha(0.9f));
    g.drawLine((float)volumeFader.getX() - 3.0f, zeroY, (float)volumeFader.getRight() + 3.0f, zeroY, 2.0f);

    // --- TEXTOS DE LA EQ (Bajados un poco para que no toquen el knob) ---
    g.setFont(11.0f);
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawText("HIGH", eqHigh.getBounds().translated(0, 22), juce::Justification::centred, false);
    g.drawText("MID-H", eqMidHigh.getBounds().translated(0, 22), juce::Justification::centred, false);
    g.drawText("MID-L", eqMidLow.getBounds().translated(0, 22), juce::Justification::centred, false);
    g.drawText("LOW", eqLow.getBounds().translated(0, 22), juce::Justification::centred, false);
}

void LayerMixerModule::resized()
{
    auto area = getLocalBounds();

    // Cogemos el 40% del ancho total del módulo para hacerlo todo mucho más grande
    auto leftSection = area.removeFromLeft(area.getWidth() * 0.4f).reduced(5);

    // De ese bloque, el 35% de la derecha es para el Fader profesional
    auto faderArea = leftSection.removeFromRight(leftSection.getWidth() * 0.35f);
    volumeFader.setBounds(faderArea.reduced(2, 5));

    // El resto (65%) es para que los knobs de EQ se luzcan
    auto eqArea = leftSection;

    // Dividimos el alto entre 4
    int knobHeight = eqArea.getHeight() / 4;

    // Márgenes reducidos para que los knobs ocupen casi todo el espacio
    eqHigh.setBounds(eqArea.removeFromTop(knobHeight).reduced(3));
    eqMidHigh.setBounds(eqArea.removeFromTop(knobHeight).reduced(3));
    eqMidLow.setBounds(eqArea.removeFromTop(knobHeight).reduced(3));
    eqLow.setBounds(eqArea.removeFromTop(knobHeight).reduced(3));
}
