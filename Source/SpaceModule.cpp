/*
  ==============================================================================
    SpaceModule.cpp
  ==============================================================================
*/

#include "SpaceModule.h"

SpaceModule::SpaceModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : apvtsRef(apvts), layerPrefix(prefix)
{
    auto setupKnob = [this](juce::Slider& k) {
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(k);
        };

    setupKnob(sizeKnob);
    setupKnob(fbackKnob);
    setupKnob(mixKnob);

    // Inicializar Labels (Textos debajo de los knobs)
    sizeLabel.setText("Size", juce::dontSendNotification);
    sizeLabel.setJustificationType(juce::Justification::centred);
    sizeLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    sizeLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.5f));
    addAndMakeVisible(sizeLabel);

    fbackLabel.setText("Fback", juce::dontSendNotification);
    fbackLabel.setJustificationType(juce::Justification::centred);
    fbackLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    fbackLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.5f));
    addAndMakeVisible(fbackLabel);

    // Texto de Mix en GRIS CLARO y fijo
    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    mixLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(mixLabel);

    juce::Colour titaniumColor = juce::Colour(0xffd0d0d0);
    auto setKnobPlatino = [&](juce::Slider& k) {
        k.setColour(juce::Slider::rotarySliderOutlineColourId, titaniumColor.withAlpha(0.2f));
        k.setColour(juce::Slider::rotarySliderFillColourId, titaniumColor.withAlpha(0.8f));
        k.setColour(juce::Slider::thumbColourId, titaniumColor);
        };

    setKnobPlatino(sizeKnob);
    setKnobPlatino(fbackKnob);

    sizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPACE_SIZE", sizeKnob);
    fbackAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPACE_FBACK", fbackKnob);

    setLayer(1);
}

SpaceModule::~SpaceModule() {}

void SpaceModule::paint(juce::Graphics& g) {}

void SpaceModule::setLayer(int layerIndex)
{
    juce::Colour layerColor;
    juce::Colour dotColor;

    if (layerIndex == 1) {
        layerPrefix = "L1_";
        layerColor = juce::Colours::cyan;
        dotColor = juce::Colours::white;
    }
    else if (layerIndex == 2) {
        layerPrefix = "L2_";
        layerColor = juce::Colours::magenta;
        dotColor = juce::Colours::pink;
    }
    else if (layerIndex == 3) {
        layerPrefix = "L3_";
        layerColor = juce::Colours::orange;
        dotColor = juce::Colours::whitesmoke;
    }
    else if (layerIndex == 4) {
        layerPrefix = "L4_";
        layerColor = juce::Colours::lime;
        dotColor = juce::Colours::lightgrey.withAlpha(0.9f);
    }

    // Solo cambiamos el color del anillo del Mix, NO de la letra
    mixKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);
    mixKnob.setColour(juce::Slider::thumbColourId, dotColor);

    mixAttach.reset();
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "SPACE_MIX", mixKnob);
}

void SpaceModule::resized()
{
    auto area = getLocalBounds().reduced(2);

    // Dividimos exactamente al 50% para que Size/Fback sean tan grandes como Mix
    auto topArea = area.removeFromTop(area.getHeight() * 0.5f);
    auto bottomArea = area;

    // --- FILA SUPERIOR: Size y Feedback ---
    int halfTopWidth = topArea.getWidth() / 2;

    // Ajustamos al máximo: quitamos márgenes internos para que el knob crezca
    auto sizeCell = topArea.removeFromLeft(halfTopWidth);
    auto sizeLabelArea = sizeCell.removeFromBottom(15);
    sizeKnob.setBounds(sizeCell);
    sizeLabel.setBounds(sizeLabelArea);

    auto fbackCell = topArea;
    auto fbackLabelArea = fbackCell.removeFromBottom(15);
    fbackKnob.setBounds(fbackCell);
    fbackLabel.setBounds(fbackLabelArea);

    // --- FILA INFERIOR: MIX GIGANTE ---
    auto mixLabelArea = bottomArea.removeFromBottom(15);
    int mixSize = juce::jmin(bottomArea.getWidth(), bottomArea.getHeight()) - 2;
    mixKnob.setBounds(bottomArea.withSizeKeepingCentre(mixSize, mixSize));
    mixLabel.setBounds(mixLabelArea);
}