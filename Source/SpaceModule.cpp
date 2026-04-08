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

    sizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPACE_SIZE", sizeKnob);
    fbackAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPACE_FBACK", fbackKnob);

    setLayer(1);
}

SpaceModule::~SpaceModule() {}

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

    // Solo cambiamos el color y el cable del MIX (el independiente)
    mixKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);
    mixKnob.setColour(juce::Slider::thumbColourId, dotColor);

    mixAttach.reset();
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "SPACE_MIX", mixKnob);
}

void SpaceModule::resized()
{
    auto area = getLocalBounds().reduced(10);
    int widthUnit = area.getWidth() / 3;
    sizeKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    fbackKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    mixKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
}