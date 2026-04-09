/*
  ==============================================================================

    DistModule.cpp
    Created: 24 Mar 2026 1:50:24am
    Author:  franc

  ==============================================================================
*/

#include "DistModule.h"

DistModule::DistModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : apvtsRef(apvts), layerPrefix(prefix)
{
    auto setupKnob = [this](juce::Slider& slider) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(slider);
        };

    setupKnob(driveKnob);
    setupKnob(mixKnob);

    typeCombo.addItemList({ "Soft Clip", "Hard Clip", "Foldback", "Bitcrush" }, 1);
    typeCombo.setJustificationType(juce::Justification::centred);

    typeCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    typeCombo.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);

    addAndMakeVisible(typeCombo);

    setLayer(1); // Arrancamos en la capa 1
}

DistModule::~DistModule() {}

void DistModule::setLayer(int layerIndex)
{
    currentLayer = layerIndex;

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

    // Colores Knobs
    driveKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor.withAlpha(0.7f));
    driveKnob.setColour(juce::Slider::thumbColourId, dotColor);

    mixKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor.withAlpha(0.7f));
    mixKnob.setColour(juce::Slider::thumbColourId, dotColor);

    // Colores ComboBox (Menú desplegable)
    typeCombo.setColour(juce::ComboBox::textColourId, layerColor.withAlpha(0.8f));
    typeCombo.setColour(juce::ComboBox::arrowColourId, layerColor.withAlpha(0.8f));

    // Desenchufar y Enchufar Cables
    driveAttach.reset();
    mixAttach.reset();
    typeAttach.reset();

    driveAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "DIST_DRIVE", driveKnob);
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "DIST_MIX", mixKnob);
    typeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvtsRef, layerPrefix + "DIST_TYPE", typeCombo);
}

void DistModule::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(11.0f, juce::Font::bold));

    g.drawText("DRIVE", driveKnob.getX(), driveKnob.getY() - 15, driveKnob.getWidth(), 15, juce::Justification::centred);
    g.drawText("MIX", mixKnob.getX(), mixKnob.getY() - 15, mixKnob.getWidth(), 15, juce::Justification::centred);
}

void DistModule::resized()
{
    auto area = getLocalBounds().reduced(2);
    auto bottomArea = area.removeFromBottom(20);
    typeCombo.setBounds(bottomArea.removeFromRight(85));
    area.removeFromTop(15);
    int knobWidth = area.getWidth() / 2;

    driveKnob.setBounds(area.removeFromLeft(knobWidth));
    mixKnob.setBounds(area);
}