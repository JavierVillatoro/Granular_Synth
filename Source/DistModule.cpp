/*
  ==============================================================================

    DistModule.cpp
    Created: 24 Mar 2026 1:50:24am
    Author:  franc

  ==============================================================================
*/

#include "DistModule.h"

// AÑADIDO: Recibimos y guardamos el prefijo
DistModule::DistModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : apvtsRef(apvts), layerPrefix(prefix)
{
    auto setupKnob = [this](juce::Slider& slider, const juce::String& paramID, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

        slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan.withAlpha(0.7f));
        slider.setColour(juce::Slider::thumbColourId, juce::Colours::cyan);

        addAndMakeVisible(slider);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, paramID, slider);
        };

    // USAMOS PREFIJO DINÁMICO
    setupKnob(driveKnob, layerPrefix + "DIST_DRIVE", driveAttach);
    setupKnob(mixKnob, layerPrefix + "DIST_MIX", mixAttach);

    typeCombo.addItemList({ "Soft Clip", "Hard Clip", "Foldback", "Bitcrush" }, 1);
    typeCombo.setJustificationType(juce::Justification::centred);

    typeCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    typeCombo.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    typeCombo.setColour(juce::ComboBox::textColourId, juce::Colours::cyan.withAlpha(0.8f));
    typeCombo.setColour(juce::ComboBox::arrowColourId, juce::Colours::cyan.withAlpha(0.8f));

    addAndMakeVisible(typeCombo);
    // PREFIJO EN COMBOBOX
    typeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvtsRef, layerPrefix + "DIST_TYPE", typeCombo);
}

DistModule::~DistModule() {}

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
