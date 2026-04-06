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

    juce::Colour titaniumColor = juce::Colour(0xffd0d0d0);
    auto setKnobPlatino = [&](juce::Slider& k) {
        k.setColour(juce::Slider::rotarySliderOutlineColourId, titaniumColor.withAlpha(0.2f));
        k.setColour(juce::Slider::rotarySliderFillColourId, titaniumColor.withAlpha(0.8f));
        k.setColour(juce::Slider::thumbColourId, titaniumColor);
        };

    // Size y Feedback son siempre grises (globales)
    setKnobPlatino(sizeKnob);
    setKnobPlatino(fbackKnob);

    // Los attachments globales se quedan fijos
    sizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPACE_SIZE", sizeKnob);
    fbackAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPACE_FBACK", fbackKnob);

    setLayer(1); // Arrancamos en Capa 1
}

void SpaceModule::setLayer(int layerIndex)
{
    layerPrefix = (layerIndex == 1) ? "L1_" : "L2_";

    juce::Colour layerColor = (layerIndex == 1) ? juce::Colours::cyan : juce::Colours::magenta;
    juce::Colour dotColor = (layerIndex == 1) ? juce::Colours::dodgerblue : juce::Colours::pink;

    // Solo cambiamos el color y el cable del MIX
    mixKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);
    mixKnob.setColour(juce::Slider::thumbColourId, dotColor);

    mixAttach.reset();
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "SPACE_MIX", mixKnob);
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
