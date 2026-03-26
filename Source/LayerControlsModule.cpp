/*
  ==============================================================================

    LayerControlsModule.cpp
    Created: 26 Mar 2026 2:14:20am
    Author:  franc

  ==============================================================================
*/

#include "LayerControlsModule.h"

LayerControlsModule::LayerControlsModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : apvtsRef(apvts), paramPrefix(prefix)
{
    // Función rápida para configurar los 3 botones igual
    auto setupButton = [this](juce::TextButton& btn, juce::String text, juce::String paramId, std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attach) {
        btn.setButtonText(text);
        btn.setClickingTogglesState(true);

        btn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.4f));
        btn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::cyan.withAlpha(0.3f));
        btn.setColour(juce::TextButton::textColourOnId, juce::Colours::cyan);

        addAndMakeVisible(btn);
        // Conectamos el botón uniendo el prefijo ("L1_") + "PLAY", etc.
        attach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvtsRef, paramId, btn);
        };

    setupButton(playButton, "PLAY", paramPrefix + "PLAY", playAttach);
    setupButton(midiButton, "MIDI", paramPrefix + "MIDI", midiAttach);
    setupButton(holdButton, "HOLD", paramPrefix + "HOLD", holdAttach);
}

LayerControlsModule::~LayerControlsModule() {}

void LayerControlsModule::paint(juce::Graphics& g)
{
    // Fondo oscuro semitransparente para que se lea bien sobre la onda
    //g.setColour(juce::Colours::black.withAlpha(0.7f));
    //g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

    // Borde cyan muy sutil
    //g.setColour(juce::Colours::cyan.withAlpha(0.3f));
    //g.drawRoundedRectangle(getLocalBounds().toFloat(), 4.0f, 1.0f);
}

void LayerControlsModule::resized()
{
    auto area = getLocalBounds().reduced(2);
    int btnWidth = area.getWidth() / 3;

    // Repartimos el espacio en 3 columnas exactas
    playButton.setBounds(area.removeFromLeft(btnWidth));
    midiButton.setBounds(area.removeFromLeft(btnWidth));
    holdButton.setBounds(area);
}
