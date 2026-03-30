/*
  ==============================================================================

    Spraymodule.cpp
    Created: 16 Mar 2026 12:13:23am
    Author:  franc

  ==============================================================================
*/

#include "SprayModule.h"

// AÑADIDO: Recibimos y guardamos el prefijo
SprayModule::SprayModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : layerPrefix(prefix)
{
    auto setupKnob = [this](juce::Slider& k) {
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(k);
        };

    setupKnob(posSprayKnob);
    setupKnob(pitchSprayKnob);
    setupKnob(panSprayKnob);

    // USAMOS EL PREFIJO DINÁMICO
    posSprayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, layerPrefix + "SPRAY_POS", posSprayKnob);
    pitchSprayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, layerPrefix + "SPRAY_PITCH", pitchSprayKnob);
    panSprayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, layerPrefix + "SPRAY_PAN", panSprayKnob);
}

void SprayModule::resized()
{
    auto area = getLocalBounds().reduced(10);
    int widthUnit = area.getWidth() / 3;

    posSprayKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    pitchSprayKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    panSprayKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
}
