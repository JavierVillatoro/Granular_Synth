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
    //setupButton(muteButton, "MUTE", paramPrefix + "MUTE", muteAttach);

    // --- NUEVO: SLIDER DE PANEO ---
    panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panSlider.setColour(juce::Slider::thumbColourId, mainColor);
    panSlider.setColour(juce::Slider::trackColourId, juce::Colours::white.withAlpha(0.1f));
    // ¡Aquí está la magia del doble clic para centrar!
    panSlider.setDoubleClickReturnValue(true, 0.0);

    addAndMakeVisible(panSlider);
    panAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, paramPrefix + "PAN", panSlider);
}

LayerControlsModule::~LayerControlsModule() {}

void LayerControlsModule::paint(juce::Graphics& g) {}

void LayerControlsModule::resized()
{
    auto area = getLocalBounds();

    // Separamos la mitad superior para los botones (20 píxeles de alto)
    auto topRow = area.removeFromTop(20);

    // Calculamos el ancho para que los 3 botones midan exactamente lo mismo
    int btnWidth = topRow.getWidth() / 3;

    playButton.setBounds(topRow.removeFromLeft(btnWidth));
    midiButton.setBounds(topRow.removeFromLeft(btnWidth));
    holdButton.setBounds(topRow.removeFromLeft(btnWidth));

    // El slider de paneo se queda con la fila de abajo al completo
    panSlider.setBounds(area);
}
