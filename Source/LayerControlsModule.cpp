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
    // Detectamos el color basándonos en nuestro "DNI" (El prefijo)
    juce::Colour mainColor = juce::Colours::cyan;
    if (paramPrefix == "L2_") mainColor = juce::Colours::magenta;
    if (paramPrefix == "L3_") mainColor = juce::Colours::orange;
    if (paramPrefix == "L4_") mainColor = juce::Colours::lime; // <-- ¡NUESTRA NUEVA CAPA!

    auto setupButton = [this, mainColor](juce::TextButton& btn, juce::String text, juce::String paramId, std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attach) {
        btn.setButtonText(text);
        btn.setClickingTogglesState(true);

        btn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.4f));

        // Usamos el color dinámico
        btn.setColour(juce::TextButton::buttonOnColourId, mainColor.withAlpha(0.3f));
        btn.setColour(juce::TextButton::textColourOnId, mainColor);

        addAndMakeVisible(btn);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvtsRef, paramId, btn);
        };

    setupButton(playButton, "PLAY", paramPrefix + "PLAY", playAttach);
    setupButton(midiButton, "MIDI", paramPrefix + "MIDI", midiAttach);
    setupButton(holdButton, "HOLD", paramPrefix + "HOLD", holdAttach);
}

LayerControlsModule::~LayerControlsModule() {}

void LayerControlsModule::paint(juce::Graphics& g) {}

void LayerControlsModule::resized()
{
    auto area = getLocalBounds().reduced(2);
    int btnWidth = area.getWidth() / 3;

    playButton.setBounds(area.removeFromLeft(btnWidth));
    midiButton.setBounds(area.removeFromLeft(btnWidth));
    holdButton.setBounds(area);
}
