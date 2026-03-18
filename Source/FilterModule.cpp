/*
  ==============================================================================

    FilterModule.cpp
    Created: 18 Mar 2026 9:10:29pm
    Author:  franc

  ==============================================================================
*/

#include "FilterModule.h"

FilterModule::FilterModule(juce::AudioProcessorValueTreeState& apvts)
{
    // Función rápida para el estilo visual
    auto setupKnob = [this](juce::Slider& k) {
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(k);
    };

    setupKnob(lpfKnob);
    setupKnob(resKnob);
    setupKnob(hpfKnob);

    // Conectamos a los parámetros del Processor que creamos antes
    lpfAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "FILTER_LPF", lpfKnob);
    resAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "FILTER_RES", resKnob);
    hpfAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "FILTER_HPF", hpfKnob);
}

FilterModule::~FilterModule() {}

void FilterModule::resized()
{
    auto area = getLocalBounds().reduced(10);
    int widthUnit = area.getWidth() / 3;

    // Colocamos los botones de izquierda a derecha (LPF -> RES -> HPF)
    lpfKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    resKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    hpfKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
}
