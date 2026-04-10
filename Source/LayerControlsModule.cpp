/*
  ==============================================================================

    LayerControlsModule.cpp

  ==============================================================================
*/

#include "LayerControlsModule.h"

LayerControlsModule::LayerControlsModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : apvtsRef(apvts), paramPrefix(prefix)
{
    // Permite que los clics en zonas vacías pasen a la onda de audio de debajo
    setInterceptsMouseClicks(false, true);

    juce::Colour mainColor = juce::Colours::cyan;
    if (paramPrefix == "L2_") mainColor = juce::Colours::magenta;
    if (paramPrefix == "L3_") mainColor = juce::Colours::orange;
    if (paramPrefix == "L4_") mainColor = juce::Colours::lime;

    auto setupButton = [this, mainColor](juce::TextButton& btn, juce::String text, juce::String paramId, std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attach) {
        btn.setButtonText(text);
        btn.setClickingTogglesState(true);

        btn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.4f));

        btn.setColour(juce::TextButton::buttonOnColourId, mainColor.withAlpha(0.3f));
        btn.setColour(juce::TextButton::textColourOnId, mainColor);

        addAndMakeVisible(btn);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvtsRef, paramId, btn);
        };

    setupButton(playButton, "PLAY", paramPrefix + "PLAY", playAttach);
    setupButton(midiButton, "MIDI", paramPrefix + "MIDI", midiAttach);
    setupButton(holdButton, "HOLD", paramPrefix + "HOLD", holdAttach);
    setupButton(muteButton, "MUTE", paramPrefix + "MUTE", muteAttach);
}

LayerControlsModule::~LayerControlsModule() {}

void LayerControlsModule::paint(juce::Graphics& g) {}

void LayerControlsModule::resized()
{
    auto area = getLocalBounds();
    int btnWidth = 50;

    // Alineados a la izquierda
    playButton.setBounds(area.removeFromLeft(btnWidth));
    midiButton.setBounds(area.removeFromLeft(btnWidth));
    holdButton.setBounds(area.removeFromLeft(btnWidth));

    // Alineado a la derecha
    muteButton.setBounds(area.removeFromRight(btnWidth));
}
