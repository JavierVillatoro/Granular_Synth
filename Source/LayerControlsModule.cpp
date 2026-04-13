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

    auto setupButton = [this, mainColor](juce::TextButton& btn, juce::String text, juce::String paramId, std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attach, juce::Colour onColor) {
        btn.setButtonText(text);
        btn.setClickingTogglesState(true);
        btn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.4f));
        btn.setColour(juce::TextButton::buttonOnColourId, onColor.withAlpha(0.3f));
        btn.setColour(juce::TextButton::textColourOnId, onColor);
        addAndMakeVisible(btn);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvtsRef, paramId, btn);
        };

    // 1. Botones Estándar
    setupButton(playButton, "PLAY", paramPrefix + "PLAY", playAttach, mainColor);
    setupButton(midiButton, "MIDI", paramPrefix + "MIDI", midiAttach, mainColor);
    setupButton(holdButton, "HOLD", paramPrefix + "HOLD", holdAttach, mainColor);
    setupButton(muteButton, "MUTE", paramPrefix + "MUTE", muteAttach, mainColor);

    // 2. Botón REC (Se ilumina en Rojo)
    setupButton(recButton, "REC", paramPrefix + "REC", recAttach, juce::Colours::red);

    // 3. Menú Desplegable de Grabación
    recModeBox.addItem("DAW Input / Mic", 1);
    recModeBox.addItem("WiFi Live (UDP)", 2);
    recModeBox.addItem("WiFi File (TCP)", 3);
    recModeBox.setJustificationType(juce::Justification::centred);
    recModeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    recModeBox.setColour(juce::ComboBox::textColourId, juce::Colours::white.withAlpha(0.7f));
    recModeBox.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(recModeBox);
    recModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvtsRef, paramPrefix + "REC_MODE", recModeBox);

    // 4. Slider de Paneo
    panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panSlider.setColour(juce::Slider::thumbColourId, mainColor);
    panSlider.setColour(juce::Slider::trackColourId, juce::Colours::white.withAlpha(0.3f));
    panSlider.setDoubleClickReturnValue(true, 0.0);
    addAndMakeVisible(panSlider);
    panAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, paramPrefix + "PAN", panSlider);
}

LayerControlsModule::~LayerControlsModule() {}
void LayerControlsModule::paint(juce::Graphics& g) {}

void LayerControlsModule::resized()
{
    auto area = getLocalBounds();
    auto topRow = area.removeFromTop(area.getHeight() / 2);
    int btnWidth = 45;

    // Fila 1: Transporte (Izquierda)
    playButton.setBounds(topRow.removeFromLeft(btnWidth));
    midiButton.setBounds(topRow.removeFromLeft(btnWidth));
    holdButton.setBounds(topRow.removeFromLeft(btnWidth));
    muteButton.setBounds(topRow.removeFromLeft(btnWidth));

    // Fila 1: Botón REC (Derecha)
    recButton.setBounds(topRow.removeFromRight(50).reduced(2, 2));

    // Fila 2: Paneo (Ocupa el ancho exacto de Play, Midi y Hold)
    auto bottomRow = area;
    panSlider.setBounds(bottomRow.removeFromLeft(btnWidth * 3).reduced(5, 2));

    // Fila 2: Desplegable REC_MODE (Debajo del Mute y el REC)
    recModeBox.setBounds(bottomRow.removeFromRight(120).reduced(2, 2));
}
