/*
  ==============================================================================

    LayerControlsModule.cpp

  ==============================================================================
*/

#include "LayerControlsModule.h"

LayerControlsModule::LayerControlsModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : apvtsRef(apvts), paramPrefix(prefix)
{
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
    // AQUÍ ESTÁ LA CLAVE PARA QUE APAREZCA EL MUTE:
    setupButton(muteButton, "MUTE", paramPrefix + "MUTE", muteAttach);

    panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panSlider.setColour(juce::Slider::thumbColourId, mainColor);
    panSlider.setColour(juce::Slider::trackColourId, juce::Colours::white.withAlpha(0.1f));
    panSlider.setDoubleClickReturnValue(true, 0.0);

    addAndMakeVisible(panSlider);
    panAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, paramPrefix + "PAN", panSlider);
}

LayerControlsModule::~LayerControlsModule() {}
void LayerControlsModule::paint(juce::Graphics& g) {}

void LayerControlsModule::resized()
{
    auto area = getLocalBounds();

    // Dividimos en dos filas
    auto topRow = area.removeFromTop(area.getHeight() / 2);
    int btnWidth = 45;

    // Fila 1: Botones (45px cada uno)
    playButton.setBounds(topRow.removeFromLeft(btnWidth));
    midiButton.setBounds(topRow.removeFromLeft(btnWidth));
    holdButton.setBounds(topRow.removeFromLeft(btnWidth));
    muteButton.setBounds(topRow.removeFromLeft(btnWidth)); // Se pone a la derecha de Hold

    // Fila 2: Slider de Paneo (Ocupa el ancho de los 3 primeros botones: 45 * 3 = 135)
    auto bottomRow = area;
    panSlider.setBounds(bottomRow.removeFromLeft(btnWidth * 3).reduced(5, 2));
}
