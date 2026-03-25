/*
  ==============================================================================

    BpmModule.cpp
    Created: 25 Mar 2026 12:13:37am
    Author:  franc

  ==============================================================================
*/

#include "BpmModule.h"

BpmModule::BpmModule(juce::AudioProcessorValueTreeState& apvts) : apvtsRef(apvts)
{
    // 1. CONFIGURAMOS EL KNOB MANUAL (Estética Global Gris/Blanco)
    bpmKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    bpmKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    bpmKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::lightgrey);
    bpmKnob.setColour(juce::Slider::thumbColourId, juce::Colours::white.withAlpha(0.9f));
    addAndMakeVisible(bpmKnob);
    bpmAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, "MANUAL_BPM", bpmKnob);

    // 2. CONFIGURAMOS EL BOTÓN DE SYNC
    syncButton.setButtonText("DAW SYNC");
    syncButton.setClickingTogglesState(true);

    // Estética del botón: Transparente cuando está apagado, iluminado cuando está encendido
    syncButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    syncButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::white.withAlpha(0.2f));
    syncButton.setColour(juce::TextButton::textColourOffId, juce::Colours::grey);
    syncButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(syncButton);
    syncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvtsRef, "SYNC_TO_DAW", syncButton);

    // 3. MAGIA VISUAL: Desactivar el knob si el Sync está encendido
    syncButton.onStateChange = [this]() {
        bool isSynced = syncButton.getToggleState();
        bpmKnob.setEnabled(!isSynced);
        // Hacemos el knob semitransparente si está desactivado
        bpmKnob.setAlpha(isSynced ? 0.3f : 1.0f);
        };

    // Disparamos la comprobación inicial
    syncButton.onStateChange();
}

BpmModule::~BpmModule() {}

void BpmModule::paint(juce::Graphics& g)
{
    // Texto del Knob
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText("BPM", bpmKnob.getX(), bpmKnob.getY() - 15, bpmKnob.getWidth(), 15, juce::Justification::centred);
}

void BpmModule::resized()
{
    auto area = getLocalBounds().reduced(2);

    // El botón de Sync va abajo
    auto bottomArea = area.removeFromBottom(25);
    syncButton.setBounds(bottomArea.reduced(5, 2));

    // Dejamos hueco arriba para el título y el resto es para el Knob
    area.removeFromTop(15);
    bpmKnob.setBounds(area.reduced(5));
}
