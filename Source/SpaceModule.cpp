#include "SpaceModule.h"

// AÑADIDO: Recibimos y guardamos el prefijo
SpaceModule::SpaceModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : layerPrefix(prefix)
{
    auto setupKnob = [this](juce::Slider& k) {
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(k);
        };

    setupKnob(sizeKnob);
    setupKnob(fbackKnob);
    setupKnob(mixKnob);

    juce::Colour titaniumColor = juce::Colour(0xffd0d0d0);
    auto setKnobPlatino = [&](juce::Slider& k) {
        k.setColour(juce::Slider::rotarySliderOutlineColourId, titaniumColor.withAlpha(0.2f));
        k.setColour(juce::Slider::rotarySliderFillColourId, titaniumColor.withAlpha(0.8f));
        k.setColour(juce::Slider::thumbColourId, titaniumColor);
        };

    setKnobPlatino(sizeKnob);
    setKnobPlatino(fbackKnob);

    // CONECTAMOS: Size y Feedback son globales, Mix lleva prefijo
    sizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPACE_SIZE", sizeKnob);
    fbackAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPACE_FBACK", fbackKnob);
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, layerPrefix + "SPACE_MIX", mixKnob);
}

SpaceModule::~SpaceModule() {}

void SpaceModule::resized()
{
    auto area = getLocalBounds().reduced(10);
    int widthUnit = area.getWidth() / 3;
    sizeKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    fbackKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    mixKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
}
